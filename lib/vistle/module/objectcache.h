#ifndef OBJECTCACHE_H
#define OBJECTCACHE_H

#include <string>
#include <map>
#include <deque>

#include <vistle/util/enum.h>
#include <vistle/core/object.h>

namespace vistle {

typedef std::deque<vistle::Object::const_ptr> ObjectList;

class ObjectCache {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(CacheMode,
                                        (CacheDefault)(CacheNone)(CacheByName)(CacheDeleteEarly)(CacheDeleteLate))

    ObjectCache();
    ~ObjectCache();

    void clear();
    void clearOld();
    void clear(const std::string &portname);
    CacheMode cacheMode() const;
    void setCacheMode(CacheMode mode);

    void addObject(const std::string &portname, Object::const_ptr object);
    std::pair<ObjectList, bool> getObjects(const std::string &portname) const;

private:
    CacheMode m_cacheMode;

    struct Entry {
        Entry(Object::const_ptr object, bool cacheByNameOnly);
        std::string name;
        Object::const_ptr object;
        int block;
        int timestep;
        int iteration;
    };
    typedef std::vector<Entry> EntryList;
    std::map<std::string, EntryList> m_cache, m_oldCache;

    std::map<std::string, Meta> m_meta;
    ObjectList m_emptyList;
};

V_ENUM_OUTPUT_OP(CacheMode, ObjectCache)

} // namespace vistle
#endif
