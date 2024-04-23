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
    typedef std::deque<std::string> NameList;
    std::map<std::string, ObjectList> m_cache, m_oldCache;
    std::map<std::string, NameList> m_nameCache;
    std::map<std::string, Meta> m_meta;
    ObjectList m_emptyList;
};

V_ENUM_OUTPUT_OP(CacheMode, ObjectCache)

} // namespace vistle
#endif
