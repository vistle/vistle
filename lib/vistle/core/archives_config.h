#ifndef VISTLE_ARCHIVES_CONFIG_H
#define VISTLE_ARCHIVES_CONFIG_H

//#define USE_INTROSPECTION_ARCHIVE
//#define USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE_MPI
#define USE_YAS

#ifdef USE_INTROSPECTION_ARCHIVE
#ifndef USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE
#endif
#endif

#ifdef USE_BOOST_ARCHIVE
#ifndef USE_BOOST_ARCHIVE_MPI
#define USE_BOOST_ARCHIVE_MPI
#endif
#endif

#include <cstdlib>
#include <vector>

#include <vistle/util/buffer.h>

#include "export.h"
#include "index.h"
#include "archives_compression_settings.h"

namespace vistle {

namespace detail {
template<class Archive>
struct archive_tag;

template<class tag>
struct archive_helper {
    template<class Base>
    class BaseClass;
    template<class Base, class Derived>
    static BaseClass<Base> serialize_base(Derived &d);

    template<class Object>
    class NameValuePair;
    template<class Object>
    static NameValuePair<Object> make_nvp(const char *name, Object &d);

    template<class T>
    class ArrayWrapper;
    template<class T, class S>
    static ArrayWrapper<T> wrap_array(T *t, bool exact, S nx, S ny = 1, S nz = 1);

    template<class Archive>
    class StreamType;
};

template<class Base, class Derived, class Archive>
typename archive_helper<typename archive_tag<Archive>::type>::template BaseClass<Base> serialize_base(const Archive &,
                                                                                                      Derived &d)
{
    return archive_helper<typename archive_tag<Archive>::type>::template serialize_base<Base>(d);
}

template<class Archive, class Object>
typename archive_helper<typename archive_tag<Archive>::type>::template NameValuePair<Object> make_nvp(const char *name,
                                                                                                      Object &d)
{
    return archive_helper<typename archive_tag<Archive>::type>::make_nvp(name, d);
}

template<class Archive, class T, class S>
typename archive_helper<typename archive_tag<Archive>::type>::template ArrayWrapper<T>
wrap_array(T *t, bool exact, S nx, S ny = 1, S nz = 1)
{
    return archive_helper<typename archive_tag<Archive>::type>::wrap_array(t, exact, nx, ny, nz);
}

} // namespace detail
} // namespace vistle

#ifdef USE_BOOST_ARCHIVE
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/array_optimization.hpp>

namespace vistle {
class boost_oarchive;
class boost_iarchive;

namespace detail {

struct boost_tag {};
template<>
struct archive_tag<boost_oarchive> {
    typedef boost_tag type;
};
template<>
struct archive_tag<boost_iarchive> {
    typedef boost_tag type;
};

template<>
struct archive_helper<boost_tag> {
    template<class Base>
    using BaseClass = Base &;
    template<class Base, class Derived>
    static BaseClass<Base> serialize_base(Derived &d)
    {
        return boost::serialization::base_object<Base>(d);
    }

    template<class Object>
    using NameValuePair = const boost::serialization::nvp<Object>;
    template<class Object>
    static NameValuePair<Object> make_nvp(const char *name, Object &obj)
    {
        return boost::serialization::make_nvp(name, obj);
    }

    template<class T>
    using ArrayWrapper = const boost::serialization::array_wrapper<T>;
    template<class T, class S>
    static ArrayWrapper<T> wrap_array(T *t, bool exact, S nx, S ny, S nz)
    {
        return boost::serialization::make_array(t, nx * ny * nz);
    }
};
} // namespace detail
} // namespace vistle
#endif

#ifdef USE_YAS
#include <yas/binary_iarchive.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/serialize.hpp>
#include <yas/boost_types.hpp>
#include <yas/types/concepts/array.hpp>

namespace vistle {

class yas_oarchive;
class yas_iarchive;

namespace detail {
const std::size_t yas_flags = yas::binary | yas::ehost;

struct yas_tag {};
template<>
struct archive_tag<yas_oarchive> {
    typedef yas_tag type;
};
template<>
struct archive_tag<yas_iarchive> {
    typedef yas_tag type;
};

template<class Archive>
struct ArchiveStreamType;

template<>
struct ArchiveStreamType<yas_iarchive> {
    typedef yas::mem_istream type;
};
template<>
struct ArchiveStreamType<yas_oarchive> {
    typedef yas::mem_ostream type;
};


template<>
struct archive_helper<yas_tag> {
    template<class Base>
    using BaseClass = Base &;
    template<class Base, class Derived>
    static BaseClass<Base> serialize_base(Derived &d)
    {
        return yas::base_object<Base>(d);
    }

    template<class Object>
    using NameValuePair = const Object &;
    template<class Object>
    static NameValuePair<Object> make_nvp(const char *name, Object &obj)
    {
        //return YAS_OBJECT_NVP(name, obj);
        return obj;
    }


    template<class T>
    struct ArrayWrapper {
        typedef T value_type;
        typedef T &reference;

        T *m_begin, *m_end;
        Index m_dim[3] = {0, 1, 1};
        bool m_exact = true;

        ArrayWrapper(T *begin, T *end);
        T *begin();
        const T *begin() const;
        T *end();
        const T *end() const;
        bool empty() const;
        Index size() const;
        T &operator[](size_t idx);
        const T &operator[](size_t idx) const;
        void push_back(const T &);
        void resize(std::size_t sz);
        void setDimensions(size_t sx, size_t sy, size_t sz);
        void setExact(bool exact);

        template<class Archive>
        void serialize(Archive &ar) const;
        template<class Archive>
        void serialize(Archive &ar);
        template<class Archive>
        void load(Archive &ar);
        template<class Archive>
        void save(Archive &ar) const;
    };

    template<class T, class S>
    static ArrayWrapper<T> wrap_array(T *t, bool exact, S nx, S ny, S nz)
    {
        ArrayWrapper<T> wrap(t, t + nx * ny * nz);
        wrap.setExact(exact);
        wrap.setDimensions(nx, ny, nz);
        return wrap;
    }

    template<class Archive>
    using StreamType = typename ArchiveStreamType<Archive>::type;
};

} // namespace detail
} // namespace vistle
#endif

namespace vistle {
#ifdef USE_INTROSPECTION_ARCHIVE
class FindObjectReferenceOArchive;
#endif

#if defined(USE_YAS)
typedef yas_oarchive oarchive;
typedef yas_iarchive iarchive;
#elif defined(USE_BOOST_ARCHIVE)
typedef boost_oarchive oarchive;
typedef boost_iarchive iarchive;
#endif

using detail::serialize_base;
} // namespace vistle

#ifdef USE_BOOST_ARCHIVE_MPI
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#endif
#ifdef USE_BOOST_ARCHIVE
#include <boost/serialization/assume_abstract.hpp>
#include <boost/archive/archive_exception.hpp>

#define ARCHIVE_ASSUME_ABSTRACT_BOOST(obj) BOOST_SERIALIZATION_ASSUME_ABSTRACT(obj)
#else
#define ARCHIVE_ASSUME_ABSTRACT_BOOST(obj)
#endif

#define ARCHIVE_ASSUME_ABSTRACT_YAS(obj)
#ifdef USE_YAS
#include <yas/serialize.hpp>
#include <yas/object.hpp>
#include <yas/types/std/set.hpp>
#include <yas/std_traits.hpp>
#include <yas/std_streams.hpp>
#include <yas/boost_types.hpp>

#endif

//#define DEBUG_SERIALIZATION
#ifdef DEBUG_SERIALIZATION
#include <iostream>

template<class T>
inline const boost::serialization::nvp<T> vistle_make_nvp(const char *name, T &t)
{
    std::cerr << "<" << name << ">" << std::endl;
    return boost::serialization::make_nvp<T>(name, t);
}

#define V_NAME(name, obj) \
    vistle_make_nvp(name, (obj)); \
    std::cerr << "</" << name << ">" << std::endl;
#else
#if 0
#define V_NAME(name, obj) boost::serialization::make_nvp(name, (obj))
#else
//#define V_NAME(ar, name, obj) vistle::make_nvp(ar, name, obj)
#define V_NAME(ar, name, obj) (obj)
#endif
#endif

#ifdef USE_BOOST_ARCHIVE_MPI
#define ARCHIVE_ACCESS_BOOST friend class boost::serialization::access;
#else
#define ARCHIVE_ACCESS_BOOST
#endif
#if defined(USE_BOOST_ARCHIVE_MPI)
#define ARCHIVE_FORWARD_SERIALIZE \
    template<class Archive> \
    void serialize(Archive &ar, const unsigned int /* version */) \
    { \
        serialize(ar); \
    }
#else
#define ARCHIVE_FORWARD_SERIALIZE
#endif
#ifdef USE_YAS
#define ARCHIVE_ACCESS_YAS public:
#else
#define ARCHIVE_ACCESS_YAS
#endif
#define ARCHIVE_ACCESS ARCHIVE_ACCESS_BOOST ARCHIVE_ACCESS_YAS ARCHIVE_FORWARD_SERIALIZE
#define ARCHIVE_ASSUME_ABSTRACT(obj) ARCHIVE_ASSUME_ABSTRACT_BOOST(obj) ARCHIVE_ASSUME_ABSTRACT_YAS(obj)

#ifdef USE_BOOST_ARCHIVE_MPI
#define BOOST_ARCHIVE_ACCESS_SPLIT \
    friend class boost::serialization::access; \
    template<class Archive> \
    void serialize(Archive &ar, const unsigned int version) \
    { \
        boost::serialization::split_member(ar, *this, version); \
    } \
    template<class Archive> \
    void save(Archive &ar, const unsigned int version) const \
    { \
        save(ar); \
    } \
    template<class Archive> \
    void load(Archive &ar, const unsigned int version) \
    { \
        load(ar); \
    }
#else
#define BOOST_ARCHIVE_ACCESS_SPLIT
#endif
#define YAS_ARCHIVE_ACCESS_SPLIT \
public: \
    template<class Archive> \
    void serialize(Archive &ar) const \
    { \
        save(ar); \
    } \
    template<class Archive> \
    void serialize(Archive &ar) \
    { \
        load(ar); \
    }

#define ARCHIVE_ACCESS_SPLIT \
public: \
    BOOST_ARCHIVE_ACCESS_SPLIT YAS_ARCHIVE_ACCESS_SPLIT

#ifdef USE_BOOST_ARCHIVE
#define ARCHIVE_REGISTRATION_BOOST(override) \
    virtual void saveToArchive(boost_oarchive &ar) const override; \
    virtual void loadFromArchive(boost_iarchive &ar) override;
#define ARCHIVE_REGISTRATION_BOOST_INLINE \
    void saveToArchive(boost_oarchive &ar) const override \
    { \
        this->serialize(ar); \
    } \
    void loadFromArchive(boost_iarchive &ar) override \
    { \
        this->serialize(ar); \
    }
#define ARCHIVE_REGISTRATION_BOOST_IMPL(ObjType, prefix) \
    prefix void ObjType::saveToArchive(boost_oarchive &ar) const \
    { \
        this->serialize(ar); \
    } \
    prefix void ObjType::loadFromArchive(boost_iarchive &ar) \
    { \
        this->serialize(ar); \
    }
#else
#define ARCHIVE_REGISTRATION_BOOST(override)
#define ARCHIVE_REGISTRATION_BOOST_INLINE
#define ARCHIVE_REGISTRATION_BOOST_IMPL(ObjType, prefix)
#endif

#ifdef USE_YAS
#define ARCHIVE_REGISTRATION_YAS(override) \
    virtual void saveToArchive(yas_oarchive &ar) const override; \
    virtual void loadFromArchive(yas_iarchive &ar) override;
#define ARCHIVE_REGISTRATION_YAS_INLINE \
    void saveToArchive(yas_oarchive &ar) const override \
    { \
        this->serialize(ar); \
    } \
    void loadFromArchive(yas_iarchive &ar) override \
    { \
        this->serialize(ar); \
    }
#define ARCHIVE_REGISTRATION_YAS_IMPL(ObjType, prefix) \
    prefix void ObjType::saveToArchive(yas_oarchive &ar) const \
    { \
        this->serialize(ar); \
    } \
    prefix void ObjType::loadFromArchive(yas_iarchive &ar) \
    { \
        this->serialize(ar); \
    }
#else
#define ARCHIVE_REGISTRATION_YAS(override)
#define ARCHIVE_REGISTRATION_YAS_INLINE
#define ARCHIVE_REGISTRATION_YAS_IMPL(ObjType, prefix)
#endif
#ifdef USE_INTROSPECTION_ARCHIVE
#define ARCHIVE_REGISTRATION_INTROSPECT \
    void save(FindObjectReferenceOArchive &ar) const override \
    { \
        const_cast<ObjType *>(this)->serialize(ar, 0); \
    }
#else
#define ARCHIVE_REGISTRATION_INTROSPECT
#endif
#define ARCHIVE_REGISTRATION(override) \
    ARCHIVE_REGISTRATION_BOOST(override) ARCHIVE_REGISTRATION_YAS(override) ARCHIVE_REGISTRATION_INTROSPECT
#define ARCHIVE_REGISTRATION_INLINE \
    ARCHIVE_REGISTRATION_BOOST_INLINE ARCHIVE_REGISTRATION_YAS_INLINE ARCHIVE_REGISTRATION_INTROSPECT
#define ARCHIVE_REGISTRATION_IMPL(ObjType, prefix) \
    ARCHIVE_REGISTRATION_BOOST_IMPL(ObjType, prefix) ARCHIVE_REGISTRATION_YAS_IMPL(ObjType, prefix)

#endif
