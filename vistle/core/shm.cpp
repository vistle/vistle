
#include <util/sysdep.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>

#include <limits.h>

#include <util/valgrind.h>
#include "messagequeue.h"
#include "scalars.h"
#include "assert.h"

#ifndef TEMPLATES_IN_HEADERS
#define VISTLE_IMPL
#endif

#include "archives.h"
#include "shm.h"
#include "shm_reference.h"
#include "object.h"

using namespace boost::interprocess;
namespace interprocess = boost::interprocess;

namespace vistle {

template<int> size_t memorySize();

template<> size_t memorySize<4>() {

   return INT_MAX;
}

template<> size_t memorySize<8>() {

   if (RUNNING_ON_VALGRIND) {
      std::cerr << "running under valgrind: reducing shmem size" << std::endl;
      return (size_t)1 << 32;
   } else {
#if defined(__APPLE__)
      return (size_t)1 << 32;
#elif defined(_WIN32)
      return (size_t)1 << 30; // 1 GB
#else
      return (size_t)1 << 38; // 256 GB
#endif
   }
}

Shm* Shm::s_singleton = nullptr;
#ifdef SHMDEBUG
#ifdef NO_SHMEM
shm<ShmDebugInfo>::vector *Shm::s_shmdebug = new shm<ShmDebugInfo>::vector;
std::recursive_mutex *Shm::s_shmdebugMutex = new std::recursive_mutex;
#else
shm<ShmDebugInfo>::vector *Shm::s_shmdebug = nullptr;
boost::interprocess::interprocess_recursive_mutex *Shm::s_shmdebugMutex = nullptr;
#endif
#endif

std::string Shm::instanceName(const std::string &host, unsigned short port) {

   std::stringstream str;
   str << "vistle_" << host << "_u" << getuid() << "_p" << port;
   return str.str();
}

Shm::Shm(const std::string &name, const int m, const int r, const size_t size,
         message::MessageQueue * mq, bool create)
   : m_allocator(nullptr)
   , m_name(name)
   , m_remove(create)
   , m_moduleId(m)
   , m_rank(r)
   , m_objectId(0)
   , m_arrayId(0)
   , m_shmDeletionMutex(nullptr)
   , m_objectDictionaryMutex(nullptr)
#ifndef NO_SHMEM
   , m_shm(nullptr)
#endif
{

#ifdef SHMDEBUG
      if (create) {
          std::cerr << "SHMDEBUG: won't remove shm " << m_name << std::endl;
          m_remove = false;
      }
#endif

#ifdef NO_SHMEM
      m_allocator = new void_allocator();
      m_shmDeletionMutex = new std::recursive_mutex;
      m_objectDictionaryMutex = new std::recursive_mutex;
#else
      if (create) {
         m_shm = new managed_shared_memory(open_or_create, m_name.c_str(), size);
      } else {
         m_shm = new managed_shared_memory(open_only, m_name.c_str());
      }

      m_allocator = new void_allocator(shm().get_segment_manager());

      m_shmDeletionMutex = m_shm->find_or_construct<boost::interprocess::interprocess_recursive_mutex>("shmdelete_mutex")();
      m_objectDictionaryMutex = m_shm->find_or_construct<boost::interprocess::interprocess_recursive_mutex>("shm_dictionary_mutex")();

#ifdef SHMDEBUG
      s_shmdebugMutex = m_shm->find_or_construct<boost::interprocess::interprocess_recursive_mutex>("shmdebug_mutex")();
      s_shmdebug = m_shm->find_or_construct<vistle::shm<ShmDebugInfo>::vector>("shmdebug")(0, ShmDebugInfo(), allocator());
#endif
#endif

      m_lockCount = 0;
}

Shm::~Shm() {

#ifndef NO_SHMEM
   if (m_remove) {
      shared_memory_object::remove(m_name.c_str());
      std::cerr << "removed shm " << m_name << std::endl;
   }
   delete m_shm;
#endif

   delete m_allocator;
}

void Shm::detach() {

   delete s_singleton;
   s_singleton = NULL;
}

void Shm::setRemoveOnDetach() {

   m_remove = true;
}

std::string Shm::shmIdFilename() {

   std::stringstream name;
#ifdef _WIN32
   
   
    DWORD dwRetVal = 0;
    UINT uRetVal   = 0;
    TCHAR szTempFileName[MAX_PATH];  
    TCHAR lpTempPathBuffer[MAX_PATH];

    //  Gets the temp path env string (no guarantee it's a valid path).
    dwRetVal = GetTempPath(MAX_PATH,          // length of the buffer
                           lpTempPathBuffer); // buffer for path 
    if (dwRetVal > MAX_PATH || (dwRetVal == 0))
    {
        name << "/tmp/vistle_shmids_";
    }

    //  Generates a temporary file name. 
    uRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
                              TEXT("DEMO"),     // temp file name prefix 
                              0,                // create unique name 
                              szTempFileName);  // buffer for name 
    if (uRetVal == 0)
    {
        
        name << "/tmp/vistle_shmids_";
    }
	name << szTempFileName;
#else
   name << "/tmp/vistle_shmids_" << getuid() << ".txt";
#endif
   return name.str();
}

void Shm::lockObjects() const {
#ifndef NO_SHMEM
#ifndef SHMPERRANK
   if (m_lockCount != 0) {
       //std::cerr << "Shm::lockObjects(): lockCount=" << m_lockCount << std::endl;
   }
   //assert(m_lockCount==0);
   ++m_lockCount;
   m_shmDeletionMutex->lock();
#endif
#endif
}

void Shm::unlockObjects() const {
#ifndef NO_SHMEM
#ifndef SHMPERRANK
   m_shmDeletionMutex->unlock();
   --m_lockCount;
   //assert(m_lockCount==0);
   if (m_lockCount != 0) {
       //std::cerr << "Shm::unlockObjects(): lockCount=" << m_lockCount << std::endl;
   }
#endif
#endif
}

void Shm::lockDictionary() const {
#ifdef NO_SHMEM
   m_objectDictionaryMutex->lock();
#endif
}

void Shm::unlockDictionary() const {
#ifdef NO_SHMEM
   m_objectDictionaryMutex->unlock();
#endif
}

namespace {

std::string shmSegName(const std::string &name, const int rank) {
#ifdef SHMPERRANK
    return name + "_r" + std::to_string(rank);
#else
    (void)rank;
    return name;
#endif
}

}

bool Shm::cleanAll(int rank) {

   std::fstream shmlist;
   shmlist.open(shmIdFilename().c_str(), std::ios::in);
   ::remove(shmIdFilename().c_str());

   bool ret = true;

   while (!shmlist.eof() && !shmlist.fail()) {
      std::string shmid;
      shmlist >> shmid;
      if (!shmid.empty()) {
         bool ok = true;
         bool log = true;

         if (shmid.find("_send_") != std::string::npos || shmid.find("_recv_") != std::string::npos) {
            //std::cerr << "removing message queue: id " << shmid << std::flush;
            ok = message::MessageQueue::message_queue::remove(shmid.c_str());
            log = false;
         } else {
            std::cerr << "removing shared memory: id " << shmid << std::flush;
            ok = shared_memory_object::remove(shmSegName(shmid.c_str(), rank).c_str());
         }
         if (log)
             std::cerr << ": " << (ok ? "ok" : "failure") << std::endl;

         if (!ok)
            ret = false;
      }
   }
   shmlist.close();

   return ret;
}

std::string Shm::name() const {

    return shmSegName(instanceName(), m_rank);
}

const std::string &Shm::instanceName() const {

   return m_name;
}

const Shm::void_allocator &Shm::allocator() const {

   return *m_allocator;
}

Shm &Shm::the() {
#ifndef MODULE_THREAD
   assert(s_singleton && "make sure to create or attach to a shm segment first");
   if (!s_singleton)
      exit(1);
#endif
   return *s_singleton;
}

bool Shm::isAttached() {
    return s_singleton;
}

bool Shm::remove(const std::string &name, const int moduleID, const int rank) {

   std::string n = shmSegName(name, rank);
   return interprocess::shared_memory_object::remove(n.c_str());
}

Shm & Shm::create(const std::string &name, const int moduleID, const int rank,
                    message::MessageQueue * mq) {

   std::string n = shmSegName(name, rank);

   if (!s_singleton) {
      {
         // store name of shared memory segment for possible clean up
         std::ofstream shmlist;
         shmlist.open(shmIdFilename().c_str(), std::ios::out | std::ios::app);
         shmlist << name << std::endl;
      }

      size_t memsize = memorySize<sizeof(void *)>();

      do {
         try {
            s_singleton = new Shm(n, moduleID, rank, memsize, mq, true);
         } catch (boost::interprocess::interprocess_exception &ex) {
            memsize /= 2;
         }
      } while (!s_singleton && memsize >= 4096);

      if (!s_singleton) {
         throw(shm_exception("failed to create shared memory segment"));

         std::cerr << "failed to create shared memory: module id: " << moduleID
            << ", rank: " << rank << ", message queue: " << (mq ? mq->getName() : "n/a") << std::endl;
      }
   }

   assert(s_singleton && "failed to create shared memory");

   return *s_singleton;
}

Shm & Shm::attach(const std::string &name, const int moduleID, const int rank,
                    message::MessageQueue * mq) {

   std::string n = shmSegName(name, rank);

   if (!s_singleton) {
      size_t memsize = memorySize<sizeof(void *)>();

      do {
         try {
            s_singleton = new Shm(n, moduleID, rank, memsize, mq, false);
         } catch (boost::interprocess::interprocess_exception &ex) {
            memsize /= 2;
         }
      } while (!s_singleton && memsize >= 4096);

      if (!s_singleton) {
         std::cerr << "failed to attach to shared memory: module id: " << moduleID
            << ", rank: " << rank << ", message queue: " << (mq ? mq->getName() : "n/a") << std::endl;
      }
   }

   if (!s_singleton) {

      throw(shm_exception("failed to attach to shared memory segment "+name));
   }

   assert(s_singleton && "failed to attach to shared memory");

   return *s_singleton;
}

#ifndef NO_SHMEM
managed_shared_memory & Shm::shm() {

   return *m_shm;
}

const managed_shared_memory & Shm::shm() const {

   return *m_shm;
}
#endif

std::string Shm::createObjectId(const std::string &id) {

#ifndef MODULE_THREAD
   vassert(m_moduleId > 0 || !id.empty());
#endif
   vassert(id.size() < sizeof(shm_name_t));
   if (!id.empty()) {
      return id;
   }
   std::stringstream name;
   name
#ifndef MODULE_THREAD
        << m_moduleId << "m"
#endif
        << m_objectId++ << "o"
        << m_rank << "r";

   vassert(name.str().size() < sizeof(shm_name_t));

   return name.str();
}

std::string Shm::createArrayId(const std::string &id) {
#ifndef MODULE_THREAD
   vassert(m_moduleId > 0 || !id.empty());
#endif
   vassert(id.size() < sizeof(shm_name_t));
   if (!id.empty()) {
      return id;
   }

   std::stringstream name;
   name
#ifndef MODULE_THREAD
        << m_moduleId << "m"
#endif
        << m_arrayId++ << "a"
        << m_rank << "r";

   vassert(name.str().size() < sizeof(shm_name_t));

   return name.str();
}

void Shm::markAsRemoved(const std::string &name) {
#ifdef SHMDEBUG
   s_shmdebugMutex->lock();

   for (size_t i=0; i<s_shmdebug->size(); ++i) {
      if (!strncmp(name.c_str(), (*s_shmdebug)[i].name, sizeof(shm_name_t))) {
         assert((*s_shmdebug)[i].deleted == 0);
         ++(*s_shmdebug)[i].deleted;
      }
   }

   s_shmdebugMutex->unlock();
#endif
}

void Shm::addObject(const std::string &name, const shm_handle_t &handle) {
#ifdef SHMDEBUG
   s_shmdebugMutex->lock();
   Shm::the().s_shmdebug->push_back(ShmDebugInfo('O', name, handle));
   s_shmdebugMutex->unlock();
#endif
}

shm_handle_t Shm::getHandleFromObject(Object::const_ptr object) const {

#ifdef NO_SHMEM
   return object->d();
#else
   try {
      return m_shm->get_handle_from_address(object->d());

   } catch (interprocess_exception &ex) { }

   return 0;
#endif
}

shm_handle_t Shm::getHandleFromObject(const Object *object) const {

#ifdef NO_SHMEM
   return object->d();
#else
   try {
      return m_shm->get_handle_from_address(object->d());

   } catch (interprocess_exception &ex) { }

   return 0;
#endif
}

Object::const_ptr Shm::getObjectFromHandle(const shm_handle_t & handle) const {
    
    Object::const_ptr ret;
    lockObjects();
    Object::Data *od = getObjectDataFromHandle(handle);
    if (od) {
        od->ref();
        unlockObjects();
        ret.reset(Object::create(od));
        od->unref();
    } else {
        unlockObjects();
    }
    return ret;
}

ObjectData *Shm::getObjectDataFromName(const std::string &name) const {

   // we have to use char here, otherwise boost-internal consistency checks fail
    return static_cast<Object::Data *>(static_cast<void *>(vistle::shm<char>::find(name)));
}

ObjectData *Shm::getObjectDataFromHandle(const shm_handle_t &handle) const
{
#ifdef NO_SHMEM
    Object::Data *od = static_cast<Object::Data *>(handle);
    return od;
#else
    try {
        Object::Data *od = static_cast<Object::Data *>(m_shm->get_address_from_handle(handle));
        return od;
    } catch (interprocess_exception &ex) {
        std::cerr << "Shm::getObjectDataFromHandle: invalid handle " << handle << std::endl;
    }
    
    return nullptr;
#endif
    
}

Object::const_ptr Shm::getObjectFromName(const std::string &name, bool onlyComplete) const {

   lockObjects();
   auto od = getObjectDataFromName(name);
   if (od) {
      if (od->isComplete() || !onlyComplete) {
          od->ref();
          unlockObjects();
          Object::const_ptr obj(Object::create(od));
          od->unref();
          return obj;
      }
      unlockObjects();
      std::cerr << "Shm::getObjectFromName: " << name << " not complete" << std::endl;
   } else {
       unlockObjects();
       std::cerr << "Shm::getObjectFromName: did not find " << name << std::endl;
   }

   return Object::const_ptr();
}

#if 0
namespace {

using namespace boost;

template <class T>
struct wrap {};

template<typename S, class Stream>
struct archive_instantiator {
   template<class T> void operator()(wrap<T>) {
      typedef ShmVector<S> V;
      V p;
      const V c = p;
      Object::ptr obj;
      V q = c;
      //q.construct(2);
      q->resize(1);
      //std::cerr << q->refcount() << std::endl;
      typedef T Archive;
      Stream stream;
      Archive ar(stream);
      ar & V_NAME(ar, "shm_ptr", p);
   }
};

struct instantiator {
   template<typename S> void operator()(S) {
      mpl::for_each<InputArchives, wrap<mpl::_1> >(archive_instantiator<S, std::ifstream>());
      mpl::for_each<OutputArchives, wrap<mpl::_1> >(archive_instantiator<S, std::ofstream>());
   }
};

} // namespace

void instantiate_shmvector() {

   mpl::for_each<VectorTypes>(instantiator());
}
#endif

} // namespace vistle
