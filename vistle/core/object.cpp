#include <iostream>
#include <iomanip>

#include <limits.h>

#include <boost/scoped_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

#include "message.h"
#include "messagequeue.h"
#include "shm.h"

#include "object.h"
#include "object_impl.h"

#include "tools.h"

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

using namespace boost::interprocess;

namespace boost {
namespace serialization {

// XXX: check these

#if 0
template<>
void access::destroy(const vistle::Object *t)
{
   delete const_cast<vistle::Object *>(t);
}

template<>
void access::construct(vistle::Object *t)
{
   ::new(t) vistle::Object(vistle::Shm::the().allocator());
}
#endif

template<>
void access::destroy( const vistle::shm<char>::string * t) // const appropriate here?
{
   delete const_cast<vistle::shm<char>::string *>(t);
}

template<>
void access::construct(vistle::shm<char>::string * t)
{
   // default is inplace invocation of default constructor
   // Note the :: before the placement new. Required if the
   // class doesn't have a class-specific placement new defined.
   ::new(t) vistle::shm<char>::string(vistle::Shm::the().allocator());
}

template<>
void access::destroy(const vistle::Object::Data::AttributeList *t)
{
   delete const_cast<vistle::Object::Data::AttributeList *>(t);
}

template<>
void access::construct(vistle::Object::Data::AttributeList *t)
{
   ::new(t) vistle::Object::Data::AttributeList(vistle::Shm::the().allocator());
}

template<>
void access::destroy(const vistle::Object::Data::AttributeMapValueType *t)
{
   delete const_cast<vistle::Object::Data::AttributeMapValueType *>(t);
}

template<>
void access::construct(vistle::Object::Data::AttributeMapValueType *t)
{
   ::new(t) vistle::Object::Data::AttributeMapValueType(vistle::Object::Data::Key(vistle::Shm::the().allocator()),
         vistle::Object::Data::AttributeList(vistle::Shm::the().allocator()));
}

} // namespace serialization
} // namespace boost



namespace vistle {

Object::ptr Object::create(Object::Data *data) {

   if (!data)
      return Object::ptr();

   return ObjectTypeRegistry::getCreator(data->type)(data);
}

void Object::publish(const Object::Data *d) {

#if defined(SHMDEBUG) || defined(SHMPUBLISH)
   shm_handle_t handle = Shm::the().shm().get_handle_from_address(d);
#else
   (void)d;
#endif

#ifdef SHMDEBUG
   Shm::the().s_shmdebug->push_back(ShmDebugInfo('O', d->name, handle));
#endif

#ifdef SHMPUBLISH
   Shm::the().publish(handle);
#endif
}

Object::Data::Data(const Type type, const std::string & n, const int b, const int t)
   : type(type)
   , refcount(0)
   , block(b)
   , timestep(t)
   , attributes(shm<AttributeMap>::construct(std::string("attr_")+n)(std::less<Key>(), Shm::the().allocator()))
{

   size_t size = min(n.size(), sizeof(name)-1);
   n.copy(name, size);
   name[size] = 0;
}

Object::Data::~Data() {

   shm<AttributeMap>::destroy(std::string("attr_")+name);
}

void *Object::Data::operator new(size_t size) {
   return Shm::the().shm().allocate(size);
}

void* Object::Data::operator new (std::size_t size, void* ptr) {
   return ptr;
}

void Object::Data::operator delete(void *p) {
   return Shm::the().shm().deallocate(p);
}



Object::Data *Object::Data::create(Type id, int b, int t) {

   std::string name = Shm::the().createObjectID();
   return shm<Data>::construct(name)(id, name, b, t);
}

Object::Object(Object::Data *data)
: m_data(data)
{
   m_data->ref();
}

Object::Object()
: m_data(NULL)
{
}

Object::~Object() {

   m_data->unref();
}

void Object::ref() const {
   d()->ref();
}

void Object::unref() const {
   d()->unref();
}

int Object::refcount() const {
   return d()->refcount;
}

void Object::save(boost::archive::text_oarchive &ar) const {

   ObjectTypeRegistry::registerArchiveType(ar);
   const Object *p = this;
   ar & V_NAME("object", p);
}

void Object::save(boost::archive::xml_oarchive &ar) const {

   ObjectTypeRegistry::registerArchiveType(ar);
   const Object *p = this;
   ar & V_NAME("object", p);
}

void Object::save(boost::archive::binary_oarchive &ar) const {

   ObjectTypeRegistry::registerArchiveType(ar);
   const Object *p = this;
   ar & V_NAME("object", p);
}

Object::const_ptr Object::load(boost::archive::binary_iarchive &ar) {

   ObjectTypeRegistry::registerArchiveType(ar);
   Object *p = NULL;
   ar & V_NAME("object", p);
   return Object::const_ptr(p);
}

Object::const_ptr Object::load(boost::archive::text_iarchive &ar) {

   ObjectTypeRegistry::registerArchiveType(ar);
   Object *p = NULL;
   ar & V_NAME("object", p);
   return Object::const_ptr(p);
}

Object::const_ptr Object::load(boost::archive::xml_iarchive &ar) {

   ObjectTypeRegistry::registerArchiveType(ar);
   Object *p = NULL;
   ar & V_NAME("object", p);
   return Object::const_ptr(p);
}

void Object::Data::ref() {
   boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mutex);
   ++refcount;
}

void Object::Data::unref() {
   mutex.lock();
   --refcount;
   assert(refcount >= 0);
   if (refcount == 0) {
      mutex.unlock();
      //shm<Data>::destroy(name);
      ObjectTypeRegistry::getDestroyer(type)(name);
      return;
   }
   mutex.unlock();
}

shm_handle_t Object::getHandle() const {

   return Shm::the().getHandleFromObject(this);
}

Object::Type Object::getType() const {

   return d()->type;
}

std::string Object::getName() const {

   return d()->name;
}

int Object::getTimestep() const {

   return d()->timestep;
}

int Object::getBlock() const {

   return d()->block;
}

void Object::setTimestep(const int time) {

   d()->timestep = time;
}

void Object::setBlock(const int blk) {

   d()->block = blk;
}

const struct ObjectTypeRegistry::FunctionTable &ObjectTypeRegistry::getType(int id) {
   TypeMap::const_iterator it = typeMap().find(id);
   assert(it != typeMap().end());
   if (it == typeMap().end()) {
      std::cerr << "ObjectTypeRegistry: no creator for type id " << id << std::endl;
      exit(1);
   }
   return (*it).second;
}

#define REG_WITH_ARCHIVE(type, func_name) \
   template<> \
   void ObjectTypeRegistry::registerArchiveType(type &ar) { \
      TypeMap::key_type key; \
      TypeMap::mapped_type funcs; \
      BOOST_FOREACH(boost::tie(key, funcs), typeMap()) { \
         funcs.func_name(ar); \
      } \
   }
REG_WITH_ARCHIVE(boost::archive::text_iarchive, registerTextIArchive)
REG_WITH_ARCHIVE(boost::archive::text_oarchive, registerTextOArchive)
REG_WITH_ARCHIVE(boost::archive::binary_iarchive, registerBinaryIArchive)
REG_WITH_ARCHIVE(boost::archive::binary_oarchive, registerBinaryOArchive)
REG_WITH_ARCHIVE(boost::archive::xml_iarchive, registerXmlIArchive)
REG_WITH_ARCHIVE(boost::archive::xml_oarchive, registerXmlOArchive)

void Object::addAttribute(const std::string &key, const std::string &value) {
   d()->addAttribute(key, value);
}

void Object::setAttributeList(const std::string &key, const std::vector<std::string> &values) {
   d()->setAttributeList(key, values);
}

void Object::copyAttributes(Object::const_ptr src, bool replace) {
   d()->copyAttributes(src->d(), replace);
}

bool Object::hasAttribute(const std::string &key) const {
   return d()->hasAttribute(key);
}

std::string Object::getAttribute(const std::string &key) const {
   return d()->getAttribute(key);
}

std::vector<std::string> Object::getAttributes(const std::string &key) const {
   return d()->getAttributes(key);
}

void Object::Data::addAttribute(const std::string &key, const std::string &value) {

   const Key skey(key.c_str(), Shm::the().allocator());
   std::pair<AttributeMap::iterator, bool> res = attributes->insert(AttributeMapValueType(skey, AttributeList(Shm::the().allocator())));
   AttributeList &a = res.first->second;
   a.push_back(Attribute(value.c_str(), Shm::the().allocator()));
}

void Object::Data::setAttributeList(const std::string &key, const std::vector<std::string> &values) {

   const Key skey(key.c_str(), Shm::the().allocator());
   std::pair<AttributeMap::iterator, bool> res = attributes->insert(AttributeMapValueType(skey, AttributeList(Shm::the().allocator())));
   AttributeList &a = res.first->second;
   a.clear();
   for (size_t i=0; i<values.size(); ++i) {
      a.push_back(Attribute(values[i].c_str(), Shm::the().allocator()));
   }
}

void Object::Data::copyAttributes(const Object::Data *src, bool replace) {

   if (replace) {

      *attributes = *src->attributes;
   } else {

      const AttributeMap &a = *src->attributes;

      for (AttributeMap::const_iterator it = a.begin(); it != a.end(); ++it) {
         const Key &key = it->first;
         const AttributeList &values = it->second;
         std::pair<AttributeMap::iterator, bool> res = attributes->insert(AttributeMapValueType(key, values));
         if (!res.second) {
            AttributeList &dest = res.first->second;
            if (replace)
               dest.clear();
            for (AttributeList::const_iterator ait = values.begin(); ait != values.end(); ++ait) {
               dest.push_back(*ait);
            }
         }
      }
   }

#if 0
   addAttribute("test");

   std::cerr << "COPY ATTR " << std::endl;
   std::cerr << "    #src " << src->attributes->size() << std::endl;
   std::cerr << "    #dst " << attributes->size() << std::endl;

   std::cerr << "COPY ATTR DONE" << std::endl;
#endif
}

bool Object::Data::hasAttribute(const std::string &key) const {

   const Key skey(key.c_str(), Shm::the().allocator());
   AttributeMap::iterator it = attributes->find(skey);
   return it != attributes->end();
}

std::string Object::Data::getAttribute(const std::string &key) const {

   const Key skey(key.c_str(), Shm::the().allocator());
   AttributeMap::iterator it = attributes->find(skey);
   if (it == attributes->end())
      return std::string();
   AttributeList &a = it->second;
   return a.back().c_str();
}

std::vector<std::string> Object::Data::getAttributes(const std::string &key) const {

   const Key skey(key.c_str(), Shm::the().allocator());
   AttributeMap::iterator it = attributes->find(skey);
   if (it == attributes->end())
      return std::vector<std::string>();
   AttributeList &a = it->second;

   std::vector<std::string> attrs;
   for (AttributeList::iterator i = a.begin(); i != a.end(); ++i) {
      attrs.push_back(i->c_str());
   }
   return attrs;
}


ObjectTypeRegistry::CreateFunc ObjectTypeRegistry::getCreator(int id) {
   return getType(id).create;
}

ObjectTypeRegistry::DestroyFunc ObjectTypeRegistry::getDestroyer(int id) {
   return getType(id).destroy;
}

ObjectTypeRegistry::TypeMap &ObjectTypeRegistry::typeMap() {
   static TypeMap m;
   return m;
}

} // namespace vistle
