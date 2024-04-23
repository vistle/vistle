#include "objectcache.h"
#include <vistle/core/shm.h>

namespace vistle {

ObjectCache::ObjectCache(): m_cacheMode(ObjectCache::CacheByName)
{}

ObjectCache::~ObjectCache()
{}

void ObjectCache::clear()
{
    m_oldCache.clear();
    if (m_cacheMode == CacheDeleteLate) {
        std::swap(m_cache, m_oldCache);
    } else {
        m_cache.clear();
    }
    m_nameCache.clear();
}

void ObjectCache::clearOld()
{
    m_oldCache.clear();
}

void ObjectCache::clear(const std::string &portname)
{
    m_cache.erase(portname);
    m_oldCache.erase(portname);
    m_nameCache.erase(portname);
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

    NameList &names = m_nameCache[portname];
    auto &cache = m_cache[portname];
    if (!names.empty()) {
        const Meta &oldmeta = m_meta[portname];
        const Meta &newmeta = object->meta();
        if (oldmeta.creator() != newmeta.creator() || oldmeta.executionCounter() != newmeta.executionCounter()) {
            names.clear();
            cache.clear();
        }
    }
    m_meta[portname] = object->meta();
    names.push_back(object->getName());
    if (m_cacheMode == CacheByName)
        return;

    ObjectList &objs = m_cache[portname];
    objs.push_back(object);
}

std::pair<ObjectList, bool> ObjectCache::getObjects(const std::string &portname) const
{
    std::map<std::string, ObjectList>::const_iterator it = m_cache.find(portname);
    if (it != m_cache.end())
        return std::make_pair(it->second, true);
    auto nit = m_nameCache.find(portname);
    if (nit == m_nameCache.end())
        return std::make_pair(m_emptyList, false);

    ObjectList objs;
    const auto &names = nit->second;
    for (auto &n: names) {
        auto o = Shm::the().getObjectFromName(n);
        if (o) {
            objs.push_back(o);
        } else {
            return std::make_pair(m_emptyList, false);
        }
    }
    return std::make_pair(objs, true);
}

} // namespace vistle
