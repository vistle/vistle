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

using namespace vistle;
namespace mpi = boost::mpi;


DEFINE_ENUM_WITH_STRING_CONVERSIONS(StartStyle, (Line)(Plane) /*(Cylinder)*/)
DEFINE_ENUM_WITH_STRING_CONVERSIONS(TraceDirection, (Both)(Forward)(Backward))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(ParticlePlacement, (InitialRank)(RankById)(RankByTimestep)(Rank0))

typedef Particle<double> ParticleT;

static_assert(Tracer::NumPorts == BlockData::NumFields);

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

DEFINE_ENUM_WITH_STRING_CONVERSIONS(
    AddDataKind, (None)(ParticleId)(Step)(Time)(StepWidth)(Distance)(TerminationReason)(CellIndex)(BlockIndex))

Tracer::Tracer(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setReducePolicy(message::ReducePolicy::PerTimestep);

    m_inPort[0] = createInputPort("data_in0", "vector field");
    m_outPort[0] = createOutputPort("data_out0", "stream lines or points with mapped vector");
    linkPorts(m_inPort[0], m_outPort[0]);
    for (int i = 1; i < NumPorts; ++i) {
        m_inPort[i] = createInputPort("data_in" + std::to_string(i), "additional field on same geometry");
        m_outPort[i] = createOutputPort("data_out" + std::to_string(i), "stream lines or points with mapped field");
        linkPorts(m_inPort[i], m_outPort[i]);
        setPortOptional(m_inPort[i], true);
    }

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
    m_traceDirection = addIntParameter("tdirection", "direction in which to trace", Both, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_traceDirection, TraceDirection);
    m_dtStep = addFloatParameter(
        "dt_step", "duration of a timestep (for moving points or when data does not specify realtime)", 1. / 25);
    setParameterRange(m_dtStep, 0.0, 1e6);
    for (int i = 0; i < NumAddPorts; ++i) {
        m_addPort[i] = createOutputPort("add_data_out" + std::to_string(i), "computed field on same geometry");
        linkPorts(m_inPort[0], m_addPort[i]);
        m_addField[i] = addIntParameter("add_data_kind" + std::to_string(i), "field to compute", 0, Parameter::Choice);
        V_ENUM_SET_CHOICES(m_addField[i], AddDataKind);
    }
    addDescription(ParticleId, "particle_id", "stream lines or points with mapped particle id");
    addDescription(Step, "step", "stream lines or points with step number");
    addDescription(Time, "time", "stream lines or points with time");
    addDescription(StepWidth, "stepwidth", "stream lines or points with stepwidth");
    addDescription(Distance, "distance", "stream lines or points with accumulated distance");
    addDescription(TerminationReason, "stop_reason", "stream lines or points with reason for finishing trace");
    addDescription(CellIndex, "cell_index", "stream lines or points with cell index");
    addDescription(BlockIndex, "block_index", "stream lines or points with block index");
    m_verbose = addIntParameter("verbose", "verbose output", false, Parameter::Boolean);

    setCurrentParameterGroup("Seed Points");
    const Integer max_no_startp = 300;
    m_numStartpoints = addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange(m_numStartpoints, (Integer)1, max_no_startp);
    m_startStyle =
        addIntParameter("startStyle", "initial particle position configuration", (Integer)Line, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_startStyle, StartStyle);
    addVectorParameter("startpoint1", "1st initial point", ParamVector(0, 0.2, 0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1, 0, 0));
    m_direction = addVectorParameter("direction", "direction for plane", ParamVector(1, 0, 0));
    m_maxStartpoints =
        addIntParameter("max_no_startp", "maximum number of startpoints (for parameter/slider limits)", max_no_startp);
    setParameterRange(m_maxStartpoints, (Integer)2, (Integer)1000000);

    setCurrentParameterGroup("Stop Conditions");
    auto tt = addFloatParameter("trace_time", "maximum trace time", 10000.0);
    setParameterMinimum(tt, 0.0);
    auto tl = addFloatParameter("trace_len", "maximum trace distance", 1.0);
    setParameterMinimum(tl, 0.0);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
    setParameterRange("steps_max", (Integer)1, (Integer)1000000);
    addFloatParameter("min_speed", "minimum particle speed", 1e-4);
    setParameterRange("min_speed", 0.0, 1e6);

    setCurrentParameterGroup("Step Length Control");
    IntParameter *integration = addIntParameter("integration", "integration method", (Integer)RK32, Parameter::Choice);
    V_ENUM_SET_CHOICES(integration, IntegrationMethod);
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

    m_particlePlacement = addIntParameter("particle_placement", "where a particle's data shall be collected",
                                          RankByTimestep, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_particlePlacement, ParticlePlacement);

    auto modulus = addIntParameter("cell_index_modulus", "modulus for cell number output", -1);
    setParameterMinimum<Integer>(modulus, -1);

    setCurrentParameterGroup("");
    m_simplificationError =
        addFloatParameter("simplification_error", "tolerable relative error for result simplification", 3e-3);
}

Tracer::~Tracer()
{}

void Tracer::addDescription(int kind, const std::string &name, const std::string &description)
{
    m_addFieldName[kind] = name;
    m_addFieldDescription[kind] = name;
}

const std::string &Tracer::getFieldName(int kind) const
{
    auto it = m_addFieldName.find(kind);
    if (it != m_addFieldName.end()) {
        return it->second;
    }
    static const std::string empty;
    return empty;
}

const std::string &Tracer::getFieldDescription(int kind) const
{
    auto it = m_addFieldDescription.find(kind);
    if (it != m_addFieldDescription.end()) {
        return it->second;
    }
    static const std::string empty;
    return empty;
}

bool Tracer::prepare()
{
    m_haveTimeSteps = false;

    grid_in.clear();
    celltree.clear();
    data_in0.clear();

    m_gridAttr.clear();
    m_gridTime.clear();
    for (int i = 0; i < NumPorts; ++i) {
        data_in[i].clear();
        m_dataAttr[i].clear();
        m_dataTime[i].clear();
        data_dim[i] = 0;
    }

    m_stopReasonCount.clear();
    m_stopReasonCount.resize(NumStopReasons, 0);
    m_numTotalParticles = 0;
    m_stopStatsPrinted = false;

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
    DataBase::const_ptr data[NumPorts];
    data[0] = std::dynamic_pointer_cast<const DataBase>(data0);
    for (int i = 1; i < NumPorts; ++i) {
        data[i] = accept<DataBase>("data_in" + std::to_string(i));
    }

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
        for (int i = 0; i < NumPorts; ++i) {
            data_in[i].resize(numSteps + 1);
        }
    }

    if (m_gridAttr.size() < numSteps + 1) {
        m_gridAttr.resize(numSteps + 1);
        m_gridTime.resize(numSteps + 1);
        for (int i = 0; i < NumPorts; ++i) {
            m_dataAttr[i].resize(numSteps + 1);
            m_dataTime[i].resize(numSteps + 1);
        }
    }

    if (useCelltree) {
        std::string tname = std::to_string(id()) + "ct:" + name();
        if (unstr) {
            celltree[t + 1].emplace_back(std::async(std::launch::async, [tname, unstr]() -> Celltree3::const_ptr {
                setThreadName(tname);
                return unstr->getCelltree();
            }));
        } else if (auto str = StructuredGrid::as(grid)) {
            celltree[t + 1].emplace_back(std::async(std::launch::async, [tname, str]() -> Celltree3::const_ptr {
                setThreadName(tname);
                return str->getCelltree();
            }));
        } else if (auto lg = LayerGrid::as(grid)) {
            celltree[t + 1].emplace_back(std::async(std::launch::async, [tname, lg]() -> Celltree3::const_ptr {
                setThreadName(tname);
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
    for (int i = 0; i < NumPorts; ++i) {
        collectAttributes(m_dataAttr[i][t + 1], m_dataTime[i][t + 1], data[i]);
    }

    grid_in[t + 1].push_back(grid);
    data_in0[t + 1].push_back(data0);
    for (int i = 0; i < NumPorts; ++i) {
        data_in[i][t + 1].push_back(data[i]);
        if (data_dim[i] == 0 && data[i])
            data_dim[i] = data[i]->dimension();
    }

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
    auto printGlobalStopStats = [this, timestep]() {
        if (rank() == 0 && timestep == -1 && !m_stopStatsPrinted) {
            std::stringstream str;
            str << "Stop stats for " << m_numTotalParticles << " particles in " << numTimesteps() << " timesteps:";
            for (size_t i = 0; i < m_stopReasonCount.size(); ++i) {
                str << " " << toString((StopReason)i) << ":" << m_stopReasonCount[i];
            }
            std::string s = str.str();
            if (m_verbose->getValue() || m_stopReasonCount[InitiallyOutOfDomain] > 0) {
                sendInfo("%s", s.c_str());
            } else {
                std::cerr << s << std::endl;
            }
        }
    };
    printGlobalStopStats();

    if (timestep == -1 && numTimesteps() > 0 && reducePolicy() == message::ReducePolicy::PerTimestep) {
        // all the work for stream lines has already be done per timestep
        return true;
    }

    size_t minsize = std::max(numTimesteps() + 1, timestep + 2);
    if (m_gridAttr.size() < minsize) {
        m_gridAttr.resize(minsize);
        m_gridTime.resize(minsize);
        for (int i = 0; i < NumPorts; ++i) {
            m_dataAttr[i].resize(minsize);
            m_dataTime[i].resize(minsize);
        }
    }

    if (timestep == -1) {
        for (int t = 0; t < numTimesteps(); ++t) {
            mergeAttributes(m_gridAttr[0], m_gridAttr[t + 1]);
            for (int i = 0; i < NumPorts; ++i) {
                mergeAttributes(m_dataAttr[i][0], m_dataAttr[i][t + 1]);
            }
        }
    }
    for (int i = 0; i < NumPorts; ++i) {
        data_dim[i] = mpi::all_reduce(comm(), data_dim[i], mpi::maximum<int>());
    }

    int attrGridRank = m_gridAttr[timestep + 1].empty() ? size() : rank();
    attrGridRank = mpi::all_reduce(comm(), attrGridRank, mpi::minimum<int>());
    if (attrGridRank == size()) {
        attrGridRank = m_gridTime[timestep + 1].timeStep() < 0 ? size() : rank();
        attrGridRank = mpi::all_reduce(comm(), attrGridRank, mpi::minimum<int>());
    }
    if (attrGridRank == size()) {
        attrGridRank = 0;
    }
    if (attrGridRank != size()) {
        mpi::broadcast(comm(), m_gridAttr[timestep + 1], attrGridRank);
        mpi::broadcast(comm(), m_gridTime[timestep + 1], attrGridRank);
    }

    int attrDataRank[NumPorts];
    for (int i = 0; i < NumPorts; ++i) {
        attrDataRank[i] = m_dataAttr[i][timestep + 1].empty() ? size() : rank();
        attrDataRank[i] = mpi::all_reduce(comm(), attrDataRank[i], mpi::minimum<int>());
        if (attrDataRank[i] == size()) {
            attrDataRank[i] = m_dataTime[i][timestep + 1].timeStep() < 0 ? size() : rank();
            attrDataRank[i] = mpi::all_reduce(comm(), attrDataRank[i], mpi::minimum<int>());
        }
        if (attrDataRank[i] == size()) {
            attrDataRank[i] = 0;
        }
        if (attrDataRank[i] != size()) {
            mpi::broadcast(comm(), m_dataAttr[i][timestep + 1], attrDataRank[i]);
            mpi::broadcast(comm(), m_dataTime[i][timestep + 1], attrDataRank[i]);
        }
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
    StartStyle startStyle = (StartStyle)m_startStyle->getValue();
    Vector3 startpoint1 = getVectorParameter("startpoint1");
    Vector3 startpoint2 = getVectorParameter("startpoint2");
    Vector3 direction = m_direction->getValue();
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
    global.module = this;
    global.int_mode = (IntegrationMethod)getIntParameter("integration");
    global.task_type = (TraceType)getIntParameter("taskType");
    global.dt_step = m_dtStep->getValue();
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

    global.computeVector = isConnected(*m_outPort[0]);
    global.computeField[0] = global.computeVector;
    global.fields.resize(NumPorts);
    global.numScalars = 0;
    for (int i = 1; i < NumPorts; ++i) {
        global.computeField[i] = isConnected(*m_outPort[i]);
        global.numScalars += global.computeField[i] ? data_dim[i] : 0;
    }
    for (int i = 0; i < NumAddPorts; ++i) {
        if (!isConnected(*m_addPort[i])) {
            continue;
        }
        switch (m_addField[i]->getValue()) {
        case ParticleId:
            global.computeId = true;
            break;
        case Step:
            global.computeStep = true;
            break;
        case Time:
            global.computeTime = true;
            break;
        case StepWidth:
            global.computeStepWidth = true;
            break;
        case Distance:
            global.computeDist = true;
            break;
        case TerminationReason:
            global.computeStopReason = true;
        case CellIndex:
            global.computeCellIndex = true;
            break;
        case BlockIndex:
            global.computeBlockIndex = true;
            break;
        }
    }

    std::vector<Index> stopReasonCount(NumStopReasons, 0);
    std::vector<std::shared_ptr<ParticleT>> allParticles;
    std::set<std::shared_ptr<ParticleT>> localParticles, activeParticles;

    Index numconstant = grid_in.size() ? grid_in[0].size() : 0;
    for (Index i = 0; i < numconstant; ++i) {
        if (useCelltree && !celltree.empty()) {
            if (celltree[0].size() > i && celltree[0][i].valid())
                celltree[0][i].get();
        }
    }

    // create particles
    bool rankById = m_particlePlacement->getValue() == RankById;
    Index id = 0;
    for (int t = 0; t < numtime; ++t) {
        if (timestep != t && timestep != -1)
            continue;
        Index numblocks = size_t(t) + 1 >= grid_in.size() ? 0 : grid_in[t + 1].size();

        int rank = -1;
        switch (m_particlePlacement->getValue()) {
        case InitialRank:
            rank = -1;
            break;
        case Rank0:
            rank = 0;
            break;
        case RankByTimestep:
            rank = t % size();
            break;
        }

        //create BlockData objects
        global.blocks[t].resize(numblocks + numconstant);
        for (Index b = 0; b < numconstant; b++) {
            DataBase::const_ptr din[NumPorts];
            for (int i = 0; i < NumPorts; ++i) {
                din[i] = data_in[i][0][b];
            }
            global.blocks[t][b].reset(new BlockData(b, grid_in[0][b], data_in0[0][b], din));
        }
        for (Index b = 0; b < numblocks; b++) {
            if (useCelltree && celltree.size() > size_t(t + 1)) {
                if (celltree[t + 1].size() > b && celltree[t + 1][b].valid())
                    celltree[t + 1][b].get();
            }
            DataBase::const_ptr din[NumPorts];
            for (int i = 0; i < NumPorts; ++i) {
                din[i] = data_in[i][t + 1][b];
            }
            global.blocks[t][b + numconstant].reset(
                new BlockData(b + numconstant, grid_in[t + 1][b], data_in0[t + 1][b], din));
        }

        //create particle objects, 2 if traceDirection==Both
        allParticles.reserve(allParticles.size() + numparticles);
        Index i = 0;
        for (; i < numpoints; i++) {
            if (rankById)
                rank = i % size();
            if (traceDirection != Backward) {
                allParticles.emplace_back(new ParticleT(id++, rank, i, startpoints[i], true, global, t));
                ++m_numTotalParticles;
            }
            if (traceDirection != Forward) {
                allParticles.emplace_back(new ParticleT(id++, rank, i, startpoints[i], false, global, t));
                ++m_numTotalParticles;
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
                particle->Deactivate(InitiallyOutOfDomain);
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
                    particle->Deactivate(OutOfDomain);
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
                        p->Deactivate(OutOfDomain);
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
            global.points.emplace_back(new Points(size_t(0)));
            global.points.back()->x().reserve(allParticles.size());
            global.points.back()->y().reserve(allParticles.size());
            global.points.back()->z().reserve(allParticles.size());
            applyAttributes(global.points.back(), m_gridAttr[timestep + 1]);
        } else {
            global.lines.emplace_back(new Lines(0, 0, 0));
            applyAttributes(global.lines.back(), m_gridAttr[timestep + 1]);
        }

        if (global.computeVector) {
            global.vecField.emplace_back(new Vec<Scalar, 3>(size_t(0)));
            applyAttributes(global.vecField.back(), m_dataAttr[0][timestep + 1]);
            if (taskType == MovingPoints) {
                global.vecField.back()->x().reserve(allParticles.size());
                global.vecField.back()->y().reserve(allParticles.size());
                global.vecField.back()->z().reserve(allParticles.size());
            }
        }
        for (int i = 1; i < NumPorts; ++i) {
            if (global.computeField[i]) {
                if (data_dim[i] == 3) {
                    auto vec = std::make_shared<Vec<Scalar, 3>>(size_t(0));
                    global.fields[i].emplace_back(vec);
                    if (taskType == MovingPoints) {
                        vec->x().reserve(allParticles.size());
                        vec->y().reserve(allParticles.size());
                        vec->z().reserve(allParticles.size());
                    }
                } else {
                    auto scal = std::make_shared<Vec<Scalar>>(size_t(0));
                    global.fields[i].emplace_back(scal);
                    if (taskType == MovingPoints) {
                        scal->x().reserve(allParticles.size());
                    }
                }
            }
        }
        if (global.computeId) {
            global.idField.emplace_back(new Vec<Index>(size_t(0)));
            if (taskType == MovingPoints) {
                global.idField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeStep) {
            global.stepField.emplace_back(new Vec<Index>(size_t(0)));
            if (taskType == MovingPoints) {
                global.stepField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeTime) {
            global.timeField.emplace_back(new Vec<Scalar>(size_t(0)));
            if (taskType == MovingPoints) {
                global.timeField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeStepWidth) {
            global.stepWidthField.emplace_back(new Vec<Scalar>(size_t(0)));
            if (taskType == MovingPoints) {
                global.stepWidthField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeDist) {
            global.distField.emplace_back(new Vec<Scalar>(size_t(0)));
            if (taskType == MovingPoints) {
                global.distField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeStopReason) {
            global.stopReasonField.emplace_back(new Vec<Index>(size_t(0)));
            if (taskType == MovingPoints) {
                global.stopReasonField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeCellIndex) {
            global.cellField.emplace_back(new Vec<Index>(size_t(0)));
            if (taskType == MovingPoints) {
                global.cellField.back()->x().reserve(allParticles.size());
            }
        }
        if (global.computeBlockIndex) {
            global.blockField.emplace_back(new Vec<Index>(size_t(0)));
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
                applyAttributes(global.vecField.back(), m_dataAttr[0][timestep + 1]);
            } else {
                assert(global.vecField.size() > size_t(idx));
                applyAttributes(global.vecField[idx], m_dataAttr[0][timestep + 1]);
            }
        }
        for (int i = 1; i < NumPorts; ++i) {
            if (!global.computeField[i])
                continue;
            if (taskType == Streamlines) {
                assert(!global.fields[i].empty());
                applyAttributes(global.fields[i].back(), m_dataAttr[i][timestep + 1]);
            } else {
                assert(global.fields[i].size() > size_t(idx));
                applyAttributes(global.fields[i][idx], m_dataAttr[i][timestep + 1]);
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
            if (m_dataTime[0][timestep + 1].timeStep() >= 0) {
                meta.setNumTimesteps(m_dataTime[0][timestep + 1].numTimesteps());
                meta.setTimeStep(m_dataTime[i][timestep + 1].timeStep());
                meta.setRealTime(m_dataTime[i][timestep + 1].realTime());
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

        for (int i = 0; i < NumAddPorts; ++i) {
            DataBase::ptr field;
            if (m_addField[i]->getValue() == ParticleId) {
                field = global.idField[i];
            } else if (m_addField[i]->getValue() == Step) {
                field = global.stepField[i];
            } else if (m_addField[i]->getValue() == Time) {
                field = global.timeField[i];
            } else if (m_addField[i]->getValue() == StepWidth) {
                field = global.stepWidthField[i];
            } else if (m_addField[i]->getValue() == Distance) {
                field = global.distField[i];
            } else if (m_addField[i]->getValue() == TerminationReason) {
                field = global.stopReasonField[i];
            } else if (m_addField[i]->getValue() == CellIndex) {
                field = global.cellField[i];
            } else if (m_addField[i]->getValue() == BlockIndex) {
                field = global.blockField[i];
            }

            if (isConnected(*m_addPort[i])) {
                if (field) {
                    bool initField = true;
                    for (int j = 0; j < i; ++j) {
                        if (m_addField[j]->getValue() == m_addField[i]->getValue()) {
                            initField = false;
                            break;
                        }
                    }
                    if (initField) {
                        field->setGrid(geo);
                        field->setMeta(meta);
                        auto kind = m_addField[i]->getValue();
                        field->addAttribute(attribute::Species, getFieldName(kind));
                        updateMeta(field);
                    }
                    addObject(m_addPort[i], field);
                } else {
                    addObject(m_addPort[i], geo);
                }
            }
        }

        if (global.computeVector) {
            global.vecField[i]->setGrid(geo);
            global.vecField[i]->setMeta(meta);
            updateMeta(global.vecField[i]);
            addObject(m_outPort[0], global.vecField[i]);
        }

        for (int p = 1; p < NumPorts; ++p) {
            if (!global.computeField[p]) {
                continue;
            }
            global.fields[p][i]->setGrid(geo);
            global.fields[p][i]->setMeta(meta);
            updateMeta(global.fields[p][i]);
            addObject(m_outPort[p], global.fields[p][i]);
        }
    }

    if (rank() == 0) {
        for (size_t i = 0; i < stopReasonCount.size(); ++i) {
            m_stopReasonCount[i] += stopReasonCount[i];
        }

        std::stringstream str;
        str << "Stop stats for " << allParticles.size() << " particles:";
        for (size_t i = 0; i < stopReasonCount.size(); ++i) {
            str << " " << toString((StopReason)i) << ":" << stopReasonCount[i];
        }
        if (m_verbose->getValue() == false) {
            std::cerr << str.str() << std::endl;
        } else {
            m_stopStatsPrinted = true;
            std::string s = str.str();
            sendInfo("%s", s.c_str());
        }
    }

    printGlobalStopStats();
    return true;
}

bool Tracer::changeParameter(const Parameter *param)
{
    if (param == m_maxStartpoints) {
        setParameterRange(m_numStartpoints, (Integer)1, m_maxStartpoints->getValue());
    } else if (param == m_taskType) {
        setParameterReadOnly(m_traceDirection, m_taskType->getValue() != Streamlines);
        setParameterReadOnly(m_dtStep, m_taskType->getValue() == Streamlines);

        if (m_taskType->getValue() == Streamlines)
            setReducePolicy(message::ReducePolicy::PerTimestep);
        else
            setReducePolicy(message::ReducePolicy::OverAll);
        switch (m_taskType->getValue()) {
        case Streamlines:
            setItemInfo("Streamlines");
            break;
        case Pathlines:
            setItemInfo("Pathlines");
            break;
        case Streaklines:
            setItemInfo("Streaklines");
            break;
        case MovingPoints:
            setItemInfo("Points");
            break;
        }
    } else if (param == m_startStyle) {
        setParameterReadOnly(m_direction, m_startStyle->getValue() != Plane);
    }
    return Module::changeParameter(param);
}

MODULE_MAIN(Tracer)
