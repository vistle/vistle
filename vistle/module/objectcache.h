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
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(CacheMode, (CacheDefault)(CacheNone)(CacheDeleteEarly)(CacheDeleteLate))

    ObjectCache();
    ~ObjectCache();

    void clear();
    void clearOld();
    void clear(const std::string &portname);
    CacheMode cacheMode() const;
    void setCacheMode(CacheMode mode);

    void addObject(const std::string &portname, Object::const_ptr object);
    const ObjectList &getObjects(const std::string &portname) const;

private:
    CacheMode m_cacheMode;
    std::map<std::string, ObjectList> m_cache, m_oldCache;
    ObjectList m_emptyList;
};

V_ENUM_OUTPUT_OP(CacheMode, ObjectCache)

} // namespace vistle
#endif
