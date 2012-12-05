#ifndef OBJECT_H
#define OBJECT_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>

#include <boost/serialization/access.hpp>

#include "shm.h"

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

namespace bi = boost::interprocess;

typedef bi::managed_shared_memory::handle_t shm_handle_t;
typedef char shm_name_t[32];

class Shm;

#define V_NAME(name, obj) \
   boost::serialization::make_nvp(name, (obj))

class Object {
   friend class Shm;
   friend class ObjectTypeRegistry;
#ifdef SHMDEBUG
   friend void Shm::markAsRemoved(const std::string &name);
#endif

public:
   typedef boost::shared_ptr<Object> ptr;
   typedef boost::shared_ptr<const Object> const_ptr;

   enum Type {
      UNKNOWN           = -1,
      VECFLOAT          =  0,
      VECINT            =  1,
      VECCHAR           =  2,
      VEC3FLOAT         =  3,
      VEC3INT           =  4,
      VEC3CHAR          =  5,
      TRIANGLES         =  6,
      LINES             =  7,
      POLYGONS          =  8,
      UNSTRUCTUREDGRID  =  9,
      SET               = 10,
      GEOMETRY          = 11,
      TEXTURE1D         = 12,
      POINTS            = 13,
   };

   struct Info {
      uint64_t infosize;
      uint64_t itemsize;
      uint64_t offset;
      uint64_t type;
      uint64_t block;
      uint64_t timestep;
      virtual ~Info() {}; // for RTTI
   };

   virtual ~Object();

   shm_handle_t getHandle() const;

   Info *getInfo(Info *info = NULL) const;

   Type getType() const;
   std::string getName() const;

   int getBlock() const;
   int getTimestep() const;

   void setBlock(const int block);
   void setTimestep(const int timestep);

   void addAttribute(const std::string &key, const std::string &value = "");
   void setAttributeList(const std::string &key, const std::vector<std::string> &values);
   void copyAttributes(Object::const_ptr src, bool replace = true);
   bool hasAttribute(const std::string &key) const;
   std::string getAttribute(const std::string &key) const;
   std::vector<std::string> getAttributes(const std::string &key) const;

   void ref() const;
   void unref() const;
   int refcount() const;

   virtual void serialize(boost::archive::text_iarchive &ar) = 0;
   virtual void serialize(boost::archive::text_oarchive &ar) const = 0;
   virtual void serialize(boost::archive::binary_iarchive &ar) = 0;
   virtual void serialize(boost::archive::binary_oarchive &ar) const = 0;
   virtual void serialize(boost::archive::xml_iarchive &ar) = 0;
   virtual void serialize(boost::archive::xml_oarchive &ar) const = 0;

 protected:
   struct Data {
      Type type;
      shm_name_t name;
      int refcount;
      boost::interprocess::interprocess_mutex mutex;

      int block;
      int timestep;

      typedef shm<char>::string Attribute;
      typedef shm<char>::string Key;
      typedef shm<Attribute>::vector AttributeList;
      typedef std::pair<const Key, AttributeList> AttributeMapValueType;
      typedef shm<AttributeMapValueType>::allocator AttributeMapAllocator;
      typedef bi::map<Key, AttributeList, std::less<Key>, AttributeMapAllocator> AttributeMap;
      bi::offset_ptr<AttributeMap> attributes;
      void addAttribute(const std::string &key, const std::string &value = "");
      void setAttributeList(const std::string &key, const std::vector<std::string> &values);
      void copyAttributes(const Data *src, bool replace);
      bool hasAttribute(const std::string &key) const;
      std::string getAttribute(const std::string &key) const;
      std::vector<std::string> getAttributes(const std::string &key) const;

      Data(Type id, const std::string &name, int b, int t);
      ~Data();
      void *operator new(size_t size);
      void *operator new (std::size_t size, void* ptr);
      void operator delete(void *ptr);
      void ref();
      void unref();
      static Data *create(Type id, int b, int t);

      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);

      // not implemented
      Data &operator=(const Data &);
      Data();
      Data(const Data &);
   };

   Data *m_data;
 public:
   Data *d() const { return m_data; }
 protected:

   Object(Data *data);

   static void publish(const Data *d);
   static Object::ptr create(Data *);
 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         d()->serialize<Archive>(ar, version);
      }
   // not implemented
   Object();
   Object(const Object &);
   Object &operator=(const Object &);
};

class ObjectTypeRegistry {
   friend struct Object::Data;
   friend Object::ptr Object::create(Object::Data *);
   public:
   typedef Object::ptr (*CreateFunc)(Object::Data *d);
   typedef void (*DestroyFunc)(const std::string &name);

   template<class O>
   static void registerType(int id) {
      assert(typeMap().find(id) == typeMap().end());
      struct FunctionTable t = {
         O::createFromData,
         O::destroy,
      };
      typeMap()[id] = t;
   }

   private:
   struct FunctionTable {
      CreateFunc create;
      DestroyFunc destroy;
   };
   static const struct FunctionTable &getType(int id);
   typedef std::map<int, FunctionTable> TypeMap;
   //static TypeMap s_typeMap;

   static TypeMap &typeMap();
   static CreateFunc getCreator(int id);
   static DestroyFunc getDestroyer(int id);
};

//! declare a new Object type
#define V_OBJECT(Type) \
   public: \
   typedef boost::shared_ptr<Type> ptr; \
   typedef boost::shared_ptr<const Type> const_ptr; \
   typedef Type Class; \
   static boost::shared_ptr<const Type> as(boost::shared_ptr<const Object> ptr) { return boost::dynamic_pointer_cast<const Type>(ptr); } \
   static boost::shared_ptr<Type> as(boost::shared_ptr<Object> ptr) { return boost::dynamic_pointer_cast<Type>(ptr); } \
   static Object::ptr createFromData(Object::Data *data) { return Object::ptr(new Type(static_cast<Type::Data *>(data))); } \
   static void destroy(const std::string &name) { shm<Type::Data>::destroy(name); } \
   virtual void serialize(boost::archive::text_iarchive &tar); \
   virtual void serialize(boost::archive::text_oarchive &tar) const; \
   virtual void serialize(boost::archive::binary_iarchive &bar); \
   virtual void serialize(boost::archive::binary_oarchive &bar) const; \
   virtual void serialize(boost::archive::xml_iarchive &xar); \
   virtual void serialize(boost::archive::xml_oarchive &xar) const; \
   protected: \
   struct Data; \
   Data *d() const { return static_cast<Data *>(m_data); } \
   Type(Data *data) : Base(data) {} \
   private: \
   friend class boost::serialization::access; \
   template<class Archive> \
      void serialize(Archive &ar, const unsigned int version) { \
         d()->template serialize<Archive>(ar, version); \
      } \
   friend boost::shared_ptr<const Object> Shm::getObjectFromHandle(const shm_handle_t &); \
   friend shm_handle_t Shm::getHandleFromObject(boost::shared_ptr<const Object>); \
   friend boost::shared_ptr<Object> Object::create(Object::Data *); \
   friend class ObjectTypeRegistry


#define V_SERIALIZERS2(Type, prefix) \
   prefix \
   void Type::serialize(boost::archive::text_iarchive &ar) { \
      ar & V_NAME("object", *this); \
   } \
   prefix \
   void Type::serialize(boost::archive::xml_iarchive &ar) { \
      ar & V_NAME("object", *this); \
   } \
   prefix \
   void Type::serialize(boost::archive::binary_iarchive &ar) { \
      ar & V_NAME("object", *this); \
   } \
   prefix \
   void Type::serialize(boost::archive::text_oarchive &ar) const { \
      ar & V_NAME("object", *this); \
   } \
   prefix \
   void Type::serialize(boost::archive::xml_oarchive &ar) const { \
      ar & V_NAME("object", *this); \
   } \
   prefix \
   void Type::serialize(boost::archive::binary_oarchive &ar) const { \
      ar & V_NAME("object", *this); \
   }

#define V_SERIALIZERS(Type) \
   V_SERIALIZERS2(Type,)

//! register a new Object type (complex form, specify suffix for symbol names)
#define V_OBJECT_TYPE3(Type, suffix, id) \
   namespace { \
      class RegisterObjectType_##suffix { \
         public: \
                 RegisterObjectType_##suffix() { \
                    ObjectTypeRegistry::registerType<Type >(id); \
                 } \
      }; \
\
      static RegisterObjectType_##suffix registerObjectType_##suffix; \
   }

//! register a new Object type (simple form for non-templates, symbol suffix determined automatically)
#define V_OBJECT_TYPE(Type, id) \
   V_SERIALIZERS(Type) \
   V_OBJECT_TYPE3(Type, Type, id)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "object_impl.h"
#endif
#endif
