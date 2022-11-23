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
    if (m_cache.empty()) {
        return false;
    }
    const auto &cache = m_cache.back();
    auto it = cache.find(key);
    if (it == cache.end()) {
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
    if (m_cache.empty()) {
        m_cache.emplace_back();
        m_borrowCount.emplace_back(0);
    }
    auto &cache = m_cache.back();
    auto it = cache.find(key);
    if (it == cache.end()) {
        auto &ent = cache[key];
        ent.generation = m_generation;
        ent.mutex.lock();
        ++m_borrowCount.back();
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

    std::unique_lock<std::mutex> guard(m_mutex);
    assert(entry->generation - m_purgedGenerations >= 0);
    assert(entry->generation - m_purgedGenerations < m_borrowCount.size());
    if (--m_borrowCount[entry->generation - m_purgedGenerations] == 0) {
        purgeOldGenerations();
    }

    return true;
}

template<class Result>
void ResultCache<Result>::clear()
{
    std::unique_lock<std::mutex> guard(m_mutex);
    if (!m_cache.empty()) {
        ++m_generation;
        m_cache.emplace_back();
        m_borrowCount.emplace_back(0);
        purgeOldGenerations();
    }
}

template<class Result>
void ResultCache<Result>::purgeOldGenerations()
{
    while (m_borrowCount.size() > 1) {
        if (m_borrowCount.front() > 0)
            break;
        ++m_purgedGenerations;
        m_borrowCount.pop_front();
        m_cache.pop_front();
    }
}

} // namespace vistle

#endif
