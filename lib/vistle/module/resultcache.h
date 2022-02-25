#ifndef RESULT_CACHE_H
#define RESULT_CACHE_H

#include "export.h"

#include <string>
#include <map>
#include <mutex>

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
        Result data;
    };

    //! if available, retrieve value for key and store to result, return false otherwise
    bool tryGet(const std::string &key, Result &result);
    //! if available, retrieve value for key, store to result, and return nullptr;
    //! otherwise the entry corresponding to key is locked and has to be updated with storeAndUnlock via the returned Entry
    Entry *getOrLock(const std::string &key, Result &result);
    //! update value stored for entry with data and unlock it
    bool storeAndUnlock(Entry *entry, const Result &data);
    //! discard all currently stored values
    void clear() override;

protected:
    std::map<std::string, Entry> m_cache;
    Entry m_empty;
    std::mutex m_mutex;
};

} // namespace vistle
#endif

#ifndef RESULTCACHE_SKIP_DEFINITION
#include "resultcache_impl.h"
#endif
