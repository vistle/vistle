#ifndef SHM_H
#define SHM_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <util/exception.h>
#include "index.h"
#include "export.h"

//#define USE_BOOST_VECTOR
#define SHMDEBUG
//#define SHMPUBLISH

#ifndef USE_BOOST_VECTOR
#include "shm_array.h"
#endif

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

struct V_COREEXPORT shm_name_t {
   std::array<char, 32> name;
   shm_name_t(const std::string &s = "INVALID");

   operator const char *() const;
   operator char *();
   operator std::string () const;
   bool operator==(const std::string &rhs) const;

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
   typedef boost::interprocess::offset_ptr<vector> ptr;
#ifdef USE_BOOST_VECTOR
   typedef boost::interprocess::vector<T, allocator> array;
#else
   typedef vistle::shm_array<T, allocator> array;
#endif
   typedef boost::interprocess::offset_ptr<array> array_ptr;
   static typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type construct(const std::string &name);
   static T *find(const std::string &name);
   static void destroy(const std::string &name);
};

class V_COREEXPORT Shm {

 public:
   static std::string instanceName(const std::string &host, unsigned short port);
   static Shm & the();
   static Shm & create(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   static Shm & attach(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   static bool isAttached();

   void detach();
   void setRemoveOnDetach();

   const std::string &name() const;

   typedef boost::interprocess::allocator<void, boost::interprocess::managed_shared_memory::segment_manager> void_allocator;
   const void_allocator &allocator() const;

   boost::interprocess::managed_shared_memory &shm();
   const boost::interprocess::managed_shared_memory &shm() const;
   std::string createArrayId(const std::string &name="");
   std::string createObjectId(const std::string &name="");

   boost::shared_ptr<const Object> getObjectFromHandle(const shm_handle_t & handle) const;
   shm_handle_t getHandleFromObject(boost::shared_ptr<const Object> object) const;
   shm_handle_t getHandleFromObject(const Object *object) const;
   boost::shared_ptr<const Object> getObjectFromName(const std::string &name, bool onlyComplete=true) const;
   void *getArrayFromName(const std::string &name) const;

   static std::string shmIdFilename();
   static bool cleanAll();

#ifdef SHMDEBUG
   static vistle::shm<ShmDebugInfo>::vector *s_shmdebug;
   static boost::interprocess::interprocess_recursive_mutex *s_shmdebugMutex;
   void markAsRemoved(const std::string &name);
   void addObject(const std::string &name, const shm_handle_t &handle);
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
void shm<T>::destroy(const std::string &name) {
      Shm::the().shm().destroy<T>(name.c_str());
#ifdef SHMDEBUG
      Shm::the().markAsRemoved(name);
#endif
}

template<typename T>
class ShmVector {
#ifdef SHMDEBUG
   friend void Shm::markAsRemoved(const std::string &name);
#endif

   public:
      class ptr {
         public:
            ptr(ShmVector *p = NULL);
            ptr(const ptr &ptr);
            ~ptr();
            ptr &operator=(const ptr &other);
            ptr &operator=(ShmVector *p);

            ShmVector &operator*() {
               return *m_p;
            }
            ShmVector *operator->() {
               return &*m_p;
            }
            const ShmVector &operator*() const {
               return *m_p;
            }
            const ShmVector *operator->() const {
               return &*m_p;
            }

            bool valid() const {
                return m_p;
            }

            template<class Archive>
            bool request(Archive &ar, const unsigned int version);

         private:
            friend class boost::serialization::access;
            template<class Archive>
               void serialize(Archive &ar, const unsigned int version);
            template<class Archive>
               void save(Archive &ar, const unsigned int version) const;
            template<class Archive>
               void load(Archive &ar, const unsigned int version);

            boost::interprocess::offset_ptr<ShmVector> m_p;
      };

      static int typeId();

      ShmVector(Index size = 0, const std::string &name="");
      int type() const { return m_type; }

      int refcount() const;
      void* operator new(size_t size);
      void operator delete(void *p, size_t size);

      T &operator[](Index i) { return (*m_x)[i]; }
      const T &operator[](Index i) const { return (*m_x)[i]; }
      T *data() { return m_x->data(); }
      const T *data() const { return m_x->data(); }

      bool empty() const { return m_x->empty(); }
      Index size() const { return m_x->size(); }
      void resize(Index s);
      void clear();

      typename shm<T>::array_ptr &operator()() { return m_x; }
      typename shm<const T>::array_ptr &operator()() const { return m_x; }

      void push_back(const T &d) { m_x->push_back(d); }

   private:
      ~ShmVector();
      void ref();
      void unref();

      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version);
      template<class Archive>
         void save(Archive &ar, const unsigned int version) const;
      template<class Archive>
         void load(Archive &ar, const unsigned int version);

      boost::interprocess::interprocess_mutex m_mutex;
      int m_type;
      int m_refcount;
      shm_name_t m_name;
      typename shm<T>::array_ptr m_x;
};

} // namespace vistle

#endif

#ifdef VISTLE_IMPL
#include "shm_impl.h"
#endif
