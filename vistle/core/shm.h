#ifndef SHM_H
#define SHM_H

#include <memory>

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/serialization/access.hpp>

#include <util/exception.h>

#include "export.h"
#include "index.h"
#include "shm_array.h"

//#define SHMDEBUG
//#define SHMPUBLISH

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

struct V_COREEXPORT shm_name_t {
   std::array<char, 32> name;
   //shm_name_t(const std::string &s = "INVALID");
   shm_name_t(const std::string &s = std::string());

   operator const char *() const;
   operator char *();
   operator std::string () const;
   bool operator==(const std::string &rhs) const;
   bool operator==(const shm_name_t &rhs) const;
   bool empty() const;
   void clear();

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
   template<class Archive>
      void save(Archive &ar, const unsigned int version) const;
   template<class Archive>
      void load(Archive &ar, const unsigned int version);
};
std::string operator+(const std::string &s, const shm_name_t &n);

namespace message {
   class MessageQueue;
}

class V_COREEXPORT shm_exception: public exception {
   public:
   shm_exception(const std::string &what = "shared memory error") : exception(what) {}
};

class Object;
struct ObjectData;

#ifdef SHMDEBUG
struct ShmDebugInfo {
   shm_name_t name;
   shm_handle_t handle;
   char deleted;
   char type;

   ShmDebugInfo(char type='\0', const std::string &name = "", shm_handle_t handle = 0)
      : handle(handle)
      , deleted(0)
      , type(type)
   {
      memset(this->name, '\0', sizeof(this->name));
      strncpy(this->name, name.c_str(), sizeof(this->name)-1);
   }
};
#endif

template<typename T>
struct shm {
   typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> allocator;
   typedef boost::interprocess::basic_string<T, std::char_traits<T>, allocator> string;
   typedef boost::interprocess::vector<T, allocator> vector;
   typedef vistle::shm_array<T, allocator> array;
   typedef boost::interprocess::offset_ptr<array> array_ptr;
   static typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type construct(const std::string &name);
   static T *find(const std::string &name);
   static bool destroy(const std::string &name);
   static void destroy_ptr(T *ptr);
};

template<class T>
class shm_ref;
template<class T>
using ShmVector = shm_ref<shm_array<T, typename shm<T>::allocator>>;

class V_COREEXPORT Shm {

 public:
   static std::string instanceName(const std::string &host, unsigned short port);
   static Shm & the();
   static bool remove(const std::string &shmname, const int moduleID, const int rank);
   static Shm & create(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   static Shm & attach(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   static bool isAttached();

   void detach();
   void setRemoveOnDetach();

   std::string name() const;
   const std::string &instanceName() const;

   typedef boost::interprocess::allocator<void, boost::interprocess::managed_shared_memory::segment_manager> void_allocator;
   const void_allocator &allocator() const;

   boost::interprocess::managed_shared_memory &shm();
   const boost::interprocess::managed_shared_memory &shm() const;
   std::string createArrayId(const std::string &name="");
   std::string createObjectId(const std::string &name="");

   void lockObjects() const;
   void unlockObjects() const;

   std::shared_ptr<const Object> getObjectFromHandle(const shm_handle_t & handle) const;
   shm_handle_t getHandleFromObject(std::shared_ptr<const Object> object) const;
   shm_handle_t getHandleFromObject(const Object *object) const;
   ObjectData *getObjectDataFromName(const std::string &name) const;
   std::shared_ptr<const Object> getObjectFromName(const std::string &name, bool onlyComplete=true) const;
   template<typename T>
   const ShmVector<T> getArrayFromName(const std::string &name) const;

   static std::string shmIdFilename();
   static bool cleanAll(int rank);

   void markAsRemoved(const std::string &name);
   void addObject(const std::string &name, const shm_handle_t &handle);
#ifdef SHMDEBUG
   static boost::interprocess::vector<ShmDebugInfo, vistle::shm<ShmDebugInfo>::allocator> *s_shmdebug;
   static boost::interprocess::interprocess_recursive_mutex *s_shmdebugMutex;
#endif

 private:
   Shm(const std::string &name, const int moduleID, const int rank, const size_t size,
       message::MessageQueue *messageQueue, bool create);
   ~Shm();

   void_allocator *m_allocator;
   std::string m_name;
   bool m_remove;
   const int m_moduleId;
   const int m_rank;
   int m_objectId;
   int m_arrayId;
   static Shm *s_singleton;
   boost::interprocess::managed_shared_memory *m_shm;
   mutable boost::interprocess::interprocess_recursive_mutex *m_shmDeletionMutex;
   mutable int m_lockCount = 0;
};

template<typename T>
typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type shm<T>::construct(const std::string &name) {
   return Shm::the().shm().construct<T>(name.c_str());
}

template<typename T>
T *shm<T>::find(const std::string &name) {
   return Shm::the().shm().find<T>(name.c_str()).first;
}

template<typename T>
void shm<T>::destroy_ptr(T *ptr) {
   return Shm::the().shm().destroy_ptr<T>(ptr);
}

template<typename T>
bool shm<T>::destroy(const std::string &name) {
    const bool ret = Shm::the().shm().destroy<T>(name.c_str());
    Shm::the().markAsRemoved(name);
    return ret;
}

} // namespace vistle

#endif

#ifdef VISTLE_IMPL
#include "shm_impl.h"
#endif
