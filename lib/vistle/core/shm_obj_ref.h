#ifndef VISTLE_CORE_SHM_OBJ_REF_H
#define VISTLE_CORE_SHM_OBJ_REF_H

#include <string>
#include "shmname.h"
#include "archives_config.h"
#ifndef NO_SHMEM
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/offset_ptr.hpp>
#endif

namespace vistle {

template<class T>
class shm_obj_ref {
    typedef T ObjType;
    typedef typename ObjType::Data ObjData;

public:
    shm_obj_ref();
    shm_obj_ref(const std::string &name, ObjType *p);
    shm_obj_ref(const shm_obj_ref &other);
    ~shm_obj_ref();

    template<typename... Args>
    static shm_obj_ref create(const Args &...args);

    bool find();

    template<typename... Args>
    void construct(const Args &...args);

    const shm_obj_ref &operator=(const shm_obj_ref &rhs);
    const shm_obj_ref &operator=(typename ObjType::const_ptr rhs);
    const shm_obj_ref &operator=(typename ObjType::ptr rhs);
    const shm_obj_ref &operator=(const typename ObjType::Data *rhs);
    bool valid() const;
    typename ObjType::const_ptr getObject() const;
    const typename ObjType::Data *getData() const;
    operator bool() const;
    const shm_name_t &name() const;

private:
    shm_name_t m_name;
#ifdef NO_SHMEM
    const ObjData *m_d;
#else
    boost::interprocess::offset_ptr<const ObjData> m_d;
#endif

    void ref();
    void unref();

    ARCHIVE_ACCESS_SPLIT
    template<class Archive>
    void save(Archive &ar) const;
    template<class Archive>
    void load(Archive &ar);
};
} // namespace vistle

#include "shm_obj_ref_impl.h"

#endif
