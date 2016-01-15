#include <sstream>
#include <iostream>
#include <limits>
#include <algorithm>
#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>
#include <boost/mpi/collectives/all_gather.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/mpi/collectives/broadcast.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/operations.hpp>
#include <core/vec.h>
#include <module/module.h>
#include <core/scalars.h>
#include <core/paramvector.h>
#include <core/message.h>
#include <core/coords.h>
#include <core/lines.h>
#include <core/unstr.h>
#include <core/points.h>
#include "Tracer.h"
#include <core/celltree.h>
#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <math.h>
#include <boost/chrono.hpp>
#include "Integrator.h"
#include "TracerTimes.h"

MODULE_MAIN(Tracer)


using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(StartStyle,
      (Line)
      (Plane)
      (Cylinder)
)

Tracer::Tracer(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Tracer", shmname, name, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);
   setReducePolicy(message::ReducePolicy::OverAll);

    createInputPort("data_in0");
    createInputPort("data_in1");
    createOutputPort("data_out0");
    createOutputPort("data_out1");
    createOutputPort("particle_id");
    createOutputPort("timestep");

#if 0
    const char *TracerInteraction::P_DIRECTION = "direction";
    const char *TracerInteraction::P_TDIRECTION = "tdirection";
    const char *TracerInteraction::P_TASKTYPE = "taskType";
    const char *TracerInteraction::P_STARTSTYLE = "startStyle";
    const char *TracerInteraction::P_TRACE_LEN = "trace_len";
    const char *TracerInteraction::P_FREE_STARTPOINTS = "FreeStartPoints";
#endif

    addVectorParameter("startpoint1", "1st initial point", ParamVector(0,0.2,0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1,0,0));
    addVectorParameter("direction", "direction for Plane", ParamVector(1,0,0));
    addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange("no_startp", (Integer)0, (Integer)10000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    //addFloatParameter("trace_len", "maximum number of integrations per particle", 1000);
    IntParameter* tasktype = addIntParameter("taskType", "task type", 0, Parameter::Choice);
    std::vector<std::string> taskchoices;
    taskchoices.push_back("streamlines");
    setParameterChoices(tasktype, taskchoices);
    IntParameter *startStyle = addIntParameter("startStyle", "initial particle position configuration", (Integer)Line, Parameter::Choice);
    V_ENUM_SET_CHOICES(startStyle, StartStyle);
    IntParameter* integration = addIntParameter("integration", "integration method", (Integer)RK32, Parameter::Choice);
    V_ENUM_SET_CHOICES(integration, IntegrationMethod);
    addFloatParameter("min_speed", "miniumum particle speed", 1e-4);
    addFloatParameter("h_euler", "fixed step size for euler integration", 1e-03);
    addFloatParameter("h_min","minimum step size for rk32 integration", 1e-05);
    addFloatParameter("h_max", "maximum step size for rk32 integration", 1e-02);
    addFloatParameter("err_tol", "desired accuracy for rk32 integration", 1e-06);
    addFloatParameter("comm_threshold", "ratio of active particles that have to leave current block for starting communication phase", 0.1);
    m_useCelltree = addIntParameter("use_celltree", "use celltree for accelerated cell location", (Integer)1, Parameter::Boolean);
}

Tracer::~Tracer() {
}


BlockData::BlockData(Index i,
          UnstructuredGrid::const_ptr grid,
          Vec<Scalar, 3>::const_ptr vdata,
          Vec<Scalar>::const_ptr pdata):
m_grid(grid),
m_vecfld(vdata),
m_scafld(pdata),
m_lines(new Lines(Object::Initialized)),
m_ids(new Vec<Index>(Object::Initialized)),
m_steps(new Vec<Index>(Object::Initialized)),
m_vx(nullptr),
m_vy(nullptr),
m_vz(nullptr),
m_p(nullptr)
{
   m_ivec.emplace_back(new Vec<Scalar, 3>(Object::Initialized));

   if (m_vecfld) {
      m_vx = &m_vecfld->x()[0];
      m_vy = &m_vecfld->y()[0];
      m_vz = &m_vecfld->z()[0];
   }

   if (m_scafld) {
      m_p = &m_scafld->x()[0];
      m_iscal.emplace_back(new Vec<Scalar>(Object::Initialized));
   }
}

BlockData::~BlockData(){}

void BlockData::setMeta(const vistle::Meta &meta) {

   m_lines->setMeta(meta);
   if (m_ids)
      m_ids->setMeta(meta);
   if (m_steps)
      m_steps->setMeta(meta);
   for (auto &v: m_ivec) {
       if (v)
          v->setMeta(meta);
   }
   for (auto &s: m_iscal) {
       if (s)
          s->setMeta(meta);
   }
}

UnstructuredGrid::const_ptr BlockData::getGrid(){
    return m_grid;
}

Vec<Scalar, 3>::const_ptr BlockData::getVecFld(){
    return m_vecfld;
}

Vec<Scalar>::const_ptr BlockData::getScalFld(){
    return m_scafld;
}

Lines::ptr BlockData::getLines(){
    return m_lines;
}

std::vector<Vec<Scalar, 3>::ptr> BlockData::getIplVec(){
    return m_ivec;
}

std::vector<Vec<Scalar>::ptr> BlockData::getIplScal(){
    return m_iscal;
}

Vec<Index>::ptr BlockData::ids() const {
   return m_ids;
}

Vec<Index>::ptr BlockData::steps() const {
   return m_steps;
}

void BlockData::addLines(Index id, const std::vector<Vector3> &points,
             const std::vector<Vector3> &velocities,
             const std::vector<Scalar> &pressures,
             const std::vector<Index> &steps) {

//#pragma omp critical
    {
       Index numpoints = points.size();
       assert(numpoints == velocities.size());
       assert(!m_p || pressures.size()==numpoints);

       for(Index i=0; i<numpoints; i++) {

          m_lines->x().push_back(points[i](0));
                m_lines->y().push_back(points[i](1));
                m_lines->z().push_back(points[i](2));
                Index numcorn = m_lines->getNumCorners();
          m_lines->cl().push_back(numcorn);

          m_ivec[0]->x().push_back(velocities[i](0));
                m_ivec[0]->y().push_back(velocities[i](1));
                m_ivec[0]->z().push_back(velocities[i](2));

                if (m_p) {
             m_iscal[0]->x().push_back(pressures[i]);
          }

          m_ids->x().push_back(id);
          m_steps->x().push_back(steps[i]);
       }
       Index numcorn = m_lines->getNumCorners();
       m_lines->el().push_back(numcorn);
    }
}


Particle::Particle(Index i, const Vector3 &pos, Scalar h, Scalar hmin,
      Scalar hmax, Scalar errtol, IntegrationMethod int_mode,const std::vector<std::unique_ptr<BlockData>> &bl,
      Index stepsmax):
m_id(i),
m_x(pos),
m_v(Vector3(1,0,0)), // keep large enough so that particle moves initially
m_stp(0),
m_block(nullptr),
m_el(InvalidIndex),
m_ingrid(true),
m_searchBlock(true),
m_integrator(h,hmin,hmax,errtol,int_mode,this)
{
   if (findCell(bl)) {
      m_integrator.hInit();
   }
}

Particle::~Particle(){}

bool Particle::isActive(){

    return m_ingrid && (m_block || m_searchBlock);
}

bool Particle::inGrid(){

    return m_ingrid;
}

bool Particle::isMoving(Index maxSteps, Scalar minSpeed) {

    if (!m_ingrid) {
       return false;
    }

    bool moving = m_v.norm() > minSpeed;
    if(m_stp > maxSteps || !moving){

       PointsToLines();
       this->Deactivate();
       return false;
    }

    return true;
}

bool Particle::findCell(const std::vector<std::unique_ptr<BlockData>> &block) {

    if (!m_ingrid) {
        return false;
    }

    if (m_block) {

        UnstructuredGrid::const_ptr grid = m_block->getGrid();
        if(grid->inside(m_el, m_x)){
            return true;
        }
        times::celloc_start = times::start();
        m_el = grid->findCell(m_x);
        times::celloc_dur += times::stop(times::celloc_start);
        if (m_el!=InvalidIndex) {
            return true;
        } else {
            PointsToLines();
            m_searchBlock = true;
        }
    }

    if (m_searchBlock) {
        m_searchBlock = false;
        for(Index i=0; i<block.size(); i++) {

            UnstructuredGrid::const_ptr grid = block[i]->getGrid();
            if (block[i].get() == m_block) {
                // don't try previous block again
                m_block = nullptr;
                continue;
            }
            times::celloc_start = times::start();
            m_el = grid->findCell(m_x);
            times::celloc_dur += times::stop(times::celloc_start);
            if (m_el!=InvalidIndex) {

                m_block = block[i].get();
                return true;
            }
        }
        m_block = nullptr;
    }

    return false;
}

void Particle::PointsToLines(){

    m_block->addLines(m_id,m_xhist,m_vhist,m_pressures,m_steps);

    m_xhist.clear();
    m_vhist.clear();
    m_pressures.clear();
    m_steps.clear();
}

void Particle::Deactivate(){

    m_block = nullptr;
    m_ingrid = false;
}

void Particle::EmitData(bool havePressure) {

   m_xhist.push_back(m_xold);
   m_vhist.push_back(m_v);
   m_steps.push_back(m_stp);
   if (havePressure)
      m_pressures.push_back(m_p); // will be ignored later on
}

bool Particle::Step() {

   const auto &grid = m_block->getGrid();
   const auto inter = grid->getInterpolator(m_el, m_x);
   m_v = inter(m_block->m_vx, m_block->m_vy, m_block->m_vz);
   if (m_block->m_p) {
      m_p = inter(m_block->m_p);
   }

   m_xold = m_x;

   EmitData(m_block->m_p);

   bool ret = m_integrator.Step();
   ++m_stp;
   return ret;
}

void Particle::Communicator(boost::mpi::communicator mpi_comm, int root, bool havePressure){

    boost::mpi::broadcast(mpi_comm, m_x, root);
    boost::mpi::broadcast(mpi_comm, m_xold, root);
    boost::mpi::broadcast(mpi_comm, m_v, root);
    boost::mpi::broadcast(mpi_comm, m_stp, root);
    if(havePressure)
       boost::mpi::broadcast(mpi_comm, m_p, root);
    boost::mpi::broadcast(mpi_comm, m_ingrid, root);
    boost::mpi::broadcast(mpi_comm, m_integrator.m_h, root);

    if (mpi_comm.rank()!=root) {

        m_searchBlock = true;
    }
}


bool Tracer::prepare(){

    m_havePressure = true;

    grid_in.clear();
    celltree.clear();
    data_in0.clear();
    data_in1.clear();

    return true;
}


bool Tracer::compute() {

    bool useCelltree = m_useCelltree->getValue();
    auto data0 = expect<Vec<Scalar, 3>>("data_in0");
    auto data1 = accept<Vec<Scalar>>("data_in1");

    if (!data0)
       return true;
    auto grid = UnstructuredGrid::as(data0->grid());
    if (!grid) {
        sendError("grid attachment required at data_in0");
        return true;
    }

    int t = data0->getTimestep();
    if (grid->getTimestep() >= 0) {
       if (t != grid->getTimestep()) {
          std::cerr << "timestep mismatch: grid=" << grid->getTimestep() << ", velocity=" << t << std::endl;
       }
    }
    if (t < 0)
       t = 0;

    const size_t numSteps = t+1;
    if (grid_in.size() < numSteps) {
       grid_in.resize(numSteps);
       celltree.resize(numSteps);
       data_in0.resize(numSteps);
       data_in1.resize(numSteps);
    }

    if (useCelltree) {
       celltree[t].emplace_back(std::async(std::launch::async, [grid]() -> Celltree3::const_ptr { return grid->getCelltree(); }));
    }

    grid_in[t].push_back(grid);
    data_in0[t].push_back(data0);
    data_in1[t].push_back(data1);
    if (!data1)
        m_havePressure = false;

    return true;
}




bool Tracer::reduce(int timestep) {

   times::celloc_dur =0;
   times::integr_dur =0;
   times::interp_dur =0;
   times::comm_dur =0;
   times::total_dur=0;
   times::no_interp =0;
   times::total_start = times::start();

   //get parameters
   bool useCelltree = m_useCelltree->getValue();
   Index numpoints = getIntParameter("no_startp");
   Index steps_max = getIntParameter("steps_max");

   //determine startpoints
   std::vector<Vector3> startpoints(numpoints);
   Vector3 startpoint1 = getVectorParameter("startpoint1");
   Vector3 startpoint2 = getVectorParameter("startpoint2");
   Vector3 delta = (startpoint2-startpoint1)/(numpoints-1);
   for(Index i=0; i<numpoints; i++){

      startpoints[i] = startpoint1 + i*delta;
   }

   IntegrationMethod int_mode = (IntegrationMethod)getIntParameter("integration");
   Scalar dt = getFloatParameter("h_euler");
   Scalar dtmin = getFloatParameter("h_min");
   Scalar dtmax = getFloatParameter("h_max");
   Scalar errtol = getFloatParameter("err_tol");
   Scalar commthresh = getFloatParameter("comm_threshold");
   Scalar minspeed = getFloatParameter("min_speed");

   Index numtime = grid_in.size();
   for (Index t=0; t<numtime; ++t) {
      Index numblocks = grid_in[t].size();

      //create BlockData objects
      std::vector<std::unique_ptr<BlockData>> block(numblocks);
      for(Index i=0; i<numblocks; i++){

         if (useCelltree) {
            celltree[t][i].get();
         }
         block[i].reset(new BlockData(i, grid_in[t][i], data_in0[t][i], data_in1[t][i]));
      }

      Index numActive = numpoints;

      //create particle objects
      std::vector<std::unique_ptr<Particle>> particle(numpoints);
      for(Index i=0; i<numpoints; i++){

         particle[i].reset(new Particle(i, startpoints[i],dt,dtmin,dtmax,errtol,int_mode,block,steps_max));

         times::comm_start = times::start();
         bool active = boost::mpi::all_reduce(comm(), particle[i]->isActive(), std::logical_or<bool>());
         times::comm_dur += times::stop(times::comm_start);
         if(!active){
            particle[i]->Deactivate();
            --numActive;
         }
      }
      const int mpisize = comm().size();

      while(numActive > 0) {

         std::vector<Index> sendlist;

//#pragma omp parallel for
         // trace local particles
         for(Index i=0; i<numpoints; i++) {
            bool traced = false;
            while(particle[i]->isMoving(steps_max, minspeed)
                  && particle[i]->findCell(block)) {
               double celloc_old = times::celloc_dur;
               double integr_old = times::integr_dur;
               times::integr_start = times::start();
               particle[i]->Step();
               times::integr_dur += times::stop(times::integr_start)-(times::celloc_dur-celloc_old)-(times::integr_dur-integr_old);
               traced = true;
            }
            if(traced) {
//#pragma omp critical
               sendlist.push_back(i);
               if (commthresh > 1.) {
                  if (sendlist.size() >= commthresh)
                     break;
               } else {
                  if (sendlist.size() >= commthresh*numActive)
                     break;
               }
            }
         }

         // communicate
         Index num_send = sendlist.size();
         std::vector<Index> num_transmit(mpisize);
         boost::mpi::all_gather(comm(), num_send, num_transmit);
         for(int mpirank=0; mpirank<mpisize; mpirank++) {
            times::comm_start = times::start();
            times::comm_dur += times::stop(times::comm_start);
            Index num_recv = num_transmit[mpirank];
            if(num_recv>0) {
               std::vector<Index> tmplist = sendlist;
               times::comm_start = times::start();
               boost::mpi::broadcast(comm(), tmplist, mpirank);
               times::comm_dur += times::stop(times::comm_start);
               for(Index i=0; i<num_recv; i++){
                  times::comm_start = times::start();
                  Index p_index = tmplist[i];
                  particle[p_index]->Communicator(comm(), mpirank, m_havePressure);
                  times::comm_dur += times::stop(times::comm_start);
               }
               if (mpirank != rank()) {
                  for(Index i=0; i<num_recv; i++){
                     Index p_index = tmplist[i];
                     if (particle[p_index]->isMoving(steps_max, minspeed)
                         && particle[p_index]->findCell(block)) {
                        // if the particle trajectory continues in this block, repeat last data point from previous block
                        particle[p_index]->EmitData(m_havePressure);
                     }
                  }
               }
               for(Index i=0; i<num_recv; i++){
                  Index p_index = tmplist[i];
                  times::comm_start = times::start();
                  bool active = particle[p_index]->isActive();
                  active = boost::mpi::all_reduce(comm(), particle[p_index]->isActive(), std::logical_or<bool>());
                  times::comm_dur += times::stop(times::comm_start);
                  if(!active){
                     particle[p_index]->Deactivate();
                     --numActive;
                  }
               }
            }
         }
      }

      //add Lines-objects to output port
      for(Index i=0; i<numblocks; i++){

         Meta meta(i, t);
         meta.setNumTimesteps(grid_in.size());
         meta.setNumBlocks(numblocks);

         block[i]->setMeta(meta);
         Lines::ptr lines = block[i]->getLines();
         block[i]->ids()->setGrid(lines);
         addObject("particle_id", block[i]->ids());
         block[i]->steps()->setGrid(lines);
         addObject("timestep", block[i]->steps());

         std::vector<Vec<Scalar, 3>::ptr> v_vec = block[i]->getIplVec();
         Vec<Scalar, 3>::ptr v;
         if(v_vec.size()>0){
            v = v_vec[0];
            v->setGrid(lines);
            addObject("data_out0", v);
         }

         if(data_in1[t][0]){
            std::vector<Vec<Scalar>::ptr> p_vec = block[i]->getIplScal();
            Vec<Scalar>::ptr p;
            if(p_vec.size()>0){
               p = p_vec[0];
               p->setGrid(lines);
               addObject("data_out1", p);
            }
         }
      }
   }

   comm().barrier();
   times::total_dur = times::stop(times::total_start);
   times::output();
   return true;
}
