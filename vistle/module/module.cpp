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



Module::Module(const std::string &desc, const std::string &shmname,
      const std::string &moduleName, const int moduleId)
: m_name(moduleName)
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
, m_inParameterChanged(false)
, m_traceMessages(message::INVALID)
, m_benchmark(false)
, m_avgComputeTime(1.)
, m_comm(MPI_COMM_WORLD, mpi::comm_attach)
, m_numTimesteps(-1)
, m_cancelRequested(false)
, m_cancelExecuteCalled(false)
, m_prepared(false)
, m_computed(false)
, m_reduced(false)
, m_readyForQuit(false)
{
   m_size = comm().size();
   m_rank = comm().rank();

   message::DefaultSender::init(m_id, m_rank);

   try {
      Shm::attach(shmname, id(), rank());
   } catch (interprocess::interprocess_exception &ex) {
      std::stringstream str;
      throw vistle::exception(std::string("attaching to shared memory ") + shmname + ": " + ex.what());
   }

   // names are swapped relative to communicator
   std::string smqName = message::MessageQueue::createName("rmq", id(), rank());
   try {
      sendMessageQueue = message::MessageQueue::open(smqName);
   } catch (interprocess::interprocess_exception &ex) {
      throw vistle::exception(std::string("opening send message queue ") + smqName + ": " + ex.what());
   }

   std::string rmqName = message::MessageQueue::createName("smq", id(), rank());
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
        std::vector<vistle::Parameter *> toRemove;
        for (auto &param: parameters) {
            toRemove.push_back(param.second.get());
        }
        for (auto &param: toRemove) {
            removeParameter(param);
        }

        vistle::message::ModuleExit m;
        m.setDestId(Id::ForBroadcast);
        sendMessage(m);

        m_cache.clear();
        m_cache.clearOld();
        Shm::the().detach();
    }

    m_readyForQuit = true;
}

const HubData &Module::getHub() const {

    int hubId = m_stateTracker->getHub(id());
    return m_stateTracker->getHubData(hubId);
}

void Module::initDone() {

   m_streambuf = new msgstreambuf<char>(this);
   m_origStreambuf = std::cerr.rdbuf(m_streambuf);

   message::Started start(name());
   start.setDestId(Id::ForBroadcast);
   sendMessage(start);

   for (auto &pair: parameters) {
      parameterChangedWrapper(pair.second.get());
   }
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

void Module::setCurrentParameterGroup(const std::string &group) {

   m_currentParameterGroup = group;
}

const std::string &Module::currentParameterGroup() const {

   return m_currentParameterGroup;
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

   parameters[name] = param;

   message::AddParameter add(*param, m_name);
   add.setDestId(Id::ForBroadcast);
   sendMessage(add);
   message::SetParameter set(m_id, name, param);
   set.setDestId(Id::ForBroadcast);
   set.setInit();
   set.setReferrer(add.uuid());
   sendMessage(set);

   return param.get();
}

bool Module::removeParameter(const std::string &name) {

   vassert(havePort(name));
   if (!havePort(name)) {
      CERR << "removeParameter: no port with name " << name << std::endl;
      return false;
   }

   auto it = parameters.find(name);
   if (it == parameters.end()) {
      CERR << "removeParameter: no parameter with name " << name << std::endl;
      return false;
   }

   return removeParameter(it->second.get());
}

bool Module::removeParameter(Parameter *param) {

   auto it = parameters.find(param->getName());
   if (it == parameters.end()) {
      CERR << "removeParameter: no parameter with name " << param->getName() << std::endl;
      return false;
   }

   message::RemoveParameter remove(*param, m_name);
   remove.setDestId(Id::ForBroadcast);
   sendMessage(remove);

   parameters.erase(it);

   return true;
}

bool Module::sendObject(Object::const_ptr obj, int destRank) const {

    vecostreambuf<char> memstr;
    vistle::oarchive memar(memstr);
    auto saver = std::make_shared<DeepArchiveSaver>();
    memar.setSaver(saver);
    obj->save(memar);
    const std::vector<char> &mem = memstr.get_vector();
    comm().send(destRank, 0, mem);
    auto dir = saver->getDirectory();
    comm().send(destRank, 0, dir);
    for (auto &ent: dir) {
        comm().send(destRank, 0, ent.data, ent.size);
    }
    return true;
#if 0
    vecostreambuf<char> memstr;
    vistle::oarchive memar(memstr);
    auto saver = std::make_shared<DeepArchiveSaver>();
    memar.setSaver(saver);
    obj->save(memar);
    const std::vector<char> &mem = memstr.get_vector();
    uint64_t len = mem.size();
    std::cerr << "Rank " << rank() << ": Broadcasting " << len << " bytes, type=" << obj->getType() << " (" << obj->getName() << ")" << std::endl;
    const char *data = mem.data();

    MPI_Request r1, r2;
    MPI_Isend(&len, 1, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &r1);
    MPI_Isend(const_cast<char *>(data), len, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &r2);
    MPI_Wait(&r1, MPI_STATUS_IGNORE);
    MPI_Wait(&r2, MPI_STATUS_IGNORE);
    //FIXME: directory

    return true;
#endif
}

Object::const_ptr Module::receiveObject(int sourceRank) const {

    std::vector<char> mem;
    comm().recv(sourceRank, 0, mem);
    vistle::SubArchiveDirectory dir;
    std::map<std::string, std::vector<char>> objects, arrays;
    comm().recv(sourceRank, 0, dir);
    for (auto &ent: dir) {
        if (ent.is_array) {
            arrays[ent.name].resize(ent.size);
            ent.data = arrays[ent.name].data();
        } else {
            objects[ent.name].resize(ent.size);
            ent.data = objects[ent.name].data();
        }
        comm().recv(sourceRank, 0, ent.data, ent.size);
    }
    vecistreambuf<char> membuf(mem);
    vistle::iarchive memar(membuf);
    auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays);
    memar.setFetcher(fetcher);
    Object::ptr p(Object::load(memar));
    return p;
#if 0
    obj.reset(Object::load(memar));

    uint64_t len = 0;
    MPI_Request r;
    MPI_Irecv(&len, 1, MPI_UINT64_T, sourceRank, 0, MPI_COMM_WORLD, &r);
    MPI_Wait(&r, MPI_STATUS_IGNORE);
    if (len > 0) {
        //std::cerr << "Rank " << rank() << ": Waiting to receive " << len << " bytes" << std::endl;
        std::vector<char> mem(len);
        char *data = mem.data();
        vistle::SubArchiveDirectory dir;
        std::map<std::string, std::vector<char>> objects, arrays;
        MPI_Request r;
        MPI_Irecv(data, mem.size(), MPI_BYTE, sourceRank, 0, MPI_COMM_WORLD, &r);
        MPI_Wait(&r, MPI_STATUS_IGNORE);
        //FIXME: directory

        vecistreambuf<char> membuf(mem);
        vistle::iarchive memar(membuf);
        auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays);
        memar.setFetcher(fetcher);
        Object::ptr obj(Object::load(memar));
        return obj;
    }

    return Object::const_ptr();
#endif
}

bool Module::broadcastObject(Object::const_ptr &obj, int root) const {

    if (rank() == root) {
        vecostreambuf<char> memstr;
        vistle::oarchive memar(memstr);
        auto saver = std::make_shared<DeepArchiveSaver>();
        memar.setSaver(saver);
        obj->save(memar);
        const std::vector<char> &mem = memstr.get_vector();
        mpi::broadcast(comm(), const_cast<std::vector<char>&>(mem), root);
        auto dir = saver->getDirectory();
        mpi::broadcast(comm(), dir, root);
        for (auto &ent: dir) {
            mpi::broadcast(comm(), ent.data, ent.size, root);
        }
    } else {
        std::vector<char> mem;
        mpi::broadcast(comm(), mem, root);
        vistle::SubArchiveDirectory dir;
        std::map<std::string, std::vector<char>> objects, arrays;
        mpi::broadcast(comm(), dir, root);
        for (auto &ent: dir) {
            if (ent.is_array) {
                arrays[ent.name].resize(ent.size);
                ent.data = arrays[ent.name].data();
            } else {
                objects[ent.name].resize(ent.size);
                ent.data = objects[ent.name].data();
            }
            mpi::broadcast(comm(), ent.data, ent.size, root);
        }
        vecistreambuf<char> membuf(mem);
        vistle::iarchive memar(membuf);
        auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays);
        memar.setFetcher(fetcher);
        obj.reset(Object::load(memar));
    }

    return true;
}

bool Module::updateParameter(const std::string &name, const Parameter *param, const message::SetParameter *inResponseTo, Parameter::RangeType rt) {

   auto i = parameters.find(name);

   if (i == parameters.end()) {
      CERR << "setParameter: " << name << " not found" << std::endl;
      return false;
   }

   if (i->second->type() != param->type()) {
      CERR << "setParameter: type mismatch for " << name << " " << i->second->type() << " != " << param->type() << std::endl;
      return false;
   }

   if (i->second.get() != param) {
      CERR << "setParameter: pointer mismatch for " << name << std::endl;
      return false;
   }

   message::SetParameter set(m_id, name, i->second, rt);
   if (inResponseTo) {
      set.setReferrer(inResponseTo->uuid());
   }
   set.setDestId(Id::ForBroadcast);
   sendMessage(set);

   return true;
}

void Module::setParameterChoices(const std::string &name, const std::vector<std::string> &choices)
{
   auto p = findParameter(name);
   if (p)
      setParameterChoices(p.get(), choices);
}

void Module::setParameterChoices(Parameter *param, const std::vector<std::string> &choices)
{
    message::SetParameterChoices sc(param->getName(), choices);
    sc.setDestId(Id::ForBroadcast);
    sendMessage(sc);
}

template<class T>
Parameter *Module::addParameter(const std::string &name, const std::string &description, const T &value, Parameter::Presentation pres) {

   std::shared_ptr<Parameter> p(new ParameterBase<T>(id(), name, value));
   p->setDescription(description);
   p->setGroup(currentParameterGroup());
   p->setPresentation(pres);

   return addParameterGeneric(name, p);
}

std::shared_ptr<Parameter> Module::findParameter(const std::string &name) const {

   auto i = parameters.find(name);

   if (i == parameters.end())
      return std::shared_ptr<Parameter>();

   return i->second;
}

StringParameter *Module::addStringParameter(const std::string & name, const std::string &description,
                              const std::string & value, Parameter::Presentation p) {

   return dynamic_cast<StringParameter *>(addParameter(name, description, value, p));
}

bool Module::setStringParameter(const std::string & name,
                              const std::string & value, const message::SetParameter *inResponseTo) {

   return setParameter(name, value, inResponseTo);
}

std::string Module::getStringParameter(const std::string & name) const {

   std::string value = "";
   getParameter(name, value);
   return value;
}

FloatParameter *Module::addFloatParameter(const std::string &name, const std::string &description,
                               const Float value) {

   return dynamic_cast<FloatParameter *>(addParameter(name, description, value));
}

bool Module::setFloatParameter(const std::string & name,
                               const Float value, const message::SetParameter *inResponseTo) {

   return setParameter(name, value, inResponseTo);
}

Float Module::getFloatParameter(const std::string & name) const {

   Float value = 0.;
   getParameter(name, value);
   return value;
}

IntParameter *Module::addIntParameter(const std::string & name, const std::string &description,
                             const Integer value, Parameter::Presentation p) {

   return dynamic_cast<IntParameter *>(addParameter(name, description, value, p));
}

bool Module::setIntParameter(const std::string & name,
                             Integer value, const message::SetParameter *inResponseTo) {

   return setParameter(name, value, inResponseTo);
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
}

Integer Module::getIntParameter(const std::string & name) const {

   Integer value = 0;
   getParameter(name, value);
   return value;
}

VectorParameter *Module::addVectorParameter(const std::string & name, const std::string &description,
                                const ParamVector & value) {

   return dynamic_cast<VectorParameter *>(addParameter(name, description, value));
}

bool Module::setVectorParameter(const std::string & name,
                                const ParamVector & value, const message::SetParameter *inResponseTo) {

   return setParameter(name, value, inResponseTo);
}

ParamVector Module::getVectorParameter(const std::string & name) const {

   ParamVector value;
   getParameter(name, value);
   return value;
}

IntVectorParameter *Module::addIntVectorParameter(const std::string & name, const std::string &description,
                                const IntParamVector & value) {

   return dynamic_cast<IntVectorParameter *>(addParameter(name, description, value));
}

bool Module::setIntVectorParameter(const std::string & name,
                                const IntParamVector & value, const message::SetParameter *inResponseTo) {

   return setParameter(name, value, inResponseTo);
}

IntParamVector Module::getIntVectorParameter(const std::string & name) const {

   IntParamVector value;
   getParameter(name, value);
   return value;
}

void Module::updateMeta(vistle::Object::ptr obj) const {

   if (obj) {
      obj->setCreator(id());
      obj->setExecutionCounter(m_executionCount);
      if (obj->getIteration() == -1)
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

bool Module::parameterChangedWrapper(const Parameter *p) {

   if (m_inParameterChanged) {
      return true;
   }
   m_inParameterChanged = true;

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

      m_inParameterChanged = false;
      return true;
   }

   bool ret = changeParameter(p);
   m_inParameterChanged = false;
   return ret;
}

bool Module::changeParameter(const Parameter *p) {

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

bool Module::dispatch() {

   bool again = true;

   try {
      if (parentProcessDied())
         throw(except::parent_died());

      message::Buffer buf;
      if (!messageBacklog.empty()) {
          buf = messageBacklog.front();
          messageBacklog.pop_front();
      } else {
          receiveMessageQueue->receive(buf);
      }

      if (syncMessageProcessing()) {
         int sync = 0, allsync = 0;

         switch (buf.type()) {
            case vistle::message::OBJECTRECEIVED:
            case vistle::message::QUIT:
               sync = 1;
               break;
            default:
               break;
         }

         mpi::all_reduce(comm(), sync, allsync, mpi::maximum<int>());

         do {
            switch (buf.type()) {
               case vistle::message::OBJECTRECEIVED:
               case vistle::message::QUIT:
                  sync = 1;
                  break;
               default:
                  break;
            }

            m_stateTracker->handle(buf);
            again &= handleMessage(&buf);

            if (allsync && !sync) {
                if (!messageBacklog.empty()) {
                    buf = messageBacklog.front();
                    messageBacklog.pop_front();
                } else {
                    receiveMessageQueue->receive(buf);
                }
            }

         } while(allsync && !sync);
      } else {

         m_stateTracker->handle(buf);
         again &= handleMessage(&buf);
      }
   } catch (vistle::except::parent_died &e) {

      // if parent died something is wrong - make sure that shm get cleaned up
      Shm::the().setRemoveOnDetach();
      throw(e);
   }

   return again;
}


void Module::sendMessage(const message::Message &message) const {

   // exclude SendText messages to avoid circular calls
   if (message.type() != message::SENDTEXT
         && (m_traceMessages == message::ANY || m_traceMessages == message.type())) {
      CERR << "SEND: " << message << std::endl;
   }
   if (rank() == 0 || message::Router::the().toRank0(message))
       sendMessageQueue->send(message);
}

bool Module::handleMessage(const vistle::message::Message *message) {

   using namespace vistle::message;

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
               std::cerr << name() << " did not find port " << ownPortName << std::endl;
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
            std::cerr << "unknown input port " << add->getDestPort() << " in AddObject" << std::endl;
            return true;
         }
         if (!obj) {
            std::cerr << "did not find object " << add->objectName() << " for port " << add->getDestPort() << " in AddObject" << std::endl;
         }
         addInputObject(add->senderId(), add->getSenderPort(), add->getDestPort(), obj);
         if (!objectAdded(add->senderId(), add->getSenderPort(), p)) {
            std::cerr << "error in objectAdded(" << add->getSenderPort() << ")" << std::endl;
            return false;
         }

         break;
      }

      case message::SETPARAMETER: {

         const message::SetParameter *param =
            static_cast<const message::SetParameter *>(message);

         if (param->destId() == id()) {

            // sent by controller
            switch (param->getParameterType()) {
               case Parameter::Integer:
                  setIntParameter(param->getName(), param->getInteger(), param);
                  break;
               case Parameter::Float:
                  setFloatParameter(param->getName(), param->getFloat(), param);
                  break;
               case Parameter::Vector:
                  setVectorParameter(param->getName(), param->getVector(), param);
                  break;
               case Parameter::IntVector:
                  setIntVectorParameter(param->getName(), param->getIntVector(), param);
                  break;
               case Parameter::String:
                  setStringParameter(param->getName(), param->getString(), param);
                  break;
               default:
                  std::cerr << "Module::handleMessage: unknown parameter type " << param->getParameterType() << std::endl;
                  vassert("unknown parameter type" == 0);
                  break;
            }

            // notification of controller about current value happens in set...Parameter
         } else {

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

      case message::OBJECTRECEIVED:
         // currently only relevant for renderers
         break;

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

    if (schedulingPolicy() == message::SchedulingPolicy::Ignore)
        return true;

    bool ret = true;

    Busy busy;
    busy.setReferrer(exec->uuid());
    busy.setDestId(Id::LocalManager);
    sendMessage(busy);
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

        Index numObject = 0;
        int startTimestep = -1;
        if (exec->what() == Execute::ComputeExecute) {
            // Compute not triggered by adding an object, get objects from cache
            Index numConnected = 0;
            for (auto &port: inputPorts) {
                port.second->objects() = m_cache.getObjects(port.first);
                if (!isConnected(port.second))
                    continue;
                ++numConnected;
                if (numObject == 0) {
                    numObject = port.second->objects().size();
                } else if (numObject != port.second->objects().size()) {
                    std::cerr << name() << "::compute(): input mismatch - expected " << numObject << " objects, have " << port.second->objects().size() << std::endl;
                    throw vistle::except::exception("input object mismatch");
                    return false;
                }
            }

            if (m_prioritizeVisible && !gang && !exec->allRanks()) {
                reordered = true;

                if (exec->animationStep() > 0.) {
                    direction = 1;
                } else if (exec->animationStep() < 0.) {
                    direction = -1;
                } else {
                    direction = 0;
                }

                int headStart = 0;
                if (std::abs(exec->animationStep()) > 1e-5)
                    headStart = m_avgComputeTime / std::abs(exec->animationStep());
                headStart = std::max(1, headStart)+2;

                struct TimeIndex {
                    double t = 0.;
                    size_t idx = 0;

                    bool operator<(const TimeIndex &other) const {
                        return t < other.t;
                    }
                };
                std::vector<TimeIndex> sortKey(numObject);
                for (auto &port: inputPorts) {
                    const auto &objs = port.second->objects();
                    size_t i=0;
                    for (auto &obj: objs) {
                        sortKey[i].idx = i;
                        if (sortKey[i].t == 0. && obj->getTimestep() != -1)
                            sortKey[i].t = obj->getTimestep();
                        ++i;
                    }
                }
                size_t bestIdx = 0;
                if (numObject > 0) {
                    std::sort(sortKey.begin(), sortKey.end());
                    auto best = sortKey[0];
                    ssize_t idx=0;
                    for (auto &ti: sortKey) {
                        if (std::abs(ti.t - exec->animationRealTime()) < std::abs(best.t - exec->animationRealTime())) {
                            best = ti;
                            bestIdx = idx;
                        }
                        ++idx;
                    }

                    for (auto &port: inputPorts) {
                        if (!isConnected(port.second))
                            continue;
                        port.second->objects().clear();
                        auto objs = m_cache.getObjects(port.first);
                        ssize_t start = bestIdx;
                        int step = 1;
                        if (direction < 0) {
                            start = (bestIdx+numObject-headStart)%numObject;
                            step = -1;
                            if (start > startTimestep)
                                startTimestep = start;
                        }  else if (direction > 0) {
                            start = (bestIdx+numObject+headStart)%numObject;
                            if (start < startTimestep && start >= 0)
                                startTimestep = start;
                        }
                        ssize_t cur = start;
                        for (size_t i=0; i<numObject; ++i) {
                            port.second->objects().push_back(objs[sortKey[cur].idx]);
                            cur = (cur+step+numObject)%numObject;
                        }
                    }
                }

                if (startTimestep == -1)
                    startTimestep = 0;
                if (direction < 0) {
                    startTimestep = mpi::all_reduce(comm(), startTimestep, mpi::maximum<int>());
                } else {
                    startTimestep = mpi::all_reduce(comm(), startTimestep, mpi::minimum<int>());
                }

                if (reducePolicy() == message::ReducePolicy::PerTimestepOrdered || reducePolicy() == message::ReducePolicy::PerTimestepZeroFirst) {
                    startTimestep = 0;
                    direction = 1;
                }
            }
            if (numConnected == 0) {
                // call compute at least once, e.g. for readers
                numObject = 1;
            }
            if (gang) {
                numObject = mpi::all_reduce(comm(), numObject, mpi::maximum<int>());
            }
        } else {
            // just process one tuple of objects at a time
            numObject = 1;
        }

        for (auto &port: inputPorts) {
            if (!isConnected(port.second))
                continue;
            const auto &objs = port.second->objects();
            for (Index i=0; i<numObject && i<objs.size(); ++i) {
                const auto obj = objs[i];
                int t = obj->getTimestep();
                m_numTimesteps = std::max(t+1, m_numTimesteps);
            }
        }
#ifdef REDUCE_DEBUG
        CERR << "compute with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif

        if (m_executionCount < exec->getExecutionCount()) {
            m_executionCount = exec->getExecutionCount();
            m_iteration = -1;
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
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for timesteps with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif
            m_numTimesteps = mpi::all_reduce(comm(), m_numTimesteps, mpi::maximum<int>());
#ifdef REDUCE_DEBUG
            CERR << "all_reduce for timesteps finished with #objects=" << numObject << ", #timesteps=" << m_numTimesteps << std::endl;
#endif
        }

        int prevTimestep = -1;
        if (m_numTimesteps > 0)
            prevTimestep = (startTimestep+m_numTimesteps-1)%m_numTimesteps;
        int numReductions = 0;
        bool reducePerTimestep = reducePolicy()==message::ReducePolicy::PerTimestep || reducePolicy()==message::ReducePolicy::PerTimestepZeroFirst || reducePolicy()==message::ReducePolicy::PerTimestepOrdered;
        bool computeOk = false;
        for (Index i=0; i<numObject; ++i) {
            computeOk = false;
            try {
                double start = Clock::time();
                int timestep = -1;
                for (auto &port: inputPorts) {
                    if (!isConnected(port.second))
                        continue;
                    const auto &objs = port.second->objects();
                    if (!objs.empty()) {
                        int t = objs.front()->getTimestep();
                        if (t != -1)
                            timestep = t;
                    }
                }
                if (cancelRequested()) {
                    computeOk = true;
                } else {
                    computeOk = compute();
                }
                auto runReduce = [this, &numReductions](int timestep) -> bool {
                    std::cerr << "running reduce for timestep " << timestep << std::endl;
                    if (cancelRequested(true))
                        return true;
                    ++numReductions;
                    return reduce(timestep);
                };
                if (reordered && m_numTimesteps>0 && reducePerTimestep) {
                    if (direction >= 0) {
                        if (prevTimestep > timestep) {
                            for (int t=prevTimestep; t<m_numTimesteps; ++t) {
                                computeOk &= runReduce(t);
                            }
                            for (int t=0; t<timestep; ++t) {
                                computeOk &= runReduce(t);
                            }
                        } else {
                            for (int t=prevTimestep; t<timestep; ++t) {
                                computeOk &= runReduce(t);
                            }
                        }
                    } else {
                        if (prevTimestep < timestep) {
                            for (int t=prevTimestep; t>=0; --t) {
                                computeOk &= runReduce(t);
                            }
                            for (int t=m_numTimesteps-1; t>timestep; --t) {
                                computeOk &= runReduce(t);
                            }
                        } else {
                            for (int t=prevTimestep; t>timestep; --t) {
                                computeOk &= runReduce(t);
                            }
                        }
                    }
                }
                prevTimestep = timestep;
                double duration = Clock::time() - start;
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
            while (numReductions < m_numTimesteps) {
                ++numReductions;
                reduce(prevTimestep);
                if (direction >= 0) {
                    prevTimestep = (prevTimestep+1)%m_numTimesteps;
                } else {
                    prevTimestep = (prevTimestep+m_numTimesteps-1)%m_numTimesteps;
                }
            }
        }

        --m_executionDepth;
        //vassert(m_executionDepth == 0);
    }

    if (exec->what() == Execute::ComputeExecute
            || exec->what() == Execute::Reduce) {
        ret &= reduceWrapper(exec);
        m_cache.clearOld();
    }
    message::Idle idle;
    idle.setReferrer(exec->uuid());
    idle.setDestId(Id::LocalManager);
    sendMessage(idle);

    return ret;
}

std::string Module::getModuleName(int id) const {

   return m_stateTracker->getModuleName(id);
}

Module::~Module() {

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

namespace {

using namespace boost;

class InstModule: public Module {
public:
   InstModule()
   : Module("description", "shmid", "name", 1)
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

   m_numTimesteps = 0;
   m_cancelRequested = false;
   m_cancelExecuteCalled = false;

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

   message::ExecutionProgress start(message::ExecutionProgress::Start);
   start.setReferrer(exec->uuid());
   start.setDestId(Id::LocalManager);
   sendMessage(start);

   if (m_benchmark) {

      comm().barrier();
      m_benchmarkStart = Clock::time();
   }

   //CERR << "prepareWrapper: prepared=" << m_prepared << std::endl;
   m_prepared = true;

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

bool Module::reduceWrapper(const message::Execute *exec) {

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
       m_cancelRequested = boost::mpi::all_reduce(comm(), m_cancelRequested, std::logical_or<bool>());
       m_numTimesteps = boost::mpi::all_reduce(comm(), m_numTimesteps, boost::mpi::maximum<int>());
   }

   m_reduced = true;

   bool ret = false;
   try {
       switch(reducePolicy()) {
       case message::ReducePolicy::Never: {
           ret = true;
           break;
       }
       case message::ReducePolicy::PerTimestep:
       case message::ReducePolicy::PerTimestepZeroFirst:
       case message::ReducePolicy::PerTimestepOrdered: {
           if (m_numTimesteps > 0) {
               ret = true;
               if (exec->what() != message::Execute::ComputeExecute) {
                   for (int t=0; t<m_numTimesteps; ++t) {
                       if (!cancelRequested(sync))
                           ret &= reduce(t);
                   }
               }
               break;
           }
       }
       case message::ReducePolicy::Locally:
       case message::ReducePolicy::OverAll: {
           if (!cancelRequested(sync)) {
               ret = reduce(-1);
           }
           break;
       }
       }
   } catch (std::exception &e) {
           std::cout << name() << "::reduce(): exception - " << e.what() << std::endl << std::flush;
           std::cerr << name() << "::reduce(): exception - " << e.what() << std::endl;
   }

   if (m_benchmark) {
      comm().barrier();
      double duration = Clock::time() - m_benchmarkStart;
      if (rank() == 0) {
#ifdef _OPENMP
         int nthreads = -1;
         nthreads = omp_get_max_threads();
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

   message::ExecutionProgress fin(message::ExecutionProgress::Finish);
   fin.setReferrer(exec->uuid());
   fin.setDestId(Id::LocalManager);
   sendMessage(fin);

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

    return true;
}

int Module::numTimesteps() const {

    return m_numTimesteps;
}

bool Module::cancelRequested(bool sync) {

    if (sync) {
        m_cancelRequested = boost::mpi::all_reduce(comm(), m_cancelRequested, std::logical_or<bool>());
        if (m_cancelRequested && !m_cancelExecuteCalled) {
            if (rank() == 0)
                sendInfo("canceling execution");
            cancelExecute();
            m_cancelExecuteCalled = true;
        }
    }

    if (m_cancelRequested)
        return true;

    bool justCanceled = false;
    message::Buffer buf;
    if (receiveMessageQueue->tryReceive(buf)) {
        messageBacklog.push_back(buf);
        if (buf.type() == message::CANCELEXECUTE) {
            const auto &cancel = buf.as<message::CancelExecute>();
            if (cancel.getModule() == id()) {
                std::cerr << "canceling execution requested" << std::endl;
                justCanceled = true;
            }
        }
    }

    if (sync) {
        justCanceled = boost::mpi::all_reduce(comm(), justCanceled, std::logical_or<bool>());

        if (justCanceled) {
            m_cancelRequested = true;
            std::cerr << "canceling execution" << std::endl;
            if (rank() == 0)
                sendInfo("canceling execution");
            cancelExecute();
            m_cancelExecuteCalled = true;
            return true;
        }
    }

    return justCanceled;
}

} // namespace vistle
