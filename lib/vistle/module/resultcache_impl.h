#ifndef RESULTCACHE_IMPL_H
#define RESULTCACHE_IMPL_H

#include "resultcache.h"

namespace vistle {

template<class Result>
bool ResultCache<Result>::tryGet(const std::string &key, Result &result)
{
    if (!m_enabled)
        return false;

    std::unique_lock<std::mutex> guard(m_mutex);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return false;
    }

    auto &ent = it->second;
    guard.unlock();
    if (!ent.mutex.try_lock()) {
        return false;
    }

    result = ent.data;
    ent.mutex.unlock();
    return true;
}

template<class Result>
typename ResultCache<Result>::Entry *ResultCache<Result>::getOrLock(const std::string &key, Result &result)
{
    if (!m_enabled)
        return &m_empty;

    std::unique_lock<std::mutex> guard(m_mutex);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        auto &ent = m_cache[key];
        ent.mutex.lock();
        return &ent;
    }

    auto &ent = it->second;
    guard.unlock();
    std::unique_lock<std::mutex> member_guard(ent.mutex);
    result = ent.data;
    return nullptr;
}

template<class Result>
bool ResultCache<Result>::storeAndUnlock(ResultCache<Result>::Entry *entry, const Result &data)
{
    if (!entry)
        return false;
    if (entry == &m_empty)
        return false;
    entry->data = data;
    entry->mutex.unlock();

    return true;
}

template<class Result>
void ResultCache<Result>::clear()
{
    std::unique_lock<std::mutex> guard(m_mutex);
    m_cache.clear();
}

} // namespace vistle

#endif
