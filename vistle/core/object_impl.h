#ifndef OBJECT_IMPL_H
#define OBJECT_IMPL_H

// include headers that implement an archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


#include "serialize.h"

BOOST_CLASS_IMPLEMENTATION(vistle::shm<char>::string, boost::serialization::primitive_type)

namespace boost {
namespace serialization {

template<>
void access::destroy(const vistle::shm<char>::string *t);

template<>
void access::construct(vistle::shm<char>::string *t);

template<>
void access::destroy(const vistle::Object::Data::AttributeList *t);

template<>
void access::construct(vistle::Object::Data::AttributeList *t);

template<>
void access::destroy(const vistle::Object::Data::AttributeMapValueType *t);

template<>
void access::construct(vistle::Object::Data::AttributeMapValueType *t);

} // namespace serialization
} // namespace boost


namespace vistle {

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
   ar & V_NAME("attributes", *attributes);
#ifdef DEBUG_SERIALIZATION
   int checktype2 = type;
   ar & V_NAME("checktype2", checktype2);
   if (type != checktype2) {
      std::cerr << "typecheck 2: should be " << type << ", read " << checktype2 << std::endl;
   }
   assert(checktype2 == type);
#endif
}

template<>
void ObjectTypeRegistry::registerArchiveType(boost::archive::text_iarchive &ar);
template<>
void ObjectTypeRegistry::registerArchiveType(boost::archive::text_oarchive &ar);
template<>
void ObjectTypeRegistry::registerArchiveType(boost::archive::binary_iarchive &ar);
template<>
void ObjectTypeRegistry::registerArchiveType(boost::archive::binary_oarchive &ar);
template<>
void ObjectTypeRegistry::registerArchiveType(boost::archive::xml_iarchive &ar);
template<>
void ObjectTypeRegistry::registerArchiveType(boost::archive::xml_oarchive &ar);

} // namespace vistle

#endif
