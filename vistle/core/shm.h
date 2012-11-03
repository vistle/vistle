#ifndef SHM_H
#define SHM_H

#include <boost/scoped_ptr.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/shared_ptr.hpp>

#define SHMDEBUG

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;
typedef char shm_name_t[32];

namespace message {
   class MessageQueue;
}

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

template<typename T>
class ShmVector;
#endif

template<typename T>
struct shm {
   typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> allocator;
   typedef boost::interprocess::vector<T, allocator> vector;
   typedef boost::interprocess::offset_ptr<vector> ptr;
   static allocator alloc_inst();
   //static ptr construct_vector(size_t s) { return Shm::instance().getShm().construct<vector>(Shm::instance().createObjectID().c_str())(s, T(), alloc_inst()); }
   static typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type construct(const std::string &name);
   static void destroy(const std::string &name);
};

class Shm {

 public:
   static Shm & instance();
   static Shm & create(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   static Shm & attach(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   ~Shm();

   const std::string &getName() const;

   boost::interprocess::managed_shared_memory & getShm();
   std::string createObjectID();

   void publish(const shm_handle_t & handle);
   boost::shared_ptr<const Object> getObjectFromHandle(const shm_handle_t & handle);
   shm_handle_t getHandleFromObject(boost::shared_ptr<const Object> object);
   shm_handle_t getHandleFromObject(const Object *object);

   static std::string shmIdFilename();
   static bool cleanAll();

#ifdef SHMDEBUG
   static shm<ShmDebugInfo>::vector *s_shmdebug;
   void markAsRemoved(const std::string &name);
#endif

 private:
   Shm(const std::string &name, const int moduleID, const int rank, const size_t size,
       message::MessageQueue *messageQueue, bool create);

   std::string m_name;
   bool m_created;
   const int m_moduleID;
   const int m_rank;
   int m_objectID;
   static Shm *s_singleton;
   boost::interprocess::managed_shared_memory *m_shm;
   message::MessageQueue *m_messageQueue;
};

template<typename T>
typename shm<T>::allocator shm<T>::alloc_inst() {
   return allocator(Shm::instance().getShm().get_segment_manager());
}

//static ptr construct_vector(size_t s) { return Shm::instance().getShm().construct<vector>(Shm::instance().createObjectID().c_str())(s, T(), alloc_inst()); }

template<typename T>
typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type shm<T>::construct(const std::string &name) {
   return Shm::instance().getShm().construct<T>(name.c_str());
}

template<typename T>
void shm<T>::destroy(const std::string &name) {
      Shm::instance().getShm().destroy<T>(name.c_str());
#ifdef SHMDEBUG
      Shm::instance().markAsRemoved(name);
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
            ptr(ShmVector *p) : m_p(p) {
               m_p->ref();
            }
            ptr(const ptr &ptr) : m_p(ptr.m_p) {
               m_p->ref();
            }
            ~ptr() {
               m_p->unref();
            }
            ShmVector &operator*() {
               return *m_p;
            }
            ShmVector *operator->() {
               return &*m_p;
            }
            ptr &operator=(ptr &other) {
               m_p->unref();
               m_p = other.m_p;
               m_p->ref();
            }

         private:
            boost::interprocess::offset_ptr<ShmVector> m_p;
      };

      ShmVector(size_t size = 0)
         : m_refcount(0)
      {
         std::string n(Shm::instance().createObjectID());
         size_t nsize = n.size();
         if (nsize >= sizeof(m_name)) {
            nsize = sizeof(m_name)-1;
         }
         n.copy(m_name, nsize);
         assert(n.size() < sizeof(m_name));
         m_x = Shm::instance().getShm().construct<typename shm<T>::vector>(m_name)(size, T(), shm<T>::alloc_inst());
#ifdef SHMDEBUG
         shm_handle_t handle = Shm::instance().getShm().get_handle_from_address(this);
         Shm::instance().s_shmdebug->push_back(ShmDebugInfo('V', m_name, handle));
#endif
      }
      int refcount() const {
         return m_refcount;
      }
      void* operator new(size_t size) {
         return Shm::instance().getShm().allocate(size);
      }
      void operator delete(void *p) {
         return Shm::instance().getShm().deallocate(p);
      }

      T &operator[](size_t i) { return (*m_x)[i]; }
      const T &operator[](size_t i) const { return (*m_x)[i]; }

      size_t size() const { return m_x->size(); }
      void resize(size_t s) { m_x->resize(s); }

      typename shm<T>::ptr &operator()() { return m_x; }
      typename shm<const T>::ptr &operator()() const { return m_x; }

      void push_back(const T &d) { m_x->push_back(d); }

   private:
      ~ShmVector() {
         shm<T>::destroy(m_name);
      }
      void ref() {
         boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_mutex);
         ++m_refcount;
      }
      void unref() {
         m_mutex.lock();
         --m_refcount;
         assert(m_refcount >= 0);
         if (m_refcount == 0) {
            m_mutex.unlock();
            delete this;
            return;
         }
         m_mutex.unlock();
      }

      boost::interprocess::interprocess_mutex m_mutex;
      int m_refcount;
      shm_name_t m_name;
      typename shm<T>::ptr m_x;
};

} // namespace vistle
#endif
