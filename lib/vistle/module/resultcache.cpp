#include "resultcache.h"
#include "resultcache_impl.h"

namespace vistle {

ResultCacheBase::~ResultCacheBase() = default;

void ResultCacheBase::enable(bool on)
{
    m_enabled = on;
}
} // namespace vistle
