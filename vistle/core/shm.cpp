#include <iostream>
#include <iomanip>

#include <limits.h>

#include "message.h"
#include "messagequeue.h"
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

   return (size_t)1 << 34;
}

Shm* Shm::s_singleton = NULL;
#ifdef SHMDEBUG
shm<ShmDebugInfo>::vector *Shm::s_shmdebug = NULL;
#endif

Shm::Shm(const std::string &name, const int m, const int r, const size_t size,
         message::MessageQueue * mq, bool create)
   : m_name(name), m_created(create), m_moduleID(m), m_rank(r), m_objectID(0), m_messageQueue(mq) {

      if (create) {
         m_shm = new managed_shared_memory(open_or_create, m_name.c_str(), size);
      } else {
         m_shm = new managed_shared_memory(open_only, m_name.c_str());
      }

      m_allocator = new void_allocator(getShm().get_segment_manager());

#ifdef SHMDEBUG
      s_shmdebug = m_shm->find_or_construct<shm<ShmDebugInfo>::vector>("shmdebug")(0, ShmDebugInfo(), allocator());
#endif
}

Shm::~Shm() {

   if (m_created) {
      shared_memory_object::remove(m_name.c_str());
      std::cerr << "removed shm " << m_name << std::endl;
   }

   delete m_allocator;

   delete m_shm;
}

std::string Shm::shmIdFilename() {

   std::stringstream name;
   name << "/tmp/vistle_shmids_" << getuid() << ".txt";
   return name.str();
}

bool Shm::cleanAll() {

   std::fstream shmlist;
   shmlist.open(shmIdFilename().c_str(), std::ios::in);
   remove(shmIdFilename().c_str());

   while (!shmlist.eof() && !shmlist.fail()) {
      std::string shmid;
      shmlist >> shmid;
      if (!shmid.empty()) {
         if (shmid.find("mq_") == 0) {
            std::cerr << "removing message queue: id " << shmid << std::endl;
            message_queue::remove(shmid.c_str());
         } else {
            std::cerr << "removing shared memory: id " << shmid << std::endl;
            shared_memory_object::remove(shmid.c_str());
         }
      }
   }
   shmlist.close();

   return true;
}

const std::string &Shm::getName() const {

   return m_name;
}

const Shm::void_allocator &Shm::allocator() const {

   return *m_allocator;
}

Shm &Shm::instance() {
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

   assert(s_singleton && "failed to attach to shared memory");

   return *s_singleton;
}

managed_shared_memory & Shm::getShm() {

   return *m_shm;
}

const managed_shared_memory & Shm::getShm() const {

   return *m_shm;
}

void Shm::publish(const shm_handle_t & handle) {

   if (m_messageQueue) {
      vistle::message::NewObject n(m_moduleID, m_rank, handle);
      m_messageQueue->getMessageQueue().send(&n, sizeof(n), 0);
   }
}

std::string Shm::createObjectID() {

   std::stringstream name;
   name << "m" << std::setw(8) << std::setfill('0') << m_moduleID
        << "r" << std::setw(8) << std::setfill('0') << m_rank
        << "id" << std::setw(8) << std::setfill('0') << m_objectID++
        << "OBJ";

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

shm_handle_t Shm::getHandleFromObject(Object::const_ptr object) {

   try {
      return m_shm->get_handle_from_address(object->d());

   } catch (interprocess_exception &ex) { }

   return 0;
}

shm_handle_t Shm::getHandleFromObject(const Object *object) {

   try {
      return m_shm->get_handle_from_address(object->d());

   } catch (interprocess_exception &ex) { }

   return 0;
}

Object::const_ptr Shm::getObjectFromHandle(const shm_handle_t & handle) {

   try {
      Object::Data *od = static_cast<Object::Data *>
         (m_shm->get_address_from_handle(handle));

      return Object::create(od);
   } catch (interprocess_exception &ex) { }

   return Object::const_ptr();
}

} // namespace vistle
