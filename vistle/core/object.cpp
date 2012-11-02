#include <iostream>
#include <iomanip>

#include <limits.h>

#include "message.h"
#include "messagequeue.h"
#include "shm.h"
#include "object.h"

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

using namespace boost::interprocess;

namespace vistle {

Object::ptr Object::create(Object::Data *data) {

   if (!data)
      return Object::ptr();

   return ObjectTypeRegistry::getCreator(data->type)(data);
}

void Object::publish(const Object::Data *d) {

   shm_handle_t handle = Shm::instance().getShm().get_handle_from_address(d);

#ifdef SHMDEBUG
   Shm::instance().s_shmdebug->push_back(ShmDebugInfo('O', d->name, handle));
#endif

#if 0
   Shm::instance().publish(handle);
#endif
}

Object::Data::Data(const Type type, const std::string & n,
               const int b, const int t): type(type), refcount(0), block(b), timestep(t) {

   size_t size = min(n.size(), sizeof(name)-1);
   n.copy(name, size);
   name[size] = 0;
}

Object::Object(Object::Data *data)
: m_data(data)
{
   m_data->ref();
}

Object::~Object() {

   m_data->unref();
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

Object::Info *Object::getInfo(Object::Info *info) const {

   if (!info)
      info = new Info;

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->type = getType();
   info->block = getBlock();
   info->timestep = getTimestep();

   return info;
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
   TypeMap::const_iterator it = s_typeMap.find(id);
   assert(it != s_typeMap.end());
   if (it == s_typeMap.end()) {
      std::cerr << "ObjectTypeRegistry: no creator for type id " << id << std::endl;
      exit(1);
   }
   return (*it).second;
}


ObjectTypeRegistry::CreateFunc ObjectTypeRegistry::getCreator(int id) {
   return getType(id).create;
}

ObjectTypeRegistry::DestroyFunc ObjectTypeRegistry::getDestroyer(int id) {
   return getType(id).destroy;
}

ObjectTypeRegistry::TypeMap ObjectTypeRegistry::s_typeMap;

} // namespace vistle
