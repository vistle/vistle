#include <iostream>
#include <iomanip>

#include <limits.h>

#include <boost/scoped_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

#include <boost/mpl/for_each.hpp>

#include <util/tools.h>

#ifndef TEMPLATES_IN_HEADERS
#define VISTLE_IMPL
#endif
#include "object.h"
#ifndef TEMPLATES_IN_HEADERS
#undef VISTLE_IMPL
#endif

#include "object_impl.h"

#include "message.h"
#include "messagequeue.h"
#include "shm.h"
#include "archives.h"
#include "assert.h"

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

using namespace boost::interprocess;

namespace mpl = boost::mpl;

namespace boost {
namespace serialization {

// XXX: check these

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

const char *Object::toString(Type v) {

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
            V_OBJECT_CASE(UNIFORMGRID)
            V_OBJECT_CASE(RECTILINEARGRID)
            V_OBJECT_CASE(STRUCTUREDGRID)

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
        if (scalidx < ScalarTypeNames.size())
            scalstr = ScalarTypeNames[scalidx];
#if 0
        switch (scalidx) {
        case 0: scalstr = "unsigned char"; break;
        case 1: scalstr = "int"; break;
        case 2: scalstr = "unsigned int"; break;
        case 3: scalstr = "size_t"; break;
        case 4: scalstr = "float"; break;
        case 5: scalstr = "double"; break;
        }
#endif
        snprintf(buf, sizeof(buf), "VEC<%s,%d>", scalstr, dim);
    }

    return buf;
}

Object::ptr Object::create(Object::Data *data) {

   if (!data)
      return Object::ptr();

   return ObjectTypeRegistry::getCreator(data->type)(data);
}

bool Object::isComplete() const {
    return d()->isComplete();
}

void Object::publish(const Object::Data *d) {

#if defined(SHMDEBUG) || defined(SHMPUBLISH)
   shm_handle_t handle = Shm::the().shm().get_handle_from_address(d);
#else
   (void)d;
#endif

#ifdef SHMDEBUG
   Shm::the().addObject(d->name, handle);
#endif

#ifdef SHMPUBLISH
   Shm::the().publish(handle);
#endif
}

ObjectData::ObjectData(const Object::Type type, const std::string & n, const Meta &m)
   : type(type)
   , name(n)
   , refcount(0)
   , unresolvedReferences(0)
   , meta(m)
   , attributes(shm<AttributeMap>::construct(std::string("attr_")+n)(std::less<Key>(), Shm::the().allocator()))
   , attachments(shm<AttachmentMap>::construct(std::string("attach_")+n)(std::less<Key>(), Shm::the().allocator()))
{
}

ObjectData::ObjectData(const Object::Data &o, const std::string &name, Object::Type id)
: type(id==Object::UNKNOWN ? o.type : id)
, name(name)
, refcount(0)
, unresolvedReferences(0)
, meta(o.meta)
, attributes(shm<AttributeMap>::construct(std::string("attr_")+name)(std::less<Key>(), Shm::the().allocator()))
, attachments(shm<AttachmentMap>::construct(std::string("attach_")+name)(std::less<Key>(), Shm::the().allocator()))
{
   copyAttributes(&o, true);
   copyAttachments(&o, true);
}

ObjectData::~ObjectData() {

   //std::cerr << "SHM DESTROY OBJ: " << name << std::endl;

   shm<AttributeMap>::destroy(std::string("attr_")+name);
   { 
      boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(attachment_mutex);
      for (auto &objd: *attachments) {
         // referenced in addAttachment
         objd.second->unref();
      }
      shm<AttachmentMap>::destroy(std::string("attach_")+name);
   }
}

bool Object::Data::isComplete() const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(ref_mutex);
   // a reference is only established upon return from Object::load
   return refcount>0 && unresolvedReferences==0;
}

void Object::Data::referenceResolved(const std::function<void()> &completeCallback) {
    //std::cerr << "reference (from " << unresolvedReferences << ") resolved in " << name << std::endl;
    vassert(unresolvedReferences > 0);
    --unresolvedReferences;
    if (isComplete()) {
        completeCallback();
    }
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

void Object::Data::operator delete(void *p, void *voidp2) {
   return Shm::the().shm().deallocate(p);
}


ObjectData *ObjectData::create(Object::Type id, const std::string &objId, const Meta &m) {

   std::string name = Shm::the().createObjectId(objId);
   return shm<ObjectData>::construct(name)(id, name, m);
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

Object::ptr Object::clone() const {

   return cloneInternal();
}

Object::ptr Object::createEmpty() {

   vassert("cannot create generic Object" == nullptr);
   return Object::ptr();
}

Object::ptr Object::cloneType() const {

   return cloneTypeInternal();
}

void Object::refresh() const {
}

bool Object::check() const {

   V_CHECK (d()->refcount >= 0);

   bool terminated = false;
   for (size_t i=0; i<sizeof(shm_name_t); ++i) {
      if (d()->name[i] == '\0') {
         terminated = true;
         break;
      }
   }
   V_CHECK (terminated);

   V_CHECK(d()->type > UNKNOWN);

   V_CHECK (d()->meta.timeStep() >= -1);
   V_CHECK (d()->meta.timeStep() < d()->meta.numTimesteps() || d()->meta.numTimesteps()==-1);
   V_CHECK (d()->meta.animationStep() >= -1);
   V_CHECK (d()->meta.animationStep() < d()->meta.numAnimationSteps() || d()->meta.numAnimationSteps()==-1);
   V_CHECK (d()->meta.iteration() >= -1);
   V_CHECK (d()->meta.block() >= -1);
   V_CHECK (d()->meta.block() < d()->meta.numBlocks() || d()->meta.numBlocks()==-1);
   V_CHECK (d()->meta.executionCounter() >= -1);

   return true;
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

bool Object::isEmpty() const {
   return true;
}

namespace { 

template<class T>
struct wrap {};

struct instantiate_load {
   template<class Archive>
   void operator()(wrap<Archive>) {
      Archive ar(std::cin);
      Object::load(ar);
   }
};

struct instantiate_save {
   template<class Archive>
   void operator()(wrap<Archive>) {
      Archive ar(std::cout);
      Object::const_ptr obj;
      obj->save(ar);
   }
};

}

void instantiate_all_io() {
      mpl::for_each<InputArchives, wrap<mpl::_1> >(instantiate_load());
      mpl::for_each<OutputArchives, wrap<mpl::_1> >(instantiate_save());
}

void Object::Data::ref() const {
   boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(ref_mutex);
   ++refcount;
}

void Object::Data::unref() const {
   ref_mutex.lock();
   --refcount;
   assert(refcount >= 0);
   if (refcount == 0) {
      ref_mutex.unlock();
      ObjectTypeRegistry::getDestroyer(type)(name);
      return;
   }
   ref_mutex.unlock();
}

shm_handle_t Object::getHandle() const {

   return Shm::the().getHandleFromObject(this);
}

Object::Type Object::getType() const {

   return d()->type;
}

std::string Object::getName() const {

   return (d()->name.operator std::string());
}

const Meta &Object::meta() const {

   return d()->meta;
}

void Object::setMeta(const Meta &meta) {

   d()->meta = meta;
}

double Object::getRealTime() const {

   return d()->meta.realTime();
}

int Object::getTimestep() const {

   return d()->meta.timeStep();
}

int Object::getNumTimesteps() const {
   
   return d()->meta.numTimesteps();
}

int Object::getBlock() const {

   return d()->meta.block();
}

int Object::getNumBlocks() const {
   
   return d()->meta.numBlocks();
}

int Object::getExecutionCounter() const {

   return d()->meta.executionCounter();
}

int Object::getCreator() const {

   return d()->meta.creator();
}

void Object::setRealTime(const double time) {

   d()->meta.setRealTime(time);
}

void Object::setTimestep(const int time) {

   d()->meta.setTimeStep(time);
}

void Object::setNumTimesteps(const int num) {

   d()->meta.setNumTimesteps(num);
}

void Object::setBlock(const int blk) {

   d()->meta.setBlock(blk);
}

void Object::setNumBlocks(const int num) {

   d()->meta.setNumBlocks(num);
}

void Object::setExecutionCounter(const int count) {

   d()->meta.setExecutionCounter(count);
}

void Object::setCreator(const int id) {

   d()->meta.setCreator(id);
}

const struct ObjectTypeRegistry::FunctionTable &ObjectTypeRegistry::getType(int id) {
   TypeMap::const_iterator it = typeMap().find(id);
   if (it == typeMap().end()) {
      std::cerr << "ObjectTypeRegistry: no creator for type id " << id << " (" << typeMap().size() << " total entries)" << std::endl;
      assert(it != typeMap().end());
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
REG_WITH_ARCHIVE(iarchive, registerIArchive)
REG_WITH_ARCHIVE(oarchive, registerOArchive)

void Object::addAttribute(const std::string &key, const std::string &value) {
   d()->addAttribute(key, value);
}

void Object::setAttributeList(const std::string &key, const std::vector<std::string> &values) {
   d()->setAttributeList(key, values);
}

void Object::copyAttributes(Object::const_ptr src, bool replace) {

   if (replace) {
      auto &m = d()->meta;
      auto &sm = src->meta();
      m.setBlock(sm.block());
      m.setNumBlocks(sm.numBlocks());
      m.setTimeStep(sm.timeStep());
      m.setNumTimesteps(sm.numTimesteps());
      m.setRealTime(sm.realTime());
      m.setAnimationStep(sm.animationStep());
      m.setNumAnimationSteps(sm.numAnimationSteps());
      m.setIteration(sm.iteration());
   }

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

std::vector<std::string> Object::getAttributeList() const {
   return d()->getAttributeList();
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

void Object::Data::copyAttributes(const ObjectData *src, bool replace) {

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

std::vector<std::string> Object::Data::getAttributeList() const {

   std::vector<std::string> result;
   for (AttributeMap::iterator it = attributes->begin();
         it != attributes->end();
         ++it) {
      auto key = it->first;
      result.push_back(key.c_str());
   }
   return result;
}

bool Object::addAttachment(const std::string &key, Object::const_ptr obj) const {

   return d()->addAttachment(key, obj);
}

void Object::copyAttachments(Object::const_ptr src, bool replace) {

   d()->copyAttachments(src->d(), replace);
}

bool Object::hasAttachment(const std::string &key) const {

   return d()->hasAttachment(key);
}

Object::const_ptr Object::getAttachment(const std::string &key) const {

   return d()->getAttachment(key);
}

bool Object::removeAttachment(const std::string &key) const {

   return d()->removeAttachment(key);
}

bool Object::Data::hasAttachment(const std::string &key) const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(attachment_mutex);
   const Key skey(key.c_str(), Shm::the().allocator());
   AttachmentMap::iterator it = attachments->find(skey);
   return it != attachments->end();
}

Object::const_ptr ObjectData::getAttachment(const std::string &key) const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(attachment_mutex);
   const Key skey(key.c_str(), Shm::the().allocator());
   AttachmentMap::iterator it = attachments->find(skey);
   if (it == attachments->end()) {
      return Object::ptr();
   }
   return Object::create(it->second.get());
}

bool Object::Data::addAttachment(const std::string &key, Object::const_ptr obj) {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(attachment_mutex);
   const Key skey(key.c_str(), Shm::the().allocator());
   AttachmentMap::iterator it = attachments->find(skey);
   if (it != attachments->end()) {
      return false;
   }

   obj->ref();
   attachments->insert(AttachmentMapValueType(skey, obj->d()));

   return true;
}

void Object::Data::copyAttachments(const ObjectData *src, bool replace) {

   const AttachmentMap &a = *src->attachments;

   for (AttachmentMap::const_iterator it = a.begin(); it != a.end(); ++it) {
      const Key &key = it->first;
      const Attachment &value = it->second;
      auto res = attachments->insert(AttachmentMapValueType(key, value));
      if (res.second) {
         value->ref();
      } else {
         if (replace) {
            const Attachment &oldVal = res.first->second;
            oldVal->unref();
            res.first->second = value;
            value->ref();
         }
      }
   }
}

bool Object::Data::removeAttachment(const std::string &key) {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(attachment_mutex);
   const Key skey(key.c_str(), Shm::the().allocator());
   AttachmentMap::iterator it = attachments->find(skey);
   if (it == attachments->end()) {
      return false;
   }

   it->second->unref();
   attachments->erase(it);

   return true;
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
