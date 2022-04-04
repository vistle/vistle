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
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <vistle/core/vec.h>
#include <vistle/core/paramvector.h>
#include <vistle/core/lines.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/serialize.h>
#include <vistle/core/objectmeta_impl.h>
#include <vistle/alg/objalg.h>
#include <vistle/util/threadname.h>

MODULE_MAIN(Tracer)


using namespace vistle;
namespace mpi = boost::mpi;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(StartStyle, (Line)(Plane)(Cylinder))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(TraceDirection, (Both)(Forward)(Backward))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ParticlePlacement, (InitialRank)(RankById))

template<class Value>
bool agree(const boost::mpi::communicator &comm, const Value &value)
{
    int mismatch = 0;

    std::vector<Value> vals(comm.size());
    boost::mpi::all_gather(comm, value, vals);
    for (int i = 0; i < comm.size(); ++i) {
        if (value != vals[i])
            ++mismatch;
    }

    const int minmismatch = boost::mpi::all_reduce(comm, mismatch, boost::mpi::minimum<int>());
    int rank = comm.size();
    if (mismatch > 0 && mismatch == minmismatch)
        rank = comm.rank();
    const int minrank = boost::mpi::all_reduce(comm, rank, boost::mpi::minimum<int>());
    if (comm.rank() == minrank) {
        std::cerr << "agree on rank " << comm.rank() << ": common value " << value << std::endl;
        for (int i = 0; i < comm.size(); ++i) {
            if (vals[i] != value)
                std::cerr << "\t"
                          << "agree on rank " << comm.rank() << ": common value " << value << std::endl;
        }
    }

    return mismatch == 0;
}

Tracer::Tracer(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);
    setReducePolicy(message::ReducePolicy::PerTimestep);

    createInputPort("data_in0");
    createInputPort("data_in1");
    createOutputPort("data_out0");
    createOutputPort("data_out1");
    createOutputPort("particle_id");
    createOutputPort("step");
    createOutputPort("time");
    createOutputPort("stepwidth");
    createOutputPort("distance");
    createOutputPort("stop_reason");
    createOutputPort("cell_index");
    createOutputPort("block_index");

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
    addVectorParameter("startpoint1", "1st initial point", ParamVector(0, 0.2, 0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1, 0, 0));
    addVectorParameter("direction", "direction for plane", ParamVector(1, 0, 0));
    const Integer max_no_startp = 300;
    m_numStartpoints = addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange(m_numStartpoints, (Integer)1, max_no_startp);
    m_maxStartpoints = addIntParameter("max_no_startp", "maximum number of startpoints", max_no_startp);
    setParameterRange(m_maxStartpoints, (Integer)2, (Integer)1000000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    setParameterRange("steps_max", (Integer)1, (Integer)1000000);
    auto tl = addFloatParameter("trace_len", "maximum trace distance", 1.0);
    setParameterMinimum(tl, 0.0);
    auto tt = addFloatParameter("trace_time", "maximum trace time", 10000.0);
    setParameterMinimum(tt, 0.0);
    IntParameter *traceDirection =
        addIntParameter("tdirection", "direction in which to trace", Both, Parameter::Choice);
    V_ENUM_SET_CHOICES(traceDirection, TraceDirection);
    IntParameter *startStyle =
        addIntParameter("startStyle", "initial particle position configuration", (Integer)Line, Parameter::Choice);
    V_ENUM_SET_CHOICES(startStyle, StartStyle);
    IntParameter *integration = addIntParameter("integration", "integration method", (Integer)RK32, Parameter::Choice);
    V_ENUM_SET_CHOICES(integration, IntegrationMethod);
    addFloatParameter("min_speed", "minimum particle speed", 1e-4);
    setParameterRange("min_speed", 0.0, 1e6);
    addFloatParameter("dt_step", "duration of a timestep (for moving points or when data does not specify realtime",
                      1. / 25);
    setParameterRange("dt_step", 0.0, 1e6);

    setCurrentParameterGroup("Step Length Control");
    addFloatParameter("h_init", "fixed step size for euler integration", 1e-03);
    setParameterRange("h_init", 0.0, 1e6);
    addFloatParameter("h_min", "minimum step size for rk32 integration", 1e-04);
    setParameterRange("h_min", 0.0, 1e6);
    addFloatParameter("h_max", "maximum step size for rk32 integration", .5);
    setParameterRange("h_max", 0.0, 1e6);
    addFloatParameter("err_tol_abs", "absolute error tolerance for rk32 integration", 1e-04);
    setParameterRange("err_tol_abs", 0.0, 1e6);
    addFloatParameter("err_tol_rel", "relative error tolerance for rk32 integration", 1e-03);
    setParameterRange("err_tol_rel", 0.0, 1.0);
    addIntParameter("cell_relative", "whether step length control should take into account cell size", 1,
                    Parameter::Boolean);
    addIntParameter("velocity_relative", "whether step length control should take into account velocity", 1,
                    Parameter::Boolean);

    setCurrentParameterGroup("Performance Tuning");
    m_useCelltree =
        addIntParameter("use_celltree", "use celltree for accelerated cell location", (Integer)1, Parameter::Boolean);
    auto num_active =
        addIntParameter("num_active", "number of particles to trace simultaneously on each node (0: no. of cores)", 0);
    setParameterRange(num_active, (Integer)0, (Integer)10000);

    m_particlePlacement = addIntParameter("particle_placement", "where a particle's data shall be collected", RankById,
                                          Parameter::Choice);
    V_ENUM_SET_CHOICES(m_particlePlacement, ParticlePlacement);

    auto modulus = addIntParameter("cell_index_modulus", "modulus for cell number output", -1);
    setParameterMinimum<Integer>(modulus, -1);

    setCurrentParameterGroup("");
    m_simplificationError =
        addFloatParameter("simplification_error", "tolerable relative error for result simplification", 3e-3);
}

Tracer::~Tracer()
{}


bool Tracer::prepare()
{
    m_havePressure = true;
    m_haveTimeSteps = false;

    grid_in.clear();
    celltree.clear();
    data_in0.clear();
    data_in1.clear();

    m_gridAttr.clear();
    m_data0Attr.clear();
    m_data1Attr.clear();

    m_gridTime.clear();
    m_data0Time.clear();
    m_data1Time.clear();

    m_numStartpointsPrinted = false;

    return true;
}

void collectAttributes(std::map<std::string, std::string> &attrs, Meta &meta, vistle::Object::const_ptr obj)
{
    if (!obj)
        return;

    auto al = obj->getAttributeList();
    for (auto &a: al) {
        attrs[a] = obj->getAttribute(a);
    }

    meta.setNumTimesteps(obj->getNumTimesteps());
    meta.setTimeStep(obj->getTimestep());
    meta.setRealTime(obj->getRealTime());
}


bool Tracer::compute()
{
    bool useCelltree = m_useCelltree->getValue();
    auto data0 = expect<Vec<Scalar, 3>>("data_in0");
    auto data1 = accept<Vec<Scalar>>("data_in1");

    if (!data0)
        return true;

    auto split0 = splitContainerObject(data0);
    auto grid = split0.geometry;
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

    const size_t numSteps = t + 1;
    if (grid_in.size() < numSteps + 1) {
        grid_in.resize(numSteps + 1);
        celltree.resize(numSteps + 1);
        data_in0.resize(numSteps + 1);
        data_in1.resize(numSteps + 1);
    }

    if (m_gridAttr.size() < numSteps + 1) {
        m_gridAttr.resize(numSteps + 1);
        m_data0Attr.resize(numSteps + 1);
        m_data1Attr.resize(numSteps + 1);

        m_gridTime.resize(numSteps + 1);
        m_data0Time.resize(numSteps + 1);
        m_data1Time.resize(numSteps + 1);
    }

    if (useCelltree) {
        if (unstr) {
            celltree[t + 1].emplace_back(std::async(std::launch::async, [unstr]() -> Celltree3::const_ptr {
                setThreadName("Tracer:Celltree");
                return unstr->getCelltree();
            }));
        } else if (auto str = StructuredGrid::as(grid)) {
            celltree[t + 1].emplace_back(std::async(std::launch::async, [str]() -> Celltree3::const_ptr {
                setThreadName("Tracer:Celltree");
                return str->getCelltree();
            }));
        } else if (auto lg = LayerGrid::as(grid)) {
            celltree[t + 1].emplace_back(std::async(std::launch::async, [lg]() -> Celltree3::const_ptr {
                setThreadName("Tracer:Celltree");
                return lg->getCelltree();
            }));
        }
    }

    if (getIntParameter("integration") == ConstantVelocity) {
        // initialize VertexOwnerList
        if (unstr) {
            unstr->getNeighborElements(InvalidIndex);
        }
    }

    collectAttributes(m_gridAttr[t + 1], m_gridTime[t + 1], grid);
    collectAttributes(m_data0Attr[t + 1], m_data0Time[t + 1], data0);
    collectAttributes(m_data1Attr[t + 1], m_data1Time[t + 1], data1);

    grid_in[t + 1].push_back(grid);
    data_in0[t + 1].push_back(data0);
    data_in1[t + 1].push_back(data1);
    if (!data1)
        m_havePressure = false;

    return true;
}

namespace {

void mergeAttributes(Tracer::AttributeMap &dest, const Tracer::AttributeMap &other)
{
    for (auto &kv: other) {
        dest.emplace(kv);
    }
}

void applyAttributes(vistle::Object::ptr obj, const Tracer::AttributeMap &attrs)
{
    if (!obj)
        return;

    for (auto &a: attrs) {
        obj->addAttribute(a.first, a.second);
    }
}

} // namespace

bool Tracer::reduce(int timestep)
{
    if (timestep == -1 && numTimesteps() > 0 && reducePolicy() == message::ReducePolicy::PerTimestep) {
        // all the work for stream lines has already be done per timestep
        return true;
    }

    size_t minsize = std::max(numTimesteps() + 1, timestep + 2);
    if (m_gridAttr.size() < minsize) {
        m_gridAttr.resize(minsize);
        m_data0Attr.resize(minsize);
        m_data1Attr.resize(minsize);

        m_gridTime.resize(minsize);
        m_data0Time.resize(minsize);
        m_data1Time.resize(minsize);
    }

    if (timestep == -1) {
        for (int i = 0; i < numTimesteps(); ++i) {
            mergeAttributes(m_gridAttr[0], m_gridAttr[i + 1]);
            mergeAttributes(m_data0Attr[0], m_data0Attr[i + 1]);
            mergeAttributes(m_data1Attr[0], m_data1Attr[i + 1]);
        }
    }

    int attrGridRank = m_gridAttr[timestep + 1].empty() ? size() : rank();
    int attrData0Rank = m_data0Attr[timestep + 1].empty() ? size() : rank();
    int attrData1Rank = m_data1Attr[timestep + 1].empty() ? size() : rank();
    attrGridRank = mpi::all_reduce(comm(), attrGridRank, mpi::minimum<int>());
    attrData0Rank = mpi::all_reduce(comm(), attrData0Rank, mpi::minimum<int>());
    attrData1Rank = mpi::all_reduce(comm(), attrData1Rank, mpi::minimum<int>());
    if (attrGridRank == size()) {
        attrGridRank = m_gridTime[timestep + 1].timeStep() < 0 ? size() : rank();
        attrGridRank = mpi::all_reduce(comm(), attrGridRank, mpi::minimum<int>());
    }
    if (attrGridRank == size()) {
        attrGridRank = 0;
    }
    if (attrData0Rank == size()) {
        attrData0Rank = m_data0Time[timestep + 1].timeStep() < 0 ? size() : rank();
        attrData0Rank = mpi::all_reduce(comm(), attrData0Rank, mpi::minimum<int>());
    }
    if (attrData0Rank == size()) {
        attrData0Rank = 0;
    }
    if (attrData1Rank == size()) {
        attrData1Rank = m_data1Time[timestep + 1].timeStep() < 0 ? size() : rank();
        attrData1Rank = mpi::all_reduce(comm(), attrData1Rank, mpi::minimum<int>());
    }
    if (attrData1Rank == size()) {
        attrData1Rank = 0;
    }
    if (attrGridRank != size()) {
        mpi::broadcast(comm(), m_gridAttr[timestep + 1], attrGridRank);
        mpi::broadcast(comm(), m_gridTime[timestep + 1], attrGridRank);
    }
    if (attrData0Rank != size()) {
        mpi::broadcast(comm(), m_data0Attr[timestep + 1], attrData0Rank);
        mpi::broadcast(comm(), m_data0Time[timestep + 1], attrData0Rank);
    }
    if (attrData1Rank != size()) {
        mpi::broadcast(comm(), m_data1Attr[timestep + 1], attrData1Rank);
        mpi::broadcast(comm(), m_data1Time[timestep + 1], attrData1Rank);
    }

    //get parameters
    bool useCelltree = m_useCelltree->getValue();
    Index numpoints = m_numStartpoints->getValue();
    Index maxNumActive = getIntParameter("num_active");
    if (maxNumActive <= 0) {
        maxNumActive = std::thread::hardware_concurrency();
    }
    Index maxNumActiveGlobal = mpi::all_reduce(comm(), maxNumActive, mpi::maximum<Index>());
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
        Vector3 v = startpoint2 - startpoint1;
        Scalar l = direction.dot(v);
        Vector3 v0 = direction * l;
        Vector3 v1 = v - v0;
        Scalar r = v0.norm();
        Scalar s = v1.norm();
        Index n0, n1;
        if (r > s) {
            n1 = Index(sqrt(numpoints * s / r)) + 1;
            if (n1 <= 1)
                n1 = 2;
            n0 = numpoints / n1;
            if (n0 <= 1)
                n0 = 2;
        } else {
            n0 = Index(sqrt(numpoints * r / s)) + 1;
            if (n0 <= 1)
                n0 = 2;
            n1 = numpoints / n0;
            if (n1 <= 1)
                n1 = 2;
        }
        //setIntParameter("no_startp", numpoints);
        if (rank() == 0 && numpoints != n0 * n1 && !m_numStartpointsPrinted) {
            sendInfo("actually using %lu*%lu=%lu start points", (unsigned long)n0, (unsigned long)n1,
                     (unsigned long)(n0 * n1));
            m_numStartpointsPrinted = true;
        }
        numpoints = n0 * n1;
        startpoints.resize(numpoints);

        Scalar s0 = Scalar(1) / (n0 - 1);
        Scalar s1 = Scalar(1) / (n1 - 1);
        for (Index i = 0; i < n0; ++i) {
            for (Index j = 0; j < n1; ++j) {
                startpoints[i * n1 + j] = startpoint1 + v0 * s0 * i + v1 * s1 * j;
            }
        }
    } else {
        startpoints.resize(numpoints);

        if (numpoints == 1) {
            startpoints[0] = (startpoint1 + startpoint2) * 0.5;
        } else {
            Vector3 delta = (startpoint2 - startpoint1) / (numpoints - 1);
            for (Index i = 0; i < numpoints; i++) {
                startpoints[i] = startpoint1 + i * delta;
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
    numtime = std::min(timestep + 1, numtime);
    if (numtime <= 0)
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
    global.cell_index_modulus = getIntParameter("cell_index_modulus");
    global.simplification_error = getFloatParameter("simplification_error");

    global.computeVector = isConnected("data_out0");
    global.computeScalar = isConnected("data_out1");
    global.computeId = isConnected("particle_id");
    global.computeStep = isConnected("step");
    global.computeStopReason = isConnected("stop_reason");
    global.computeCellIndex = isConnected("cell_index");
    global.computeBlockIndex = isConnected("block_index");
    global.computeTime = isConnected("time");
    global.computeDist = isConnected("distance");
    global.computeStepWidth = isConnected("stepwidth");

    std::vector<Index> stopReasonCount(Particle::NumStopReasons, 0);
    std::vector<std::shared_ptr<Particle>> allParticles;
    std::set<std::shared_ptr<Particle>> localParticles, activeParticles;

    Index numconstant = grid_in.size() ? grid_in[0].size() : 0;
    for (Index i = 0; i < numconstant; ++i) {
        if (useCelltree && !celltree.empty()) {
            if (celltree[0].size() > i && celltree[0][i].valid())
                celltree[0][i].get();
        }
    }

    // create particles
    bool initialrank = m_particlePlacement->getValue() == InitialRank;
    Index id = 0;
    for (int t = 0; t < numtime; ++t) {
        if (timestep != t && timestep != -1)
            continue;
        Index numblocks = size_t(t) + 1 >= grid_in.size() ? 0 : grid_in[t + 1].size();

        //create BlockData objects
        global.blocks[t].resize(numblocks + numconstant);
        for (Index i = 0; i < numconstant; i++) {
            global.blocks[t][i].reset(new BlockData(i, grid_in[0][i], data_in0[0][i], data_in1[0][i]));
        }
        for (Index i = 0; i < numblocks; i++) {
            if (useCelltree && celltree.size() > size_t(t + 1)) {
                if (celltree[t + 1].size() > i && celltree[t + 1][i].valid())
                    celltree[t + 1][i].get();
            }
            global.blocks[t][i + numconstant].reset(
                new BlockData(i + numconstant, grid_in[t + 1][i], data_in0[t + 1][i], data_in1[t + 1][i]));
        }

        //create particle objects, 2 if traceDirecton==Both
        allParticles.reserve(allParticles.size() + numparticles);
        Index i = 0;
        for (; i < numpoints; i++) {
            int rank = initialrank ? -1 : i % size();
            if (traceDirection != Backward) {
                allParticles.emplace_back(new Particle(id++, rank, i, startpoints[i], true, global, t));
            }
            if (traceDirection != Forward) {
                allParticles.emplace_back(new Particle(id++, rank, i, startpoints[i], false, global, t));
            }
        }
    }

    Index nextParticleToStart = 0;
    auto startParticles = [this, &nextParticleToStart, &allParticles, &activeParticles, &localParticles,
                           maxNumActive](int toStart) -> int {
        int started = 0;
        while (nextParticleToStart < allParticles.size()) {
            if (started >= toStart)
                break;

            Index idx = nextParticleToStart;
            ++nextParticleToStart;
            auto particle = allParticles[idx];
            int r = particle->searchRank(comm());
            if (r >= 0) {
                ++started;
                if (rank() == r) {
                    if (activeParticles.size() < maxNumActive) {
                        activeParticles.emplace(particle);
                        particle->startTracing();
                    } else {
                        localParticles.emplace(particle);
                    }
                }
            } else {
                particle->Deactivate(Particle::InitiallyOutOfDomain);
            }
        }
        return started;
    };

    std::vector<Index> datasendlist;
    std::vector<std::pair<Index, int>> datarecvlist; // particle id, source mpi rank
    const int mpisize = comm().size();
    Index numActiveMax = 0, numActiveMin = std::numeric_limits<Index>::max();
    do {
        std::vector<Index> sendlist;
        Index numStart = maxNumActiveGlobal > numActiveMin ? maxNumActiveGlobal - numActiveMin : 0;
        startParticles(numStart);

        bool first = true;
        int numDeactivated = 0;
        // build list of particles to send to their owner
        for (auto it = activeParticles.begin(), next = it; it != activeParticles.end(); it = next) {
            next = it;
            ++next;

            auto particle = it->get();

            bool wait = mpisize == 1 && first;
            first = false;
            if (!particle->isTracing(wait)) {
                if (mpisize == 1) {
                    particle->Deactivate(Particle::OutOfDomain);
                } else if (particle->madeProgress()) {
                    sendlist.push_back(particle->id());
                }
                activeParticles.erase(it);
                ++numDeactivated;
            }
        }

        while (numDeactivated > 0 && !localParticles.empty()) {
            --numDeactivated;
            auto p = *localParticles.begin();
            activeParticles.emplace(p);
            p->startTracing();
            localParticles.erase(localParticles.begin());
        }

        if (mpisize == 1) {
            numActiveMax = numActiveMin = activeParticles.size() + localParticles.size();
            continue;
        }

        // communicate
        Index num_send = sendlist.size();
        std::vector<Index> num_transmit(mpisize);
        mpi::all_gather(comm(), num_send, num_transmit);
        for (int mpirank = 0; mpirank < mpisize; mpirank++) {
            Index num_recv = num_transmit[mpirank];
            if (num_recv > 0) {
                std::vector<Index> recvlist;
                auto &curlist = rank() == mpirank ? sendlist : recvlist;
                mpi::broadcast(comm(), curlist, mpirank);
                assert(curlist.size() == num_recv);
                for (Index i = 0; i < num_recv; i++) {
                    Index p_index = curlist[i];
                    auto p = allParticles[p_index];
                    assert(!p->isTracing(false));
                    p->broadcast(comm(), mpirank);
                    if (p->rank() == rank() && rank() != mpirank) {
                        datarecvlist.emplace_back(p->id(), mpirank);
                    }
                    p->finishSegment();
                    int r = p->searchRank(comm());
                    if (r < 0) {
                        p->Deactivate(Particle::OutOfDomain);
                    } else if (r == rank()) {
                        if (activeParticles.size() < maxNumActive) {
                            activeParticles.emplace(p);
                            p->startTracing();
                        } else {
                            localParticles.emplace(p);
                        }
                    }
                }
            }
        }

        std::copy(sendlist.begin(), sendlist.end(), std::back_inserter(datasendlist));

        numActiveMin = mpi::all_reduce(comm(), activeParticles.size() + localParticles.size(), mpi::minimum<Index>());
        numActiveMax = mpi::all_reduce(comm(), activeParticles.size() + localParticles.size(), mpi::maximum<Index>());
        //std::cerr << "recvlist: " << datarecvlist.size() << ", sendlist: " << sendlist.size() << ", #active="<<activeParticles.size() << std::endl;
    } while (numActiveMax > 0 || nextParticleToStart < allParticles.size());

    if (mpisize == 1) {
        for (auto p: allParticles) {
            p->finishSegment();
        }
    } else {
        // iterate over all other ranks
        for (int i = 1; i < mpisize; ++i) {
            // start sending particles to owning rank
            int dst = (rank() + i) % size();
            for (auto id: datasendlist) {
                auto p = allParticles[id];
                if (p->rank() == dst) {
                    //std::cerr << "initiate sending " << p->id() << " to " << dst << std::endl;
                    p->startSendData(comm());
                }
            }
        }
        for (int i = 1; i < mpisize; ++i) {
            // receive locally owned particles
            int src = (rank() - i + size()) % size();
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
        for (int i = 1; i < mpisize; ++i) {
            // finish sending particles to owning rank
            int dst = (rank() + i) % size();
            for (auto id: datasendlist) {
                auto p = allParticles[id];
                if (p->rank() == dst) {
                    //std::cerr << "finish sending " << p->id() << " to " << dst << std::endl;
                    p->finishSendData();
                }
            }
        }
    }

    Scalar maxTime = 0;
    for (auto &p: allParticles) {
        maxTime = std::max(p->time(), maxTime);
    }
    maxTime = mpi::all_reduce(comm(), maxTime, mpi::maximum<Scalar>());
    unsigned numout = 1;
    if (taskType != Streamlines) {
        numtime = maxTime / global.dt_step + 1;
        if (numtime < 1)
            numtime = 1;
        numout = numtime;
    }

    while (global.points.size() < numout && global.lines.size() < numout) {
        if (taskType == MovingPoints) {
            global.points.emplace_back(new Points(Index(0)));
            global.points.back()->x().reserve(allParticles.size());
            global.points.back()->y().reserve(allParticles.size());
            global.points.back()->z().reserve(allParticles.size());
            applyAttributes(global.points.back(), m_gridAttr[timestep + 1]);
        } else {
            global.lines.emplace_back(new Lines(0, 0, 0));
            applyAttributes(global.lines.back(), m_gridAttr[timestep + 1]);
        }

        if (global.computeVector) {
            global.vecField.emplace_back(new Vec<Scalar, 3>(Index(0)));
            applyAttributes(global.vecField.back(), m_data0Attr[timestep + 1]);
            if (taskType == MovingPoints) {
                global.vecField.back()->x().reserve(allParticles.size());
                global.vecField.back()->y().reserve(allParticles.size());
                global.vecField.back()->z().reserve(allParticles.size());
            }
        }
        if (global.computeScalar) {
            global.scalField.emplace_back(new Vec<Scalar>(Index(0)));
            applyAttributes(global.scalField.back(), m_data1Attr[timestep + 1]);
            if (taskType == MovingPoints) {
                global.scalField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeId) {
            global.idField.emplace_back(new Vec<Index>(Index(0)));
            if (taskType == MovingPoints) {
                global.idField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeStep) {
            global.stepField.emplace_back(new Vec<Index>(Index(0)));
            if (taskType == MovingPoints) {
                global.stepField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeTime) {
            global.timeField.emplace_back(new Vec<Scalar>(Index(0)));
            if (taskType == MovingPoints) {
                global.timeField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeStepWidth) {
            global.stepWidthField.emplace_back(new Vec<Scalar>(Index(0)));
            if (taskType == MovingPoints) {
                global.stepWidthField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeDist) {
            global.distField.emplace_back(new Vec<Scalar>(Index(0)));
            if (taskType == MovingPoints) {
                global.distField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeStopReason) {
            global.stopReasonField.emplace_back(new Vec<Index>(Index(0)));
            if (taskType == MovingPoints) {
                global.stopReasonField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeCellIndex) {
            global.cellField.emplace_back(new Vec<Index>(Index(0)));
            if (taskType == MovingPoints) {
                global.cellField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeBlockIndex) {
            global.blockField.emplace_back(new Vec<Index>(Index(0)));
            if (taskType == MovingPoints) {
                global.blockField.back()->x().reserve(allParticles.size());
            }
        }
    }

    {
        int idx = timestep;
        if (idx < 0)
            idx = 0;
        if (taskType == MovingPoints) {
            assert(global.points.size() > size_t(idx));
            applyAttributes(global.points[idx], m_gridAttr[timestep + 1]);
        } else {
            if (taskType == Streamlines) {
                assert(!global.lines.empty());
                applyAttributes(global.lines.back(), m_gridAttr[timestep + 1]);
            } else {
                assert(global.lines.size() > size_t(idx));
                applyAttributes(global.lines[idx], m_gridAttr[timestep + 1]);
            }
        }
        if (global.computeVector) {
            if (taskType == Streamlines) {
                assert(!global.vecField.empty());
                applyAttributes(global.vecField.back(), m_data0Attr[timestep + 1]);
            } else {
                assert(global.vecField.size() > size_t(idx));
                applyAttributes(global.vecField[idx], m_data0Attr[timestep + 1]);
            }
        }
        if (global.computeScalar) {
            if (taskType == Streamlines) {
                assert(!global.scalField.empty());
                applyAttributes(global.scalField.back(), m_data1Attr[timestep + 1]);
            } else {
                assert(global.scalField.size() > size_t(idx));
                applyAttributes(global.scalField[idx], m_data1Attr[timestep + 1]);
            }
        }
    }

    for (auto &p: allParticles) {
        if (rank() == 0)
            ++stopReasonCount[p->stopReason()];
        if (p->rank() == rank()) {
            if (traceDirection == Both && p->isForward()) {
                auto other = allParticles[p->id() + 1];
                p->fetchSegments(*other);
            }
            p->addToOutput();
        }
    }

    Meta meta;
    meta.setNumBlocks(size());
    meta.setBlock(rank());
    for (int t = 0; t < numtime; ++t) {
        Index i = taskType == Streamlines ? 0 : t;

        if (timestep != t && timestep != -1)
            continue;

        if (taskType == Streamlines) {
            if (m_data0Time[timestep + 1].timeStep() >= 0) {
                meta.setNumTimesteps(m_data0Time[timestep + 1].numTimesteps());
                meta.setTimeStep(m_data0Time[timestep + 1].timeStep());
                meta.setRealTime(m_data0Time[timestep + 1].realTime());
            } else {
                meta.setNumTimesteps(m_gridTime[timestep + 1].numTimesteps());
                meta.setTimeStep(m_gridTime[timestep + 1].timeStep());
                meta.setRealTime(m_gridTime[timestep + 1].realTime());
            }
        } else {
            meta.setNumTimesteps(numtime > 1 ? numtime : -1);
            meta.setTimeStep(numtime > 1 ? t : -1);
        }

        Object::ptr geo = taskType == MovingPoints ? Object::ptr(global.points[i]) : Object::ptr(global.lines[i]);
        geo->setMeta(meta);
        updateMeta(geo);

        auto addField = [this, geo, meta](const char *name, DataBase::ptr field) {
            field->setGrid(geo);
            field->setMeta(meta);
            field->addAttribute("_species", name);
            updateMeta(field);
            addObject(name, field);
        };

        if (global.computeId) {
            addField("particle_id", global.idField[i]);
        }
        if (global.computeStep) {
            addField("step", global.stepField[i]);
        }
        if (global.computeTime) {
            addField("time", global.timeField[i]);
        }
        if (global.computeDist) {
            addField("distance", global.distField[i]);
        }
        if (global.computeStepWidth) {
            addField("stepwidth", global.stepWidthField[i]);
        }
        if (global.computeStopReason) {
            addField("stop_reason", global.stopReasonField[i]);
        }
        if (global.computeCellIndex) {
            addField("cell_index", global.cellField[i]);
        }
        if (global.computeBlockIndex) {
            addField("block_index", global.blockField[i]);
        }

        if (global.computeVector) {
            global.vecField[i]->setGrid(geo);
            global.vecField[i]->setMeta(meta);
            updateMeta(global.vecField[i]);
            addObject("data_out0", global.vecField[i]);
        }

        if (global.computeScalar) {
            global.scalField[i]->setGrid(geo);
            global.scalField[i]->setMeta(meta);
            updateMeta(global.scalField[i]);
            addObject("data_out1", global.scalField[i]);
        }
    }

    if (rank() == 0) {
        std::stringstream str;
        str << "Stop stats for " << allParticles.size() << " particles:";
        for (size_t i = 0; i < stopReasonCount.size(); ++i) {
            str << " " << Particle::toString((Particle::StopReason)i) << ":" << stopReasonCount[i];
        }
        std::string s = str.str();
        sendInfo("%s", s.c_str());
    }

    return true;
}

bool Tracer::changeParameter(const Parameter *param)
{
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
