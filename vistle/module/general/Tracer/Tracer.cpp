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
    setParameterRange("no_startp", (Integer)1, (Integer)100000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    auto tl = addFloatParameter("trace_len", "maximum trace distance", 1.0);
    setParameterMinimum(tl, 0.0);
    IntParameter* tasktype = addIntParameter("taskType", "task type", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(tasktype, TraceType);
    IntParameter *traceDirection = addIntParameter("tdirection", "direction in which to trace", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(traceDirection, TraceDirection);
    IntParameter *startStyle = addIntParameter("startStyle", "initial particle position configuration", (Integer)Line, Parameter::Choice);
    V_ENUM_SET_CHOICES(startStyle, StartStyle);
    IntParameter* integration = addIntParameter("integration", "integration method", (Integer)RK32, Parameter::Choice);
    V_ENUM_SET_CHOICES(integration, IntegrationMethod);
    addFloatParameter("min_speed", "miniumum particle speed", 1e-4);
    addFloatParameter("h_init", "initial step size/fixed step size for euler integration", 1e-03);
    addFloatParameter("h_min","minimum step size for rk32 integration", 1e-04);
    addFloatParameter("h_max", "maximum step size for rk32 integration", .5);
    addFloatParameter("err_tol", "desired accuracy for rk32 integration", 1e-07);
    addIntParameter("cell_relative", "whether step length control should take into account cell size", 1, Parameter::Boolean);
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

    if (getIntParameter("integration") == ConstantVelocity) {
        // initialize VertexOwnerList
        if (unstr) {
            unstr->getNeighborElements(InvalidIndex);
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

   //get parameters
   bool useCelltree = m_useCelltree->getValue();
   TraceDirection traceDirection = (TraceDirection)getIntParameter("tdirection");
   Index steps_max = getIntParameter("steps_max");
   Index numpoints = getIntParameter("no_startp");

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

       if (numpoints == 1) {
           startpoints[0] = (startpoint1+startpoint2)*0.5;
       } else {
           Vector3 delta = (startpoint2-startpoint1)/(numpoints-1);
           for(Index i=0; i<numpoints; i++){

               startpoints[i] = startpoint1 + i*delta;
           }
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

   Index numtime = boost::mpi::all_reduce(comm(), grid_in.size(), [](Index a, Index b){ return std::max<Index>(a,b); });

   Scalar dt = getFloatParameter("h_init");
   Scalar dtmin = getFloatParameter("h_min");
   Scalar dtmax = getFloatParameter("h_max");
   Scalar errtol = getFloatParameter("err_tol");
   Scalar minspeed = getFloatParameter("min_speed");


   GlobalData global;
   global.int_mode = (IntegrationMethod)getIntParameter("integration");
   global.task_type = (TraceType)getIntParameter("taskType");
   global.h_init = dt;
   global.h_min = dtmin;
   global.h_max = dtmax;
   global.errtol = errtol;
   global.trace_len = getFloatParameter("trace_len");
   global.min_vel = minspeed;
   global.max_step = steps_max;
   global.cell_relative = getIntParameter("cell_relative");
   global.blocks.resize(numtime);

   Index totalParticles = 0;
   std::vector<Index> stopReasonCount(Particle::NumStopReasons, 0);
   for (Index t=0; t<numtime; ++t) {
       Index numblocks = t>=grid_in.size() ? 0 : grid_in[t].size();

       //create BlockData objects
       global.blocks[t].resize(numblocks);
       auto &blocks = global.blocks[t];
       for(Index i=0; i<numblocks; i++){

           if (useCelltree && celltree.size() > t) {
               if (celltree[t].size() > i)
                   celltree[t][i].get();
           }
           global.blocks[t][i].reset(new BlockData(i, grid_in[t][i], data_in0[t][i], data_in1[t][i]));
       }

       //create particle objects, 2 if traceDirecton==Both
       std::vector<std::shared_ptr<Particle>> allParticles;
       allParticles.reserve(numparticles);
       Index i = 0;
       if (traceDirection != Backward) {
           for(; i<numpoints; i++){
               allParticles.emplace_back(new Particle(i, startpoints[i], true, global, t));
           }
       }
       if (traceDirection != Forward) {
           for(; i<numparticles; i++){
               allParticles.emplace_back(new Particle(i, startpoints[i], false, global, t));
           }
       }


       std::set<Index> checkSet; // set of particles to check for deactivation because out of domain
       std::set<std::shared_ptr<Particle>> activeParticles;
       for(auto it = allParticles.begin(); it!= allParticles.end(); ++it) {
           auto particle = it->get();
           particle->enableCelltree(useCelltree);
           ++totalParticles;

           bool active = boost::mpi::all_reduce(comm(), particle->isActive(), std::logical_or<bool>());
           if (active) {
               activeParticles.insert(*it);
               particle->startTracing();
               checkSet.insert(particle->id());
           } else {
               particle->Deactivate(Particle::InitiallyOutOfDomain);
           }
       }

       auto checkActivity = [&checkSet, &allParticles](const boost::mpi::communicator &comm) -> Index {
           for (auto it = checkSet.begin(), next=it; it != checkSet.end(); it=next) {
               ++next;

               auto particle = allParticles[*it];
               bool active = boost::mpi::all_reduce(comm, particle->isActive(), std::logical_or<bool>());
               if (!active) {
                   particle->Deactivate(Particle::OutOfDomain);
                   checkSet.erase(it);
               }
           }
           return checkSet.size();
       };

       const int mpisize = comm().size();
       bool working = activeParticles.size()>0;
       //trace particles until none remain active
       while (working) {
           working = false;
           bool first = true;
           std::vector<Index> sendlist;
           for(auto it = activeParticles.begin(), next=it; it!= activeParticles.end(); it=next) {
               next = it;
               ++next;

               auto particle = it->get();
               if (!particle->inGrid())
                   continue;

               bool wait = mpisize==1 && first;
               first = false;
               if (particle->isTracing(wait)) {
                   working = true;
               } else {
                   if (mpisize==1) {
                       particle->Deactivate(Particle::OutOfDomain);
                   } else if (particle->madeProgress()) {
                       sendlist.push_back(particle->id());
                   }
                   activeParticles.erase(it);
               }
           }

           checkActivity(comm());

           if (mpisize==1)
               continue;

           // communicate
           Index num_send = sendlist.size();
           std::vector<Index> num_transmit(mpisize);
           boost::mpi::all_gather(comm(), num_send, num_transmit);
           for(int mpirank=0; mpirank<mpisize; mpirank++) {
               Index num_recv = num_transmit[mpirank];
               if(num_recv>0) {
                   std::vector<Index> recvlist;
                   auto &curlist = rank()==mpirank ? sendlist : recvlist;
                   boost::mpi::broadcast(comm(), curlist, mpirank);
                   for(Index i=0; i<num_recv; i++){
                       Index p_index = curlist[i];
                       auto &p = allParticles[p_index];
                       // wait until former (fruitless) tracing attempts are finished
                       while(p->isTracing(true))
                           ;
                       p->broadcast(comm(), mpirank);
                       checkSet.insert(p_index);
                       if (rank() != mpirank) {
                           activeParticles.insert(p);
                           p->startTracing();
                           working = true;
                       }
                   }
               }
           }
           working = boost::mpi::all_reduce(comm(), working, std::logical_or<bool>());
       }

       checkActivity(comm());

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

      if (rank() == 0) {
          for(Index i=0; i<numparticles; i++) {
               ++stopReasonCount[allParticles[i]->stopReason()];
           }
      }
   }

   if (rank() == 0) {
       std::stringstream str;
       str << "Stop stats for " << totalParticles << " particles:";
       for (size_t i=0; i<stopReasonCount.size(); ++i) {
           str << " " << Particle::toString((Particle::StopReason)i) << ":" << stopReasonCount[i];
       }
       std::string s = str.str();
       sendInfo("%s", s.c_str());
   }

   grid_in.clear();
   celltree.clear();
   data_in0.clear();
   data_in1.clear();

   return true;
}
