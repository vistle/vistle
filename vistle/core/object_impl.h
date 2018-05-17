#ifndef OBJECT_IMPL_H
#define OBJECT_IMPL_H

#include "archives.h"
#include "serialize.h"
#include <boost/archive/basic_archive.hpp>


namespace boost {
namespace serialization {

template<>
V_COREEXPORT void access::destroy(const vistle::shm<char>::string *t);

template<>
V_COREEXPORT void access::construct(vistle::shm<char>::string *t);

template<>
V_COREEXPORT void access::destroy(const vistle::Object::Data::AttributeList *t);

template<>
V_COREEXPORT void access::construct(vistle::Object::Data::AttributeList *t);

template<>
V_COREEXPORT void access::destroy(const vistle::Object::Data::AttributeMapValueType *t);

template<>
V_COREEXPORT void access::construct(vistle::Object::Data::AttributeMapValueType *t);

} // namespace serialization
} // namespace boost


namespace vistle {

template<class Archive>
void Object::serialize(Archive &ar, const unsigned int version)
{
    d()->serialize<Archive>(ar, version);
}

template<class Archive>
void Object::Data::serialize(Archive &ar, const unsigned int version) {
#ifdef DEBUG_SERIALIZATION
   int checktype1 = type;
   ar & V_NAME("checktype1", checktype1);
   if (type != checktype1) {
      std::cerr << "typecheck 1: should be " << type << ", read " << checktype1 << std::endl;
   }
   assert(checktype1 == type);
#endif
   ar & V_NAME("meta", meta);
   boost::serialization::split_member(ar, *this, version);
#ifdef DEBUG_SERIALIZATION
   int checktype2 = type;
   ar & V_NAME("checktype2", checktype2);
   if (type != checktype2) {
      std::cerr << "typecheck 2: should be " << type << ", read " << checktype2 << std::endl;
   }
   assert(checktype2 == type);
#endif
}

template<class Archive>
void Object::Data::save(Archive &ar, const unsigned int version) const {
   StdAttributeMap attrMap;
   auto attrs = getAttributeList();
   for (auto a: attrs) {
       attrMap[a] = getAttributes(a);
   }
   ar & V_NAME("attributes", attrMap);
}

template<class Archive>
void Object::Data::load(Archive &ar, const unsigned int version) {
   attributes.clear();
   StdAttributeMap attrMap;
   ar & V_NAME("attributes", attrMap);
   for (auto &kv: attrMap) {
       auto &a = kv.first;
       auto &vallist = kv.second;
       setAttributeList(a, vallist);
   }
}

template<class Archive>
void Object::save(Archive &ar) const {

   ObjectTypeRegistry::registerArchiveType(ar);
   const Object *p = this;
   ar & V_NAME("object", p);
}

template<class Archive>
Object *Object::load(Archive &ar) {

   ObjectTypeRegistry::registerArchiveType(ar);
   Object *p = NULL;
   try {
      ar & V_NAME("object", p);
   } catch (const boost::archive::archive_exception &ex) {
      std::cerr << "Boost.Archive exception: " << ex.what() << std::endl;
      if (ex.code == boost::archive::archive_exception::unsupported_version) {
         std::cerr << "***" << std::endl;
         std::cerr << "*** received archive from Boost.Archive version " << ar.get_library_version() << ", supported is up to " << boost::archive::BOOST_ARCHIVE_VERSION() << std::endl;
         std::cerr << "***" << std::endl;
      }
      return p;
   }
   p->ref();
   assert(ar.currentObject() == p->d());
   if (p->d()->unresolvedReferences == 0) {
       p->refresh();
       if (ar.objectCompletionHandler())
           ar.objectCompletionHandler()();
   }
   return p;
}

template<>
V_COREEXPORT void ObjectTypeRegistry::registerArchiveType(iarchive &ar);
template<>
V_COREEXPORT void ObjectTypeRegistry::registerArchiveType(oarchive &ar);

} // namespace vistle

#endif
