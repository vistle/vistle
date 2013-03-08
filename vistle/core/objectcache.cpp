#include "objectcache.h"

namespace vistle {

ObjectCache::ObjectCache()
: m_cacheMode(ObjectCache::CacheNone)
{
}

ObjectCache::~ObjectCache() {
}

void ObjectCache::clear() {

   m_cache.clear();
}

ObjectCache::CacheMode ObjectCache::cacheMode() const {

   return m_cacheMode;
}

void ObjectCache::setCacheMode(ObjectCache::CacheMode mode) {

   m_cacheMode = mode;

   if (m_cacheMode == CacheNone)
      clear();
}

void ObjectCache::addObject(const std::string &portname, Object::const_ptr object) {

   if (m_cacheMode == CacheNone)
      return;

   ObjectList &objs = m_cache[portname];
   if (!objs.empty()) {
      const Meta &oldmeta = objs.front()->meta();
      const Meta &newmeta = object->meta();
      if (oldmeta.creator() != newmeta.creator()
            || oldmeta.executionCounter() != newmeta.executionCounter()) {
         objs.clear();
      }
   }
   objs.push_back(object);
}

const ObjectList &ObjectCache::getObjects(const std::string &portname) const {

   std::map<std::string, ObjectList>::const_iterator it = m_cache.find(portname);
   if (it == m_cache.end())
      return m_emptyList;

   return it->second;
}

} // namespace vistle
