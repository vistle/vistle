#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include <util/sysdep.h>
#include <boost/shared_ptr.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>

#include <boost/serialization/access.hpp>

#include <boost/mpl/size.hpp>

#include <util/enum.h>

#include "shm.h"
#include "objectmeta.h"
#include "scalars.h"
#include "dimensions.h"
#include "export.h"

namespace boost {
namespace archive {

class text_iarchive;
class text_oarchive;
class xml_oarchive;
class xml_iarchive;
class binary_oarchive;
class binary_iarchive;


} // namespace archive
} // namespace boost


namespace vistle {

namespace interprocess = boost::interprocess;

typedef interprocess::managed_shared_memory::handle_t shm_handle_t;

class Shm;

class V_COREEXPORT Object {
   friend class Shm;
   friend class ObjectTypeRegistry;
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

      TEXTURE1D         = 17,
      GEOMETRY          = 18,

      POINTS            = 19,
      LINES             = 20,
      TRIANGLES         = 21,
      POLYGONS          = 22,
      UNSTRUCTUREDGRID  = 23,

      NORMALS           = 99,

      VEC               = 100, // base type id for all Vec types
   };

   static inline const char *toString(Type v) {

#define V_OBJECT_CASE(sym) case sym: return #sym;
      switch(v) {
         V_OBJECT_CASE(UNKNOWN)
         V_OBJECT_CASE(PLACEHOLDER)
         V_OBJECT_CASE(TEXTURE1D)
         V_OBJECT_CASE(GEOMETRY)
         V_OBJECT_CASE(POINTS)
         V_OBJECT_CASE(LINES)
         V_OBJECT_CASE(TRIANGLES)
         V_OBJECT_CASE(POLYGONS)
         V_OBJECT_CASE(UNSTRUCTUREDGRID)
         V_OBJECT_CASE(CELLTREE)
         V_OBJECT_CASE(NORMALS)
         default:
            break;
      }
#undef V_OBJECT_CASE

      static char buf[80];
      snprintf(buf, sizeof(buf), "invalid Object::Type (%d)", v);
      if (v >= VEC) {
         const int NumScalars = boost::mpl::size<Scalars>::value;
         int dim = (v-Object::VEC) % (MaxDimension+1);
         int scalidx = (v-Object::VEC) / (MaxDimension+1);
         assert(scalidx < NumScalars);
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

   virtual Object::ptr clone() const = 0;

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
   void copyAttributes(Object::const_ptr src, bool replace = true);
   bool hasAttribute(const std::string &key) const;
   std::string getAttribute(const std::string &key) const;
   std::vector<std::string> getAttributes(const std::string &key) const;

   void ref() const;
   void unref() const;
   int refcount() const;

   template<class Archive>
   static Object::ptr load(Archive &ar);

   template<class Archive>
   void save(Archive &ar) const;

 public:
    struct Data {
      Type type;
      shm_name_t name;
      int refcount;
      boost::interprocess::interprocess_mutex mutex;

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
      void copyAttributes(const Data *src, bool replace);
      bool hasAttribute(const std::string &key) const;
      std::string getAttribute(const std::string &key) const;
      std::vector<std::string> getAttributes(const std::string &key) const;

      V_COREEXPORT Data(Type id = UNKNOWN, const std::string &name = "", const Meta &m=Meta());
      V_COREEXPORT Data(const Data &other, const std::string &name, Type id=UNKNOWN); //! shallow copy, except for attributes
      V_COREEXPORT ~Data();
      V_COREEXPORT void *operator new(size_t size);
      V_COREEXPORT void *operator new (std::size_t size, void* ptr);
      V_COREEXPORT void operator delete(void *ptr);
      void ref();
      void unref();
      static Data *create(Type id, const Meta &m);

      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);

      // not implemented
      Data(const Data &);
      Data &operator=(const Data &);
   };

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
      void serialize(Archive &ar, const unsigned int version) {
         d()->serialize<Archive>(ar, version);
      }
   // not implemented
   Object(const Object &);
   Object &operator=(const Object &);
};

class ObjectTypeRegistry {
   friend struct Object::Data;
   friend Object::ptr Object::create(Object::Data *);
   public:
   typedef Object::ptr (*CreateFunc)(Object::Data *d);
   typedef void (*DestroyFunc)(const std::string &name);
   typedef void (*RegisterTextIArchiveFunc)(boost::archive::text_iarchive &ar);
   typedef void (*RegisterTextOArchiveFunc)(boost::archive::text_oarchive &ar);
   typedef void (*RegisterXmlIArchiveFunc)(boost::archive::xml_iarchive &ar);
   typedef void (*RegisterXmlOArchiveFunc)(boost::archive::xml_oarchive &ar);
   typedef void (*RegisterBinaryIArchiveFunc)(boost::archive::binary_iarchive &ar);
   typedef void (*RegisterBinaryOArchiveFunc)(boost::archive::binary_oarchive &ar);

   template<class O>
   static void registerType(int id) {
#ifndef VISTLE_STATIC
      assert(typeMap().find(id) == typeMap().end());
#endif
      struct FunctionTable t = {
         O::createFromData,
         O::destroy,
         O::registerBinaryIArchive,
         O::registerBinaryOArchive,
         O::registerTextIArchive,
         O::registerTextOArchive,
         O::registerXmlIArchive,
         O::registerXmlOArchive,
      };
      typeMap()[id] = t;
   }

   template<class Archive>
   static void registerArchiveType(Archive &ar);

   private:
   struct FunctionTable {
      CreateFunc create;
      DestroyFunc destroy;
      RegisterBinaryIArchiveFunc registerBinaryIArchive;
      RegisterBinaryOArchiveFunc registerBinaryOArchive;
      RegisterTextIArchiveFunc registerTextIArchive;
      RegisterTextOArchiveFunc registerTextOArchive;
      RegisterXmlIArchiveFunc registerXmlIArchive;
      RegisterXmlOArchiveFunc registerXmlOArchive;
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
      << "CONSISTENCY CHECK FAILURE: " \
          << #true_expr << std::endl; \
      std::cerr << "   PID: " << getpid() << std::endl; \
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
   static boost::shared_ptr<const ObjType> as(boost::shared_ptr<const Object> ptr) { return boost::dynamic_pointer_cast<const ObjType>(ptr); } \
   static boost::shared_ptr<ObjType> as(boost::shared_ptr<Object> ptr) { return boost::dynamic_pointer_cast<ObjType>(ptr); } \
   static Object::ptr createFromData(Object::Data *data) { return Object::ptr(new ObjType(static_cast<ObjType::Data *>(data))); } \
   Object::ptr clone() const { \
      const std::string n(Shm::the().createObjectID()); \
            Data *data = shm<Data>::construct(n)(*d(), n); \
            publish(data); \
            return createFromData(data); \
   } \
   template<class OtherType> \
   static ObjType::ptr clone(typename OtherType::ptr other) { \
      const std::string n(Shm::the().createObjectID()); \
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
   static void registerTextIArchive(boost::archive::text_iarchive &ar); \
   static void registerTextOArchive(boost::archive::text_oarchive &ar); \
   static void registerXmlIArchive(boost::archive::xml_iarchive &ar); \
   static void registerXmlOArchive(boost::archive::xml_oarchive &ar); \
   static void registerBinaryIArchive(boost::archive::binary_iarchive &ar); \
   static void registerBinaryOArchive(boost::archive::binary_oarchive &ar); \
   ObjType(Object::InitializedFlags) : Base(ObjType::Data::create()) {} \
   virtual bool isEmpty() const; \
   bool check() const { if (isEmpty()) {}; if (!Base::check()) return false; return checkImpl(); } \
   struct Data; \
   Data *d() const { return static_cast<Data *>(Object::m_data); } \
   protected: \
   bool checkImpl() const; \
   ObjType(Data *data) : Base(data) {} \
   ObjType() : Base() {} \
   private: \
   friend class boost::serialization::access; \
   template<class Archive> \
      void serialize(Archive &ar, const unsigned int version) { \
         boost::serialization::split_member(ar, *this, version); \
      } \
   template<class Archive> \
      void load(Archive &ar, const unsigned int version) { \
         int type = Object::UNKNOWN; \
         ar & V_NAME("type", type); \
         Object::m_data = Data::create(); \
         Object::ref(); \
         d()->template serialize<Archive>(ar, version); \
         assert(type == Object::getType()); \
      } \
   template<class Archive> \
      void save(Archive &ar, const unsigned int version) const { \
         int type = Object::getType(); \
         ar & V_NAME("type", type); \
         d()->template serialize<Archive>(ar, version); \
      } \
   friend boost::shared_ptr<const Object> Shm::getObjectFromHandle(const shm_handle_t &) const; \
   friend shm_handle_t Shm::getHandleFromObject(boost::shared_ptr<const Object>) const; \
   friend boost::shared_ptr<Object> Object::create(Object::Data *); \
   friend class ObjectTypeRegistry

#define V_DATA_BEGIN(ObjType) \
   public: \
   struct Data: public Base::Data { \
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
   void Type1, Type2::registerTextIArchive(boost::archive::text_iarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   } \
   prefix1, prefix2 \
   void Type1, Type2::registerTextOArchive(boost::archive::text_oarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   } \
   prefix1, prefix2 \
   void Type1, Type2::registerXmlIArchive(boost::archive::xml_iarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   } \
   prefix1, prefix2 \
   void Type1, Type2::registerXmlOArchive(boost::archive::xml_oarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   } \
   prefix1, prefix2 \
   void Type1, Type2::registerBinaryIArchive(boost::archive::binary_iarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   } \
   prefix1, prefix2 \
   void Type1, Type2::registerBinaryOArchive(boost::archive::binary_oarchive &ar) { \
      ar.register_type<Type1, Type2 >(); \
   }

#define V_SERIALIZERS(ObjType) \
   void ObjType::registerTextIArchive(boost::archive::text_iarchive &ar) { \
      ar.register_type<ObjType >(); \
   } \
   void ObjType::registerTextOArchive(boost::archive::text_oarchive &ar) { \
      ar.register_type<ObjType >(); \
   } \
   void ObjType::registerXmlIArchive(boost::archive::xml_iarchive &ar) { \
      ar.register_type<ObjType >(); \
   } \
   void ObjType::registerXmlOArchive(boost::archive::xml_oarchive &ar) { \
      ar.register_type<ObjType >(); \
   } \
   void ObjType::registerBinaryIArchive(boost::archive::binary_iarchive &ar) { \
      ar.register_type<ObjType >(); \
   } \
   void ObjType::registerBinaryOArchive(boost::archive::binary_oarchive &ar) { \
      ar.register_type<ObjType >(); \
   }

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

//! register a new Object type (simple form for non-templates, symbol suffix determined automatically)
#define V_OBJECT_TYPE(ObjType, id) \
   V_SERIALIZERS(ObjType) \
   V_OBJECT_TYPE3(ObjType, ObjType, id) \
   Object::Type ObjType::type() { \
      return id; \
   }

void V_COREEXPORT registerTypes();

V_ENUM_OUTPUT_OP(Type, Object)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "object_impl.h"
#endif
#endif
