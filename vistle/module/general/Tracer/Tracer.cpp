#include "Tracer.h"
#include "BlockData.h"
#include "Particle.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <boost/mpi/collectives/all_reduce.hpp>
#include <boost/mpi/collectives/all_gather.hpp>
#include <boost/mpi/collectives/broadcast.hpp>
#include <boost/mpi/operations.hpp>
#include <core/vec.h>
#include <core/paramvector.h>
#include <core/lines.h>
#include <core/unstr.h>
#include <core/structuredgridbase.h>
#include <core/structuredgrid.h>

#ifdef TIMING
#include "TracerTimes.h"
#endif

MODULE_MAIN(Tracer)


using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(StartStyle,
      (Line)
      (Plane)
      (Cylinder)
)

DEFINE_ENUM_WITH_STRING_CONVERSIONS(TraceDirection,
      (Both)
      (Forward)
      (Backward)
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
    createOutputPort("step");
    createOutputPort("time");
    createOutputPort("distance");

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
    addVectorParameter("direction", "direction for plane", ParamVector(1,0,0));
    addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange("no_startp", (Integer)0, (Integer)10000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    auto tl = addFloatParameter("trace_len", "maximum trace distance", 1.0);
    setParameterMinimum(tl, 0.0);
    IntParameter* tasktype = addIntParameter("taskType", "task type", 0, Parameter::Choice);
    std::vector<std::string> taskchoices;
    taskchoices.push_back("streamlines");
    setParameterChoices(tasktype, taskchoices);
    IntParameter *traceDirection = addIntParameter("tdirection", "direction in which to trace", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(traceDirection, TraceDirection);
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
    auto grid = data0->grid();
    auto unstr = UnstructuredGrid::as(grid);
    auto strb = StructuredGridBase::as(grid);
    if (!strb && !unstr) {
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
       if (unstr) {
           celltree[t].emplace_back(std::async(std::launch::async, [unstr]() -> Celltree3::const_ptr { return unstr->getCelltree(); }));
       } else if(auto str = StructuredGrid::as(grid)) {
           celltree[t].emplace_back(std::async(std::launch::async, [str]() -> Celltree3::const_ptr { return str->getCelltree(); }));
       }
    }

    grid_in[t].push_back(grid);
    data_in0[t].push_back(data0);
    data_in1[t].push_back(data1);
    if (!data1)
        m_havePressure = false;

    return true;
}




bool Tracer::reduce(int timestep) {

#ifdef TIMING
   times::celloc_dur =0;
   times::integr_dur =0;
   times::interp_dur =0;
   times::comm_dur =0;
   times::total_dur=0;
   times::no_interp =0;
   times::total_start = times::start();
#endif

   //get parameters
   bool useCelltree = m_useCelltree->getValue();
   TraceDirection traceDirection = (TraceDirection)getIntParameter("tdirection");
   Index steps_max = getIntParameter("steps_max");
   Index numpoints = getIntParameter("no_startp");
   Scalar trace_len = getFloatParameter("trace_len");

   //determine startpoints
   std::vector<Vector3> startpoints;
   StartStyle startStyle = (StartStyle)getIntParameter("startStyle");
   Vector3 startpoint1 = getVectorParameter("startpoint1");
   Vector3 startpoint2 = getVectorParameter("startpoint2");
   Vector3 direction = getVectorParameter("direction");
   if (startStyle == Plane) {
       direction.normalize();
       Vector3 v = startpoint2-startpoint1;
       Scalar l = direction.dot(v);
       Vector3 v0 = direction*l;
       Vector3 v1 = v-v0;
       Scalar r = v0.norm();
       Scalar s = v1.norm();
       Index n0, n1;
       if (r > s) {
           n1 = Index(sqrt(numpoints*s/r))+1;
           if (n1 <= 1)
               n1 = 2;
           n0 = numpoints / n1;
           if (n0 <= 1)
               n0 = 2;
       } else {
           n0 = Index(sqrt(numpoints*r/s))+1;
           if (n0 <= 1)
               n0 = 2;
           n1 = numpoints / n0;
           if (n1 <= 1)
               n1 = 2;
       }
       numpoints = n0*n1;
       setIntParameter("no_startp", numpoints);
       startpoints.resize(numpoints);

       Scalar s0 = Scalar(1)/(n0-1);
       Scalar s1 = Scalar(1)/(n1-1);
       for (Index i=0; i<n0; ++i) {
           for (Index j=0; j<n1; ++j) {
               startpoints[i*n1+j] = startpoint1 + v0*s0*i + v1*s1*j;
           }
       }
   } else {
       startpoints.resize(numpoints);

       Vector3 delta = (startpoint2-startpoint1)/(numpoints-1);
       for(Index i=0; i<numpoints; i++){

           startpoints[i] = startpoint1 + i*delta;
       }
   }
   Index numparticles = numpoints;
   if (traceDirection == Both) {
       numparticles *= 2;
       startpoints.resize(numparticles);

       for(Index i=0; i<numpoints; i++){
           startpoints[i+numpoints] = startpoints[i];
       }
   }

   IntegrationMethod int_mode = (IntegrationMethod)getIntParameter("integration");
   Scalar dt = getFloatParameter("h_euler");
   Scalar dtmin = getFloatParameter("h_min");
   Scalar dtmax = getFloatParameter("h_max");
   Scalar errtol = getFloatParameter("err_tol");
   Scalar commthresh = getFloatParameter("comm_threshold");
   Scalar minspeed = getFloatParameter("min_speed");

   bool havePressure = boost::mpi::all_reduce(comm(), m_havePressure, std::logical_and<bool>());
   Index numtime = boost::mpi::all_reduce(comm(), grid_in.size(), [](Index a, Index b){ return std::max<Index>(a,b); });
   for (Index t=0; t<numtime; ++t) {
       Index numblocks = t>=grid_in.size() ? 0 : grid_in[t].size();

       //create BlockData objects
       std::vector<std::unique_ptr<BlockData>> blocks(numblocks);
       for(Index i=0; i<numblocks; i++){

           if (useCelltree && celltree.size() > t) {
               if (celltree[t].size() > i)
                   celltree[t][i].get();
           }
           blocks[i].reset(new BlockData(i, grid_in[t][i], data_in0[t][i], data_in1[t][i]));
       }

       //create particle objects, 2 if traceDirecton==Both
       std::vector<std::unique_ptr<Particle>> particle(numparticles);
       Index i = 0;
       if (traceDirection != Backward) {
           for(; i<numpoints; i++){

               particle[i].reset(new Particle(i, startpoints[i],dt,dtmin,dtmax,errtol,int_mode,blocks,steps_max, true));
           }
       }
       if (traceDirection != Forward) {
           for(; i<numparticles; i++){

               particle[i].reset(new Particle(i, startpoints[i],dt,dtmin,dtmax,errtol,int_mode,blocks,steps_max, false));
           }
       }

       Index numActive = numparticles;
       for(Index i=0; i<numparticles; ++i) {
           particle[i]->enableCelltree(useCelltree);

#ifdef TIMING
           times::comm_start = times::start();
#endif
           bool active = boost::mpi::all_reduce(comm(), particle[i]->isActive(), std::logical_or<bool>());
#ifdef TIMING
           times::comm_dur += times::stop(times::comm_start);
#endif
           if(!active) {
               particle[i]->Deactivate(Particle::InitiallyOutOfDomain);
               --numActive;
           }
       }

       const int mpisize = comm().size();
       //trace particles until none remain active
       while(numActive > 0) {

           std::vector<Index> sendlist;

           #pragma omp parallel for schedule(dynamic,1)
           // trace local particles
           for (Index i=0; i<numparticles; i++) {
               bool traced = false;
               while(particle[i]->isMoving(steps_max, trace_len, minspeed)
                     && particle[i]->findCell(blocks)) {
#ifdef TIMING
                   double celloc_old = times::celloc_dur;
                   double integr_old = times::integr_dur;
                   times::integr_start = times::start();
#endif
                   particle[i]->Step();
#ifdef TIMING
                   times::integr_dur += times::stop(times::integr_start)-(times::celloc_dur-celloc_old)-(times::integr_dur-integr_old);
#endif
                   traced = true;
               }
               if(traced) {
#ifdef _OPENMP
                   #pragma omp critical
                   sendlist.push_back(i);
#else
                   sendlist.push_back(i);
                   if (commthresh > 1.) {
                       if (sendlist.size() >= commthresh)
                           break;
                   } else {
                       if (sendlist.size() >= commthresh*numActive)
                           break;
                   }
#endif
               }
           }

           // communicate
           Index num_send = sendlist.size();
           std::vector<Index> num_transmit(mpisize);
           boost::mpi::all_gather(comm(), num_send, num_transmit);
           for(int mpirank=0; mpirank<mpisize; mpirank++) {
#ifdef TIMING
               times::comm_start = times::start();
               times::comm_dur += times::stop(times::comm_start);
#endif
               Index num_recv = num_transmit[mpirank];
               if(num_recv>0) {
                   std::vector<Index> tmplist = sendlist;
#ifdef TIMING
                   times::comm_start = times::start();
#endif
                   boost::mpi::broadcast(comm(), tmplist, mpirank);
#ifdef TIMING
                   times::comm_dur += times::stop(times::comm_start);
#endif
                   for(Index i=0; i<num_recv; i++){
#ifdef TIMING
                       times::comm_start = times::start();
#endif
                       Index p_index = tmplist[i];
                       particle[p_index]->Communicator(comm(), mpirank, havePressure);
#ifdef TIMING
                       times::comm_dur += times::stop(times::comm_start);
#endif
                   }
                   if (mpirank != rank()) {
                       for(Index i=0; i<num_recv; i++){
                           Index p_index = tmplist[i];
                           if (particle[p_index]->isMoving(steps_max, trace_len, minspeed)
                                   && particle[p_index]->findCell(blocks)) {
                               // if the particle trajectory continues in this block, repeat last data point from previous block
                               particle[p_index]->EmitData(havePressure);
                           }
                       }
                   }
                   for(Index i=0; i<num_recv; i++){
                       Index p_index = tmplist[i];
#ifdef TIMING
                       times::comm_start = times::start();
#endif
                       bool active = particle[p_index]->isActive();
                       active = boost::mpi::all_reduce(comm(), particle[p_index]->isActive(), std::logical_or<bool>());
#ifdef TIMING
                       times::comm_dur += times::stop(times::comm_start);
#endif
                       if(!active){
                           particle[p_index]->Deactivate(Particle::OutOfDomain);
                           --numActive;
                       }
                   }
               }
           }
       }

       if (rank() == 0) {
           std::vector<Index> stopReasonCount(Particle::NumStopReasons, 0);
           for(Index i=0; i<numparticles; i++) {
               ++stopReasonCount[particle[i]->stopReason()];
           }
           std::stringstream str;
           str << "Stop stats for timestep " << t << ":";
           for (size_t i=0; i<stopReasonCount.size(); ++i) {
               str << " " << Particle::toString((Particle::StopReason)i) << ":" << stopReasonCount[i];
           }
           std::string s = str.str();
           sendInfo("%s", s.c_str());
       }

      //add Lines-objects to output port
      for(Index i=0; i<numblocks; i++){

         Meta meta(i, t);
         meta.setNumTimesteps(numtime);
         meta.setNumBlocks(numblocks);

         blocks[i]->setMeta(meta);
         Lines::ptr lines = blocks[i]->getLines();
         blocks[i]->ids()->setGrid(lines);
         addObject("particle_id", blocks[i]->ids());
         blocks[i]->steps()->setGrid(lines);
         addObject("step", blocks[i]->steps());
         blocks[i]->times()->setGrid(lines);
         addObject("time", blocks[i]->times());
         blocks[i]->distances()->setGrid(lines);
         addObject("distance", blocks[i]->distances());

         std::vector<Vec<Scalar, 3>::ptr> v_vec = blocks[i]->getIplVec();
         Vec<Scalar, 3>::ptr v;
         if(v_vec.size()>0){
            v = v_vec[0];
            v->setGrid(lines);
            addObject("data_out0", v);
         }

         if(data_in1[t][0]){
            std::vector<Vec<Scalar>::ptr> p_vec = blocks[i]->getIplScal();
            Vec<Scalar>::ptr p;
            if(p_vec.size()>0){
               p = p_vec[0];
               p->setGrid(lines);
               addObject("data_out1", p);
            }
         }
      }
   }

   grid_in.clear();
   celltree.clear();
   data_in0.clear();
   data_in1.clear();

#ifdef TIMING
   comm().barrier();
   times::total_dur = times::stop(times::total_start);
   times::output();
#endif
   return true;
}
