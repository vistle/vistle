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

const vistle::Scalar Epsilon = 1e-8;

MODULE_MAIN(Tracer)


using namespace vistle;


Tracer::Tracer(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Tracer", shmname, name, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheAll);
   setReducePolicy(message::ReducePolicy::OverAll);

    createInputPort("grid_in");
    createInputPort("data_in0");
    createInputPort("data_in1");
    createOutputPort("geom_out");
    createOutputPort("data_out0");
    createOutputPort("data_out1");

    addVectorParameter("startpoint1", "1st initial point", ParamVector(0,0.2,0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1,0,0));
    addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange("no_startp", (Integer)0, (Integer)10000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    IntParameter* tasktype = addIntParameter("taskType", "task type", 0, Parameter::Choice);
    std::vector<std::string> taskchoices;
    taskchoices.push_back("streamlines");
    setParameterChoices(tasktype, taskchoices);
    IntParameter* integration = addIntParameter("integration", "integration method", 0, Parameter::Choice);
    std::vector<std::string> integrchoices(2);
    integrchoices[0] = "rk32";
    integrchoices[1] = "euler";
    setParameterChoices(integration,integrchoices);
    addFloatParameter("h_min","minimum step size for rk32 integration", 1e-05);
    addFloatParameter("h_max", "maximum step size for rk32 integration", 1e-02);
    addFloatParameter("err_tol", "desired accuracy for rk32 integration", 1e-06);
    addFloatParameter("h_euler", "fixed step size for euler integration", 1e-03);
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
m_vx(nullptr),
m_vy(nullptr),
m_vz(nullptr),
m_p(nullptr)
{

   if (m_vecfld) {
      m_vx = m_vecfld->x().data();
      m_vy = m_vecfld->y().data();
      m_vz = m_vecfld->z().data();
   }

   if (m_scafld) {
      m_p = m_scafld->x().data();
   }
}

BlockData::~BlockData(){}

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

void BlockData::addLines(const std::vector<Vector3> &points,
             const std::vector<Vector3> &velocities,
             const std::vector<Scalar> &pressures) {

   Index numpoints = points.size();
   assert(numpoints == velocities.size());
   assert(!m_p || pressures.size()==numpoints);

   if(m_ivec.empty()) {
      m_ivec.emplace_back(new Vec<Scalar, 3>(Object::Initialized));
   }

   if(m_iscal.empty() && m_p) {
      m_iscal.emplace_back(new Vec<Scalar>(Object::Initialized));
   }

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
   }
   Index numcorn = m_lines->getNumCorners();
   m_lines->el().push_back(numcorn);
}


Particle::Particle(Index i, const Vector3 &pos, Scalar h, Scalar hmin,
      Scalar hmax, Scalar errtol, int int_mode,const std::vector<std::unique_ptr<BlockData>> &bl,
      Index stepsmax):
m_x(pos),
m_v(Vector3(1,0,0)), // keep large enough so that particle moves initially
m_stp(0),
m_block(nullptr),
m_el(InvalidIndex),
m_ingrid(true),
m_init(true),
m_integrator(h,hmin,hmax,errtol,int_mode,this),
m_stpmax(stepsmax) {

   if(findCell(bl)) {
      if(int_mode==0) {
         m_integrator.hInit();
      }
   }
}

Particle::~Particle(){}

bool Particle::isActive(){

    return m_ingrid && m_block;
}

bool Particle::inGrid(){

    return m_ingrid;
}

bool Particle::findCell(const std::vector<std::unique_ptr<BlockData>> &block){

    if(!(m_block||m_init) || !m_ingrid){
        return false;
    }

    bool moving = m_v.norm() > Epsilon;
    if(m_stp == m_stpmax || !moving){

        PointsToLines();
        this->Deactivate();
        m_ingrid = false;
        return false;
    }

    if(m_block) {

        UnstructuredGrid::const_ptr grid = m_block->getGrid();
        if(grid->inside(m_el, m_x)){
            return true;
        }
        times::celloc_start = times::start();
        m_el = grid->findCell(m_x);
        times::celloc_dur += times::stop(times::celloc_start);
        if(m_el!=InvalidIndex) {
            return true;
        }
        else
        {
            PointsToLines();
            m_block = nullptr;
            m_init = true;
        }
    }

    if(m_init) {
        m_init = false;
        for(Index i=0; i<block.size(); i++) {

            UnstructuredGrid::const_ptr grid = block[i]->getGrid();
            times::celloc_start = times::start();
            m_el = grid->findCell(m_x);
            times::celloc_dur += times::stop(times::celloc_start);
            if(m_el!=InvalidIndex) {

                m_block = block[i].get();
                return true;
            }
        }
    }

    return false;
}

void Particle::PointsToLines(){

    m_block->addLines(m_xhist,m_vhist,m_pressures);

    m_xhist.clear();
    m_vhist.clear();
    m_pressures.clear();
}

void Particle::Deactivate(){

    m_block = nullptr;
    m_ingrid = false;
}

bool Particle::Step() {

    bool ret = m_integrator.Step();
    m_stp++;
    return ret;
}

void Particle::Communicator(boost::mpi::communicator mpi_comm, int root){

    boost::mpi::broadcast(mpi_comm, m_x, root);
    boost::mpi::broadcast(mpi_comm, m_stp, root);
    boost::mpi::broadcast(mpi_comm, m_ingrid, root);
    boost::mpi::broadcast(mpi_comm, m_integrator.m_h, root);

    if(mpi_comm.rank()!=root){

        m_init = true;
    }
}


bool Tracer::prepare(){

    grid_in.clear();
    celltree.clear();
    data_in0.clear();
    data_in1.clear();

    return true;
}


bool Tracer::compute() {

    bool useCelltree = m_useCelltree->getValue();
    auto grid = expect<UnstructuredGrid>("grid_in");
    auto data0 = expect<Vec<Scalar, 3>>("data_in0");
    auto data1 = accept<Vec<Scalar>>("data_in1");

    if (!grid || !data0)
       return true;

    int t = data0->getTimestep();
    if (grid->getTimestep() >= 0) {
       if (t != grid->getTimestep()) {
          std::cerr << "timestep mismatch: grid=" << grid->getTimestep() << ", velocity=" << t << std::endl;
       }
    }
    if (t < 0)
       t = 0;

    if (grid_in.size() <= t) {
       grid_in.resize(t+1);
       celltree.resize(t+1);
       data_in0.resize(t+1);
       data_in1.resize(t+1);
    }

    if (useCelltree) {
       celltree[t].emplace_back(std::async(std::launch::async, [grid]() -> Celltree3::const_ptr { return grid->getCelltree(); }));
    }

    grid_in[t].push_back(grid);
    data_in0[t].push_back(data0);
    data_in1[t].push_back(data1);

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

   int int_mode = getIntParameter("integration");
   Scalar dt = getFloatParameter("h_euler");
   Scalar dtmin = getFloatParameter("h_min");
   Scalar dtmax = getFloatParameter("h_max");
   Scalar errtol = getFloatParameter("err_tol");


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


      //create particle objects
      std::vector<std::unique_ptr<Particle>> particle(numpoints);
      for(Index i=0; i<numpoints; i++){

         particle[i].reset(new Particle(i, startpoints[i],dt,dtmin,dtmax,errtol,int_mode,block,steps_max));
      }
      times::comm_start = times::start();
      for(Index i=0; i<numpoints; i++){

         bool active = particle[i]->isActive();
         active = boost::mpi::all_reduce(comm(), active, std::logical_or<bool>());
         if(!active){
            particle[i]->Deactivate();
         }
      }
      times::comm_dur += times::stop(times::comm_start);
      bool ingrid = true;
      int mpisize = comm().size();
      while(ingrid){

         std::vector<Index> sendlist;

//#pragma omp parallel for
         for(Index i=0; i<numpoints; i++){
            bool traced = false;
            while(particle[i]->findCell(block)){
               double celloc_old = times::celloc_dur;
               double integr_old = times::integr_dur;
               times::integr_start = times::start();
               particle[i]->Step();
               times::integr_dur += times::stop(times::integr_start)-(times::celloc_dur-celloc_old)-(times::integr_dur-integr_old);
               traced = true;
            }
            if(traced){
//#pragma omp critical
               sendlist.push_back(i);
            }
         }

         for(int mpirank=0; mpirank<mpisize; mpirank++) {
            times::comm_start = times::start();
            Index num_send = sendlist.size();
            boost::mpi::broadcast(comm(), num_send, mpirank);
            times::comm_dur += times::stop(times::comm_start);
            if(num_send>0){
               std::vector<Index> tmplist = sendlist;
               times::comm_start = times::start();
               boost::mpi::broadcast(comm(), tmplist, mpirank);
               times::comm_dur += times::stop(times::comm_start);
               for(Index i=0; i<num_send; i++){
                  times::comm_start = times::start();
                  Index p_index = tmplist[i];
                  particle[p_index]->Communicator(comm(), mpirank);
                  times::comm_dur += times::stop(times::comm_start);
                  particle[p_index]->findCell(block);
                  times::comm_start = times::start();
                  bool active = particle[p_index]->isActive();
                  active = boost::mpi::all_reduce(comm(), particle[p_index]->isActive(), std::logical_or<bool>());
                  times::comm_dur += times::stop(times::comm_start);
                  if(!active){
                     particle[p_index]->Deactivate();
                  }
               }
            }

            ingrid = false;
            for(Index i=0; i<numpoints; i++){

               if(particle[i]->inGrid()){

                  ingrid = true;
                  break;
               }
            }
         }
      }

      //add Lines-objects to output port
      for(Index i=0; i<numblocks; i++){

         Meta meta(i, t);
         meta.setNumTimesteps(grid_in.size());
         meta.setNumBlocks(numblocks);

         Lines::ptr lines = block[i]->getLines();
         lines->setMeta(meta);
         addObject("geom_out", lines);

         std::vector<Vec<Scalar, 3>::ptr> v_vec = block[i]->getIplVec();
         Vec<Scalar, 3>::ptr v;
         if(v_vec.size()>0){
            v = v_vec[0];
            v->setMeta(meta);
            addObject("data_out0", v);
         }

         if(data_in1[t][0]){
            std::vector<Vec<Scalar>::ptr> p_vec = block[i]->getIplScal();
            Vec<Scalar>::ptr p;
            if(p_vec.size()>0){
               p = p_vec[0];
               p->setMeta(meta);
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
