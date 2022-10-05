#include "objectcache.h"

namespace vistle {

ObjectCache::ObjectCache(): m_cacheMode(ObjectCache::CacheNone)
{}

ObjectCache::~ObjectCache()
{}

void ObjectCache::clear()
{
    if (m_cacheMode == CacheDeleteLate) {
        m_oldCache.clear();
        std::swap(m_cache, m_oldCache);
    } else {
        m_cache.clear();
    }
}

void ObjectCache::clearOld()
{
    m_oldCache.clear();
}

void ObjectCache::clear(const std::string &portname)
{
    m_cache.erase(portname);
    m_oldCache.erase(portname);
}

ObjectCache::CacheMode ObjectCache::cacheMode() const
{
    return m_cacheMode;
}

void ObjectCache::setCacheMode(ObjectCache::CacheMode mode)
{
    if (mode != m_cacheMode) {
        if (m_cacheMode == CacheDeleteLate) {
            m_oldCache.clear();
        }
        if (mode == CacheNone)
            clear();
    }

    m_cacheMode = mode;
}

void ObjectCache::addObject(const std::string &portname, Object::const_ptr object)
{
    if (m_cacheMode == CacheNone)
        return;

    ObjectList &objs = m_cache[portname];
    if (!objs.empty()) {
        const Meta &oldmeta = objs.front()->meta();
        const Meta &newmeta = object->meta();
        if (oldmeta.creator() != newmeta.creator() || oldmeta.executionCounter() != newmeta.executionCounter() ||
            oldmeta.iteration() != newmeta.iteration()) {
            objs.clear();
        }
    }
    objs.push_back(object);
}

const ObjectList &ObjectCache::getObjects(const std::string &portname) const
{
    std::map<std::string, ObjectList>::const_iterator it = m_cache.find(portname);
    if (it == m_cache.end())
        return m_emptyList;

    return it->second;
}

} // namespace vistle
