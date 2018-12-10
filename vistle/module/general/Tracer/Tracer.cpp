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
namespace mpi = boost::mpi;

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

template<class Value>
bool agree(const boost::mpi::communicator &comm, const Value &value) {
    int mismatch = 0;

    std::vector<Value> vals(comm.size());
    boost::mpi::all_gather(comm, value, vals);
    for (int i=0; i<comm.size(); ++i) {
        if (value != vals[i])
            ++mismatch;
    }

    const int minmismatch = boost::mpi::all_reduce(comm, mismatch, boost::mpi::minimum<int>());
    int rank = comm.size();
    if (mismatch>0 && mismatch==minmismatch)
        rank = comm.rank();
    const int minrank = boost::mpi::all_reduce(comm, rank, boost::mpi::minimum<int>());
    if (comm.rank() == minrank) {
        std::cerr << "agree on rank " << comm.rank() << ": common value " << value << std::endl;
        for (int i=0; i<comm.size(); ++i) {
            if (vals[i] != value)
                std::cerr << "\t" << "agree on rank " << comm.rank() << ": common value " << value << std::endl;
        }
    }

    return mismatch==0;
}

Tracer::Tracer(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("compute particle traces", name, moduleID, comm) {

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);
   setReducePolicy(message::ReducePolicy::PerTimestep);

    createInputPort("data_in0");
    createInputPort("data_in1");
    createOutputPort("data_out0");
    createOutputPort("data_out1");
    createOutputPort("particle_id");
    createOutputPort("step");
    createOutputPort("time");
    createOutputPort("distance");
    createOutputPort("stop_reason");

#if 0
    const char *TracerInteraction::P_DIRECTION = "direction";
    const char *TracerInteraction::P_TDIRECTION = "tdirection";
    const char *TracerInteraction::P_TASKTYPE = "taskType";
    const char *TracerInteraction::P_STARTSTYLE = "startStyle";
    const char *TracerInteraction::P_TRACE_LEN = "trace_len";
    const char *TracerInteraction::P_FREE_STARTPOINTS = "FreeStartPoints";
#endif

    m_taskType = addIntParameter("taskType", "task type", Streamlines, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_taskType, TraceType);
    addVectorParameter("startpoint1", "1st initial point", ParamVector(0,0.2,0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1,0,0));
    addVectorParameter("direction", "direction for plane", ParamVector(1,0,0));
    const Integer max_no_startp = 300;
    m_numStartpoints = addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange(m_numStartpoints, (Integer)1, max_no_startp);
    m_maxStartpoints = addIntParameter("max_no_startp", "maximum number of startpoints", max_no_startp);
    setParameterRange(m_maxStartpoints, (Integer)2, (Integer)10000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    setParameterRange("steps_max", (Integer)1, (Integer)1000000);
    auto tl = addFloatParameter("trace_len", "maximum trace distance", 1.0);
    setParameterMinimum(tl, 0.0);
    auto tt = addFloatParameter("trace_time", "maximum trace time", 10000.0);
    setParameterMinimum(tt, 0.0);
    IntParameter *traceDirection = addIntParameter("tdirection", "direction in which to trace", Both, Parameter::Choice);
    V_ENUM_SET_CHOICES(traceDirection, TraceDirection);
    IntParameter *startStyle = addIntParameter("startStyle", "initial particle position configuration", (Integer)Line, Parameter::Choice);
    V_ENUM_SET_CHOICES(startStyle, StartStyle);
    IntParameter* integration = addIntParameter("integration", "integration method", (Integer)RK32, Parameter::Choice);
    V_ENUM_SET_CHOICES(integration, IntegrationMethod);
    addFloatParameter("min_speed", "miniumum particle speed", 1e-4);
    setParameterRange("min_speed", 0.0, 1e6);
    addFloatParameter("dt_step", "duration of a timestep (for moving points or when data does not specify realtime", 1./25);
    setParameterRange("dt_step", 0.0, 1e6);

    setCurrentParameterGroup("Step Length Control");
    addFloatParameter("h_init", "initial step size/fixed step size for euler integration", 1e-03);
    setParameterRange("h_init", 0.0, 1e6);
    addFloatParameter("h_min","minimum step size for rk32 integration", 1e-04);
    setParameterRange("h_min", 0.0, 1e6);
    addFloatParameter("h_max", "maximum step size for rk32 integration", .5);
    setParameterRange("h_max", 0.0, 1e6);
    addFloatParameter("err_tol_abs", "absolute error tolerance for rk32 integration", 1e-04);
    setParameterRange("err_tol_abs", 0.0, 1e6);
    addFloatParameter("err_tol_rel", "relative error tolerance for rk32 integration", 1e-03);
    setParameterRange("err_tol_rel", 0.0, 1.0);
    addIntParameter("cell_relative", "whether step length control should take into account cell size", 1, Parameter::Boolean);
    addIntParameter("velocity_relative", "whether step length control should take into account velocity", 1, Parameter::Boolean);

    setCurrentParameterGroup("Performance Tuning");
    m_useCelltree = addIntParameter("use_celltree", "use celltree for accelerated cell location", (Integer)1, Parameter::Boolean);
    auto num_active = addIntParameter("num_active", "number of particles to trace simultaneously on each node (0: no. of cores)", 0);
    setParameterRange(num_active, (Integer)0, (Integer)10000);
}

Tracer::~Tracer() {
}


bool Tracer::prepare(){

    m_havePressure = true;
    m_haveTimeSteps = false;

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
       t = -1;

    if (t >= 0)
        m_haveTimeSteps = true;

    const size_t numSteps = t+1;
    if (grid_in.size() < numSteps+1) {
       grid_in.resize(numSteps+1);
       celltree.resize(numSteps+1);
       data_in0.resize(numSteps+1);
       data_in1.resize(numSteps+1);
    }

    if (useCelltree) {
       if (unstr) {
           celltree[t+1].emplace_back(std::async(std::launch::async, [unstr]() -> Celltree3::const_ptr { return unstr->getCelltree(); }));
       } else if(auto str = StructuredGrid::as(grid)) {
           celltree[t+1].emplace_back(std::async(std::launch::async, [str]() -> Celltree3::const_ptr { return str->getCelltree(); }));
       }
    }

    if (getIntParameter("integration") == ConstantVelocity) {
        // initialize VertexOwnerList
        if (unstr) {
            unstr->getNeighborElements(InvalidIndex);
        }
    }

    grid_in[t+1].push_back(grid);
    data_in0[t+1].push_back(data0);
    data_in1[t+1].push_back(data1);
    if (!data1)
        m_havePressure = false;

    return true;
}




bool Tracer::reduce(int timestep) {

   if (timestep == -1 && numTimesteps()>0 && reducePolicy() == message::ReducePolicy::PerTimestep)
       return true;

   //get parameters
   bool useCelltree = m_useCelltree->getValue();
   Index numpoints = m_numStartpoints->getValue();
   Index maxNumActive = getIntParameter("num_active");
   if (maxNumActive <= 0) {
       maxNumActive = std::thread::hardware_concurrency();
   }
   maxNumActive = mpi::all_reduce(comm(), maxNumActive, mpi::minimum<Index>());
   auto taskType = (TraceType)getIntParameter("taskType");
   TraceDirection traceDirection = (TraceDirection)getIntParameter("tdirection");
   if (taskType != Streamlines) {
       traceDirection = Forward;
   }

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
       //setIntParameter("no_startp", numpoints);
       if (rank() == 0 && numpoints != n0*n1)
           sendInfo("actually using %d*%d=%d start points", n0, n1, n0*n1);
       numpoints = n0*n1;
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
   }

   int numtime = numTimesteps();
#if 0
   std::cerr << "reduce(" << timestep << ") with " << numtime << " steps, #blocks="
             << (timestep+1>grid_in.size() ? 0 : grid_in[timestep+1].size())
             << std::endl;
#endif
   numtime = std::min(timestep+1, numtime);
   if (numtime == 0)
       numtime = 1;


   GlobalData global;
   global.int_mode = (IntegrationMethod)getIntParameter("integration");
   global.task_type = (TraceType)getIntParameter("taskType");
   global.dt_step = getFloatParameter("dt_step");
   global.h_init = getFloatParameter("h_init");
   global.h_min = getFloatParameter("h_min");
   global.h_max = getFloatParameter("h_max");
   global.errtolrel = getFloatParameter("err_tol_rel");
   global.errtolabs = getFloatParameter("err_tol_abs");
   global.use_celltree = getIntParameter("use_celltree");
   global.trace_len = getFloatParameter("trace_len");
   global.trace_time = getFloatParameter("trace_time");
   global.min_vel = getFloatParameter("min_speed");
   global.max_step = getIntParameter("steps_max");
   global.cell_relative = getIntParameter("cell_relative");
   global.velocity_relative = getIntParameter("velocity_relative");
   global.blocks.resize(numtime);

   std::vector<Index> stopReasonCount(Particle::NumStopReasons, 0);
   std::vector<std::shared_ptr<Particle>> allParticles;
   std::set<std::shared_ptr<Particle>> activeParticles;

   std::set<Index> checkSet; // set of particles to check for deactivation because out of domain
   int checkActivityCounter = 0;
   auto checkActivity = [&checkActivityCounter, &checkSet, &allParticles](const mpi::communicator &comm) -> Index {
       agree(comm, checkActivityCounter);
       ++checkActivityCounter;
       for (auto it = checkSet.begin(), next=it; it != checkSet.end(); it=next) {
           ++next;

           auto particle = allParticles[*it];
           const bool active = mpi::all_reduce(comm, particle->isActive(), std::logical_or<bool>());
           if (!active) {
               particle->Deactivate(Particle::OutOfDomain);
               checkSet.erase(it);
           }
       }
       return checkSet.size();
   };

   Index numconstant = grid_in[0].size();
   for (Index i=0; i<numconstant; ++i) {
       if (useCelltree && !celltree.empty()) {
           if (celltree[0].size() > i && celltree[0][i].valid())
               celltree[0][i].get();
       }
   }

   // create particles
   Index id=0;
   for (int t=0; t<numtime; ++t) {
       if (timestep != t && timestep != -1)
           continue;
       Index numblocks = size_t(t)+1>=grid_in.size() ? 0 : grid_in[t+1].size();

       //create BlockData objects
       global.blocks[t].resize(numblocks+numconstant);
       for (Index i=0; i<numconstant; i++) {
           global.blocks[t][i].reset(new BlockData(i, grid_in[0][i], data_in0[0][i], data_in1[0][i]));
       }
       for (Index i=0; i<numblocks; i++) {

           if (useCelltree && celltree.size() > size_t(t+1)) {
               if (celltree[t+1].size() > i && celltree[t+1][i].valid())
                   celltree[t+1][i].get();
           }
           global.blocks[t][i+numconstant].reset(new BlockData(i+numconstant, grid_in[t+1][i], data_in0[t+1][i], data_in1[t+1][i]));
       }

       //create particle objects, 2 if traceDirecton==Both
       allParticles.reserve(allParticles.size()+numparticles);
       Index i = 0;
       for(; i<numpoints; i++) {
           if (traceDirection != Backward) {
               allParticles.emplace_back(new Particle(id++, i%size(), i, startpoints[i], true, global, t));
           }
           if (traceDirection != Forward) {
               allParticles.emplace_back(new Particle(id++, i%size(), i, startpoints[i], false, global, t));
           }
       }
   }

   Index nextParticleToStart = 0;
   auto startParticles = [this, &nextParticleToStart, &allParticles, &activeParticles, &checkSet](int toStart) -> int {
       int started = 0;
       while (nextParticleToStart < allParticles.size()) {
           if (started >= toStart)
               break;

           Index idx = nextParticleToStart;
           ++nextParticleToStart;
           auto particle = allParticles[idx];
           if (particle->startTracing(comm()) >= 0) {
               activeParticles.insert(particle);
               checkSet.insert(particle->id());
               ++started;
           } else {
               particle->Deactivate(Particle::InitiallyOutOfDomain);
           }
       }
       return started;
   };

   const int mpisize = comm().size();
   Index numActiveMax = 0, numActiveMin = std::numeric_limits<Index>::max();
   do {
      Index numStart = numActiveMin>maxNumActive ? maxNumActive : maxNumActive-numActiveMin;
      startParticles(numStart);

      bool first = true;
      std::vector<Index> sendlist;
      // build list of particles to send to their owner
      for(auto it = activeParticles.begin(), next=it; it!= activeParticles.end(); it=next) {
          next = it;
          ++next;

          auto particle = it->get();

          bool wait = mpisize==1 && first;
          first = false;
          if (!particle->isTracing(wait)) {
              if (mpisize==1) {
                  particle->Deactivate(Particle::OutOfDomain);
              } else if (particle->madeProgress()) {
                  sendlist.push_back(particle->id());
              }
              activeParticles.erase(it);
          }
      }

      if (mpisize==1) {
          checkSet.clear();
          numActiveMax = numActiveMin = activeParticles.size();
          continue;
      }

      // communicate
      checkActivity(comm());

      std::vector<std::pair<Index, int>> datarecvlist; // particle id, source mpi rank
      Index num_send = sendlist.size();
      std::vector<Index> num_transmit(mpisize);
      mpi::all_gather(comm(), num_send, num_transmit);
      for(int mpirank=0; mpirank<mpisize; mpirank++) {
          Index num_recv = num_transmit[mpirank];
          if(num_recv>0) {
              std::vector<Index> recvlist;
              auto &curlist = rank()==mpirank ? sendlist : recvlist;
              mpi::broadcast(comm(), curlist, mpirank);
              assert(curlist.size() == num_recv);
              for(Index i=0; i<num_recv; i++){
                  Index p_index = curlist[i];
                  auto p = allParticles[p_index];
                  assert(!p->isTracing(false));
                  p->broadcast(comm(), mpirank);
                  checkSet.insert(p_index);
                  if (p->rank() == rank()) {
                      if (rank() != mpirank) {
                          datarecvlist.emplace_back(p->id(), mpirank);
                      }
                  }
                  p->finishSegment(comm());
                  if (p->startTracing(comm()) >= 0) {
                      activeParticles.insert(p);
                  }
              }
          }
      }

      numActiveMin = mpi::all_reduce(comm(), activeParticles.size(), mpi::minimum<Index>());
      numActiveMax = mpi::all_reduce(comm(), activeParticles.size(), mpi::maximum<Index>());
      //std::cerr << "recvlist: " << datarecvlist.size() << ", sendlist: " << sendlist.size() << ", #active="<<activeParticles.size() << ", #check=" << checkSet.size() << std::endl;
      // iterate over all other ranks
      for(int i=1; i<mpisize; ++i) {
          // start sending particles to owning rank
          int dst = (rank()+i)%size();
          for (auto id: sendlist) {
              auto p = allParticles[id];
              if (p->rank() == dst) {
                  //std::cerr << "initiate sending " << p->id() << " to " << dst << std::endl;
                  p->startSendData(comm());
              }
          }
      }
      for(int i=1; i<mpisize; ++i) {
          // receive locally owned particles
          int src = (rank()-i+size())%size();
          for (const auto &part: datarecvlist) {
              auto id = part.first;
              auto rank = part.second;
              auto p = allParticles[id];
              assert(p->rank() == this->rank());
              if (rank == src) {
                  //std::cerr << "receiving " << p->id() << " from " << src << std::endl;
                  p->receiveData(comm(), src);
              }
          }
      }
      for(int i=1; i<mpisize; ++i) {
          // finish sending particles to owning rank
          int dst = (rank()+i)%size();
          for (auto id: sendlist) {
              auto p = allParticles[id];
              if (p->rank() == dst) {
                  //std::cerr << "finish sending " << p->id() << " to " << dst << std::endl;
                  p->finishSendData();
              }
          }
      }
   } while (numActiveMax > 0 || nextParticleToStart<allParticles.size() || !checkSet.empty());

   if (mpisize == 1) {
       for (auto p: allParticles) {
           p->finishSegment(comm());
       }
   }

   Scalar maxTime = 0;
   for (auto &p: allParticles) {
       maxTime = std::max(p->time(), maxTime);
   }
   maxTime = mpi::all_reduce(comm(), maxTime, mpi::maximum<Scalar>());
   if (taskType == MovingPoints) {
       numtime = maxTime / global.dt_step;
       if (numtime < 1)
           numtime = 1;
   }

   for (int i=0; i<numtime; ++i) {
       if (timestep != i && timestep != -1)
           continue;
       if (taskType == MovingPoints) {
           global.points.emplace_back(new Points(Index(0)));
       } else {
           global.lines.emplace_back(new Lines(0,0,0));
       }

       global.vecField.emplace_back(new Vec<Scalar,3>(Index(0)));
       global.scalField.emplace_back(new Vec<Scalar>(Index(0)));
       global.idField.emplace_back(new Vec<Index>(Index(0)));
       global.stepField.emplace_back(new Vec<Index>(Index(0)));
       global.timeField.emplace_back(new Vec<Scalar>(Index(0)));
       global.distField.emplace_back(new Vec<Scalar>(Index(0)));
       global.stopReasonField.emplace_back(new Vec<Index>(Index(0)));
   }

   for (auto &p: allParticles) {
       if (rank() == 0)
           ++stopReasonCount[p->stopReason()];
       if (p->rank() == rank()) {
           if (traceDirection == Both && p->isForward()) {
               auto other = allParticles[p->id()+1];
               p->fetchSegments(*other);
           }
           p->addToOutput();
       }
   }

   Meta meta;
   meta.setNumTimesteps(numtime);
   meta.setNumBlocks(size());
   Index i = 0;
   for (int t=0; t<numtime; ++t) {
       if (timestep != t && timestep != -1)
           continue;
       meta.setBlock(rank());
       meta.setTimeStep(t);

       Object::ptr geo = taskType==MovingPoints ? Object::ptr(global.points[i]) : Object::ptr(global.lines[i]);
       geo->setMeta(meta);

       global.idField[i]->setGrid(geo);
       global.idField[i]->setMeta(meta);
       addObject("particle_id", global.idField[i]);

       global.stepField[i]->setGrid(geo);
       global.stepField[i]->setMeta(meta);
       addObject("step", global.stepField[i]);

       global.timeField[i]->setGrid(geo);
       global.timeField[i]->setMeta(meta);
       addObject("time", global.timeField[i]);

       global.distField[i]->setGrid(geo);
       global.distField[i]->setMeta(meta);
       addObject("distance", global.distField[i]);

       global.vecField[i]->setGrid(geo);
       global.vecField[i]->setMeta(meta);
       addObject("data_out0", global.vecField[i]);

       global.scalField[i]->setGrid(geo);
       global.scalField[i]->setMeta(meta);
       addObject("data_out1", global.scalField[i]);

       global.stopReasonField[i]->setGrid(geo);
       global.stopReasonField[i]->setMeta(meta);
       addObject("stop_reason", global.stopReasonField[i]);

       ++t;
   }

   if (rank() == 0) {
       std::stringstream str;
       str << "Stop stats for " << allParticles.size() << " particles:";
       for (size_t i=0; i<stopReasonCount.size(); ++i) {
           str << " " << Particle::toString((Particle::StopReason)i) << ":" << stopReasonCount[i];
       }
       std::string s = str.str();
       sendInfo("%s", s.c_str());
   }

   return true;
}

bool Tracer::changeParameter(const Parameter *param) {

    if (param == m_maxStartpoints) {
        setParameterRange(m_numStartpoints, (Integer)1, m_maxStartpoints->getValue());
    } else if (param == m_taskType) {
        if (m_taskType->getValue() == Streamlines)
            setReducePolicy(message::ReducePolicy::PerTimestep);
        else
            setReducePolicy(message::ReducePolicy::OverAll);
    }
    return Module::changeParameter(param);
}
