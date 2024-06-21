#ifndef RESULTCACHE_IMPL_H
#define RESULTCACHE_IMPL_H

#include "resultcache.h"

#include <cassert>

namespace vistle {

template<class Result>
void ResultCache<Result>::modifyBorrowCount(size_t generation, int delta)
{
    assert(generation >= m_purgedGenerations);
    assert(generation < m_purgedGenerations + m_borrowCount.size());

    m_borrowCount[generation - m_purgedGenerations] += delta;
    if (m_borrowCount[generation - m_purgedGenerations] == 0) {
        purgeOldGenerations();
    }
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
        modifyBorrowCount(m_generation, 1);
        return &ent;
    }

    auto &ent = it->second;
    auto generation = ent.generation;
    modifyBorrowCount(generation, 1);
    guard.unlock();

    std::unique_lock<std::mutex> member_guard(ent.mutex);
    result = ent.data;
    member_guard.unlock();

    guard.lock();
    modifyBorrowCount(generation, -1);

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
    auto generation = entry->generation;
    entry->mutex.unlock();

    std::unique_lock<std::mutex> guard(m_mutex);
    modifyBorrowCount(generation, -1);

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
