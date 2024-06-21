#ifndef RESULT_CACHE_H
#define RESULT_CACHE_H

#include "export.h"

#include <string>
#include <map>
#include <mutex>
#include <deque>

namespace vistle {

class V_MODULEEXPORT ResultCacheBase {
public:
    virtual ~ResultCacheBase();
    virtual void clear() = 0;
    virtual void enable(bool on);

protected:
    bool m_enabled = true;
};

//! data structure for retaining data that can be reused between timesteps
template<class Result>
class ResultCache: public ResultCacheBase {
public:
    struct Entry {
        friend class ResultCache;

    private:
        std::mutex mutex;
        size_t generation = 0;
        Result data;
    };

    //! if available, retrieve value for key, store to result, and return nullptr;
    //! otherwise the entry corresponding to key is locked and has to be updated with storeAndUnlock via the returned Entry
    Entry *getOrLock(const std::string &key, Result &result);
    //! update value stored for entry with data and unlock it
    bool storeAndUnlock(Entry *entry, const Result &data);
    //! discard all currently stored values
    void clear() override;

protected:
    std::mutex m_mutex;
    size_t m_generation = 0, m_purgedGenerations = 0;
    Entry m_empty;
    std::deque<std::map<std::string, Entry>> m_cache;
    std::deque<size_t> m_borrowCount;

    void modifyBorrowCount(size_t generation, int delta); // assumes that m_mutex is already locked
    void purgeOldGenerations(); // assumes that m_mutex is already locked
};

} // namespace vistle
#endif

#ifndef RESULTCACHE_SKIP_DEFINITION
#include "resultcache_impl.h"
#endif
