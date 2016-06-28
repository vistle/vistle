#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include <util/sysdep.h>
#include <boost/shared_ptr.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <boost/mpl/size.hpp>

#include <util/enum.h>

#include "shm.h"
#include "objectmeta.h"
#include "scalars.h"
#include "dimensions.h"
#include "export.h"
#include "pointeroarchive.h"

namespace vistle {

namespace interprocess = boost::interprocess;

typedef interprocess::managed_shared_memory::handle_t shm_handle_t;

class oarchive;
class iarchive;

class Shm;
template<class T>
class shm_obj_ref;

struct ObjectData;

class V_COREEXPORT Object {
   friend class Shm;
   friend class ObjectTypeRegistry;
   friend struct ObjectData;
   template<class ObjType>
   friend class shm_obj_ref;
#ifdef SHMDEBUG
   friend void Shm::markAsRemoved(const std::string &name);
#endif

public:
   typedef boost::shared_ptr<Object> ptr;
   typedef boost::shared_ptr<const Object> const_ptr;
   
   enum InitializedFlags {
      Initialized
   };

   enum Type {

      UNKNOWN           = -1,

      PLACEHOLDER       = 11,

      TEXTURE1D         = 16,

      POINTS            = 18,
      SPHERES           = 19,
      LINES             = 20,
      TUBES             = 21,
      TRIANGLES         = 22,
      POLYGONS          = 23,
      UNSTRUCTUREDGRID  = 24,

      VERTEXOWNERLIST   = 95,
      CELLTREE1         = 96,
      CELLTREE2         = 97,
      CELLTREE3         = 98,
      NORMALS           = 99,

      VEC               = 100, // base type id for all Vec types
   };

   static inline const char *toString(Type v) {

#define V_OBJECT_CASE(sym) case sym: return #sym;
      switch(v) {
         V_OBJECT_CASE(UNKNOWN)
         V_OBJECT_CASE(PLACEHOLDER)

         V_OBJECT_CASE(TEXTURE1D)

         V_OBJECT_CASE(POINTS)
         V_OBJECT_CASE(SPHERES)
         V_OBJECT_CASE(LINES)
         V_OBJECT_CASE(TUBES)
         V_OBJECT_CASE(TRIANGLES)
         V_OBJECT_CASE(POLYGONS)
         V_OBJECT_CASE(UNSTRUCTUREDGRID)
         V_OBJECT_CASE(VERTEXOWNERLIST)
         V_OBJECT_CASE(CELLTREE1)
         V_OBJECT_CASE(CELLTREE2)
         V_OBJECT_CASE(CELLTREE3)
         V_OBJECT_CASE(NORMALS)

         default:
            break;
      }
#undef V_OBJECT_CASE

      static char buf[80];
      snprintf(buf, sizeof(buf), "invalid Object::Type (%d)", v);
      if (v >= VEC) {
         int dim = (v-Object::VEC) % (MaxDimension+1);
         int scalidx = (v-Object::VEC) / (MaxDimension+1);
#ifndef NDEBUG
         const int NumScalars = boost::mpl::size<Scalars>::value;
         assert(scalidx < NumScalars);
#endif
         const char *scalstr = "(invalid)";
         switch (scalidx) {
            case 0: scalstr = "unsigned char"; break;
            case 1: scalstr = "int"; break;
            case 2: scalstr = "unsigned int"; break;
            case 3: scalstr = "size_t"; break;
            case 4: scalstr = "float"; break;
            case 5: scalstr = "double"; break;
         }
         snprintf(buf, sizeof(buf), "VEC<%s, %d>", scalstr, dim);
      }

      return buf;
   }

   virtual ~Object();

   bool isComplete() const; //! check whether all references have been resolved

   static const char *typeName() { return "Object"; }
   Object::ptr clone() const;
   virtual Object::ptr cloneInternal() const = 0;

   Object::ptr createEmpty() const;
   virtual Object::ptr createEmptyInternal() const = 0;

   virtual void refresh() const; //!< refresh cached pointers from shm
   virtual bool check() const;

   virtual bool isEmpty() const;

   template<class ObjectType>
   static boost::shared_ptr<const Object> as(boost::shared_ptr<const ObjectType> ptr) { return boost::static_pointer_cast<const Object>(ptr); }
   template<class ObjectType>
   static boost::shared_ptr<Object> as(boost::shared_ptr<ObjectType> ptr) { return boost::static_pointer_cast<Object>(ptr); }

   shm_handle_t getHandle() const;

   Type getType() const;
   std::string getName() const;

   int getBlock() const;
   int getNumBlocks() const;
   double getRealTime() const;
   int getTimestep() const;
   int getNumTimesteps() const;
   int getExecutionCounter() const;
   int getCreator() const;

   void setBlock(const int block);
   void setNumBlocks(const int num);
   void setRealTime(double time);
   void setTimestep(const int timestep);
   void setNumTimesteps(const int num);
   void setExecutionCounter(const int count);
   void setCreator(const int id);

   const Meta &meta() const;
   void setMeta(const Meta &meta);

   void addAttribute(const std::string &key, const std::string &value = "");
   void setAttributeList(const std::string &key, const std::vector<std::string> &values);
   virtual void copyAttributes(Object::const_ptr src, bool replace = true);
   bool hasAttribute(const std::string &key) const;
   std::string getAttribute(const std::string &key) const;
   std::vector<std::string> getAttributes(const std::string &key) const;
   std::vector<std::string> getAttributeList() const;

   // attachments, e.g. Celltrees
   bool addAttachment(const std::string &key, Object::const_ptr att) const;
   bool hasAttachment(const std::string &key) const;
   void copyAttachments(Object::const_ptr src, bool replace = true);
   Object::const_ptr getAttachment(const std::string &key) const;
   bool removeAttachment(const std::string &key) const;

   void ref() const;
   void unref() const;
   int refcount() const;

   template<class Archive>
   static Object *load(Archive &ar);

   template<class Archive>
   void save(Archive &ar) const;

   virtual void save(PointerOArchive &ar) const = 0;

 public:
   typedef ObjectData Data;

 protected:
   Data *m_data;
 public:
   Data *d() const { return m_data; }
 protected:

   Object(Data *data);
   Object();

   static void publish(const Data *d);
   static Object::ptr create(Data *);
 private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive &ar, const unsigned int version);

   // not implemented
   Object(const Object &) = delete;
   Object &operator=(const Object &) = delete;
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Object)

struct ObjectData {
    Object::Type type;
    shm_name_t name;
    mutable boost::interprocess::interprocess_mutex ref_mutex; //< protects refcount
    mutable int refcount;

    int unresolvedReferences; //!< no. of not-yet-available arrays and referenced objects

    Meta meta;

    typedef shm<char>::string Attribute;
    typedef shm<char>::string Key;
    typedef shm<Attribute>::vector AttributeList;
    typedef std::pair<const Key, AttributeList> AttributeMapValueType;
    typedef shm<AttributeMapValueType>::allocator AttributeMapAllocator;
    typedef interprocess::map<Key, AttributeList, std::less<Key>, AttributeMapAllocator> AttributeMap;
    interprocess::offset_ptr<AttributeMap> attributes;
    void addAttribute(const std::string &key, const std::string &value = "");
    void setAttributeList(const std::string &key, const std::vector<std::string> &values);
    void copyAttributes(const ObjectData *src, bool replace);
    bool hasAttribute(const std::string &key) const;
    std::string getAttribute(const std::string &key) const;
    std::vector<std::string> getAttributes(const std::string &key) const;
    std::vector<std::string> getAttributeList() const;

    mutable boost::interprocess::interprocess_recursive_mutex attachment_mutex; //< protects attachments
    typedef interprocess::offset_ptr<ObjectData> Attachment;
    typedef std::pair<const Key, Attachment> AttachmentMapValueType;
    typedef shm<AttachmentMapValueType>::allocator AttachmentMapAllocator;
    typedef interprocess::map<Key, Attachment, std::less<Key>, AttachmentMapAllocator> AttachmentMap;
    interprocess::offset_ptr<AttachmentMap> attachments;
    bool addAttachment(const std::string &key, Object::const_ptr att);
    void copyAttachments(const ObjectData *src, bool replace);
    bool hasAttachment(const std::string &key) const;
    Object::const_ptr getAttachment(const std::string &key) const;
    bool removeAttachment(const std::string &key);

    V_COREEXPORT ObjectData(Object::Type id = Object::UNKNOWN, const std::string &name = "", const Meta &m=Meta());
    V_COREEXPORT ObjectData(const ObjectData &other, const std::string &name, Object::Type id=Object::UNKNOWN); //! shallow copy, except for attributes
    V_COREEXPORT ObjectData(const std::string &name, const Meta &meta);
    V_COREEXPORT ~ObjectData();
    V_COREEXPORT void *operator new(size_t size);
    V_COREEXPORT void *operator new (std::size_t size, void* ptr);
    V_COREEXPORT void operator delete(void *ptr);
    V_COREEXPORT void operator delete(void *ptr, void* voidptr2);
    void ref() const;
    void unref() const;
    static ObjectData *create(Object::Type id, const std::string &name, const Meta &m);
    bool isComplete() const; //! check whether all references have been resolved
    template<typename ShmVectorPtr>
    void arrayValid(const ShmVectorPtr &p);
    template<class ObjType>
    void objectValid(const shm_obj_ref<ObjType> &o);
    void referenceResolved(const std::function<void()> &completeCallback);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version);

    // not implemented
    ObjectData(const ObjectData &) = delete;
    ObjectData &operator=(const ObjectData &) = delete;
};


class ObjectTypeRegistry {
   friend struct ObjectData;
   friend Object::ptr Object::create(Object::Data *);
   public:
   typedef Object::ptr (*CreateFunc)(Object::Data *d);
   typedef void (*DestroyFunc)(const std::string &name);
   typedef void (*RegisterIArchiveFunc)(iarchive &ar);
   typedef void (*RegisterOArchiveFunc)(oarchive &ar);

   template<class O>
   static void registerType(int id) {
#ifndef VISTLE_STATIC
      assert(typeMap().find(id) == typeMap().end());
#endif
      struct FunctionTable t = {
         O::createFromData,
         O::destroy,
         O::registerIArchive,
         O::registerOArchive,
      };
      typeMap()[id] = t;
   }

   template<class Archive>
   static void registerArchiveType(Archive &ar);

   private:
   struct FunctionTable {
      CreateFunc create;
      DestroyFunc destroy;
      RegisterIArchiveFunc registerIArchive;
      RegisterOArchiveFunc registerOArchive;
   };
   static const struct FunctionTable &getType(int id);
   typedef std::map<int, FunctionTable> TypeMap;

   static TypeMap &typeMap();
   static CreateFunc getCreator(int id);
   static DestroyFunc getDestroyer(int id);
};

//! use in checkImpl
#define V_CHECK(true_expr) \
   if (!(true_expr)) { \
      std::cerr << __FILE__ << ":" << __LINE__ << ": " \
      << "CONSISTENCY CHECK FAILURE on " << this->getName() << " " << this->meta() << ": " \
          << #true_expr << std::endl; \
      std::cerr << "   PID: " << getpid() << std::endl; \
      std::stringstream str; \
      str << __FILE__ << ":" << __LINE__ << ": consistency check failure: " << #true_expr; \
      throw(vistle::except::consistency_error(str.str())); \
      sleep(30); \
      return false; \
   }

//! declare a new Object type
#define V_OBJECT(ObjType) \
   public: \
   typedef boost::shared_ptr<ObjType> ptr; \
   typedef boost::shared_ptr<const ObjType> const_ptr; \
   typedef ObjType Class; \
   static Object::Type type(); \
   static const char *typeName() { return #ObjType; } \
   static boost::shared_ptr<const ObjType> as(boost::shared_ptr<const Object> ptr) { return boost::dynamic_pointer_cast<const ObjType>(ptr); } \
   static boost::shared_ptr<ObjType> as(boost::shared_ptr<Object> ptr) { return boost::dynamic_pointer_cast<ObjType>(ptr); } \
   static Object::ptr createFromData(Object::Data *data) { return Object::ptr(new ObjType(static_cast<ObjType::Data *>(data))); } \
   Object::ptr cloneInternal() const override { \
      const std::string n(Shm::the().createObjectId()); \
            Data *data = shm<Data>::construct(n)(*d(), n); \
            publish(data); \
            return createFromData(data); \
   } \
   ObjType::ptr clone() const { \
      return ObjType::as(cloneInternal()); \
   } \
   Object::ptr createEmptyInternal() const override { \
      return Object::ptr(new ObjType(Object::Initialized)); \
   } \
   ObjType::ptr createEmpty() const { \
      return ObjType::as(createEmptyInternal()); \
   } \
   template<class OtherType> \
   static ObjType::ptr clone(typename OtherType::ptr other) { \
      const std::string n(Shm::the().createObjectId()); \
      typename ObjType::Data *data = shm<typename ObjType::Data>::construct(n)(*other->d(), n); \
      assert(data->type == ObjType::type()); \
      ObjType::ptr ret = ObjType::as(createFromData(data)); \
      assert(ret); \
      publish(data); \
      return ret; \
   } \
   template<class OtherType> \
   static ObjType::ptr clone(typename OtherType::const_ptr other) { \
      return ObjType::clone<OtherType>(boost::const_pointer_cast<OtherType>(other)); \
   } \
   static void destroy(const std::string &name) { shm<ObjType::Data>::destroy(name); } \
   static void registerIArchive(iarchive &ar); \
   static void registerOArchive(oarchive &ar); \
   void save(PointerOArchive &ar) const override { const_cast<ObjType *>(this)->serialize(ar, 0); } \
   void refresh() const override { Base::refresh(); refreshImpl(); } \
   void refreshImpl() const; \
   ObjType(Object::InitializedFlags) : Base(ObjType::Data::create()) { refreshImpl(); }  \
   virtual bool isEmpty() const override; \
   bool check() const override { refresh(); if (isEmpty()) {}; if (!Base::check()) return false; return checkImpl(); } \
   struct Data; \
   const Data *d() const { return static_cast<Data *>(Object::m_data); } \
   Data *d() { return static_cast<Data *>(Object::m_data); } \
   protected: \
   bool checkImpl() const; \
   ObjType(Data *data); \
   ObjType(); \
   private: \
   friend class boost::serialization::access; \
   template<class Archive> \
      void serialize(Archive &ar, const unsigned int version) { \
         boost::serialization::split_member(ar, *this, version); \
      } \
   template<class Archive> \
      void load(Archive &ar, const unsigned int version) { \
         std::string name; \
         ar & V_NAME("name", name); \
         int type = Object::UNKNOWN; \
         ar & V_NAME("type", type); \
         std::cerr << "Object::load: creating " << name << std::endl; \
         Object::m_data = Data::createNamed(ObjType::type(), name); \
         if (!ar.currentObject()) \
             ar.setCurrentObject(Object::m_data); \
         d()->template serialize<Archive>(ar, version); \
         assert(type == Object::getType()); \
      } \
   template<class Archive> \
      void save(Archive &ar, const unsigned int version) const { \
         int type = Object::getType(); \
         std::string name = d()->name; \
         ar & V_NAME("name", name); \
         ar & V_NAME("type", type); \
         const_cast<Data *>(d())->template serialize<Archive>(ar, version); \
      } \
   friend boost::shared_ptr<const Object> Shm::getObjectFromHandle(const shm_handle_t &) const; \
   friend shm_handle_t Shm::getHandleFromObject(boost::shared_ptr<const Object>) const; \
   friend boost::shared_ptr<Object> Object::create(Object::Data *); \
   friend class ObjectTypeRegistry

#define V_DATA_BEGIN(ObjType) \
   public: \
   struct Data: public Base::Data { \
      static Data *createNamed(Object::Type id, const std::string &name); \
      Data(Object::Type id, const std::string &name, const Meta &meta); \
      Data(const Data &other, const std::string &name) \

#define V_DATA_END(ObjType) \
      private: \
      friend class ObjType; \
      friend class boost::serialization::access; \
      template<class Archive> \
      void serialize(Archive &ar, const unsigned int version); \
   }

#define V_SERIALIZERS4(Type1, Type2, prefix1, prefix2) \
   prefix1, prefix2 \
   void Type1, Type2::registerIArchive(iarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   } \
   prefix1, prefix2 \
   void Type1, Type2::registerOArchive(oarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   }

#define V_SERIALIZERS2(ObjType, prefix) \
   prefix \
   void ObjType::registerIArchive(iarchive &ar) { \
      ar.register_type<ObjType >(); \
   } \
   prefix \
   void ObjType::registerOArchive(oarchive &ar) { \
      ar.register_type<ObjType >(); \
   }

#define V_SERIALIZERS(ObjType) V_SERIALIZERS2(ObjType,)

#ifdef VISTLE_STATIC
#define REGISTER_TYPE(ObjType, id) \
{ \
   ObjectTypeRegistry::registerType<ObjType >(id); \
   boost::serialization::void_cast_register<ObjType, ObjType::Base>( \
         static_cast<ObjType *>(NULL), static_cast<ObjType::Base *>(NULL) \
  ); \
}

#define V_INIT_STATIC
#else
#define V_INIT_STATIC static
#endif

#define V_OBJECT_TYPE3INT(ObjType, suffix, id) \
      class RegisterObjectType_##suffix { \
         public: \
                 RegisterObjectType_##suffix() { \
                    ObjectTypeRegistry::registerType<ObjType >(id); \
                    boost::serialization::void_cast_register<ObjType, ObjType::Base>( \
                          static_cast<ObjType *>(NULL), static_cast<ObjType::Base *>(NULL) \
                          ); \
                 } \
      }; \
      V_INIT_STATIC RegisterObjectType_##suffix registerObjectType_##suffix; \

//! register a new Object type (complex form, specify suffix for symbol names)
#define V_OBJECT_TYPE3(ObjType, suffix, id) \
      V_OBJECT_TYPE3INT(ObjType, suffix, id)

//! register a new Object type (complex form, specify suffix for symbol names)
#define V_OBJECT_TYPE4(Type1, Type2, suffix, id) \
      namespace suffix { \
      typedef Type1,Type2 ObjType; \
      V_OBJECT_TYPE3INT(ObjType, suffix, id) \
   }

#define V_OBJECT_TYPE_T(ObjType, id) \
   V_SERIALIZERS2(ObjType, template<>) \
   V_OBJECT_TYPE3(ObjType, ObjType, id)

//! register a new Object type (simple form for non-templates, symbol suffix determined automatically)
#define V_OBJECT_TYPE(ObjType, id) \
   V_SERIALIZERS(ObjType) \
   V_OBJECT_TYPE3(ObjType, ObjType, id) \
   Object::Type ObjType::type() { \
      return id; \
   }

#define V_OBJECT_CREATE_NAMED(ObjType) \
   ObjType::Data::Data(Object::Type id, const std::string &name, const Meta &meta) \
   : ObjType::Base::Data(id, name, meta) {} \
   ObjType::Data *ObjType::Data::createNamed(Object::Type id, const std::string &name) { \
      Data *t = shm<Data>::construct(name)(id, name, Meta()); \
      publish(t); \
      return t; \
   }

#define V_OBJECT_CTOR(ObjType) \
   V_OBJECT_CREATE_NAMED(ObjType) \
   ObjType::ObjType(ObjType::Data *data): ObjType::Base(data) { refreshImpl(); } \
   ObjType::ObjType(): ObjType::Base() { refreshImpl(); }

void V_COREEXPORT registerTypes();

V_ENUM_OUTPUT_OP(Type, Object)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "object_impl.h"
#endif

#include "shm_obj_ref.h"
#endif
