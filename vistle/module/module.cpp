#include <cstdlib>
#include <cstdio>

#include <util/hostname.h>
#include <sys/types.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <deque>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>
#include <mutex>
#include <boost/asio.hpp>

#include <util/sysdep.h>
#include <util/tools.h>
#include <util/stopwatch.h>
#include <util/exception.h>
#include <core/object.h>
#include <core/empty.h>
#include <core/export.h>
#include <core/message.h>
#include <core/messagequeue.h>
#include <core/messagerouter.h>
#include <core/parameter.h>
#include <core/shm.h>
#include <core/port.h>
#include <core/statetracker.h>

#include "objectcache.h"

#ifndef TEMPLATES_IN_HEADERS
#define VISTLE_IMPL
#endif
#include "module.h"

#include <core/shm_reference.h>
#include <core/archive_saver.h>
#include <core/archive_loader.h>

//#define DEBUG
//#define REDUCE_DEBUG
//#define DETAILED_PROGRESS

#define CERR std::cerr << m_name << "_" << id() << " [" << rank() << "/" << size() << "] "

namespace interprocess = ::boost::interprocess;
namespace mpi = ::boost::mpi;

namespace vistle {

using message::Id;

template<typename CharT, typename TraitsT = std::char_traits<CharT> >
class msgstreambuf: public std::basic_streambuf<CharT, TraitsT> {

 public:
   msgstreambuf(Module *mod)
   : m_module(mod)
   , m_console(true)
   , m_gui(true)
   {}

   ~msgstreambuf() {

      std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
      flush();
      for (const auto &s: m_backlog) {
         std::cout << s << std::endl;
         if (m_gui)
            m_module->sendText(message::SendText::Cerr, s);
      }
      m_backlog.clear();
   }

   void flush(ssize_t count=-1) {
      std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
      size_t size = count<0 ? m_buf.size() : count;
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
         m_buf.erase(m_buf.begin(), m_buf.begin()+size);
      }
   }

   int overflow(int ch) {
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

   std::streamsize xsputn (const CharT *s, std::streamsize num) {
      std::unique_lock<std::recursive_mutex> scoped_lock(m_mutex);
      size_t end = m_buf.size();
      m_buf.resize(end+num);
      memcpy(m_buf.data()+end, s, num);
      auto it = std::find(m_buf.rbegin(), m_buf.rend(), '\n');
      if (it != m_buf.rend()) {
         flush(it - m_buf.rend());
      }
      return num;
   }

   void set_console_output(bool enable) {
      m_console = enable;
   }

   void set_gui_output(bool enable) {
      m_gui = enable;
   }

   void clear_backlog() {
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


namespace {
int getTimestep(Object::const_ptr obj) {

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
}

bool Module::setup(const std::string &shmname, int moduleID, int rank) {

#ifndef MODULE_THREAD
    Shm::attach(shmname, moduleID, rank);
#endif
    return Shm::isAttached();
}

Module::Module(const std::string &desc,
      const std::string &moduleName, const int moduleId, mpi::communicator comm)
: ParameterManager(moduleName, moduleId)
, m_name(moduleName)
, m_rank(-1)
, m_size(-1)
, m_id(moduleId)
, m_executionCount(0)
, m_iteration(-1)
, m_stateTracker(new StateTracker(m_name))
, m_receivePolicy(message::ObjectReceivePolicy::Local)
, m_schedulingPolicy(message::SchedulingPolicy::Single)
, m_reducePolicy(message::ReducePolicy::Locally)
, m_executionDepth(0)
, m_defaultCacheMode(ObjectCache::CacheNone)
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

#ifndef MODULE_THREAD
   message::DefaultSender::init(m_id, m_rank);
#endif

   // names are swapped relative to communicator
   std::string smqName = message::MessageQueue::createName("recv", id(), rank());
   try {
      sendMessageQueue = message::MessageQueue::open(smqName);
   } catch (interprocess::interprocess_exception &ex) {
      throw vistle::exception(std::string("opening send message queue ") + smqName + ": " + ex.what());
   }

   std::string rmqName = message::MessageQueue::createName("send", id(), rank());
   try {
      receiveMessageQueue = message::MessageQueue::open(rmqName);
   } catch (interprocess::interprocess_exception &ex) {
      throw vistle::exception(std::string("opening receive message queue ") + rmqName + ": " + ex.what());
   }

#ifdef DEBUG
   std::cerr << "  module [" << name() << "] [" << id() << "] [" << rank()
             << "/" << size() << "] started as " << hostname() << ":"
#ifndef _WIN32
             << getpid()
#endif
             << std::endl;
#endif

   auto cm = addIntParameter("_cache_mode", "input object caching", ObjectCache::CacheDefault, Parameter::Choice);
   V_ENUM_SET_CHOICES_SCOPE(cm, CacheMode, ObjectCache);
   addIntParameter("_prioritize_visible", "prioritize currently visible timestep", m_prioritizeVisible, Parameter::Boolean);

   addVectorParameter("_position", "position in GUI", ParamVector(0., 0.));

   auto em = addIntParameter("_error_output_mode", "where stderr is shown", size()==1 ? 1 : 1, Parameter::Choice);
   std::vector<std::string> errmodes;
   errmodes.push_back("No output");
   errmodes.push_back("Console only");
   errmodes.push_back("GUI");
   errmodes.push_back("Console & GUI");
   setParameterChoices(em, errmodes);

   auto outrank = addIntParameter("_error_output_rank", "rank from which to show stderr (-1: all ranks)", -1);
   setParameterRange<Integer>(outrank, -1, size()-1);

   auto openmp_threads = addIntParameter("_openmp_threads", "number of OpenMP threads (0: system default)", 0);
   setParameterRange<Integer>(openmp_threads, 0, 4096);
   addIntParameter("_benchmark", "show timing information", m_benchmark ? 1 : 0, Parameter::Boolean);
}

void Module::prepareQuit() {

#ifdef DEBUG
    CERR << "I'm quitting" << std::endl;
#endif

    if (!m_readyForQuit) {
        ParameterManager::quit();

        m_cache.clear();
        m_cache.clearOld();

        vistle::message::ModuleExit m;
        m.setDestId(Id::ForBroadcast);
        sendMessage(m);
    }

    m_readyForQuit = true;
}

const HubData &Module::getHub() const {

    int hubId = m_stateTracker->getHub(id());
    return m_stateTracker->getHubData(hubId);
}

void Module::initDone() {

#ifndef MODULE_THREAD
   m_streambuf = new msgstreambuf<char>(this);
   m_origStreambuf = std::cerr.rdbuf(m_streambuf);
#endif

   message::Started start(name());
   start.setDestId(Id::ForBroadcast);
   sendMessage(start);

   ParameterManager::init();
}

const std::string &Module::name() const {

   return m_name;
}

int Module::id() const {

   return m_id;
}

const mpi::communicator &Module::comm() const {

   return m_comm;
}

int Module::rank() const {

   return m_rank;
}

int Module::size() const {

   return m_size;
}

int Module::openmpThreads() const {

    int ret = getIntParameter("_openmp_threads");
    if (ret <= 0) {
        ret = 0;
    }
    return ret;
}

void Module::setOpenmpThreads(int nthreads, bool updateParam) {

#ifdef _OPENMP
    if (nthreads <= 0)
       nthreads = omp_get_num_procs();
    if (nthreads > 0)
       omp_set_num_threads(nthreads);
#endif
    if (updateParam)
        setIntParameter("_openmp_threads", nthreads);
}

void Module::enableBenchmark(bool benchmark, bool updateParam) {

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

bool Module::syncMessageProcessing() const {

   return m_syncMessageProcessing;
}

void Module::setSyncMessageProcessing(bool sync) {

   m_syncMessageProcessing = sync;
}

ObjectCache::CacheMode Module::setCacheMode(ObjectCache::CacheMode mode, bool updateParam) {

   if (mode == ObjectCache::CacheDefault)
      m_cache.setCacheMode(m_defaultCacheMode);
   else
      m_cache.setCacheMode(mode);

   if (updateParam)
      setIntParameter("_cache_mode", mode);

   return m_cache.cacheMode();
}

void Module::setDefaultCacheMode(ObjectCache::CacheMode mode) {

   vassert(mode != ObjectCache::CacheDefault);
   m_defaultCacheMode = mode;
   setCacheMode(m_defaultCacheMode, false);
}


ObjectCache::CacheMode Module::cacheMode(ObjectCache::CacheMode mode) const {

   return m_cache.cacheMode();
}

bool Module::havePort(const std::string &name) {

   auto param = findParameter(name);
   if (param)
       return true;

   std::map<std::string, Port*>::iterator iout = outputPorts.find(name);
   if (iout != outputPorts.end())
       return true;

   std::map<std::string, Port*>::iterator iin = inputPorts.find(name);
   if (iin != inputPorts.end())
       return true;

   return false;
}

Port *Module::createInputPort(const std::string &name, const std::string &description, const int flags) {

   vassert(!havePort(name));
   if (havePort(name)) {
      CERR << "createInputPort: already have port/parameter with name " << name << std::endl;
      return nullptr;
   }

   Port *p = new Port(id(), name, Port::INPUT, flags);
   p->setDescription(description);
   inputPorts[name] = p;

   message::AddPort message(*p);
   message.setDestId(Id::ForBroadcast);
   sendMessage(message);
   return p;
}

Port *Module::createOutputPort(const std::string &name, const std::string &description, const int flags) {

   vassert(!havePort(name));
   if (havePort(name)) {
      CERR << "createOutputPort: already have port/parameter with name " << name << std::endl;
      return nullptr;
   }

   Port *p = new Port(id(), name, Port::OUTPUT, flags);
   p->setDescription(description);
   outputPorts[name] = p;

   message::AddPort message(*p);
   message.setDestId(Id::ForBroadcast);
   sendMessage(message);
   return p;
}

bool Module::destroyPort(const std::string &portName) {

   Port *p = findInputPort(portName);
   if (!p)
      p = findOutputPort(portName);
   if (!p)
      return false;

   vassert(p);
   return destroyPort(p);
}

bool Module::destroyPort(Port *port) {

   vassert(port);
   message::RemovePort message(*port);
   message.setDestId(Id::ForBroadcast);
   if (Port *p = findInputPort(port->getName())) {
       inputPorts.erase(port->getName());
       delete p;
   } else if (Port *p = findOutputPort(port->getName())) {
       outputPorts.erase(port->getName());
       delete p;
   } else {
       return false;
   }

   sendMessage(message);
   return true;
}

Port *Module::findInputPort(const std::string &name) const {

   std::map<std::string, Port *>::const_iterator i = inputPorts.find(name);

   if (i == inputPorts.end())
      return NULL;

   return i->second;
}

Port *Module::findOutputPort(const std::string &name) const {

   std::map<std::string, Port *>::const_iterator i = outputPorts.find(name);

   if (i == outputPorts.end())
      return NULL;

   return i->second;
}

Parameter *Module::addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> param) {

   vassert(!havePort(name));
   if (havePort(name)) {
       CERR << "addParameterGeneric: already have port/parameter with name " << name << std::endl;
      return nullptr;
   }

   return ParameterManager::addParameterGeneric(name, param);
}

bool Module::removeParameter(Parameter *param) {

   std::string name = param->getName();
   vassert(havePort(name));
   if (!havePort(name)) {
      CERR << "removeParameter: no port with name " << name << std::endl;
      return false;
   }

   return ParameterManager::removeParameter(param);
}

bool Module::sendObject(const mpi::communicator &comm, Object::const_ptr obj, int destRank) const {

    auto saver = std::make_shared<DeepArchiveSaver>();
    vecostreambuf<char> memstr;
    vistle::oarchive memar(memstr);
    memar.setSaver(saver);
    obj->saveObject(memar);
    const std::vector<char> &mem = memstr.get_vector();
    comm.send(destRank, 0, mem);
    auto dir = saver->getDirectory();
    comm.send(destRank, 0, dir);
    for (auto &ent: dir) {
        comm.send(destRank, 0, ent.data, ent.size);
    }
    return true;
}

bool Module::sendObject(Object::const_ptr object, int destRank) const {
    return sendObject(comm(), object, destRank);
}

Object::const_ptr Module::receiveObject(const mpi::communicator &comm, int sourceRank) const {

    std::vector<char> mem;
    comm.recv(sourceRank, 0, mem);
    vistle::SubArchiveDirectory dir;
    std::map<std::string, std::vector<char>> objects, arrays;
    comm.recv(sourceRank, 0, dir);
    for (auto &ent: dir) {
        if (ent.is_array) {
            arrays[ent.name].resize(ent.size);
            ent.data = arrays[ent.name].data();
        } else {
            objects[ent.name].resize(ent.size);
            ent.data = objects[ent.name].data();
        }
        comm.recv(sourceRank, 0, ent.data, ent.size);
    }
    vecistreambuf<char> membuf(mem);
    vistle::iarchive memar(membuf);
    auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays);
    memar.setFetcher(fetcher);
    Object::ptr p(Object::loadObject(memar));
    //std::cerr << "receiveObject " << p->getName() << ": refcount=" << p->refcount() << std::endl;
    return p;
}

Object::const_ptr Module::receiveObject(int destRank) const {
    return receiveObject(comm(), destRank);
}

bool Module::broadcastObject(const mpi::communicator &comm, Object::const_ptr &obj, int root) const {

    if (rank() == root) {
        vecostreambuf<char> memstr;
        vistle::oarchive memar(memstr);
        auto saver = std::make_shared<DeepArchiveSaver>();
        memar.setSaver(saver);
        obj->saveObject(memar);
        const std::vector<char> &mem = memstr.get_vector();
        mpi::broadcast(comm, const_cast<std::vector<char>&>(mem), root);
        auto dir = saver->getDirectory();
        mpi::broadcast(comm, dir, root);
        for (auto &ent: dir) {
            mpi::broadcast(comm, ent.data, ent.size, root);
        }
    } else {
        std::vector<char> mem;
        mpi::broadcast(comm, mem, root);
        vistle::SubArchiveDirectory dir;
        std::map<std::string, std::vector<char>> objects, arrays;
        mpi::broadcast(comm, dir, root);
        for (auto &ent: dir) {
            if (ent.is_array) {
                arrays[ent.name].resize(ent.size);
                ent.data = arrays[ent.name].data();
            } else {
                objects[ent.name].resize(ent.size);
                ent.data = objects[ent.name].data();
            }
            mpi::broadcast(comm, ent.data, ent.size, root);
        }
        vecistreambuf<char> membuf(mem);
        vistle::iarchive memar(membuf);
        auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays);
        memar.setFetcher(fetcher);
        obj.reset(Object::loadObject(memar));
        //std::cerr << "broadcastObject recv " << obj->getName() << ": refcount=" << obj->refcount() << std::endl;
        //obj->unref();
    }

    return true;
}

bool Module::broadcastObject(Object::const_ptr &object, int root) const {

    return broadcastObject(comm(), object, root);
}

void Module::updateCacheMode() {

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

void Module::updateOutputMode() {

#ifndef MODULE_THREAD
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
}

void Module::updateMeta(vistle::Object::ptr obj) const {

   if (obj) {
      obj->setCreator(id());
      obj->setExecutionCounter(m_executionCount);
      if (obj->getIteration() < m_iteration)
          obj->setIteration(m_iteration);
   }
}

bool Module::addObject(const std::string &portName, vistle::Object::ptr object) {

   std::map<std::string, Port *>::iterator i = outputPorts.find(portName);
   if (i != outputPorts.end()) {
      return addObject(i->second, object);
   }
   CERR << "Module::addObject: output port " << portName << " not found" << std::endl;
   vassert(i != outputPorts.end());
   return false;
}

bool Module::addObject(Port *port, vistle::Object::ptr object) {

   if (object)
       object->updateInternals();
   updateMeta(object);
   vistle::Object::const_ptr cobj = object;
   return passThroughObject(port, cobj);
}

bool Module::passThroughObject(const std::string &portName, vistle::Object::const_ptr object) {
   std::map<std::string, Port *>::iterator i = outputPorts.find(portName);
   if (i != outputPorts.end()) {
      return passThroughObject(i->second, object);
   }
   CERR << "Module::passThroughObject: output port " << portName << " not found" << std::endl;
   vassert(i != outputPorts.end());
   return false;
}

bool Module::passThroughObject(Port *port, vistle::Object::const_ptr object) {

   if (!object)
      return false;

   m_withOutput.insert(port);

   object->refresh();
   vassert(object->check());

   message::AddObject message(port->getName(), object);
   sendMessage(message);
   return true;
}

ObjectList Module::getObjects(const std::string &portName) {

   ObjectList objects;
   std::map<std::string, Port *>::iterator i = inputPorts.find(portName);

   if (i != inputPorts.end()) {

      ObjectList &olist = i->second->objects();
      for (ObjectList::const_iterator it = olist.begin(); it != olist.end(); it++) {
         Object::const_ptr object = *it;
         if (object.get()) {
            vassert(object->check());
         }
         objects.push_back(object);
      }
   } else {
      CERR << "Module::getObjects: input port " << portName << " not found" << std::endl;
      vassert(i != inputPorts.end());
   }

   return objects;
}

bool Module::hasObject(const std::string &portName) const {

   std::map<std::string, Port *>::const_iterator i = inputPorts.find(portName);

   if (i == inputPorts.end()) {
      CERR << "Module::hasObject: input port " << portName << " not found" << std::endl;
      vassert(i != inputPorts.end());

      return false;
   }

   return hasObject(i->second);
}

bool Module::hasObject(const Port *port) const {

   return !port->objects().empty();
}

vistle::Object::const_ptr Module::takeFirstObject(const std::string &portName) {

   std::map<std::string, Port *>::iterator i = inputPorts.find(portName);

   if (i == inputPorts.end()) {
      CERR << "Module::takeFirstObject: input port " << portName << " not found" << std::endl;
      vassert(i != inputPorts.end());
      return vistle::Object::ptr();
   }

   return takeFirstObject(i->second);
}

void Module::requestPortMapping(unsigned short forwardPort, unsigned short localPort) {

   message::RequestTunnel m(forwardPort, localPort);
   m.setDestId(Id::LocalManager);
   sendMessage(m);
}

void Module::removePortMapping(unsigned short forwardPort) {

   message::RequestTunnel m(forwardPort);
   m.setDestId(Id::LocalManager);
   sendMessage(m);
}

vistle::Object::const_ptr Module::takeFirstObject(Port *port) {

   if (!port->objects().empty()) {

      Object::const_ptr obj = port->objects().front();
      vassert(obj->check());
      port->objects().pop_front();
      return obj;
   }

   return vistle::Object::ptr();
}

// specialized for avoiding Object::type(), which does not exist
template<>
Object::const_ptr Module::expect<Object>(Port *port) {
   if (!port) {
       std::stringstream str;
       str << "invalid port" << std::endl;
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
   vassert(obj->check());
   return obj;
}


bool Module::addInputObject(int sender, const std::string &senderPort, const std::string & portName,
                            Object::const_ptr object) {

   if (!object) {
      CERR << "Module::addInputObject: input port " << portName << " - did not receive object" << std::endl;
      return false;
   }

   if (object)
      vassert(object->check());

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
   vassert(p);

   return false;
}

bool Module::isConnected(const std::string &portname) const {

   Port *p = findInputPort(portname);
   if (!p)
      p = findOutputPort(portname);
   if (!p)
      return false;

   return isConnected(p);
}

bool Module::isConnected(const Port *port) const {

   return !port->connections().empty();
}

bool Module::changeParameter(const Parameter *p) {

   std::string name = p->getName();
   if (name[0] == '_') {

      if (name == "_error_output_mode" || name == "_error_output_rank") {

         updateOutputMode();
      } else if (name == "_cache_mode") {

         updateCacheMode();
      } else if (name == "_openmp_threads") {

         setOpenmpThreads(getIntParameter(name), false);
      } else if (name == "_benchmark") {

         enableBenchmark(getIntParameter(name), false);
      } else if (name == "_prioritize_visible") {

          m_prioritizeVisible = getIntParameter("_prioritize_visible");
      }

   }

   return true;
}

bool Module::parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg) {

    return true;
}

int Module::schedulingPolicy() const
{
   return m_schedulingPolicy;
}

void Module::setSchedulingPolicy(int schedulingPolicy)
{
   using namespace message;

   vassert(schedulingPolicy >= SchedulingPolicy::Ignore);
   vassert(schedulingPolicy <= SchedulingPolicy::LazyGang);

   m_schedulingPolicy = schedulingPolicy;
   sendMessage(SchedulingPolicy(SchedulingPolicy::Schedule(schedulingPolicy)));
}

int Module::reducePolicy() const
{
   return m_reducePolicy;
}

void Module::setReducePolicy(int reducePolicy)
{
   vassert(reducePolicy >= message::ReducePolicy::Never);
   vassert(reducePolicy <= message::ReducePolicy::OverAll);

   m_reducePolicy = reducePolicy;
   if (m_benchmark) {
      if (reducePolicy == message::ReducePolicy::Locally)
         reducePolicy = message::ReducePolicy::OverAll;
   }
   sendMessage(message::ReducePolicy(message::ReducePolicy::Reduce(reducePolicy)));
}

bool Module::parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName) {

   (void)senderId;
   (void)name;
   (void)msg;
   (void)moduleName;
   return false;
}

bool Module::parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg) {

   (void)senderId;
   (void)name;
   (void)msg;
   return false;
}

bool Module::objectAdded(int sender, const std::string &senderPort, const Port *port) {

   return true;
}

void Module::connectionAdded(const Port *from, const Port *to) {

}

void Module::connectionRemoved(const Port *from, const Port *to) {

}

bool Module::getNextMessage(message::Buffer &buf, bool block) {

    if (!messageBacklog.empty()) {
        buf = messageBacklog.front();
        messageBacklog.pop_front();
        return true;
    }

    if (block) {
        receiveMessageQueue->receive(buf);
        return true;
    }

    return receiveMessageQueue->tryReceive(buf);
}

bool Module::needsSync(const message::Message &m) const {
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

bool Module::dispatch(bool *messageReceived) {

   bool again = true;

   try {
#ifndef MODULE_THREAD
      if (parentProcessDied())
         throw(except::parent_died());
#endif

      if (messageReceived)
         *messageReceived = true;

      message::Buffer buf;
      getNextMessage(buf);

      if (syncMessageProcessing()) {
         int sync = needsSync(buf) ? 1 : 0;
         int allsync = 0;
         mpi::all_reduce(comm(), sync, allsync, mpi::maximum<int>());

         do {
            sync = needsSync(buf) ? 1 : 0;

            again &= handleMessage(&buf);

            if (allsync && !sync) {
                getNextMessage(buf);
            }

         } while(allsync && !sync);
      } else {

         again &= handleMessage(&buf);
      }
   } catch (vistle::except::parent_died &e) {

      // if parent died something is wrong - make sure that shm get cleaned up
      Shm::the().setRemoveOnDetach();
      throw(e);
   }

   return again;
}


void Module::sendParameterMessage(const message::Message &message) const {
    sendMessage(message);
}

bool Module::sendMessage(const message::Message &message) const {

   // exclude SendText messages to avoid circular calls
   if (message.type() != message::SENDTEXT
         && (m_traceMessages == message::ANY || m_traceMessages == message.type())) {
      CERR << "SEND: " << message << std::endl;
   }
   if (rank() == 0 || message::Router::the().toRank0(message)) {
#ifdef MODULE_THREAD
       message::Buffer buf(message);
       buf.setSenderId(id());
       buf.setRank(rank());

       sendMessageQueue->send(buf);
#else
       sendMessageQueue->send(message);
#endif
   }

   return true;
}

bool Module::handleMessage(const vistle::message::Message *message) {

   using namespace vistle::message;

    m_stateTracker->handle(*message);

   if (m_traceMessages == message::ANY || message->type() == m_traceMessages) {
      CERR << "RECV: " << *message << std::endl;
   }

   switch (message->type()) {

      case vistle::message::PING: {

         const vistle::message::Ping *ping =
            static_cast<const vistle::message::Ping *>(message);

         std::cerr << "    module [" << name() << "] [" << id() << "] ["
                   << rank() << "/" << size() << "] ping ["
                   << ping->getCharacter() << "]" << std::endl;
         vistle::message::Pong m(*ping);
         m.setDestId(ping->senderId());
         sendMessage(m);
         break;
      }

      case vistle::message::PONG: {

         const vistle::message::Pong *pong =
            static_cast<const vistle::message::Pong *>(message);

         std::cerr << "    module [" << name() << "] [" << id() << "] ["
                   << rank() << "/" << size() << "] pong ["
                   << pong->getCharacter() << "]" << std::endl;
         break;
      }

      case vistle::message::TRACE: {

         const Trace *trace = static_cast<const Trace *>(message);
         if (trace->on()) {
            m_traceMessages = trace->messageType();
         } else {
            m_traceMessages = message::INVALID;
         }

         std::cerr << "    module [" << name() << "] [" << id() << "] ["
                   << rank() << "/" << size() << "] trace ["
                   << trace->on() << "]" << std::endl;
         break;
      }

      case message::QUIT: {

         const message::Quit *quit =
            static_cast<const message::Quit *>(message);
         //TODO: uuid should be included in coresponding ModuleExit message
         (void) quit;
         if (auto sbuf = dynamic_cast<msgstreambuf<char> *>(m_streambuf))
            sbuf->clear_backlog();
         return false;
         break;
      }

      case message::KILL: {

         const message::Kill *kill =
            static_cast<const message::Kill *>(message);
         //TODO: uuid should be included in coresponding ModuleExit message
         if (kill->getModule() == id() || kill->getModule() == message::Id::Broadcast) {
            if (auto sbuf = dynamic_cast<msgstreambuf<char> *>(m_streambuf))
               sbuf->clear_backlog();
            return false;
         } else {
            std::cerr << "module [" << name() << "] [" << id() << "] ["
               << rank() << "/" << size() << "]" << ": received invalid Kill message" << std::endl;
         }
         break;
      }

      case message::ADDPORT: {

         const message::AddPort *cp =
            static_cast<const message::AddPort *>(message);
         Port port = cp->getPort();
         std::string name = port.getName();
         std::string::size_type p = name.find('[');
         std::string basename = name;
         size_t idx = 0;
         if (p != std::string::npos) {
            basename = name.substr(0, p-1);
            idx = boost::lexical_cast<size_t>(name.substr(p+1));
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
                  inputPorts[name] = newport;
               }
               break;
            case Port::OUTPUT:
               existing = findOutputPort(name);
               if (!existing)
                  parent = findInputPort(basename);
               if (parent) {
                  newport = parent->child(idx, true);
                  outputPorts[name] = newport;
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
            for (Port::PortSet::iterator it = links.begin();
                  it != links.end();
                  ++it) {
               const Port *p = *it;
               message::AddPort linked(*p);
               linked.setReferrer(cp->uuid());
               sendMessage(linked);
            }
         }
         break;
      }

      case message::CONNECT: {

         const message::Connect *conn =
            static_cast<const message::Connect *>(message);
         Port *port = NULL;
         Port *other = NULL;
         const Port::ConstPortSet *ports = NULL;
         std::string ownPortName;
         bool inputConnection = false;
         //std::cerr << name() << " receiving connection: " << conn->getModuleA() << ":" << conn->getPortAName() << " -> " << conn->getModuleB() << ":" << conn->getPortBName() << std::endl;
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

         if (ports && port && other) {
            if (ports->find(other) == ports->end()) {
               port->addConnection(other);
               if (inputConnection)
                  connectionAdded(other, port);
               else
                  connectionAdded(port, other);
            } else {
               delete other;
            }
         } else {
            if (!findParameter(ownPortName))
               CERR << " did not find port " << ownPortName << std::endl;
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
            CERR << "did not find object " << add->objectName() << " for port " << add->getDestPort() << " in AddObject" << std::endl;
         }
         addInputObject(add->senderId(), add->getSenderPort(), add->getDestPort(), obj);
         if (!objectAdded(add->senderId(), add->getSenderPort(), p)) {
             CERR << "error in objectAdded(" << add->getSenderPort() << ")" << std::endl;
             return false;
         }

         break;
      }

      case message::SETPARAMETER: {

         const message::SetParameter *param =
            static_cast<const message::SetParameter *>(message);

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

         const message::AddParameter *param =
            static_cast<const message::AddParameter *>(message);

         parameterAdded(param->senderId(), param->getName(), *param, param->moduleName());
         break;
      }

      case message::REMOVEPARAMETER: {

         const message::RemoveParameter *param =
            static_cast<const message::RemoveParameter *>(message);

         parameterRemoved(param->senderId(), param->getName(), *param);
         break;
      }

      case message::BARRIER: {

         const message::Barrier *barrier = static_cast<const message::Barrier *>(message);
         message::BarrierReached reached(barrier->uuid());
         reached.setDestId(Id::LocalManager);
         sendMessage(reached);
         break;
      }

      case message::CANCELEXECUTE:
         // not relevant if not within prepare/compute/reduce
         break;

      case message::MODULEEXIT:
      case message::SPAWN:
      case message::STARTED:
      case message::MODULEAVAILABLE:
      case message::REPLAYFINISHED:
      case message::ADDHUB:
      case message::REMOVESLAVE:
         break;

      default:
         std::cerr << "    module [" << name() << "] [" << id() << "] ["
                   << rank() << "/" << size() << "] unknown message type ["
                   << message->type() << "]" << std::endl;

         break;
   }

   return true;
}

bool Module::handleExecute(const vistle::message::Execute *exec) {

    using namespace vistle::message;

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
    if (exec->what() == Execute::ComputeExecute
            || exec->what() == Execute::Prepare ) {
        ret &= prepareWrapper(exec);
    }

    bool reordered = false;
    if (exec->what() == Execute::ComputeExecute
            || exec->what() == Execute::ComputeObject) {
        //vassert(m_executionDepth == 0);
        ++m_executionDepth;

        if (reducePolicy() != message::ReducePolicy::Never) {
            vassert(m_prepared);
            vassert(!m_reduced);
        }
        m_computed = true;
        const bool gang = schedulingPolicy() == message::SchedulingPolicy::Gang
                || schedulingPolicy() == message::SchedulingPolicy::LazyGang;

        int direction = 1;

        // just process one tuple of objects at a time
        Index numObject = 1;
        int startTimestep = -1;
        bool waitForZero = false, startWithZero = false;
        if (exec->what() == Execute::ComputeExecute) {
            // Compute not triggered by adding an object, get objects from cache and determine no. of objects to process
            numObject = 0;

            Index numConnected = 0;
            for (auto &port: inputPorts) {
                port.second->objects().clear();
                if (!isConnected(port.second))
                    continue;
                port.second->objects() = m_cache.getObjects(port.first);
                ++numConnected;
                if (numObject == 0) {
                    numObject = port.second->objects().size();
                } else if (numObject != port.second->objects().size()) {
                    CERR << "::compute(): input mismatch - expected " << numObject << " objects, have " << port.second->objects().size() << std::endl;
                    throw vistle::except::exception("input object mismatch");
                    return false;
                }
            }
        }

        for (auto &port: inputPorts) {
            if (!isConnected(port.second))
                continue;
            const auto &objs = port.second->objects();
            for (Index i=0; i<numObject && i<objs.size(); ++i) {
                const auto obj = objs[i];
                int t = getTimestep(obj);
                m_numTimesteps = std::max(t+1, m_numTimesteps);
            }
        }
#ifdef REDUCE_DEBUG
        CERR << "compute with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif

        if (exec->what() == Execute::ComputeExecute || gang) {
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for timesteps with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif
            m_numTimesteps = mpi::all_reduce(comm(), m_numTimesteps, mpi::maximum<int>());
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for timesteps finished with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif
        }

        if (exec->what() == Execute::ComputeExecute) {
            if (m_prioritizeVisible && !gang && !exec->allRanks()
                    && reducePolicy() != message::ReducePolicy::PerTimestepOrdered) {
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
                    headStart = m_avgComputeTime / std::abs(exec->animationStepDuration());
                if (reducePolicy() == message::ReducePolicy::PerTimestepZeroFirst)
                    headStart *= 2;
                headStart = 1+std::max(0, headStart);

                struct TimeIndex {
                    int step = -1;
                    double time = 0.;
                    size_t idx = 0;

                    bool operator<(const TimeIndex &other) const {
                        return step < other.step;
                    }
                };
                std::vector<TimeIndex> sortKey(numObject);
                for (auto &port: inputPorts) {
                    const auto &objs = port.second->objects();
                    size_t i=0;
                    for (auto &obj: objs) {
                        sortKey[i].idx = i;
                        sortKey[i].step = getTimestep(obj);
                        sortKey[i].time = obj->getRealTime();
                        if (sortKey[i].step >= 0 && sortKey[i].time == 0.)
                            sortKey[i].time = sortKey[i].step;
                        ++i;
                    }
                }
                if (numObject > 0) {
                    std::sort(sortKey.begin(), sortKey.end());
                    auto best = sortKey[0];
                    for (auto &ti: sortKey) {
                        if (std::abs(ti.step - exec->animationRealTime()) < std::abs(best.step - exec->animationRealTime())) {
                            best = ti;
                        }
                    }
#ifdef REDUCE_DEBUG
                    CERR << "starting with timestep=" << best.step << ", anim=" << exec->animationRealTime() << ", cur=" << best.time << std::endl;
#endif
                    if (m_numTimesteps > 0) {
                        startTimestep = best.step + direction*headStart;
                    }
                }

                if (reducePolicy() == message::ReducePolicy::PerTimestepZeroFirst) {
                    waitForZero = true;
                    startWithZero = true;
                }
                const int step = direction<0 ? -1 : 1;
                if (m_numTimesteps > 0) {
                    while (startTimestep < 0)
                        startTimestep += m_numTimesteps;
                    startTimestep %= m_numTimesteps;
                }
#ifdef REDUCE_DEBUG
                CERR << "startTimestep determined to be " << startTimestep << std::endl;
#endif
                if (startTimestep == -1)
                    startTimestep = 0;
                if (direction < 0) {
                    startTimestep = mpi::all_reduce(comm(), startTimestep, mpi::maximum<int>());
                } else {
                    startTimestep = mpi::all_reduce(comm(), startTimestep, mpi::minimum<int>());
                }

                for (auto &port: inputPorts) {
                    port.second->objects().clear();
                    if (!isConnected(port.second))
                        continue;
                    auto objs = m_cache.getObjects(port.first);
                    // objects without timestep
                    ssize_t cur = step<0 ? numObject-1 : 0;
                    for (size_t i=0; i<numObject; ++i) {
                        if (sortKey[cur].step < 0)
                            port.second->objects().push_back(objs[sortKey[cur].idx]);
                        cur = (cur+step+numObject)%numObject;
                    }
                    // objects with timestep 0 (if to be handled first)
                    if (startWithZero) {
                        cur = step<0 ? numObject-1 : 0;
                        for (size_t i=0; i<numObject; ++i) {
                            if (sortKey[cur].step == 0)
                                port.second->objects().push_back(objs[sortKey[cur].idx]);
                            cur = (cur+step+numObject)%numObject;
                        }
                    }
                    // all objects from current timestep until end...
                    bool push = false;
                    cur = step<0 ? numObject-1 : 0;
                    for (size_t i=0; i<numObject; ++i) {
                        if (sortKey[cur].step == startTimestep)
                            push = true;
                        if (push && (sortKey[cur].step > 0 || (!startWithZero && sortKey[cur].step==0)))
                            port.second->objects().push_back(objs[sortKey[cur].idx]);
                        cur = (cur+step+numObject)%numObject;
                    }
                    // ...and from start until current timestep
                    cur = step<0 ? numObject-1 : 0;
                    for (size_t i=0; i<numObject; ++i) {
                        if (sortKey[cur].step == startTimestep)
                            break;
                        if (sortKey[cur].step > 0 || (!startWithZero && sortKey[cur].step==0))
                            port.second->objects().push_back(objs[sortKey[cur].idx]);
                        cur = (cur+step+numObject)%numObject;
                    }
                    if (port.second->objects().size() != numObject) {
                        CERR << "mismatch: expecting " << numObject << " objects, actually have " << port.second->objects().size() << " at port " << port.first << std::endl;
                    }
                    assert(port.second->objects().size() == numObject);
                }
            }
            if (gang) {
                numObject = mpi::all_reduce(comm(), numObject, mpi::maximum<int>());
            }
        }


        if (exec->allRanks() || gang || exec->what() == Execute::ComputeExecute) {
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for execCount with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif
            int oldExecCount = m_executionCount;
            m_executionCount = mpi::all_reduce(comm(), m_executionCount, mpi::maximum<int>());
            if (oldExecCount < m_executionCount) {
                m_iteration = -1;
            }
        }

        int prevTimestep = -1;
        if (m_numTimesteps > 0) {
            int skip=1;
            if (startTimestep==0 && startWithZero)
                skip=2;
            if (direction >= 0)
                prevTimestep = (startTimestep+m_numTimesteps-1)%m_numTimesteps;
            else
                prevTimestep = (startTimestep+1)%m_numTimesteps;
#ifdef REDUCE_DEBUG
            if (rank() == 0 && exec->what() == message::Execute::ComputeExecute)
                CERR << "last timestep to reduce: " << prevTimestep << std::endl;
#endif
        }
        int numReductions = 0;
        bool reducePerTimestep = reducePolicy()==message::ReducePolicy::PerTimestep || reducePolicy()==message::ReducePolicy::PerTimestepZeroFirst || reducePolicy()==message::ReducePolicy::PerTimestepOrdered;
        auto runReduce = [this](int timestep, int &numReductions) -> bool {
#ifdef REDUCE_DEBUG
            if (rank() == 0)
                CERR << "running reduce for timestep " << timestep << ", already did " << numReductions  << " of " << m_numTimesteps << " reductions" << std::endl;
#endif
            if (cancelRequested(true))
                return true;
            if (timestep >= 0) {
                ++numReductions;
                assert(numReductions <= m_numTimesteps);
            }
            return reduce(timestep);
        };
        bool computeOk = false;
        for (Index i=0; i<numObject; ++i) {
            computeOk = false;
            try {
                double start = Clock::time();
                int timestep = -1;
                bool objectIsEmpty = false;
                for (auto &port: inputPorts) {
                    if (!isConnected(port.second))
                        continue;
                    const auto &objs = port.second->objects();
                    if (!objs.empty()) {
                        int t = getTimestep(objs.front());
                        if (t != -1)
                            timestep = t;
                        if (Empty::as(objs.front()))
                            objectIsEmpty = true;
                    }
                }
                if (cancelRequested()) {
                    computeOk = true;
                } else if (objectIsEmpty) {
                    for (auto &port: inputPorts) {
                        if (!isConnected(port.second))
                            continue;
                        auto &objs = port.second->objects();
                        if (!objs.empty()) {
                            objs.pop_front();
                        }
                    }
                    computeOk = true;
                } else {
                    computeOk = compute();
                }

                if (reordered && m_numTimesteps>0 && reducePerTimestep) {
                    // if processing for another timestep starts, run reduction for previous timesteps
                    if (startWithZero && waitForZero) {
                        if (timestep > 0) {
                            waitForZero = false;
                            computeOk &= runReduce(0, numReductions);
                        }
                    }
                    if (waitForZero) {
                    } else if (timestep < 0) {
                    } else if (direction >= 0) {
                        if (prevTimestep > timestep) {
                            for (int t=prevTimestep; t<m_numTimesteps; ++t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                            for (int t=0; t<timestep; ++t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        } else {
                            for (int t=prevTimestep; t<timestep; ++t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        }
                        prevTimestep = timestep;
                    } else {
                        if (prevTimestep < timestep) {
                            for (int t=prevTimestep; t>=0; --t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                            for (int t=m_numTimesteps-1; t>timestep; --t) {
                                if (t > 0 || !startWithZero)
                                    computeOk &= runReduce(t, numReductions);
                            }
                        } else {
                            for (int t=prevTimestep; t>timestep; --t) {
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
                          << ", error code: " << e.get_error_code()
                          << ", native error: " << e.get_native_error()
                          << std::endl << std::flush;
                std::cerr << name() << "::compute(): interprocess_exception: " << e.what()
                          << ", error code: " << e.get_error_code()
                          << ", native error: " << e.get_native_error()
                          << std::endl;
            } catch (std::exception &e) {
                std::cout << name() << "::compute(" << i << "): exception - " << e.what() << std::endl << std::flush;
                std::cerr << name() << "::compute(" << i << "): exception - " << e.what() << std::endl;
            }
            ret &= computeOk;
            if (!computeOk) {
                break;
            }
        }

        if (reordered && m_numTimesteps>0 && reducePerTimestep) {
            // run reduction for remaining (most often just the last) timesteps
            int t = prevTimestep;
            while (numReductions < m_numTimesteps && !cancelRequested(true)) {
                runReduce(t, numReductions);
                if (direction >= 0) {
                    t = (t+1)%m_numTimesteps;
                } else {
                    t = (t+m_numTimesteps-1)%m_numTimesteps;
                }
            }
        }

        --m_executionDepth;
        //vassert(m_executionDepth == 0);
    }

    if (exec->what() == Execute::ComputeExecute
            || exec->what() == Execute::Reduce) {
        ret &= reduceWrapper(exec, reordered);
        m_cache.clearOld();
    }
#ifdef DETAILED_PROGRESS
    message::Idle idle;
    idle.setReferrer(exec->uuid());
    idle.setDestId(Id::LocalManager);
    sendMessage(idle);
#endif

    return ret;
}

std::string Module::getModuleName(int id) const {

   return m_stateTracker->getModuleName(id);
}

Module::~Module() {

#ifndef MODULE_THREAD
    Shm::the().detach();
#endif

   if (m_origStreambuf)
      std::cerr.rdbuf(m_origStreambuf);
   delete m_streambuf;
   m_streambuf = nullptr;

   if (m_readyForQuit) {
      comm().barrier();
   } else {
       CERR << "Emergency quit" << std::endl;

   }
}

void Module::eventLoop() {

    initDone();
    while (dispatch())
        ;
    prepareQuit();
}

namespace {

using namespace boost;

class InstModule: public Module {
public:
   InstModule()
   : Module("description", "name", 1, boost::mpi::communicator())
   {}
   bool compute() { return true; }
};

struct instantiator {
   template<typename P> P operator()(P) {
      InstModule m;
      ParameterBase<P> *p(new ParameterBase<P>(m.id(), "param"));
      m.setParameterMinimum(p, P());
      m.setParameterMaximum(p, P());
      return P();
   }
};
}

void instantiate_parameter_functions() {

   mpl::for_each<Parameters>(instantiator());
}

void Module::sendText(int type, const std::string &msg) const {

   message::SendText info(message::SendText::TextType(type), msg);
   sendMessage(info);
}

static void sendTextVarArgs(const Module *self, message::SendText::TextType type, const char *fmt, va_list args) {

   if(!fmt) {
      fmt = "(empty message)";
   }
   std::vector<char> text(strlen(fmt)+500);
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
   message::SendText info(type, text.data());
   self->sendMessage(info);
}

void Module::sendInfo(const char *fmt, ...) const {

   va_list args;
   va_start(args, fmt);
   sendTextVarArgs(this, message::SendText::Info, fmt, args);
   va_end(args);
}

void Module::sendInfo(const std::string &text) const {

   std::cout << "INFO: " << text << std::endl;
   message::SendText info(message::SendText::Info, text);
   sendMessage(info);
}

void Module::sendWarning(const char *fmt, ...) const {

   va_list args;
   va_start(args, fmt);
   sendTextVarArgs(this, message::SendText::Warning, fmt, args);
   va_end(args);
}

void Module::sendWarning(const std::string &text) const {

   std::cout << "WARN: " << text << std::endl;
   message::SendText info(message::SendText::Warning, text);
   sendMessage(info);
}

void Module::sendError(const char *fmt, ...) const {

   va_list args;
   va_start(args, fmt);
   sendTextVarArgs(this, message::SendText::Error, fmt, args);
   va_end(args);
}

void Module::sendError(const std::string &text) const {

   std::cout << "ERR:  " << text << std::endl;
   message::SendText info(message::SendText::Error, text);
   sendMessage(info);
}

void Module::sendError(const message::Message &msg, const char *fmt, ...) const {

   if(!fmt) {
      fmt = "(empty message)";
   }
   std::vector<char> text(strlen(fmt)+500);
   va_list args;
   va_start(args, fmt);
   vsnprintf(text.data(), text.size(), fmt, args);
   va_end(args);

   std::cout << "ERR:  " << text.data() << std::endl;
   message::SendText info(text.data(), msg);
   sendMessage(info);
}

void Module::sendError(const message::Message &msg, const std::string &text) const {

   std::cout << "ERR:  " << text << std::endl;
   message::SendText info(text, msg);
   sendMessage(info);
}

void Module::setObjectReceivePolicy(int pol) {

   m_receivePolicy = pol;
   sendMessage(message::ObjectReceivePolicy(message::ObjectReceivePolicy::Policy(pol)));
}

int Module::objectReceivePolicy() const {

    return m_receivePolicy;
}

void Module::startIteration() {

    ++m_iteration;
}

bool Module::prepareWrapper(const message::Execute *exec) {

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

   m_withOutput.clear();

   bool collective = reducePolicy() != message::ReducePolicy::Never && reducePolicy() != message::ReducePolicy::Locally;
   if (reducePolicy() != message::ReducePolicy::Never) {
      vassert(!m_prepared);
   }
   vassert(!m_computed);

   m_reduced = false;

#ifdef REDUCE_DEBUG
   if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never) {
      std::cerr << "prepare(): barrier for reduce policy " << reducePolicy() << std::endl;
      comm().barrier();
   }
#endif

   message::ExecutionProgress start(message::ExecutionProgress::Start, m_executionCount);
   start.setReferrer(exec->uuid());
   start.setDestId(Id::LocalManager);
   sendMessage(start);

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

bool Module::prepare() {

#ifndef NDEBUG
   if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never)
      comm().barrier();
#endif
   return true;
}

bool Module::compute() {
    std::cerr << "compute() should not be called unless overridden" << std::endl;
    return false;
}

bool Module::reduceWrapper(const message::Execute *exec, bool reordered) {

   //CERR << "reduceWrapper: prepared=" << m_prepared << std::endl;

   vassert(m_prepared);
   if (reducePolicy() != message::ReducePolicy::Never) {
      vassert(!m_reduced);
   }

#ifdef REDUCE_DEBUG
   if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never) {
      std::cerr << "reduce(): barrier for reduce policy " << reducePolicy() << ", request was " << *exec << std::endl;
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
       switch(reducePolicy()) {
       case message::ReducePolicy::Never: {
           break;
       }
       case message::ReducePolicy::PerTimestep:
       case message::ReducePolicy::PerTimestepZeroFirst:
       case message::ReducePolicy::PerTimestepOrdered: {
           if (!reordered) {
               for (int t=0; t<m_numTimesteps; ++t) {
                   if (!cancelRequested(sync))
                       ret &= reduce(t);
               }
           }
       }
       // FALLTHRU
       case message::ReducePolicy::Locally:
       case message::ReducePolicy::OverAll: {
           if (!cancelRequested(sync)) {
               ret = reduce(-1);
           }
           break;
       }
       }
   } catch (std::exception &e) {
           ret = false;
           std::cout << name() << "::reduce(): exception - " << e.what() << std::endl << std::flush;
           std::cerr << name() << "::reduce(): exception - " << e.what() << std::endl;
   }

   for (auto &port: outputPorts) {
       if (isConnected(port.second) && m_withOutput.find(port.second) == m_withOutput.end()) {
           Empty::ptr empty(new Empty(Object::Initialized));
           addObject(port.second, empty);
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
         printf("%s:%d: compute() took %fs (OpenMP threads: %d)",
               name().c_str(), id(), duration, nthreads);
#else
         sendInfo("compute() took %fs (no OpenMP)", duration);
         printf("%s:%d: compute() took %fs (no OpenMP)",
               name().c_str(), id(), duration);
#endif
      }
   }

   message::ExecutionProgress fin(message::ExecutionProgress::Finish, m_executionCount);
   fin.setReferrer(exec->uuid());
   fin.setDestId(Id::LocalManager);
   sendMessage(fin);

#ifndef DETAILED_PROGRESS
   message::Idle idle;
   idle.setReferrer(exec->uuid());
   idle.setDestId(Id::LocalManager);
   sendMessage(idle);
#endif

   m_computed = false;
   m_prepared = false;

   return ret;
}

bool Module::reduce(int timestep) {

#ifndef NDEBUG
   if (reducePolicy() != message::ReducePolicy::Locally && reducePolicy() != message::ReducePolicy::Never)
      comm().barrier();
#endif
   return true;
}

bool Module::cancelExecute() {

    std::cerr << "canceling execution" << std::endl;
    if (rank() == 0)
        sendInfo("canceling execution");

    return true;
}

int Module::numTimesteps() const {

    return m_numTimesteps;
}

void Module::setStatus(const std::string &text, message::UpdateStatus::Importance prio) {

    message::UpdateStatus status(text, prio);
    status.setDestId(Id::ForBroadcast);
    sendMessage(status);
}

void Module::clearStatus() {

    setStatus(std::string(), message::UpdateStatus::Bulk);
}

bool Module::cancelRequested(bool collective) {

    message::Buffer buf;
    while (!m_executeAfterCancelFound && receiveMessageQueue->tryReceive(buf)) {
        messageBacklog.push_back(buf);
        switch (buf.type()) {
        case message::CANCELEXECUTE: {
            const auto &cancel = buf.as<message::CancelExecute>();
            if (cancel.getModule() == id()) {
                std::cerr << "canceling execution requested" << std::endl;
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
        m_cancelRequested = boost::mpi::all_reduce(comm(), m_cancelRequested, std::logical_or<bool>());
    }

    if (m_cancelRequested && !m_cancelExecuteCalled) {
        cancelExecute();
        m_cancelExecuteCalled = true;
    }

    return m_cancelRequested;
}

} // namespace vistle
