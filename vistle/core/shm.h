#ifndef SHM_H
#define SHM_H

#include <memory>
#include <array>
#include <mutex>
#include <atomic>

#ifdef NO_SHMEM
#include <memory>
#include <string>
#include <vector>
#include <map>
#else
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#endif

#include "archives_config.h"

#include <util/exception.h>

#include "export.h"
#include "index.h"
//#include "shm_array.h"
#include "shmname.h"

//#define SHMDEBUG
//#define SHMPUBLISH

namespace vistle {

#ifdef NO_SHMEM
typedef void *shm_handle_t;
#else
typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;
#endif

namespace message {
   class MessageQueue;
}

class V_COREEXPORT shm_exception: public exception {
   public:
   shm_exception(const std::string &what = "shared memory error") : exception(what) {}
};

class Object;
struct ObjectData;
template<class T, class allocator>
class shm_array;

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
#ifdef NO_SHMEM
   typedef std::allocator<T> allocator;
   typedef std::basic_string<T> string;
   typedef std::vector<T> vector;
   typedef vistle::shm_array<T, allocator> array;
   typedef array *array_ptr;
   struct Constructor {
       std::string name;

       Constructor(const std::string &name);
       ~Constructor();
       template<typename... Args>
       T *operator()(Args&&... args);
   };
   static Constructor construct(const std::string &name) {
       return Constructor(name);
   }
#else
   typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> allocator;
   typedef boost::interprocess::basic_string<T, std::char_traits<T>, allocator> string;
   typedef boost::interprocess::vector<T, allocator> vector;
   typedef vistle::shm_array<T, allocator> array;
   typedef boost::interprocess::offset_ptr<array> array_ptr;
   static typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type construct(const std::string &name);
#endif
   static T *find(const std::string &name);
   static bool destroy(const std::string &name);
};

template<class T>
class shm_ref;
template<class T>
using ShmVector = shm_ref<shm_array<T, typename shm<T>::allocator>>;

class V_COREEXPORT Shm {
    template<typename T>
    friend struct shm;

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

#ifdef NO_SHMEM
   typedef std::allocator<void> void_allocator;
#else
   typedef boost::interprocess::allocator<void, boost::interprocess::managed_shared_memory::segment_manager> void_allocator;
   boost::interprocess::managed_shared_memory &shm();
   const boost::interprocess::managed_shared_memory &shm() const;
#endif
   const void_allocator &allocator() const;

   std::string createArrayId(const std::string &name="");
   std::string createObjectId(const std::string &name="");

   void lockObjects() const;
   void unlockObjects() const;
   void lockDictionary() const;
   void unlockDictionary() const;

   std::shared_ptr<const Object> getObjectFromHandle(const shm_handle_t &handle) const;
   shm_handle_t getHandleFromObject(std::shared_ptr<const Object> object) const;
   shm_handle_t getHandleFromObject(const Object *object) const;
   ObjectData *getObjectDataFromName(const std::string &name) const;
   ObjectData *getObjectDataFromHandle(const shm_handle_t &handle) const;
   std::shared_ptr<const Object> getObjectFromName(const std::string &name, bool onlyComplete=true) const;
   template<typename T>
   const ShmVector<T> getArrayFromName(const std::string &name) const;

   static std::string shmIdFilename();
   static bool cleanAll(int rank);

   void markAsRemoved(const std::string &name);
   void addObject(const std::string &name, const shm_handle_t &handle);
#ifdef SHMDEBUG
#ifdef NO_SHMEM
   static std::vector<ShmDebugInfo, vistle::shm<ShmDebugInfo>::allocator> *s_shmdebug;
   static std::recursive_mutex *s_shmdebugMutex;
#else
   static boost::interprocess::vector<ShmDebugInfo, vistle::shm<ShmDebugInfo>::allocator> *s_shmdebug;
   static boost::interprocess::interprocess_recursive_mutex *s_shmdebugMutex;
#endif
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
   std::atomic<int> m_objectId, m_arrayId;
   static Shm *s_singleton;
#ifdef NO_SHMEM
   mutable std::recursive_mutex *m_shmDeletionMutex;
   mutable std::recursive_mutex *m_objectDictionaryMutex;
   std::map<std::string, void*> m_objectDictionary;
#else
   mutable boost::interprocess::interprocess_recursive_mutex *m_shmDeletionMutex;
   mutable boost::interprocess::interprocess_recursive_mutex *m_objectDictionaryMutex;
   boost::interprocess::managed_shared_memory *m_shm;
#endif
   mutable std::atomic<int> m_lockCount;
};

template<typename T>
T *shm<T>::find(const std::string &name) {
#ifdef NO_SHMEM
    Shm::the().lockDictionary();
    auto &dict = Shm::the().m_objectDictionary;
    auto it = dict.find(name);
    if (it == dict.end()) {
        Shm::the().unlockDictionary();
        return nullptr;
    }
    Shm::the().unlockDictionary();
    return static_cast<T *>(it->second);
#else
   return Shm::the().shm().find<T>(name.c_str()).first;
#endif
}

template<typename T>
bool shm<T>::destroy(const std::string &name) {
#ifdef NO_SHMEM
    Shm::the().lockDictionary();
    auto &dict = Shm::the().m_objectDictionary;
    auto it = dict.find(name);
    bool ret = true;
    if (it == dict.end()) {
        Shm::the().unlockDictionary();
        ret = false;
    } else {
        T *t = static_cast<T *>(it->second);
        dict.erase(it);
        Shm::the().unlockDictionary();
        delete t;
    }
#else
    const bool ret = Shm::the().shm().destroy<T>(name.c_str());
#endif
    Shm::the().markAsRemoved(name);
    return ret;
}

#ifdef NO_SHMEM
template<typename T>
shm<T>::Constructor::Constructor(const std::string &name): name(name) {
    Shm::the().lockDictionary();
}

template<typename T>
shm<T>::Constructor::~Constructor() {
    Shm::the().unlockDictionary();
}

template<typename T>
template<typename... Args>
T *shm<T>::Constructor::operator()(Args&&... args) {
    T *t = new T(std::forward<Args>(args)...);
    Shm::the().lockDictionary();
    auto &dict = Shm::the().m_objectDictionary;
    dict[name] = t;
    Shm::the().unlockDictionary();
    return t;
}
#endif

} // namespace vistle

#endif

#ifdef VISTLE_IMPL
#include "shm_impl.h"
#endif
