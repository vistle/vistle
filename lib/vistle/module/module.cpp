#include <cstdlib>
#include <cstdio>

#include <sys/types.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <deque>
#include <mutex>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <boost/asio.hpp>

#include <vistle/util/sysdep.h>
#include <vistle/util/tools.h>
#include <vistle/util/stopwatch.h>
#include <vistle/util/exception.h>
#include <vistle/util/shmconfig.h>
#include <vistle/util/threadname.h>
#include <vistle/util/affinity.h>
#include <vistle/config/config.h>
#include <vistle/core/object.h>
#include <vistle/core/empty.h>
#include <vistle/core/export.h>
#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/messagerouter.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/parameter.h>
#include <vistle/core/shm.h>
#include <vistle/core/port.h>
#include <vistle/core/statetracker.h>

#include "objectcache.h"
#include "resultcache_impl.h"

#include "module.h"

#include <boost/serialization/vector.hpp>
#include <boost/lexical_cast.hpp>
#include <vistle/core/shm_reference.h>
#include <vistle/core/archive_saver.h>
#include <vistle/core/archive_loader.h>

//#define DEBUG
//#define REDUCE_DEBUG
#define DETAILED_PROGRESS
#define REDIRECT_OUTPUT

#ifdef DEBUG
#include <vistle/util/hostname.h>
#endif

#define CERR std::cerr << m_name << "_" << id() << " [" << rank() << "/" << size() << "] "

namespace interprocess = ::boost::interprocess;

namespace bigmpi {

static const size_t chunk = 1 << 30;

template<typename T>
void broadcast(const mpi::communicator &comm, T *values, size_t count, int root)
{
    for (size_t off = 0; off < count; off += chunk) {
        mpi::broadcast(comm, values + off, int(std::min(chunk, count - off)), root);
    }
}

template<typename T>
void send(const mpi::communicator &comm, int rank, int tag, const T *values, size_t count)
{
    for (size_t off = 0; off < count; off += chunk) {
        comm.send(rank, tag, values + off, int(std::min(chunk, count - off)));
    }
}

template<typename T>
void recv(const mpi::communicator &comm, int rank, int tag, T *values, size_t count)
{
    for (size_t off = 0; off < count; off += chunk) {
        comm.recv(rank, tag, values + off, int(std::min(chunk, count - off)));
    }
}

} // namespace bigmpi

namespace vistle {

using message::Id;

#ifdef REDIRECT_OUTPUT
template<typename CharT, typename TraitsT = std::char_traits<CharT>>
class msgstreambuf: public std::basic_streambuf<CharT, TraitsT> {
public:
    msgstreambuf(Module *mod): m_module(mod), m_console(true), m_gui(false) {}

    ~msgstreambuf()
    {
        std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
        flush();
        for (const auto &s: m_backlog) {
            std::cout << s << std::endl;
            if (m_gui)
                m_module->sendText(message::SendText::Cerr, s);
        }
        m_backlog.clear();
    }

    void flush(ssize_t count = -1)
    {
        std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
        size_t size = count < 0 ? m_buf.size() : count;
        if (size > 0) {
            std::string msg(m_buf.data(), size);
            m_backlog.push_back(msg);
            if (m_backlog.size() > BacklogSize)
                m_backlog.pop_front();
            if (m_gui)
                m_module->sendText(message::SendText::Cerr, msg);
            if (m_console)
                std::cout << msg << std::flush;
        }

        if (size == m_buf.size()) {
            m_buf.clear();
        } else {
            m_buf.erase(m_buf.begin(), m_buf.begin() + size);
        }
    }

    int overflow(int ch)
    {
        std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
        if (ch != EOF) {
            m_buf.push_back(ch);
            if (ch == '\n')
                flush();
            return 0;
        } else {
            return EOF;
        }
    }

    std::streamsize xsputn(const CharT *s, std::streamsize num)
    {
        std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
        size_t end = m_buf.size();
        m_buf.resize(end + num);
        memcpy(m_buf.data() + end, s, num);
        auto it = std::find(m_buf.rbegin(), m_buf.rend(), '\n');
        if (it != m_buf.rend()) {
            flush(it - m_buf.rend());
        }
        return num;
    }

    void set_console_output(bool enable) { m_console = enable; }

    void set_gui_output(bool enable) { m_gui = enable; }

    void clear_backlog()
    {
        std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
        m_backlog.clear();
    }

private:
    const size_t BacklogSize = 10;
    Module *m_module;
    std::vector<char> m_buf;
    std::recursive_mutex m_mutex;
    bool m_console, m_gui;
    std::deque<std::string> m_backlog;
};
#endif


int getTimestep(Object::const_ptr obj)
{
    if (!obj)
        return -1;

    int t = obj->getTimestep();
    if (t < 0) {
        if (auto data = DataBase::as(obj)) {
            if (auto grid = data->grid()) {
                t = grid->getTimestep();
            }
        }
    }

    return t;
}

double getRealTime(Object::const_ptr obj)
{
    if (!obj)
        return -1;

    int t = obj->getTimestep();
    if (t < 0) {
        if (auto data = DataBase::as(obj)) {
            if (auto grid = data->grid()) {
                return grid->getRealTime();
            }
        }
    }
    return obj->getRealTime();
}

bool Module::setup(const std::string &shmname, int moduleID, const std::string &cluster, int rank)
{
#ifndef MODULE_THREAD
    if (!Shm::isAttached()) {
        bool perRank = shmPerRank();
        Shm::attach(shmname, moduleID, rank, perRank);
        vistle::apply_affinity_from_environment(Shm::the().nodeRank(rank), Shm::the().numRanksOnThisNode());
        setenv("VISTLE_CLUSTER", cluster.c_str(), 1);
    }
#endif
    return Shm::isAttached();
}

bool Module::cleanup(bool dedicated_process)
{
#ifndef MODULE_THREAD
    if (dedicated_process) {
        Shm::the().detach();
    }
#endif
    return true;
}

Module::Module(const std::string &moduleName, const int moduleId, mpi::communicator comm)
: ParameterManager(moduleName, moduleId)
, m_name(moduleName)
, m_rank(-1)
, m_size(-1)
, m_id(moduleId)
, m_executionCount(0)
, m_iteration(-1)
, m_stateTracker(new StateTracker(moduleId, m_name))
, m_receivePolicy(message::ObjectReceivePolicy::Local)
, m_schedulingPolicy(message::SchedulingPolicy::Single)
, m_reducePolicy(message::ReducePolicy::Locally)
, m_defaultCacheMode(ObjectCache::CacheByName)
, m_prioritizeVisible(true)
, m_syncMessageProcessing(false)
, m_origStreambuf(nullptr)
, m_streambuf(nullptr)
, m_traceMessages(message::INVALID)
, m_benchmark(false)
, m_avgComputeTime(0.)
, m_comm(comm)
, m_numTimesteps(-1)
, m_cancelRequested(false)
, m_cancelExecuteCalled(false)
, m_prepared(false)
, m_computed(false)
, m_reduced(false)
, m_readyForQuit(false)
{
    m_size = m_comm.size();
    m_rank = m_comm.rank();
    m_configAccess = std::make_unique<config::Access>(vistle::hostname(), vistle::clustername(), m_rank);
    ParameterManager::setConfig(m_configAccess.get());
    m_configFile = m_configAccess->file("module/" + name());

#ifndef MODULE_THREAD
    message::DefaultSender::init(m_id, m_rank);
#endif

    // names are swapped relative to communicator
    std::string smqName = message::MessageQueue::createName("recv", id(), rank());
    try {
        sendMessageQueue = message::MessageQueue::open(smqName);
#ifdef DEBUG
        CERR << "sendMessageQueue name = " << smqName << std::endl;
#endif
    } catch (interprocess::interprocess_exception &ex) {
        throw vistle::exception(std::string("opening send message queue ") + smqName + ": " + ex.what());
    }

    std::string rmqName = message::MessageQueue::createName("send", id(), rank());
    try {
        receiveMessageQueue = message::MessageQueue::open(rmqName);
    } catch (interprocess::interprocess_exception &ex) {
        throw vistle::exception(std::string("opening receive message queue ") + rmqName + ": " + ex.what());
    }

    m_hardware_concurrency = mpi::all_reduce(comm, std::thread::hardware_concurrency(), mpi::minimum<unsigned>());

#ifdef DEBUG
    std::cerr << "  module [" << name() << "] [" << id() << "] [" << rank() << "/" << size() << "] started as "
              << hostname() << ":" << get_process_handle() << std::endl;
#endif

    auto cm = addIntParameter("_cache_mode", "input object caching", ObjectCache::CacheDefault, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(cm, CacheMode, ObjectCache);
    addIntParameter("_prioritize_visible", "prioritize currently visible timestep", m_prioritizeVisible,
                    Parameter::Boolean);

    addVectorParameter("_position", "position in GUI", ParamVector(0., 0.));
    auto layer = addIntParameter("_layer", "layer in GUI", Integer(0));
    setParameterMinimum(layer, Integer(-1));

    auto em = addIntParameter("_error_output_mode", "where stderr is shown", size() == 1 ? 1 : 1, Parameter::Choice);
    std::vector<std::string> errmodes;
    errmodes.push_back("No output");
    errmodes.push_back("Console only");
    errmodes.push_back("GUI");
    errmodes.push_back("Console & GUI");
    setParameterChoices(em, errmodes);

    auto outrank = addIntParameter("_error_output_rank", "rank from which to show stderr (-1: all ranks)", -1);
    setParameterRange<Integer>(outrank, -1, size() - 1);

    auto openmp_threads = addIntParameter("_openmp_threads", "number of OpenMP threads (0: system default)", 0);
    setParameterRange<Integer>(openmp_threads, 0, 4096);
    addIntParameter("_benchmark", "show timing information", m_benchmark ? 1 : 0, Parameter::Boolean);

    m_concurrency =
        addIntParameter("_concurrency", "number of tasks to keep in flight per MPI rank (-1: #cores/2)", -1);
    setParameterRange(m_concurrency, Integer(-1), Integer(hardware_concurrency()));

    int leader = Shm::the().owningRank();
    m_commShmGroup = boost::mpi::communicator(m_comm.split(leader));
    m_commShmLeaders = boost::mpi::communicator(m_comm.split(leader == m_rank ? 1 : MPI_UNDEFINED));
    mpi::all_gather(m_comm, leader, m_shmLeaders);
    int leaderSubRank = -1;
    if (shmLeader() == rank()) {
        leaderSubRank = m_commShmLeaders.rank();
    }
    mpi::broadcast(m_commShmGroup, leaderSubRank, 0);
    mpi::all_gather(m_comm, leaderSubRank, m_shmLeadersSubrank);
}

config::Access *Module::configAccess() const
{
    return m_configAccess.get();
}

config::File *Module::config() const
{
    return m_configFile.get();
}

StateTracker &Module::state()
{
    return *m_stateTracker;
}

const StateTracker &Module::state() const
{
    return *m_stateTracker;
}

void Module::prepareQuit()
{
#ifdef DEBUG
    CERR << "I'm quitting" << std::endl;
#endif

    if (!m_readyForQuit) {
        waitAllTasks();

        ParameterManager::quit();

        m_cache.clear();
        m_cache.clearOld();

        inputPorts.clear();
        outputPorts.clear();
    }

    m_readyForQuit = true;
}

const HubData &Module::getHub() const
{
    int hubId = m_stateTracker->getHub(id());
    return m_stateTracker->getHubData(hubId);
}

void Module::initDone()
{
#ifndef MODULE_THREAD
#ifdef REDIRECT_OUTPUT
    m_streambuf = new msgstreambuf<char>(this);
    m_origStreambuf = std::cerr.rdbuf(m_streambuf);
#endif
#endif

    message::Started start(name());
    start.setPid(getpid());
    start.setDestId(Id::ForBroadcast);
    sendMessage(start);

    ParameterManager::init();
}

const std::string &Module::name() const
{
    return m_name;
}

int Module::id() const
{
    return m_id;
}

unsigned Module::hardware_concurrency() const
{
    return m_hardware_concurrency;
}

const mpi::communicator &Module::comm() const
{
    return m_comm;
}

int Module::rank() const
{
    return m_rank;
}

int Module::size() const
{
    return m_size;
}

const mpi::communicator &Module::commShmGroup() const
{
    return m_commShmGroup;
}

int Module::openmpThreads() const
{
    int ret = (int)getIntParameter("_openmp_threads");
    if (ret <= 0) {
        ret = 0;
    }
    return ret;
}

void Module::setOpenmpThreads(int nthreads, bool updateParam)
{
#ifdef _OPENMP
    if (nthreads <= 0)
        nthreads = omp_get_num_procs();
    if (nthreads > 0)
        omp_set_num_threads(nthreads);
#endif
    if (updateParam)
        setIntParameter("_openmp_threads", nthreads);
}

void Module::enableBenchmark(bool benchmark, bool updateParam)
{
    m_benchmark = benchmark;
    if (updateParam)
        setIntParameter("_benchmark", benchmark ? 1 : 0);
    int red = m_reducePolicy;
    if (m_benchmark) {
        if (red == message::ReducePolicy::Locally)
            red = message::ReducePolicy::OverAll;
    }
    sendMessage(message::ReducePolicy(message::ReducePolicy::Reduce(red)));
}

bool Module::syncMessageProcessing() const
{
    return m_syncMessageProcessing;
}

void Module::setSyncMessageProcessing(bool sync)
{
    m_syncMessageProcessing = sync;
}

ObjectCache::CacheMode Module::setCacheMode(ObjectCache::CacheMode mode, bool updateParam)
{
    if (mode == ObjectCache::CacheDefault)
        m_cache.setCacheMode(m_defaultCacheMode);
    else
        m_cache.setCacheMode(mode);

    if (updateParam)
        setIntParameter("_cache_mode", mode);

    return m_cache.cacheMode();
}

void Module::setDefaultCacheMode(ObjectCache::CacheMode mode)
{
    assert(mode != ObjectCache::CacheDefault);
    m_defaultCacheMode = mode;
    setCacheMode(m_defaultCacheMode, false);
}


ObjectCache::CacheMode Module::cacheMode() const
{
    return m_cache.cacheMode();
}

bool Module::havePort(const std::string &name)
{
    auto param = findParameter(name);
    if (param)
        return true;

    auto iout = outputPorts.find(name);
    if (iout != outputPorts.end())
        return true;

    auto iin = inputPorts.find(name);
    if (iin != inputPorts.end())
        return true;

    return false;
}

Port *Module::createInputPort(const std::string &name, const std::string &description, const int flags)
{
    assert(!havePort(name));
    if (havePort(name)) {
        CERR << "createInputPort: already have port/parameter with name " << name << std::endl;
        return nullptr;
    }

    auto itp = inputPorts.emplace(name, Port(id(), name, Port::INPUT, flags));
    auto &p = itp.first->second;
    p.setDescription(description);

    message::AddPort message(p);
    message.setDestId(Id::ForBroadcast);
    sendMessage(message);
    return &p;
}

Port *Module::createOutputPort(const std::string &name, const std::string &description, const int flags)
{
    assert(!havePort(name));
    if (havePort(name)) {
        CERR << "createOutputPort: already have port/parameter with name " << name << std::endl;
        return nullptr;
    }

    auto itp = outputPorts.emplace(name, Port(id(), name, Port::OUTPUT, flags));
    auto &p = itp.first->second;
    p.setDescription(description);

    message::AddPort message(p);
    message.setDestId(Id::ForBroadcast);
    sendMessage(message);
    return &p;
}

bool Module::destroyPort(const std::string &portName)
{
    const Port *p = findInputPort(portName);
    if (!p)
        p = findOutputPort(portName);
    if (!p)
        return false;

    assert(p);
    return destroyPort(p);
}

bool Module::destroyPort(const Port *port)
{
    assert(port);
    message::RemovePort message(*port);
    message.setDestId(Id::ForBroadcast);
    if (findInputPort(port->getName())) {
        inputPorts.erase(port->getName());
    } else if (findOutputPort(port->getName())) {
        outputPorts.erase(port->getName());
    } else {
        return false;
    }

    sendMessage(message);
    return true;
}

Port *Module::findInputPort(const std::string &name)
{
    auto i = inputPorts.find(name);

    if (i == inputPorts.end())
        return nullptr;

    return &i->second;
}

const Port *Module::findInputPort(const std::string &name) const
{
    auto i = inputPorts.find(name);

    if (i == inputPorts.end())
        return nullptr;

    return &i->second;
}

Port *Module::findOutputPort(const std::string &name)
{
    auto i = outputPorts.find(name);

    if (i == outputPorts.end())
        return nullptr;

    return &i->second;
}

const Port *Module::findOutputPort(const std::string &name) const
{
    auto i = outputPorts.find(name);

    if (i == outputPorts.end())
        return nullptr;

    return &i->second;
}

Parameter *Module::addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> param)
{
    assert(!havePort(name));
    if (havePort(name)) {
        CERR << "addParameterGeneric: already have port/parameter with name " << name << std::endl;
        return nullptr;
    }

    return ParameterManager::addParameterGeneric(name, param);
}

bool Module::removeParameter(const std::string &name)
{
    assert(havePort(name));
    if (!havePort(name)) {
        CERR << "removeParameter: no port with name " << name << std::endl;
        return false;
    }

    return ParameterManager::removeParameter(name);
}

bool Module::sendObject(const mpi::communicator &comm, Object::const_ptr obj, int destRank) const
{
    auto saver = std::make_shared<DeepArchiveSaver>();
    vecostreambuf<buffer> memstr;
    vistle::oarchive memar(memstr);
    memar.setSaver(saver);
    obj->saveObject(memar);
    const buffer &mem = memstr.get_vector();
    comm.send(destRank, 0, mem);
    auto dir = saver->getDirectory();
    comm.send(destRank, 0, dir);
    for (auto &ent: dir) {
        bigmpi::send(comm, destRank, 0, ent.data, ent.size);
    }
    return true;
}

bool Module::sendObject(Object::const_ptr object, int destRank) const
{
    return sendObject(comm(), object, destRank);
}

Object::const_ptr Module::receiveObject(const mpi::communicator &comm, int sourceRank) const
{
    buffer mem;
    comm.recv(sourceRank, 0, mem);
    vistle::SubArchiveDirectory dir;
    std::map<std::string, buffer> objects, arrays;
    std::map<std::string, message::CompressionMode> comp;
    std::map<std::string, size_t> rawsizes;
    comm.recv(sourceRank, 0, dir);
    for (auto &ent: dir) {
        if (ent.is_array) {
            arrays[ent.name].resize(ent.size);
            ent.data = arrays[ent.name].data();
        } else {
            objects[ent.name].resize(ent.size);
            ent.data = objects[ent.name].data();
        }
        bigmpi::recv(comm, sourceRank, 0, ent.data, ent.size);
    }
    vecistreambuf<buffer> membuf(mem);
    vistle::iarchive memar(membuf);
    auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays, comp, rawsizes);
    memar.setFetcher(fetcher);
    Object::ptr p(Object::loadObject(memar));
    //CERR << "receiveObject " << p->getName() << ": refcount=" << p->refcount() << std::endl;
    return p;
}

Object::const_ptr Module::receiveObject(int destRank) const
{
    return receiveObject(comm(), destRank);
}

bool Module::broadcastObject(const mpi::communicator &comm, Object::const_ptr &obj, int root) const
{
    if (comm.size() == 1)
        return true;

    if (comm.rank() == root) {
        assert(obj->check());
        vecostreambuf<buffer> memstr;
        vistle::oarchive memar(memstr);
        auto saver = std::make_shared<DeepArchiveSaver>();
        memar.setSaver(saver);
        obj->saveObject(memar);
        const buffer &mem = memstr.get_vector();
        mpi::broadcast(comm, const_cast<buffer &>(mem), root);
        auto dir = saver->getDirectory();
        mpi::broadcast(comm, dir, root);
        for (auto &ent: dir) {
            bigmpi::broadcast(comm, ent.data, ent.size, root);
        }
    } else {
        buffer mem;
        mpi::broadcast(comm, mem, root);
        vistle::SubArchiveDirectory dir;
        std::map<std::string, buffer> objects, arrays;
        std::map<std::string, message::CompressionMode> comp;
        std::map<std::string, size_t> rawsizes;
        mpi::broadcast(comm, dir, root);
        for (auto &ent: dir) {
            if (ent.is_array) {
                arrays[ent.name].resize(ent.size);
                ent.data = arrays[ent.name].data();
            } else {
                objects[ent.name].resize(ent.size);
                ent.data = objects[ent.name].data();
            }
            bigmpi::broadcast(comm, ent.data, ent.size, root);
        }
        vecistreambuf<buffer> membuf(mem);
        vistle::iarchive memar(membuf);
        auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays, comp, rawsizes);
        memar.setFetcher(fetcher);
        //std::cerr << "DeepArchiveFetcher: " << *fetcher << std::endl;
        obj.reset(Object::loadObject(memar));
        obj->refresh();
        //std::cerr << "broadcastObject recv " << obj->getName() << ": refcount=" << obj->refcount() << std::endl;
        assert(obj->check());
        //obj->unref();
    }

    return true;
}

bool Module::broadcastObject(Object::const_ptr &object, int root) const
{
#if 0
    return broadcastObject(comm(), object, root);
#else
    return broadcastObjectViaShm(object, root);
#endif
}

bool Module::broadcastObjectViaShm(const mpi::communicator &comm, Object::const_ptr &object, const std::string &objName,
                                   int root) const
{
    int leader = Shm::the().owningRank();
    auto commShmGroup = boost::mpi::communicator(comm.split(leader));
    auto commShmLeaders = boost::mpi::communicator(comm.split(leader == comm.rank() ? 1 : MPI_UNDEFINED));

    if (rank() == shmLeader(root)) {
        assert(object);
    }
    bool ok = true;
    if (rank() == shmLeader(rank())) {
        ok = broadcastObject(commShmLeaders, object, m_shmLeadersSubrank[root]);
        assert(object);
    }
    if (shmLeader(rank()) != shmLeader(root)) {
        commShmGroup.barrier(); // synchronize, so that object is available on leader
    }
    if (shmLeader(rank()) == rank()) {
        assert(object);
    } else {
        object = Shm::the().getObjectFromName(objName);
        assert(object);
    }
    commShmGroup.barrier(); // synchronize, so that leader keeps object around long enough
    return ok;
}

bool Module::broadcastObjectViaShm(Object::const_ptr &object, const std::string &objName, int root) const
{
    if (rank() == shmLeader(root)) {
        assert(object);
    }
    bool ok = true;
    if (rank() == shmLeader(rank())) {
        ok = broadcastObject(m_commShmLeaders, object, m_shmLeadersSubrank[root]);
        assert(object);
    }
    if (shmLeader(rank()) != shmLeader(root)) {
        m_commShmGroup.barrier(); // synchronize, so that object is available on leader
    }
    if (shmLeader(rank()) == shmLeader(root) || shmLeader(rank()) == rank()) {
        assert(object);
    } else {
        object = Shm::the().getObjectFromName(objName);
        assert(object);
    }
    m_commShmGroup.barrier(); // synchronize, so that leader keeps object around long enough
    return ok;
}

bool Module::broadcastObjectViaShm(const mpi::communicator &comm, Object::const_ptr &object, int root) const
{
    std::string objName;
    if (comm.rank() == root) {
        assert(object);
        objName = object->getName();
    }
    mpi::broadcast(comm, objName, root);
    return broadcastObjectViaShm(comm, object, objName, root);
}

bool Module::broadcastObjectViaShm(Object::const_ptr &object, int root) const
{
    std::string objName;
    if (comm().rank() == root) {
        assert(object);
        objName = object->getName();
    }
    mpi::broadcast(comm(), objName, root);
    return broadcastObjectViaShm(object, objName, root);
}

void Module::updateCacheMode()
{
    Integer value = getIntParameter("_cache_mode");
    switch (value) {
    case ObjectCache::CacheNone:
    case ObjectCache::CacheDeleteEarly:
    case ObjectCache::CacheDeleteLate:
    case ObjectCache::CacheDefault:
        break;
    default:
        value = ObjectCache::CacheDefault;
        break;
    }

    setCacheMode(ObjectCache::CacheMode(value), false);
}

void Module::updateOutputMode()
{
#ifndef MODULE_THREAD
#ifdef REDIRECT_OUTPUT
    const Integer r = getIntParameter("_error_output_rank");
    const Integer m = getIntParameter("_error_output_mode");

    auto sbuf = dynamic_cast<msgstreambuf<char> *>(m_streambuf);
    if (!sbuf)
        return;

    if (r == -1 || r == rank()) {
        sbuf->set_console_output(m & 1);
        sbuf->set_gui_output(m & 2);
    } else {
        sbuf->set_console_output(false);
        sbuf->set_gui_output(false);
    }
#endif
#endif
}

void Module::waitAllTasks()
{
    while (!m_tasks.empty()) {
        m_tasks.front()->wait();
        m_tasks.pop_front();
    }
    if (m_lastTask) {
        m_lastTask->wait();
        m_lastTask.reset();
    }
}

void Module::updateMeta(vistle::Object::ptr obj) const
{
    if (obj) {
        obj->setCreator(id());
        obj->setExecutionCounter(m_executionCount);
        obj->setIteration(m_iteration);

        obj->updateInternals();

        // update referenced objects, if not yet valid
        auto refs = obj->referencedObjects();
        for (auto &ref: refs) {
            if (ref->getCreator() == -1) {
                auto o = std::const_pointer_cast<Object>(ref);
                o->setCreator(id());
                o->setExecutionCounter(m_executionCount);
                o->setIteration(m_iteration);

                o->updateInternals();
            }
        }
    }
}

void Module::setItemInfo(const std::string &text, const std::string &port)
{
    if (rank() != 0)
        return;
    auto &old = m_currentItemInfo[port];
    if (old != text) {
        using message::ItemInfo;
        ItemInfo info(port.empty() ? ItemInfo::Module : ItemInfo::Port, port);
        ItemInfo::Payload pl(text);
        sendMessageWithPayload(info, pl);
        old = text;
    }
}

bool Module::addObject(const std::string &portName, vistle::Object::ptr object)
{
    auto *p = findOutputPort(portName);
    if (!p) {
        CERR << "Module::addObject: output port " << portName << " not found" << std::endl;
    }
    assert(p);
    return addObject(p, object);
}

bool Module::addObject(Port *port, vistle::Object::ptr object)
{
    assert(!object || object->getCreator() == id());

    vistle::Object::const_ptr cobj = object;
    return passThroughObject(port, cobj);
}

bool Module::passThroughObject(const std::string &portName, vistle::Object::const_ptr object)
{
    auto *p = findOutputPort(portName);
    if (!p) {
        CERR << "Module::passThroughObject: output port " << portName << " not found" << std::endl;
    }
    assert(p);
    return passThroughObject(p, object);
}

bool Module::passThroughObject(Port *port, vistle::Object::const_ptr object)
{
    if (!object)
        return false;

    m_withOutput.insert(port);

    object->refresh();
    assert(object->check());

    message::AddObject message(port->getName(), object);
    sendMessage(message);

    std::string info;
    std::string species = object->getAttribute("_species");
    if (!species.empty()) {
        info += species + " - ";
    }
    std::string type = Object::toString(object->getType());
    info += type;
    if (auto d = DataBase::as(object)) {
        if (auto g = d->grid()) {
            info += " on ";
            info += Object::toString(g->getType());
        }
    }
    setItemInfo(info, port->getName());
    return true;
}

ObjectList Module::getObjects(const std::string &portName)
{
    ObjectList objects;
    auto *p = findInputPort(portName);
    if (!p) {
        CERR << "Module::getObjects: input port " << portName << " not found" << std::endl;
    }
    assert(p);

    ObjectList &olist = p->objects();
    for (ObjectList::const_iterator it = olist.begin(); it != olist.end(); it++) {
        Object::const_ptr object = *it;
        if (object.get()) {
            assert(object->check());
        }
        objects.push_back(object);
    }

    return objects;
}

bool Module::hasObject(const std::string &portName) const
{
    auto *p = findInputPort(portName);
    if (!p) {
        CERR << "Module::hasObject: input port " << portName << " not found" << std::endl;
    }
    assert(p);

    return hasObject(p);
}

bool Module::hasObject(const Port *port) const
{
    return !port->objects().empty();
}

vistle::Object::const_ptr Module::takeFirstObject(const std::string &portName)
{
    auto *p = findInputPort(portName);
    if (!p) {
        CERR << "Module::takeFirstObject: input port " << portName << " not found" << std::endl;
    }
    assert(p);

    return takeFirstObject(p);
}

void Module::addResultCache(ResultCacheBase &cache)
{
    if (!m_useResultCache) {
        m_useResultCache =
            addIntParameter("_use_result_cache", "whether to try to cache results for re-use in subseqeuent timesteps",
                            true, Parameter::Boolean);
    }
    cache.enable(m_useResultCache->getValue());
    m_resultCaches.emplace_back(&cache);
}

void Module::clearResultCaches()
{
    for (auto &c: m_resultCaches)
        c->clear();
}

void Module::enableResultCaches(bool on)
{
    for (auto &c: m_resultCaches)
        c->enable(on);
}

void Module::requestPortMapping(unsigned short forwardPort, unsigned short localPort)
{
    message::RequestTunnel m(forwardPort, localPort);
    m.setDestId(Id::LocalManager);
    sendMessage(m);
}

void Module::removePortMapping(unsigned short forwardPort)
{
    message::RequestTunnel m(forwardPort);
    m.setDestId(Id::LocalManager);
    sendMessage(m);
}

vistle::Object::const_ptr Module::takeFirstObject(Port *port)
{
    if (!port->objects().empty()) {
        Object::const_ptr obj = port->objects().front();
        assert(obj->check());
        port->objects().pop_front();
        return obj;
    }

    return vistle::Object::ptr();
}

// specialized for avoiding Object::type(), which does not exist
template<>
Object::const_ptr Module::expect<Object>(Port *port)
{
    if (!port) {
        std::stringstream str;
        str << "invalid port" << std::endl;
        sendError(str.str());
        return nullptr;
    }
    if (!isConnected(*port)) {
        std::stringstream str;
        str << "port " << port->getName() << " is not connected" << std::endl;
        sendError(str.str());
        return nullptr;
    }
    if (port->objects().empty()) {
        if (schedulingPolicy() == message::SchedulingPolicy::Single) {
            std::stringstream str;
            str << "no object available at " << port->getName() << ", but one is required" << std::endl;
            sendError(str.str());
        }
        return nullptr;
    }
    Object::const_ptr obj = port->objects().front();
    port->objects().pop_front();
    if (!obj) {
        std::stringstream str;
        str << "did not receive valid object at " << port->getName() << ", but one is required" << std::endl;
        sendError(str.str());
        return obj;
    }
    assert(obj->check());
    return obj;
}

void Module::setInputSpecies(const std::string &species)
{}

bool Module::addInputObject(int sender, const std::string &senderPort, const std::string &portName,
                            Object::const_ptr object)
{
    if (!object) {
        CERR << "Module::addInputObject: input port " << portName << " - did not receive object" << std::endl;
        return false;
    }

    assert(object->check());

    if (object->hasAttribute("_species")) {
        std::string species = object->getAttribute("_species");
        if (m_inputSpecies != species) {
            m_inputSpecies = species;
            setInputSpecies(m_inputSpecies);
        }
    }

    if (m_executionCount < object->getExecutionCounter()) {
        m_executionCount = object->getExecutionCounter();
        m_iteration = object->getIteration();
    }
    if (m_executionCount == object->getExecutionCounter()) {
        if (m_iteration < object->getIteration())
            m_iteration = object->getIteration();
    }

    Port *p = findInputPort(portName);

    if (p) {
        m_cache.addObject(portName, object);
        p->objects().push_back(object);
        return true;
    }

    CERR << "Module::addInputObject: input port " << portName << " not found" << std::endl;
    assert(p);

    return false;
}

bool Module::isConnected(const std::string &portname) const
{
    const Port *p = findInputPort(portname);
    if (!p)
        p = findOutputPort(portname);
    if (!p) {
        assert("port not connected" == 0);
        return false;
    }

    return isConnected(*p);
}

bool Module::isConnected(const Port &port) const
{
    return !port.connections().empty();
}

bool Module::changeParameter(const Parameter *p)
{
    std::string name = p->getName();
    if (name[0] == '_') {
        if (name == "_error_output_mode" || name == "_error_output_rank") {
            updateOutputMode();
        } else if (name == "_cache_mode") {
            updateCacheMode();
        } else if (name == "_openmp_threads") {
            setOpenmpThreads((int)getIntParameter(name), false);
        } else if (name == "_benchmark") {
            enableBenchmark(getIntParameter(name), false);
        } else if (name == "_prioritize_visible") {
            m_prioritizeVisible = getIntParameter("_prioritize_visible");
        } else if (name == "_use_result_cache") {
            enableResultCaches(getIntParameter(name));
        }
    }

    return true;
}

bool Module::parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg)
{
    return true;
}

int Module::schedulingPolicy() const
{
    return m_schedulingPolicy;
}

void Module::setSchedulingPolicy(int schedulingPolicy)
{
    using namespace message;

    assert(schedulingPolicy >= SchedulingPolicy::Ignore);
    assert(schedulingPolicy <= SchedulingPolicy::LazyGang);

    m_schedulingPolicy = schedulingPolicy;
    sendMessage(SchedulingPolicy(SchedulingPolicy::Schedule(schedulingPolicy)));
}

int Module::reducePolicy() const
{
    return m_reducePolicy;
}

void Module::setReducePolicy(int reducePolicy)
{
    assert(reducePolicy >= message::ReducePolicy::Never);
    assert(reducePolicy <= message::ReducePolicy::OverAll);

    m_reducePolicy = reducePolicy;
    if (m_benchmark) {
        if (reducePolicy == message::ReducePolicy::Locally)
            reducePolicy = message::ReducePolicy::OverAll;
    }
    sendMessage(message::ReducePolicy(message::ReducePolicy::Reduce(reducePolicy)));
}

bool Module::parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg,
                            const std::string &moduleName)
{
    (void)senderId;
    (void)name;
    (void)msg;
    (void)moduleName;
    return false;
}

bool Module::parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg)
{
    (void)senderId;
    (void)name;
    (void)msg;
    return false;
}

bool Module::objectAdded(int sender, const std::string &senderPort, const Port *port)
{
    return true;
}

void Module::connectionAdded(const Port *from, const Port *to)
{}

void Module::connectionRemoved(const Port *from, const Port *to)
{}

bool Module::getNextMessage(message::Buffer &buf, bool block, unsigned int minPrio)
{
    if (!receiveMessageQueue)
        return false;

    bool recv = receiveMessageQueue->tryReceive(buf);
    do {
        if (recv) {
            if (!messageBacklog.empty()) {
                auto &front = messageBacklog.front();
                if (buf.priority() <= front.priority()) {
                    // keep buffered messages sorted according to priority
                    std::swap(buf, front);
                    auto it = messageBacklog.begin(), next = it + 1;
                    while (next != messageBacklog.end()) {
                        if (it->priority() <= next->priority())
                            std::swap(*it, *next);
                        it = next;
                        ++next;
                    }
                }
            }

            if (buf.priority() >= minPrio) {
                return true;
            }

            messageBacklog.push_front(buf);
        } else if (!messageBacklog.empty()) {
            const auto &front = messageBacklog.front();
            if (front.priority() >= minPrio) {
                buf = front;
                messageBacklog.pop_front();
                return true;
            }
        }
        if (block) {
            receiveMessageQueue->receive(buf);
            recv = true;
        }
    } while (block);

    return false;
}

bool Module::needsSync(const message::Message &m) const
{
    switch (m.type()) {
    case vistle::message::QUIT:
        return true;
    case vistle::message::ADDOBJECT:
        return objectReceivePolicy() != vistle::message::ObjectReceivePolicy::Local;
    default:
        break;
    }

    return false;
}

bool Module::dispatch(bool block, bool *messageReceived, unsigned int minPrio)
{
    bool again = true;

    try {
#ifndef MODULE_THREAD
        if (parentProcessDied())
            throw(except::parent_died());
#endif

        message::Buffer buf;
        if (!getNextMessage(buf, block, minPrio)) {
            if (messageReceived)
                *messageReceived = false;
            if (block) {
                return false;
            }
            return true;
        }

        if (messageReceived)
            *messageReceived = true;

        MessagePayload pl;
        if (buf.payloadSize() > 0) {
            pl = Shm::the().getArrayFromName<char>(buf.payloadName());
            pl.unref();
        }

        if (syncMessageProcessing()) {
            int sync = needsSync(buf) ? buf.type() : 0;
            int allsync = 0;
            mpi::all_reduce(comm(), sync, allsync, mpi::maximum<int>());
            if (sync != 0 && allsync != sync) {
                std::cerr << "message types requiring collective processing do not agree: local=" << sync
                          << ", other=" << allsync << std::endl;
            }
            assert(sync == 0 || sync == allsync);

            do {
                sync = needsSync(buf) ? buf.type() : 0;

                again &= handleMessage(&buf, pl);
                if (!again) {
                    CERR << "collective, quitting after " << buf << std::endl;
                }

                if (allsync && !sync) {
                    getNextMessage(buf, true, minPrio);
                    if (buf.payloadSize() > 0) {
                        pl = Shm::the().getArrayFromName<char>(buf.payloadName());
                        pl.unref();
                    } else {
                        pl.reset();
                    }
                }

            } while (allsync && !sync);
        } else {
            again &= handleMessage(&buf, pl);
            if (!again) {
                CERR << "quitting after " << buf << std::endl;
            }
        }
    } catch (vistle::except::parent_died &e) {
        // if parent died something is wrong - make sure that shm get cleaned up
        Shm::the().setRemoveOnDetach();
        throw(e);
    } catch (vistle::exception &e) {
        std::cerr << "Vistle exception in module " << name() << ": " << e.what() << e.where() << std::endl;
        throw(e);
    } catch (std::exception &e) {
        std::cerr << "exception in module " << name() << ": " << e.what() << std::endl;
        throw(e);
    } catch (...) {
        std::cerr << "unknown exception in module " << name() << std::endl;
        throw;
    }

    return again;
}


void Module::sendParameterMessage(const message::Message &message, const buffer *payload) const
{
    sendMessage(message, payload);
}

bool Module::sendMessage(const message::Message &message, const buffer *payload) const
{
    // exclude SendText messages to avoid circular calls
    if (message.type() != message::SENDTEXT && (m_traceMessages == message::ANY || m_traceMessages == message.type())) {
        CERR << "SEND: " << message << std::endl;
    }
    if (rank() == 0 || message::Router::the().toRank0(message)) {
        message::Buffer buf(message);
        if (payload) {
            MessagePayload pl;
            pl.construct(payload->size());
            memcpy(pl->data(), payload->data(), payload->size());
            pl.ref();
            buf.setPayloadName(pl.name());
            buf.setPayloadSize(payload->size());
        }
#ifdef MODULE_THREAD
        buf.setSenderId(id());
        buf.setRank(rank());
#endif
        if (sendMessageQueue)
            sendMessageQueue->send(buf);
    }

    return true;
}

bool Module::sendMessage(const message::Message &message, const MessagePayload &payload) const
{
    // exclude SendText messages to avoid circular calls
    if (message.type() != message::SENDTEXT && (m_traceMessages == message::ANY || m_traceMessages == message.type())) {
        CERR << "SEND: " << message << std::endl;
    }
    if (rank() == 0 || message::Router::the().toRank0(message)) {
        message::Buffer buf(message);
        if (payload) {
            MessagePayload pl = payload;
            pl.ref();
            buf.setPayloadName(pl.name());
            buf.setPayloadSize(pl->size());
            buf.setPayloadRawSize(pl->size());
        }
#ifdef MODULE_THREAD
        buf.setSenderId(id());
        buf.setRank(rank());
#endif
        if (sendMessageQueue)
            sendMessageQueue->send(buf);
    }

    return true;
}

bool Module::handleMessage(const vistle::message::Message *message, const MessagePayload &payload)
{
    using namespace vistle::message;

    if (message->payloadSize() > 0) {
        assert(payload);
    }

    if (payload)
        m_stateTracker->handle(*message, payload->data(), payload->size());
    else
        m_stateTracker->handle(*message, nullptr);

    if (m_traceMessages == message::ANY || message->type() == m_traceMessages) {
        CERR << "RECV: " << *message << std::endl;
    }

    switch (message->type()) {
    case vistle::message::TRACE: {
        const Trace *trace = static_cast<const Trace *>(message);
        if (trace->on()) {
            m_traceMessages = trace->messageType();
        } else {
            m_traceMessages = message::INVALID;
        }

        std::cerr << "    module [" << name() << "] [" << id() << "] [" << rank() << "/" << size() << "] trace ["
                  << trace->on() << "]" << std::endl;
        break;
    }

    case message::QUIT: {
        const message::Quit *quit = static_cast<const message::Quit *>(message);
        //TODO: uuid should be included in corresponding ModuleExit message
        (void)quit;
#ifdef REDIRECT_OUTPUT
        if (auto sbuf = dynamic_cast<msgstreambuf<char> *>(m_streambuf))
            sbuf->clear_backlog();
#endif
        return false;
        break;
    }

    case message::KILL: {
        const message::Kill *kill = static_cast<const message::Kill *>(message);
        //TODO: uuid should be included in corresponding ModuleExit message
        if (kill->getModule() == id() || kill->getModule() == message::Id::Broadcast) {
#ifdef REDIRECT_OUTPUT
            if (auto sbuf = dynamic_cast<msgstreambuf<char> *>(m_streambuf))
                sbuf->clear_backlog();
#endif
            return false;
        } else {
            std::cerr << "module [" << name() << "] [" << id() << "] [" << rank() << "/" << size() << "]"
                      << ": received invalid Kill message: " << *kill << std::endl;
        }
        break;
    }

    case message::ADDPORT: {
        const message::AddPort *cp = static_cast<const message::AddPort *>(message);
        Port port = cp->getPort();
        std::string name = port.getName();
        std::string::size_type p = name.find('[');
        std::string basename = name;
        size_t idx = 0;
        if (p != std::string::npos) {
            basename = name.substr(0, p - 1);
            idx = boost::lexical_cast<size_t>(name.substr(p + 1));
        }
        Port *existing = NULL;
        Port *parent = NULL;
        Port *newport = NULL;
        switch (port.getType()) {
        case Port::INPUT:
            existing = findInputPort(name);
            if (!existing)
                parent = findInputPort(basename);
            if (parent) {
                newport = parent->child(idx, true);
                inputPorts.emplace(name, *newport);
            }
            break;
        case Port::OUTPUT:
            existing = findOutputPort(name);
            if (!existing)
                parent = findInputPort(basename);
            if (parent) {
                newport = parent->child(idx, true);
                outputPorts.emplace(name, *newport);
            }
            break;
        case Port::PARAMETER:
            // does not really happen - handled in AddParameter
            break;
        case Port::ANY:
            break;
        }
        if (newport) {
            message::AddPort np(*newport);
            np.setReferrer(cp->uuid());
            sendMessage(np);
            const Port::PortSet &links = newport->linkedPorts();
            for (Port::PortSet::iterator it = links.begin(); it != links.end(); ++it) {
                const Port *p = *it;
                message::AddPort linked(*p);
                linked.setReferrer(cp->uuid());
                sendMessage(linked);
            }
        }
        break;
    }

    case message::CONNECT: {
        const message::Connect *conn = static_cast<const message::Connect *>(message);
        Port *port = NULL;
        Port *other = NULL;
        const Port::ConstPortSet *ports = NULL;
        std::string ownPortName;
        bool inputConnection = false;
        //CERR << name() << " receiving connection: " << conn->getModuleA() << ":" << conn->getPortAName() << " -> " << conn->getModuleB() << ":" << conn->getPortBName() << std::endl;
        if (conn->getModuleA() == id()) {
            port = findOutputPort(conn->getPortAName());
            ownPortName = conn->getPortAName();
            if (port) {
                other = new Port(conn->getModuleB(), conn->getPortBName(), Port::INPUT);
                ports = &port->connections();
            }

        } else if (conn->getModuleB() == id()) {
            ownPortName = conn->getPortBName();
            port = findInputPort(conn->getPortBName());
            inputConnection = true;
            if (port) {
                other = new Port(conn->getModuleA(), conn->getPortAName(), Port::OUTPUT);
                ports = &port->connections();
            }
        } else {
            // ignore: not connected to us
            break;
        }

        bool added = false;
        if (ports && port && other) {
            if (ports->find(other) == ports->end()) {
                added = port->addConnection(other);
                if (inputConnection)
                    connectionAdded(other, port);
                else
                    connectionAdded(port, other);
            }
        } else {
            if (!findParameter(ownPortName))
                CERR << " did not find port " << ownPortName << std::endl;
        }
        if (!added) {
            delete other;
        }
        break;
    }

    case message::DISCONNECT: {
        const message::Disconnect *disc = static_cast<const message::Disconnect *>(message);
        Port *port = NULL;
        Port *other = NULL;
        const Port::ConstPortSet *ports = NULL;
        bool inputConnection = false;
        if (disc->getModuleA() == id()) {
            port = findOutputPort(disc->getPortAName());
            if (port) {
                other = new Port(disc->getModuleB(), disc->getPortBName(), Port::INPUT);
                ports = &port->connections();
            }

        } else if (disc->getModuleB() == id()) {
            port = findInputPort(disc->getPortBName());
            inputConnection = true;
            if (port) {
                other = new Port(disc->getModuleA(), disc->getPortAName(), Port::OUTPUT);
                ports = &port->connections();
            }
        }

        if (ports && port && other) {
            if (inputConnection) {
                connectionRemoved(other, port);
                m_cache.clear(port->getName());
            } else {
                connectionRemoved(port, other);
            }
            const Port *p = port->removeConnection(*other);
            delete other;
            delete p;
        }
        break;
    }

    case message::EXECUTE: {
        const Execute *exec = static_cast<const Execute *>(message);
        return handleExecute(exec);
        break;
    }

    case message::ADDOBJECT: {
        const message::AddObject *add = static_cast<const message::AddObject *>(message);
        auto obj = add->takeObject();
        const Port *p = findInputPort(add->getDestPort());
        if (!p) {
            CERR << "unknown input port " << add->getDestPort() << " in AddObject" << std::endl;
            return true;
        }
        if (!obj) {
            CERR << "did not find object " << add->objectName() << " for port " << add->getDestPort() << " in AddObject"
                 << std::endl;
        }
        addInputObject(add->senderId(), add->getSenderPort(), add->getDestPort(), obj);
        if (!objectAdded(add->senderId(), add->getSenderPort(), p)) {
            CERR << "error in objectAdded(" << add->getSenderPort() << ")" << std::endl;
            return false;
        }

        break;
    }

    case message::SETPARAMETER: {
        const message::SetParameter *param = static_cast<const message::SetParameter *>(message);

        if (param->destId() == id()) {
            ParameterManager::handleMessage(*param);
        } else {
            // notification of controller about current value happens in set...Parameter
            parameterChanged(param->getModule(), param->getName(), *param);
        }
        break;
    }

    case message::SETPARAMETERCHOICES: {
        const message::SetParameterChoices *choices = static_cast<const message::SetParameterChoices *>(message);
        if (choices->senderId() != id()) {
            //FIXME: handle somehow
            //parameterChangedWrapper(choices->senderId(), choices->getName());
        }
        break;
    }

    case message::ADDPARAMETER: {
        const message::AddParameter *param = static_cast<const message::AddParameter *>(message);

        parameterAdded(param->senderId(), param->getName(), *param, param->moduleName());
        break;
    }

    case message::REMOVEPARAMETER: {
        const message::RemoveParameter *param = static_cast<const message::RemoveParameter *>(message);

        parameterRemoved(param->senderId(), param->getName(), *param);
        break;
    }

    case message::BARRIER: {
        const message::Barrier *barrier = static_cast<const message::Barrier *>(message);
        message::BarrierReached reached(barrier->uuid());
        reached.setDestId(Id::LocalManager);
        if (m_upstreamIsExecuting) {
            m_delayedBarrierResponse = std::make_unique<vistle::message::Buffer>(reached);
        } else {
            sendMessage(reached);
        }
        break;
    }

    case message::CANCELEXECUTE:
        // not relevant if not within prepare/compute/reduce
        cancelExecuteMessageReceived(message);
        break;

    case message::MODULEEXIT:
    case message::SPAWN:
    case message::STARTED:
    case message::MODULEAVAILABLE:
    case message::REPLAYFINISHED:
    case message::ADDHUB:
    case message::REMOVEHUB:
    case message::UPDATESTATUS:
    case message::IDLE:
    case message::BUSY:
    case message::REMOTERENDERING:
        break;

    default:
        CERR << "unknown message type [" << message->type() << "]" << std::endl;

        break;
    }

    return true;
}

bool Module::handleExecute(const vistle::message::Execute *exec)
{
    if (exec->getModule() != id()) {
        return true;
    }

    using namespace vistle::message;

    if (exec->what() == Execute::Upstream) {
        m_upstreamIsExecuting = true;
    }

    if (m_executionCount < exec->getExecutionCount()) {
        m_executionCount = exec->getExecutionCount();
        m_iteration = -1;
    }

    if (schedulingPolicy() == message::SchedulingPolicy::Ignore)
        return true;

    bool ret = true;

#ifdef DETAILED_PROGRESS
    Busy busy;
    busy.setReferrer(exec->uuid());
    busy.setDestId(Id::LocalManager);
    sendMessage(busy);
#endif

    if (exec->what() == Execute::ComputeExecute || exec->what() == Execute::Prepare) {
        if (m_lastTask) {
            CERR << "prepare: waiting for previous tasks..." << std::endl;
            waitAllTasks();
        }

        applyDelayedChanges();
    }
    if (exec->what() == Execute::Prepare) {
        m_cache.clear();
    }
    if (exec->what() == Execute::ComputeExecute || exec->what() == Execute::Prepare) {
        ret &= prepareWrapper(exec);
    }

    bool reordered = false;
    if (exec->what() == Execute::ComputeExecute || exec->what() == Execute::ComputeObject) {
        if (reducePolicy() != message::ReducePolicy::Never) {
            assert(m_prepared);
            assert(!m_reduced);
        }
        m_computed = true;
        const bool gang = schedulingPolicy() == message::SchedulingPolicy::Gang ||
                          schedulingPolicy() == message::SchedulingPolicy::LazyGang;

#ifdef DEBUG
        CERR << "GANG scheduling: " << gang << std::endl;
#endif

        int direction = 1;

        // just process one tuple of objects at a time
        Index numObject = 1;
        int startTimestep = -1;
        bool waitForZero = false, startWithZero = false;
        if (exec->what() == Execute::ComputeExecute) {
            // Compute not triggered by adding an object, get objects from cache and determine no. of objects to process
            numObject = 0;

            for (auto &port: inputPorts) {
                ObjectList received;
                std::swap(received, port.second.objects());
                if (!isConnected(port.second))
                    continue;
                auto cache = m_cache.getObjects(port.first);
                cache.second = mpi::all_reduce(comm(), cache.second, std::logical_and<bool>());
                if (!cache.second)
                    cache.first.clear();
                port.second.objects() = cache.first;
                received.clear();
                auto srcPort = *port.second.connections().begin();
                for (const auto &o: port.second.objects()) {
                    (void)o;
                    objectAdded(srcPort->getModuleID(), srcPort->getName(), &port.second);
                }
                if (port.second.flags() & Port::NOCOMPUTE)
                    continue;
                if (numObject == 0) {
                    numObject = port.second.objects().size();
                } else if (numObject != port.second.objects().size()) {
                    CERR << "::compute(): input mismatch - expected " << numObject << " objects, have "
                         << port.second.objects().size() << std::endl;
                    throw vistle::except::exception("input object mismatch");
                    return false;
                }
            }
        }

        for (auto &port: inputPorts) {
            if (port.second.flags() & Port::NOCOMPUTE)
                continue;
            if (!isConnected(port.second))
                continue;
            const auto &objs = port.second.objects();
            for (Index i = 0; i < numObject && i < objs.size(); ++i) {
                const auto obj = objs[i];
                int t = getTimestep(obj);
                m_numTimesteps = std::max(t + 1, m_numTimesteps);
            }
        }
#ifdef REDUCE_DEBUG
        CERR << "compute with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif

        if (exec->what() == Execute::ComputeExecute || gang) {
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for timesteps with #objects=" << numObject << ", #timesteps=" << m_numTimesteps
                 << std::endl;
#endif
            m_numTimesteps = mpi::all_reduce(comm(), m_numTimesteps, mpi::maximum<int>());
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for timesteps finished with #objects=" << numObject << ", #timesteps=" << m_numTimesteps
                 << std::endl;
#endif
        }

        if (exec->what() == Execute::ComputeExecute) {
            if (m_prioritizeVisible && !gang && !exec->allRanks() &&
                reducePolicy() != message::ReducePolicy::PerTimestepOrdered) {
                reordered = true;

                if (exec->animationStepDuration() > 0.) {
                    direction = 1;
                } else if (exec->animationStepDuration() < 0.) {
                    direction = -1;
                } else {
                    direction = 0;
                }

                int headStart = 0;
                if (std::abs(exec->animationStepDuration()) > 1e-5)
                    headStart = (int)(m_avgComputeTime / std::abs(exec->animationStepDuration()));
                if (reducePolicy() == message::ReducePolicy::PerTimestepZeroFirst)
                    headStart *= 2;
                headStart = 1 + std::max(0, headStart);

                struct TimeIndex {
                    double time = 0.;
                    Index idx = 0;
                    int step = -1;

                    bool operator<(const TimeIndex &other) const { return step < other.step; }
                };
                std::vector<TimeIndex> sortKey(numObject);
                for (auto &port: inputPorts) {
                    if (port.second.flags() & Port::NOCOMPUTE)
                        continue;
                    if (!isConnected(port.second))
                        continue;
                    const auto &objs = port.second.objects();
                    size_t i = 0;
                    for (auto &obj: objs) {
                        sortKey[i].idx = i;
                        auto t = getTimestep(obj);
                        if (sortKey[i].step == -1) {
                            sortKey[i].step = t;
                        } else if (sortKey[i].step != t) {
                            reordered = false;
                            break;
                        }
                        if (sortKey[i].time == 0.)
                            sortKey[i].time = obj->getRealTime();
                        ++i;
                    }
                    if (!reordered)
                        break;
                    assert(i == numObject);
                }
                //reordered = mpi::all_reduce(comm(), reordered, std::logical_and<bool>());
                if (reordered) {
                    for (auto &key: sortKey) {
                        if (key.step >= 0 && key.time == 0.)
                            key.time = key.step;
                    }
                    if (numObject > 0) {
                        std::stable_sort(sortKey.begin(), sortKey.end());
#ifdef REDUCE_DEBUG
                        CERR << "SORT ORDER:";
                        for (auto &key: sortKey) {
                            std::cerr << " (" << key.idx << " t=" << key.step << ")";
                        }
                        std::cerr << std::endl;
#endif
                        auto best = sortKey[0];
                        for (auto &ti: sortKey) {
                            if (std::abs(ti.step - exec->animationRealTime()) <
                                std::abs(best.step - exec->animationRealTime())) {
                                best = ti;
                            }
                        }
#ifdef REDUCE_DEBUG
                        CERR << "starting with timestep=" << best.step << ", anim=" << exec->animationRealTime()
                             << ", cur=" << best.time << std::endl;
#endif
                        if (m_numTimesteps > 0) {
                            startTimestep = best.step + direction * headStart;
                        }
                    }

                    if (reducePolicy() == message::ReducePolicy::PerTimestepZeroFirst) {
                        waitForZero = true;
                        startWithZero = true;
                    }
                    if (m_numTimesteps > 0) {
                        while (startTimestep < 0)
                            startTimestep += m_numTimesteps;
                        startTimestep %= m_numTimesteps;
                    }
#ifdef REDUCE_DEBUG
                    CERR << "startTimestep determined to be " << startTimestep << std::endl;
#endif
                }

                if (startTimestep == -1)
                    startTimestep = 0;
                if (direction < 0) {
                    startTimestep = mpi::all_reduce(comm(), startTimestep, mpi::maximum<int>());
                } else {
                    startTimestep = mpi::all_reduce(comm(), startTimestep, mpi::minimum<int>());
                }
                assert(startTimestep >= 0);

                if (reordered) {
                    const int step = direction < 0 ? -1 : 1;
                    // add objects to port queue in processing order
                    for (auto &port: inputPorts) {
                        if (port.second.flags() & Port::NOCOMPUTE)
                            continue;
                        ObjectList received;
                        std::swap(received, port.second.objects());
                        if (!isConnected(port.second))
                            continue;
                        auto cache = m_cache.getObjects(port.first);
                        if (!cache.second) {
                            // FIXME: should collectively ignore cache
                            CERR << "failed to retrieve objects from input cache" << std::endl;
                            cache.first.clear();
                        }
                        auto objs = cache.first;
                        received.clear();
                        // objects without timestep
                        ssize_t cur = step < 0 ? numObject - 1 : 0;
                        for (size_t i = 0; i < numObject; ++i) {
                            if (sortKey[cur].step < 0) {
                                port.second.objects().push_back(objs[sortKey[cur].idx]);
                            }
                            cur = (cur + step + numObject) % numObject;
                        }
                        // objects with timestep 0 (if to be handled first)
                        if (startWithZero) {
                            cur = step < 0 ? numObject - 1 : 0;
                            for (size_t i = 0; i < numObject; ++i) {
                                if (sortKey[cur].step == 0) {
                                    port.second.objects().push_back(objs[sortKey[cur].idx]);
                                }
                                cur = (cur + step + numObject) % numObject;
                            }
                        }
                        // all objects from current timestep until end...
                        bool push = false;
                        cur = step < 0 ? numObject - 1 : 0;
                        for (size_t i = 0; i < numObject; ++i) {
                            if (step > 0) {
                                if (sortKey[cur].step >= startTimestep)
                                    push = true;
                            } else {
                                if (sortKey[cur].step <= startTimestep)
                                    push = true;
                            }
                            if (push && (sortKey[cur].step > 0 || (!startWithZero && sortKey[cur].step == 0))) {
                                port.second.objects().push_back(objs[sortKey[cur].idx]);
                            }
                            cur = (cur + step + numObject) % numObject;
                        }
                        // ...and from start until current timestep
                        cur = step < 0 ? numObject - 1 : 0;
                        for (size_t i = 0; i < numObject; ++i) {
                            if (step > 0) {
                                if (sortKey[cur].step >= startTimestep)
                                    break;
                            } else {
                                if (sortKey[cur].step <= startTimestep)
                                    break;
                            }
                            if (sortKey[cur].step > 0 || (!startWithZero && sortKey[cur].step == 0)) {
                                port.second.objects().push_back(objs[sortKey[cur].idx]);
                            }
                            cur = (cur + step + numObject) % numObject;
                        }
                        if (port.second.objects().size() != numObject) {
                            CERR << "mismatch: expecting " << numObject << " objects, actually have "
                                 << port.second.objects().size() << " at port " << port.first << std::endl;
                        }
                        assert(port.second.objects().size() == numObject);
                    }
                }
            }
            if (gang) {
                numObject = mpi::all_reduce(comm(), numObject, mpi::maximum<int>());
            }
        }

#ifdef REDUCE_DEBUG
        CERR << "NUM OBJECTS: " << numObject << std::endl;
        CERR << "PROCESSING ORDER:";
        for (auto &port: inputPorts) {
            if (port.second.flags() & Port::NOCOMPUTE)
                continue;
            if (!isConnected(port.second))
                continue;
            for (auto &o: port.second.objects()) {
                std::cerr << " " << getTimestep(o);
            }
            std::cerr << std::endl;
        }
#endif

        if (exec->allRanks() || gang || exec->what() == Execute::ComputeExecute) {
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for execCount " << m_executionCount << " with #objects=" << numObject
                 << ", #timesteps=" << m_numTimesteps << std::endl;
#endif
            int oldExecCount = m_executionCount;
            m_executionCount = mpi::all_reduce(comm(), m_executionCount, mpi::maximum<int>());
            if (oldExecCount < m_executionCount) {
                m_iteration = -1;
            }
            m_iteration = mpi::all_reduce(comm(), m_iteration, mpi::maximum<int>());
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for execCount finished " << m_executionCount << " with execCount=" << m_executionCount
                 << std::endl;
#endif
        }

        int prevTimestep = -1;
        if (m_numTimesteps > 0) {
            prevTimestep = startTimestep;
#ifdef REDUCE_DEBUG
            if (exec->what() == message::Execute::ComputeExecute)
                CERR << "last timestep to reduce: " << prevTimestep << std::endl;
#endif
        }
        int numReductions = 0;
        bool reducePerTimestep = reducePolicy() == message::ReducePolicy::PerTimestep ||
                                 reducePolicy() == message::ReducePolicy::PerTimestepZeroFirst ||
                                 reducePolicy() == message::ReducePolicy::PerTimestepOrdered;
        auto runReduce = [this](int timestep, int &numReductions) -> bool {
#ifdef REDUCE_DEBUG
            CERR << "running reduce for timestep " << timestep << ", already did " << numReductions << " of "
                 << m_numTimesteps << " reductions" << std::endl;
            if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never) {
                CERR << "runReduce(t=" << timestep << "): barrier for reduce policy " << reducePolicy() << std::endl;
                comm().barrier();
            }
#endif
            if (timestep >= 0) {
                assert(numReductions < m_numTimesteps);
            }
            if (timestep >= 0) {
                ++numReductions;
                assert(numReductions <= m_numTimesteps);
            }
            if (cancelRequested(true))
                return true;
#ifdef REDUCE_DEBUG
            CERR << "runReduce(t=" << timestep << "): exec count = " << m_executionCount << std::endl;
#endif
            waitAllTasks();
            return reduce(timestep);
        };
        bool computeOk = false;
        for (Index i = 0; i < numObject; ++i) {
            computeOk = false;
            try {
                double start = Clock::time();
                int timestep = -1;
                bool objectIsEmpty = false;
                for (auto &port: inputPorts) {
                    if (port.second.flags() & Port::NOCOMPUTE)
                        continue;
                    if (!isConnected(port.second))
                        continue;
                    const auto &objs = port.second.objects();
                    if (objs.empty()) {
                        objectIsEmpty = true;
                    } else {
                        int t = getTimestep(objs.front());
                        if (t != -1)
                            timestep = t;
                        if (Empty::as(objs.front()))
                            objectIsEmpty = true;
                    }
                }
#ifdef REDUCE_DEBUG
                CERR << "processing object no.: " << i << ", timestep=" << timestep << std::endl;
#endif
                if (cancelRequested()) {
                    computeOk = true;
                } else if (objectIsEmpty) {
                    //CERR << "timestep=" << timestep << ": empty objects, skipping compute" << std::endl;
                    for (auto &port: inputPorts) {
                        if (port.second.flags() & Port::NOCOMPUTE)
                            continue;
                        if (!isConnected(port.second))
                            continue;
                        auto &objs = port.second.objects();
                        if (!objs.empty()) {
                            objs.pop_front();
                        }
                    }
                    computeOk = true;
                } else {
                    computeOk = compute();
                }

                if (reordered && timestep >= 0 && m_numTimesteps > 0 && reducePerTimestep) {
                    // if processing for another timestep starts, run reduction for previous timesteps
                    // -- reordered is a requirement: otherwise, objects have not to be ordered by timestep
                    if (waitForZero) {
                        if (startWithZero && timestep > 0) {
                            waitForZero = false;
                            computeOk &= runReduce(0, numReductions);
                        }
                    } else if (direction >= 0) {
                        if (prevTimestep > timestep) {
                            for (int t = prevTimestep; t < m_numTimesteps; ++t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                            for (int t = 0; t < timestep; ++t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        } else {
                            for (int t = prevTimestep; t < timestep; ++t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        }
                        prevTimestep = timestep;
                    } else if (direction < 0) {
                        if (prevTimestep < timestep) {
                            for (int t = prevTimestep; t >= 0; --t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                            for (int t = m_numTimesteps - 1; t > timestep; --t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        } else {
                            for (int t = prevTimestep; t > timestep; --t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        }
                        prevTimestep = timestep;
                    }
                }
                double duration = Clock::time() - start;
                if (m_avgComputeTime == 0.)
                    m_avgComputeTime = duration;
                else
                    m_avgComputeTime = 0.95 * m_avgComputeTime + 0.05 * duration;
            } catch (boost::interprocess::interprocess_exception &e) {
                std::cout << name() << "::compute(): interprocess_exception: " << e.what()
                          << ", error code: " << e.get_error_code() << ", native error: " << e.get_native_error()
                          << std::endl
                          << std::flush;
                CERR << name() << "::compute(): interprocess_exception: " << e.what()
                     << ", error code: " << e.get_error_code() << ", native error: " << e.get_native_error()
                     << std::endl;
                throw(e); // rethrow and probably crash - there is no guarantee that the module terminates gracefully
            } catch (std::exception &e) {
                std::cout << name() << "::compute(" << i << "): exception - " << e.what() << std::endl << std::flush;
                CERR << name() << "::compute(" << i << "): exception - " << e.what() << std::endl;
                throw(e);
            }
            ret &= computeOk;
            if (!computeOk) {
                break;
            }
        }

        if (reordered && m_numTimesteps > 0 && reducePerTimestep) {
            // run reduction for remaining (most often just the last) timesteps
#ifdef REDUCE_DEBUG
            CERR << "doing outstanding " << m_numTimesteps - numReductions << " reductions" << std::endl;
#endif
            if (startWithZero && waitForZero) {
                waitForZero = false;
                assert(numReductions < m_numTimesteps);
                runReduce(0, numReductions);
            }
            int t = prevTimestep;
            while (numReductions < m_numTimesteps) {
                if (t != 0 || !startWithZero)
                    runReduce(t, numReductions);
                if (direction >= 0) {
                    t = (t + 1) % m_numTimesteps;
                } else {
                    t = (t + m_numTimesteps - 1) % m_numTimesteps;
                }
            }
            assert(numReductions == m_numTimesteps);
        }
    }

    if (exec->what() == Execute::ComputeExecute || exec->what() == Execute::Reduce) {
        waitAllTasks();
        ret &= reduceWrapper(exec, reordered);
        m_cache.clearOld();
    }
#ifdef DETAILED_PROGRESS
    message::Idle idle;
    idle.setReferrer(exec->uuid());
    idle.setDestId(Id::LocalManager);
    sendMessage(idle);
#endif

#ifdef REDUCE_DEBUG
    CERR << "EXEC FINISHED: count=" << m_executionCount << std::endl;
#endif

    return ret;
}

std::string Module::getModuleName(int id) const
{
    return m_stateTracker->getModuleName(id);
}

int Module::mirrorId() const
{
    return m_stateTracker->getMirrorId(m_id);
}

std::set<int> Module::getMirrors() const
{
    auto m = m_stateTracker->getMirrors(m_id);
    m.erase(m_id);
    return m;
}

void Module::execute() const
{
    message::Execute exec{message::Execute::ComputeExecute, m_id, m_executionCount};
    exec.setDestId(message::Id::MasterHub);
    sendMessage(exec);
}


Module::~Module()
{
    if (m_readyForQuit) {
        comm().barrier();
    } else {
        CERR << "Emergency quit" << std::endl;
    }

    vistle::message::ModuleExit m;
    m.setDestId(Id::ForBroadcast);
    sendMessage(m);

    delete sendMessageQueue;
    sendMessageQueue = nullptr;
    delete receiveMessageQueue;
    receiveMessageQueue = nullptr;

    if (m_origStreambuf)
        std::cerr.rdbuf(m_origStreambuf);
    delete m_streambuf;
    m_streambuf = nullptr;
}

void Module::eventLoop()
{
    initDone();
    while (dispatch())
        ;
    prepareQuit();
}

void Module::sendText(int type, const std::string &msg) const
{
    message::SendText info(static_cast<message::SendText::TextType>(type));
    message::SendText::Payload pl(msg);
    sendMessageWithPayload(info, pl);
}

static void sendTextVarArgs(const Module *self, message::SendText::TextType type, const char *fmt, va_list args)
{
    if (!fmt) {
        fmt = "(empty message)";
    }
    std::vector<char> text(strlen(fmt) + 500);
    vsnprintf(text.data(), text.size(), fmt, args);
    switch (type) {
    case message::SendText::Error:
        std::cout << "ERR:  ";
        break;
    case message::SendText::Warning:
        std::cout << "WARN: ";
        break;
    case message::SendText::Info:
        std::cout << "INFO: ";
        break;
    default:
        break;
    }
    std::cout << text.data() << std::endl;
    message::SendText info(type);
    message::SendText::Payload pl(text.data());
    self->sendMessageWithPayload(info, pl);
}

void Module::sendInfo(const char *fmt, ...) const
{
    va_list args;
    va_start(args, fmt);
    sendTextVarArgs(this, message::SendText::Info, fmt, args);
    va_end(args);
}

void Module::sendInfo(const std::string &text) const
{
    std::cout << "INFO: " << text << std::endl;
    message::SendText info(message::SendText::Info);
    message::SendText::Payload pl(text);
    sendMessageWithPayload(info, pl);
}

void Module::sendWarning(const char *fmt, ...) const
{
    va_list args;
    va_start(args, fmt);
    sendTextVarArgs(this, message::SendText::Warning, fmt, args);
    va_end(args);
}

void Module::sendWarning(const std::string &text) const
{
    std::cout << "WARN: " << text << std::endl;
    message::SendText info(message::SendText::Warning);
    message::SendText::Payload pl(text);
    sendMessageWithPayload(info, pl);
}

void Module::sendError(const char *fmt, ...) const
{
    va_list args;
    va_start(args, fmt);
    sendTextVarArgs(this, message::SendText::Error, fmt, args);
    va_end(args);
}

void Module::sendError(const std::string &text) const
{
    std::cout << "ERR:  " << text << std::endl;
    message::SendText info(message::SendText::Error);
    message::SendText::Payload pl(text);
    sendMessageWithPayload(info, pl);
}

void Module::sendError(const message::Message &msg, const char *fmt, ...) const
{
    if (!fmt) {
        fmt = "(empty message)";
    }
    std::vector<char> text(strlen(fmt) + 500);
    va_list args;
    va_start(args, fmt);
    vsnprintf(text.data(), text.size(), fmt, args);
    va_end(args);

    std::cout << "ERR:  " << text.data() << std::endl;
    message::SendText info(msg);
    message::SendText::Payload pl(text.data());
    sendMessageWithPayload(info, pl);
}

void Module::sendError(const message::Message &msg, const std::string &text) const
{
    std::cout << "ERR:  " << text << std::endl;
    message::SendText info(msg);
    message::SendText::Payload pl(text);
    sendMessageWithPayload(info, pl);
}

void Module::setObjectReceivePolicy(int pol)
{
    m_receivePolicy = pol;
    sendMessage(message::ObjectReceivePolicy(message::ObjectReceivePolicy::Policy(pol)));
}

int Module::objectReceivePolicy() const
{
    return m_receivePolicy;
}

void Module::startIteration()
{
    ++m_iteration;
    clearResultCaches();
}

bool Module::prepareWrapper(const message::Execute *exec)
{
#ifndef DETAILED_PROGRESS
    message::Busy busy;
    busy.setReferrer(exec->uuid());
    busy.setDestId(Id::LocalManager);
    sendMessage(busy);
#endif

    m_numTimesteps = 0;
    m_cancelRequested = false;
    m_cancelExecuteCalled = false;
    m_executeAfterCancelFound = false;

    clearResultCaches();

    m_withOutput.clear();

    bool collective =
        reducePolicy() != message::ReducePolicy::Never && reducePolicy() != message::ReducePolicy::Locally;
    if (reducePolicy() != message::ReducePolicy::Never) {
        assert(!m_prepared);
    }
    assert(!m_computed);

    m_reduced = false;

#ifdef REDUCE_DEBUG
    if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never) {
        CERR << "prepare(): barrier for reduce policy " << reducePolicy() << std::endl;
        comm().barrier();
    }
#endif

    message::ExecutionProgress start(message::ExecutionProgress::Start, m_executionCount);
    start.setReferrer(exec->uuid());
    start.setDestId(Id::LocalManager);
    sendMessage(start);

    if (collective) {
        int oldExecCount = m_executionCount;
        m_executionCount = boost::mpi::all_reduce(comm(), m_executionCount, boost::mpi::maximum<int>());
        if (oldExecCount < m_executionCount) {
            m_iteration = -1;
        }
        m_iteration = mpi::all_reduce(comm(), m_iteration, mpi::maximum<int>());
    }

    if (m_benchmark) {
        comm().barrier();
        m_benchmarkStart = Clock::time();
    }

    //CERR << "prepareWrapper: prepared=" << m_prepared << std::endl;
    m_prepared = true;

    if (cancelRequested(collective))
        return true;

    if (reducePolicy() == message::ReducePolicy::Never)
        return true;

    return prepare();
}

bool Module::prepare()
{
#ifndef NDEBUG
    if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never)
        comm().barrier();
#endif
    return true;
}

bool Module::compute()
{
    auto task = std::make_shared<BlockTask>(this);
    if (m_lastTask) {
        task->addDependency(m_lastTask);
    }
    m_lastTask = task;

    int concurrency = m_concurrency->getValue();
    if (concurrency <= 0)
        concurrency = hardware_concurrency() / 2;
    if (concurrency <= 1)
        concurrency = 1;

    while (m_tasks.size() >= unsigned(concurrency)) {
        m_tasks.front()->wait();
        m_tasks.pop_front();
    }
    m_tasks.push_back(task);

    std::unique_lock<std::mutex> guard(task->m_mutex);
    auto tname = name() + ":Block:" + std::to_string(m_tasks.size());
    task->m_future = std::async(std::launch::async, [this, tname, task] {
        setThreadName(tname);
        return compute(task);
    });
    return true;
}

bool Module::compute(const std::shared_ptr<BlockTask> &task) const
{
    (void)task;
    CERR << "compute() or compute(std::shared_ptr<BlockTask>) should be reimplemented from vistle::Module" << std::endl;
    return false;
}

bool Module::reduceWrapper(const message::Execute *exec, bool reordered)
{
    //CERR << "reduceWrapper: prepared=" << m_prepared << ", exec count = " << m_executionCount << std::endl;

    assert(m_prepared);
    if (reducePolicy() != message::ReducePolicy::Never) {
        assert(!m_reduced);
    }

#ifdef REDUCE_DEBUG
    if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never) {
        CERR << "reduce(): barrier for reduce policy " << reducePolicy() << ", request was " << *exec << std::endl;
        comm().barrier();
    }
#endif

    bool sync = false;
    if (reducePolicy() != message::ReducePolicy::Never && reducePolicy() != message::ReducePolicy::Locally) {
        sync = true;
        m_numTimesteps = boost::mpi::all_reduce(comm(), m_numTimesteps, boost::mpi::maximum<int>());
    }

    m_reduced = true;

    bool ret = true;
    try {
        switch (reducePolicy()) {
        case message::ReducePolicy::Never: {
            break;
        }
        case message::ReducePolicy::PerTimestep:
        case message::ReducePolicy::PerTimestepZeroFirst:
        case message::ReducePolicy::PerTimestepOrdered: {
            if (!reordered) {
                for (int t = 0; t < m_numTimesteps; ++t) {
                    if (!cancelRequested(sync)) {
                        //CERR << "run reduce(t=" << t << "): exec count = " << m_executionCount << std::endl;
                        ret &= reduce(t);
                    }
                }
            }
        }
        // FALLTHRU
        case message::ReducePolicy::Locally:
        case message::ReducePolicy::OverAll: {
            if (!cancelRequested(sync)) {
                //CERR << "run reduce(t=" << -1 << "): exec count = " << m_executionCount << std::endl;
                ret = reduce(-1);
            }
            break;
        }
        }
    } catch (std::exception &e) {
        ret = false;
        std::cout << name() << "::reduce(): exception - " << e.what() << std::endl << std::flush;
        CERR << name() << "::reduce(): exception - " << e.what() << std::endl;
    }

    for (auto &port: outputPorts) {
        if (isConnected(port.second) && m_withOutput.find(&port.second) == m_withOutput.end()) {
            Empty::ptr empty(new Empty(Object::Initialized));
            updateMeta(empty);
            addObject(&port.second, empty);
        }
    }

    if (sync) {
        m_cancelRequested = boost::mpi::all_reduce(comm(), m_cancelRequested, std::logical_or<bool>());
    }

    if (m_benchmark) {
        comm().barrier();
        double duration = Clock::time() - m_benchmarkStart;
        if (rank() == 0) {
#ifdef _OPENMP
            int nthreads = omp_get_max_threads();
            sendInfo("compute() took %fs (OpenMP threads: %d)", duration, nthreads);
            printf("%s:%d: compute() took %fs (OpenMP threads: %d)", name().c_str(), id(), duration, nthreads);
#else
            sendInfo("compute() took %fs (no OpenMP)", duration);
            printf("%s:%d: compute() took %fs (no OpenMP)", name().c_str(), id(), duration);
#endif
        }
    }

    message::ExecutionProgress fin(message::ExecutionProgress::Finish, m_executionCount);
    fin.setReferrer(exec->uuid());
    fin.setDestId(Id::LocalManager);
    sendMessage(fin);

    clearResultCaches();

#ifndef DETAILED_PROGRESS
    message::Idle idle;
    idle.setReferrer(exec->uuid());
    idle.setDestId(Id::LocalManager);
    sendMessage(idle);
#endif

    m_computed = false;
    m_prepared = false;
    m_upstreamIsExecuting = false;

    if (m_delayedBarrierResponse) {
        sendMessage(*m_delayedBarrierResponse);
        m_delayedBarrierResponse.reset();
    }

    return ret;
}

bool Module::reduce(int timestep)
{
#ifndef NDEBUG
    if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never)
        comm().barrier();
#endif
    return true;
}

bool Module::cancelExecute()
{
    std::cerr << "canceling execution" << std::endl;
    if (rank() == 0)
        sendInfo("canceling execution");

    m_upstreamIsExecuting = false;

    if (m_delayedBarrierResponse) {
        sendMessage(*m_delayedBarrierResponse);
        m_delayedBarrierResponse.reset();
    }

    return true;
}

int Module::numTimesteps() const
{
    return m_numTimesteps;
}

void Module::setStatus(const std::string &text, message::UpdateStatus::Importance prio)
{
    message::UpdateStatus status(text, prio);
    status.setDestId(Id::ForBroadcast);
    sendMessage(status);
}

void Module::clearStatus()
{
    setStatus(std::string(), message::UpdateStatus::Bulk);
}

bool Module::cancelRequested(bool collective)
{
    message::Buffer buf;
    while (!m_executeAfterCancelFound && receiveMessageQueue && receiveMessageQueue->tryReceive(buf)) {
        messageBacklog.push_back(buf);
        switch (buf.type()) {
        case message::CANCELEXECUTE: {
            const auto &cancel = buf.as<message::CancelExecute>();
            if (cancel.getModule() == id()) {
                CERR << "canceling execution requested" << std::endl;
                m_cancelRequested = true;
            }
            break;
        }
        case message::EXECUTE: {
            if (m_cancelRequested) {
                m_executeAfterCancelFound = true;
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    if (collective) {
#ifdef REDUCE_DEBUG
        CERR << "running all_reduce for CANCEL: requested=" << m_cancelRequested << std::endl;
#endif
        m_cancelRequested = boost::mpi::all_reduce(comm(), m_cancelRequested, std::logical_or<bool>());
    }

    if (m_cancelRequested && !m_cancelExecuteCalled) {
        cancelExecute();
        m_cancelExecuteCalled = true;
    }

#ifdef REDUCE_DEBUG
    if (m_cancelRequested) {
        CERR << "CANCEL requested!" << std::endl;
    }
#endif

    return m_cancelRequested;
}

bool Module::wasCancelRequested() const
{
    return m_cancelRequested;
}

void Module::cancelExecuteMessageReceived(const message::Message *msg)
{
    (void)msg;
    return;
}

#undef CERR
#define CERR \
    std::cerr << m_module->m_name << "_" << m_module->id() << ":task" \
              << " [" << m_module->rank() << "/" << m_module->size() << "] "

BlockTask::BlockTask(Module *module): m_module(module)
{
    for (auto &p: module->inputPorts) {
        m_portsByString[p.first] = &p.second;
        if (module->isConnected(p.second)) {
            if (module->hasObject(&p.second)) {
                m_input[&p.second] = module->takeFirstObject(&p.second);
            } else {
                CERR << "no input on port " << p.first << std::endl;
            }
        }
    }
    for (auto &p: module->outputPorts) {
        m_portsByString[p.first] = &p.second;
        m_ports.insert(&p.second);
    }
}

BlockTask::~BlockTask()
{
    assert(m_dependencies.empty());
    assert(m_objects.empty());
}

bool BlockTask::hasObject(const Port *p)
{
    auto it = m_input.find(p);
    return it != m_input.end();
}

Object::const_ptr BlockTask::takeObject(const Port *p)
{
    auto it = m_input.find(p);
    if (it == m_input.end())
        return Object::const_ptr();

    auto ret = it->second;
    m_input.erase(it);
    return ret;
}

void BlockTask::addDependency(std::shared_ptr<BlockTask> dep)
{
    m_dependencies.insert(dep);
}

void BlockTask::addObject(Port *port, Object::ptr obj)
{
    assert(m_ports.find(port) != m_ports.end());
    m_objects[port].emplace_back(obj);
}

void BlockTask::addObject(const std::string &port, Object::ptr obj)
{
    auto it = m_portsByString.find(port);
    assert(it != m_portsByString.end());
    if (it == m_portsByString.end()) {
        CERR << "BlockTask: port '" << port << "' not found" << std::endl;
        return;
    }
    addObject(it->second, obj);
}

void BlockTask::addAllObjects()
{
    for (auto &port_queue: m_objects) {
        auto &port = port_queue.first;
        auto &queue = port_queue.second;
        for (auto &obj: queue)
            m_module->addObject(port, obj);
    }

    m_objects.clear();
}

bool BlockTask::wait()
{
    waitDependencies();
    bool result = m_future.get();
    addAllObjects();
    return result;
}

bool BlockTask::waitDependencies()
{
    std::unique_lock<std::mutex> guard(m_mutex);
    for (auto it = m_dependencies.begin(); it != m_dependencies.end(); ++it) {
        auto &d = *it;
        d->wait();
    }

    m_dependencies.clear();

    return true;
}

template<class Payload>
bool Module::sendMessageWithPayload(message::Message &message, Payload &payload) const
{
    auto pl = addPayload(message, payload);
    return this->sendMessage(message, &pl);
}

int Module::shmLeader(int rank) const
{
    if (rank == -1)
        rank = this->rank();
    return m_shmLeaders[rank];
}

} // namespace vistle
