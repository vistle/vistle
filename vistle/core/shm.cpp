
#include <util/sysdep.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>

#include <limits.h>

#include <util/valgrind.h>
#include "message.h"
#include "messagequeue.h"
#include "scalars.h"
#include "celltree.h"

#ifndef TEMPLATES_IN_HEADERS
#define VISTLE_IMPL
#endif

#include "shm.h"

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

using namespace boost::interprocess;

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
#ifdef __APPLE__
      return (size_t)1 << 34;
#else
      return (size_t)1 << 36; // 64 GB
#endif
   }
}

Shm* Shm::s_singleton = NULL;
#ifdef SHMDEBUG
shm<ShmDebugInfo>::vector *Shm::s_shmdebug = NULL;
#endif

shm_name_t::shm_name_t(const std::string &s) {

   size_t len = s.size();
   assert(len < name.size());
   if (len >= name.size())
      len = name.size()-1;
   strncpy(name.data(), &s[0], len);
   name[len] = '\0';
}

shm_name_t::operator const char *() const {
   return name.data();
}

shm_name_t::operator char *() {
   return name.data();
}

shm_name_t::operator std::string () const {
   return name.data();
}

std::string operator+(const std::string &s, const shm_name_t &n) {
   return s + n.name.data();
}

std::string Shm::instanceName(const std::string &host, unsigned short port) {

   std::stringstream str;
   str << "vistle_" << host << "_" << getuid() << "_" << port;
   return str.str();
}

Shm::Shm(const std::string &name, const int m, const int r, const size_t size,
         message::MessageQueue * mq, bool create)
   : m_name(name), m_remove(create), m_moduleID(m), m_rank(r), m_objectID(0) {

      if (create) {
         m_shm = new managed_shared_memory(open_or_create, m_name.c_str(), size);
      } else {
         m_shm = new managed_shared_memory(open_only, m_name.c_str());
      }

      m_allocator = new void_allocator(shm().get_segment_manager());

#ifdef SHMDEBUG
      s_shmdebug = m_shm->find_or_construct<vistle::shm<ShmDebugInfo>::vector>("shmdebug")(0, ShmDebugInfo(), allocator());
#endif
}

Shm::~Shm() {

   if (m_remove) {
      shared_memory_object::remove(m_name.c_str());
      std::cerr << "removed shm " << m_name << std::endl;
   }

   delete m_allocator;

   delete m_shm;
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

bool Shm::cleanAll() {

   std::fstream shmlist;
   shmlist.open(shmIdFilename().c_str(), std::ios::in);
   remove(shmIdFilename().c_str());

   bool ret = true;

   while (!shmlist.eof() && !shmlist.fail()) {
      std::string shmid;
      shmlist >> shmid;
      if (!shmid.empty()) {
         bool ok = true;

         if (shmid.find("mq_") == 0) {
            std::cerr << "removing message queue: id " << shmid << std::flush;
            ok = message::MessageQueue::message_queue::remove(shmid.c_str());
         } else {
            std::cerr << "removing shared memory: id " << shmid << std::flush;
            ok = shared_memory_object::remove(shmid.c_str());
         }
         std::cerr << ": " << (ok ? "ok" : "failure") << std::endl;

         if (!ok)
            ret = false;
      }
   }
   shmlist.close();

   return false;
}

const std::string &Shm::name() const {

   return m_name;
}

const Shm::void_allocator &Shm::allocator() const {

   return *m_allocator;
}

Shm &Shm::the() {
   assert(s_singleton && "make sure to create or attach to a shm segment first");
   if (!s_singleton)
      exit(1);
   return *s_singleton;
}

Shm & Shm::create(const std::string &name, const int moduleID, const int rank,
                    message::MessageQueue * mq) {

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
            s_singleton = new Shm(name, moduleID, rank, memsize, mq, true);
         } catch (boost::interprocess::interprocess_exception ex) {
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

   if (!s_singleton) {
      size_t memsize = memorySize<sizeof(void *)>();

      do {
         try {
            s_singleton = new Shm(name, moduleID, rank, memsize, mq, false);
         } catch (boost::interprocess::interprocess_exception ex) {
            memsize /= 2;
         }
      } while (!s_singleton && memsize >= 4096);

      if (!s_singleton) {
         std::cerr << "failed to attach to shared memory: module id: " << moduleID
            << ", rank: " << rank << ", message queue: " << (mq ? mq->getName() : "n/a") << std::endl;
      }
   }

   if (!s_singleton) {

      throw(shm_exception("failed to attach to shared memory segment"));
   }

   assert(s_singleton && "failed to attach to shared memory");

   return *s_singleton;
}

managed_shared_memory & Shm::shm() {

   return *m_shm;
}

const managed_shared_memory & Shm::shm() const {

   return *m_shm;
}

std::string Shm::createObjectID() {

   std::stringstream name;
   name << "m" << m_moduleID
        << "id" << m_objectID++
        << "r" << m_rank
        << "OBJ";

   vassert(name.str().size() < sizeof(shm_name_t));

   return name.str();
}

std::string Shm::createArrayId() {

   std::stringstream name;
   name << "m" << m_moduleID
        << "id" << m_objectID++
        << "r" << m_rank
        << "ARR";

   vassert(name.str().size() < sizeof(shm_name_t));

   return name.str();
}

#ifdef SHMDEBUG
void Shm::markAsRemoved(const std::string &name) {
   for (size_t i=0; i<s_shmdebug->size(); ++i) {
      if (!strncmp(name.c_str(), (*s_shmdebug)[i].name, sizeof(shm_name_t)))
         ++(*s_shmdebug)[i].deleted;
   }
}
#endif

shm_handle_t Shm::getHandleFromObject(Object::const_ptr object) const {

   try {
      return m_shm->get_handle_from_address(object->d());

   } catch (interprocess_exception &ex) { }

   return 0;
}

shm_handle_t Shm::getHandleFromObject(const Object *object) const {

   try {
      return m_shm->get_handle_from_address(object->d());

   } catch (interprocess_exception &ex) { }

   return 0;
}

Object::const_ptr Shm::getObjectFromHandle(const shm_handle_t & handle) const {

   try {
      Object::Data *od = static_cast<Object::Data *>
         (m_shm->get_address_from_handle(handle));

      return Object::create(od);
   } catch (interprocess_exception &ex) { }

   return Object::const_ptr();
}

Object::const_ptr Shm::getObjectFromName(const std::string &name) const {

   // we have to use char here, otherwise boost-internal consistency checks fail
   void *mem = vistle::shm<char>::find(name);
   if (mem) {
      return Object::create(static_cast<Object::Data *>(mem));
   }

   return Object::const_ptr();
}

namespace {

using namespace boost;

template <class T>
struct wrap {};

template<typename S, class Stream>
struct archive_instantiator {
   template<class T> void operator()(wrap<T>) {
      typedef ShmVector<S> V;
      typename V::ptr p(new V);
      typename V::ptr q;
      const typename V::ptr c = p;
      q = c;
      q = new V();
      q->resize(1);
      std::cerr << q->refcount() << std::endl;
      typedef T Archive;
      Stream stream;
      Archive ar(stream);
      ar & V_NAME("shm_ptr", p);
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

   mpl::for_each<Scalars>(instantiator());

   typedef boost::mpl::vector<Celltree<Scalar, Index>::Node> CelltreeNodes;
   mpl::for_each<CelltreeNodes>(instantiator());
}

} // namespace vistle
