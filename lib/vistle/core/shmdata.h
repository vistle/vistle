#ifndef VISTLE_CORE_SHMDATA_H
#define VISTLE_CORE_SHMDATA_H

#include <atomic>
#include <string>
#include <cassert>

#include "shmname.h"

namespace vistle {

struct ShmData {
    enum ShmType {
        INVALID,
        OBJECT,
        ARRAY,
    };

    explicit ShmData(ShmType t, const std::string &name = std::string()): name(name), m_refcount(0), shmtype(t) {}

    ~ShmData() { assert(m_refcount == 0); }

    int ref() const
    {
        assert(m_refcount >= 0);
        return ++m_refcount;
    }

    int unref() const
    {
        assert(m_refcount > 0);
        return --m_refcount;
    }

    int refcount() const
    {
        assert(m_refcount >= 0);
        return m_refcount;
    }

    const shm_name_t name;
    mutable std::atomic<int> m_refcount;
    const ShmType shmtype = INVALID;
};

} // namespace vistle
#endif
