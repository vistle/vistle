#ifndef SHM_OBJ_REF_H
#define SHM_OBJ_REF_H

#include <string>

namespace vistle {

template<class T>
class shm_obj_ref {

    typedef T ObjType;
    typedef typename ObjType::Data ObjData;

 public:
    shm_obj_ref()
    : m_name()
    , m_d(nullptr)
    {
        ref();
    }

    shm_obj_ref(const std::string &name, ObjType *p)
    : m_name(name)
    , m_d(p->d())
    {
        ref();
    }

    shm_obj_ref(const shm_obj_ref &other)
    : m_name(other.m_name)
    , m_d(other.m_d)
    {
        ref();
    }

    shm_obj_ref(const shm_name_t name)
    : m_name(name)
    , m_d(shm<T>::find(name))
    {
        ref();
    }

    ~shm_obj_ref() {
        unref();
    }

    template<typename... Args>
    static shm_obj_ref create(const Args&... args) {
       shm_obj_ref result;
       result.construct(args...);
       return result;
    }

    bool find() {
        auto mem = vistle::shm<ObjData>::find(m_name);
        m_d = mem;
        ref();

        return valid();
    }

    template<typename... Args>
    void construct(const Args&... args)
    {
        unref();
        if (m_name.empty())
            m_name = Shm::the().createObjectId();
        m_d = shm<T>::construct(m_name)(args..., Shm::the().allocator());
        ref();
    }

    const shm_obj_ref &operator=(const shm_obj_ref &rhs) {
        unref();
        m_name = rhs.m_name;
        m_d = rhs.m_d;
        ref();
        return *this;
    }

    const shm_obj_ref &operator=(typename ObjType::const_ptr rhs) {
        unref();
        if (rhs) {
            m_name = rhs->getName();
            m_d = rhs->d();
        } else {
            m_name.clear();
            m_d = nullptr;
        }
        ref();
        return *this;
    }

    const shm_obj_ref &operator=(typename ObjType::ptr rhs) {
        return std::const_pointer_cast<const ObjType>(rhs);
    }

   bool valid() const {
       return m_name.empty() || m_d;
   }

   typename ObjType::const_ptr getObject() const {
       if (!valid())
           return nullptr;
       return ObjType::as(Object::create(const_cast<typename ObjType::Data *>(m_d.get())));
   }

   const typename ObjType::Data *getData() const {
       if (!valid())
           return nullptr;
       return m_d.get();
   }

   operator bool() const {
       return valid();
   }

   const shm_name_t &name() const {
       return m_name;
   }

 private:
   shm_name_t m_name;
   boost::interprocess::offset_ptr<const ObjData> m_d;

   void ref() {
       if (m_d) {
           assert(m_name == m_d->name);
           assert(m_d->refcount >= 0);
           m_d->ref();
       }
   }

   void unref() {
       if (m_d) {
          assert(m_d->refcount > 0);
          m_d->unref();
       }
       m_d = nullptr;
   }

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive &ar, const unsigned int version) {
       boost::serialization::split_member(ar, *this, version);
   }
   template<class Archive>
   void save(Archive &ar, const unsigned int version) const {
      ar & boost::serialization::make_nvp("obj_name", m_name);
      assert(valid());
      if (m_d)
          ar.saveObject(*this);
   }
   template<class Archive>
   void load(Archive &ar, const unsigned int version) {
      ar & boost::serialization::make_nvp("obj_name", m_name);

      //assert(shmname == m_name);
      std::string name = m_name;

      unref();
      m_d = nullptr;

      if (m_name.empty())
          return;

      auto obj = ar.currentObject();
      if (obj) {
          obj->unresolvedReference();
      }
      auto handler = ar.objectCompletionHandler();
      auto ref0 = ar.getObject(name, [this, name, obj, handler]() -> void {
         //std::cerr << "object completion handler: " << name << std::endl;
         auto ref2 = T::as(Shm::the().getObjectFromName(name));
         assert(ref2);
         *this = ref2;
         if (obj) {
            obj->referenceResolved(handler);
         }
      });
      auto ref1 = T::as(ref0);
      if (ref1) {
         // object already present: don't mess with count of outstanding references
         *this = ref1;
      } else {
         assert(!ref0);
         //std::cerr << "waiting for completion of object " << name << std::endl;
      }
   }
};

} // namespace vistle

#endif
