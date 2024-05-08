#include "objectcache.h"
#include "module.h"
#include <vistle/core/shm.h>

namespace vistle {

ObjectCache::ObjectCache(): m_cacheMode(ObjectCache::CacheByName)
{}

ObjectCache::~ObjectCache()
{}

int ObjectCache::generation() const
{
    return m_generation;
}

void ObjectCache::clear()
{
    ++m_generation;

    m_meta.clear();
    m_oldCache.clear();
    if (m_cacheMode == CacheDeleteLate) {
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
    ++m_generation;

    m_meta.erase(portname);
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

ObjectCache::Entry::Entry(Object::const_ptr object, bool cacheByNameOnly)
: name(object->getName())
, object(cacheByNameOnly ? nullptr : object)
, block(getBlock(object))
, timestep(getTimestep(object))
, iteration(getIteration(object))
{}

void ObjectCache::addObject(const std::string &portname, Object::const_ptr object)
{
    Meta &meta = m_meta[portname];
    const Meta oldmeta = meta;
    meta = object->meta();
    if (oldmeta.creator() != meta.creator() || oldmeta.generation() != meta.generation()) {
        ++m_generation;
    }

    if (m_cacheMode == CacheNone)
        return;

    auto &cache = m_cache[portname];
    auto time = getTimestep(object);
    assert(time >= -1);
    auto block = getBlock(object);
    assert(block >= -1);
    auto iter = getIteration(object);
    assert(iter >= -1);

    if (oldmeta.creator() != meta.creator() || oldmeta.generation() != meta.generation()) {
        cache.clear();
    } else if (iter >= 0) {
        for (auto &entry: cache) {
            if (entry.block == block && entry.timestep == time) {
                auto olditer = entry.iteration;
                if (iter >= olditer) {
                    entry = Entry(object, m_cacheMode == CacheByName);
                } else {
                    std::cerr << "ObjectCache: ignoring object with iteration " << iter << " for port " << portname
                              << " (old: " << olditer << "): " << *object << std::endl;
                }
                return;
            }
        }
    }

    cache.emplace_back(object, m_cacheMode == CacheByName);
}

std::pair<ObjectList, bool> ObjectCache::getObjects(const std::string &portname) const
{
    auto it = m_cache.find(portname);
    if (it == m_cache.end()) {
        return std::make_pair(m_emptyList, false);
    }
    const auto &cache = it->second;

    ObjectList objs;
    for (const auto &e: cache) {
        if (e.object) {
            objs.push_back(e.object);
        } else if (auto o = Shm::the().getObjectFromName(e.name)) {
            objs.push_back(o);
        } else {
            return std::make_pair(m_emptyList, false);
        }
    }
    return std::make_pair(objs, true);
}

} // namespace vistle
