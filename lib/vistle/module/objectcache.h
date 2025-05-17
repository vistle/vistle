#ifndef VISTLE_MODULE_OBJECTCACHE_H
#define VISTLE_MODULE_OBJECTCACHE_H

#include <string>
#include <map>
#include <deque>

#include <vistle/util/enum.h>
#include <vistle/core/object.h>

namespace vistle {

typedef std::deque<vistle::Object::const_ptr> ObjectList;

class ObjectCache {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(CacheMode, (CacheNone)(CacheDeleteEarly)(CacheDeleteLate)(CacheByName))

    ObjectCache();
    ~ObjectCache();

    int generation() const;

    void clear();
    void clearOld();
    void clear(const std::string &portname);
    CacheMode cacheMode() const;
    void setCacheMode(CacheMode mode);

    void addObject(const std::string &portname, Object::const_ptr object);
    std::pair<std::map<std::string, ObjectList>, bool> getObjects() const;
    // keep track of connected ports
    void addPort(const std::string &portname);
    void removePort(const std::string &portname);

private:
    CacheMode m_cacheMode;

    int m_generation = 0;

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
    std::set<std::string> m_connectedPorts;
};

V_ENUM_OUTPUT_OP(CacheMode, ObjectCache)

} // namespace vistle
#endif
