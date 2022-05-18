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

#ifdef HAVE_ZFP
#include <zfp.h>
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <vistle/util/enum.h>
#include <vistle/util/buffer.h>

#include "export.h"
#include "index.h"

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(FieldCompressionMode,
                                    (Uncompressed)(Predict)(ZfpFixedRate)(ZfpAccuracy)(ZfpPrecision))

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
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/array_optimization.hpp>
#endif

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

#if BOOST_VERSION > 105500
    template<class T>
    using ArrayWrapper = const boost::serialization::array_wrapper<T>;
#else
    template<class T>
    using ArrayWrapper = const boost::serialization::array<T>;
#endif
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

#ifdef HAVE_ZFP
struct ZfpParameters {
    FieldCompressionMode mode = Uncompressed;
    double rate = 8.;
    int precision = 20;
    double accuracy = 1e-20;
};

template<typename T>
struct zfp_type_map {
    static const zfp_type value = zfp_type_none;
};

template<>
struct zfp_type_map<int32_t> {
    static const zfp_type value = zfp_type_int32;
};
template<>
struct zfp_type_map<int64_t> {
    static const zfp_type value = zfp_type_int64;
};
template<>
struct zfp_type_map<float> {
    static const zfp_type value = zfp_type_float;
};
template<>
struct zfp_type_map<double> {
    static const zfp_type value = zfp_type_double;
};

template<zfp_type type>
bool compressZfp(buffer &compressed, const void *src, const Index dim[3], Index typeSize, const ZfpParameters &param);
template<zfp_type type>
bool decompressZfp(void *dest, const buffer &compressed, const Index dim[3]);
#endif
template<typename S>
struct Xor {
    S operator()(S prev, S cur) { return cur ^ prev; }
};
template<typename S>
struct Add {
    S operator()(S prev, S cur) { return cur + prev; }
};
template<typename S>
struct Sub {
    S operator()(S prev, S cur) { return cur - prev; }
};
template<typename S>
struct Ident {
    S operator()(S prev, S cur) { return cur; }
};

template<typename T>
struct PredictTransform {
    using type = T;
    using Op = Ident<T>;
    using Rop = Op;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<float> {
    using type = uint32_t;
    using Op = Xor<type>;
    using Rop = Op;
    static constexpr bool use = false;
};

template<>
struct PredictTransform<double> {
    using type = uint64_t;
    using Op = Xor<type>;
    using Rop = Op;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<unsigned char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<signed char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = false;
};

template<>
struct PredictTransform<unsigned int> {
    using type = uint32_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};

template<>
struct PredictTransform<unsigned long int> {
    using type = uint64_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};
template<class T, class Op, bool reverse>
struct TransformStream {
    typedef typename PredictTransform<T>::type Int;

    Int prev;
    bool first = true;
    Int operator()(const T &t)
    {
        if (first) {
            first = false;
            prev = *reinterpret_cast<const Int *>(&t);
            return prev;
        }
        Int result = Op()(prev, *reinterpret_cast<const Int *>(&t));
        prev = reverse ? result : *reinterpret_cast<const Int *>(&t);
        return result;
    }
};

template<class T>
struct TransformStream<T, Ident<T>, false> {
    typedef T Int;
    Int operator()(const T &t) { return t; }
};
template<class T>
struct TransformStream<T, Ident<T>, true> {
    typedef T Int;
    Int operator()(const T &t) { return t; }
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
        typedef TransformStream<T, typename PredictTransform<T>::Op, false> CompressStream;
        typedef TransformStream<T, typename PredictTransform<T>::Rop, true> DecompressStream;

        T *m_begin, *m_end;
        Index m_dim[3] = {0, 1, 1};
        bool m_exact = true;

        ArrayWrapper(T *begin, T *end): m_begin(begin), m_end(end) {}
        T *begin() { return m_begin; }
        const T *begin() const { return m_begin; }
        T *end() { return m_end; }
        const T *end() const { return m_end; }
        bool empty() const { return m_end == m_begin; }
        Index size() const { return (Index)(m_end - m_begin); }
        T &operator[](size_t idx) { return *(m_begin + idx); }
        const T &operator[](size_t idx) const { return *(m_begin + idx); }
        void push_back(const T &) { assert("not supported" == 0); }
        void resize(std::size_t sz)
        {
            if (size() != sz) {
                std::cerr << "requesting resize from " << size() << " to " << sz << std::endl;
                assert("not supported" == 0);
            }
        }
        void setDimensions(size_t sx, size_t sy, size_t sz)
        {
            m_dim[0] = (Index)sx;
            m_dim[1] = (Index)sy;
            m_dim[2] = (Index)sz;
        }
        void setExact(bool exact) { m_exact = exact; }

        template<class Archive>
        void serialize(Archive &ar) const
        {
            save(ar);
        }
        template<class Archive>
        void serialize(Archive &ar)
        {
            load(ar);
        }
        template<class Archive>
        void load(Archive &ar)
        {
            bool compPredict = false;
            bool compZfp = false;
            bool compress = false;
            ar &compress;
            if (compress) {
                ar &compZfp;
                compPredict = !compZfp;
            }
            if (compPredict) {
                yas::detail::concepts::array::load<yas_flags>(ar, *this);
                std::transform(m_begin, m_end, m_begin, DecompressStream());
            } else if (compZfp) {
                ar &m_dim[0] & m_dim[1] & m_dim[2];
                buffer compressed;
                ar &compressed;
#ifdef HAVE_ZFP
                Index dim[3];
                for (int c = 0; c < 3; ++c)
                    dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
                decompressZfp<zfp_type_map<T>::value>(static_cast<void *>(m_begin), compressed, dim);
#endif
            } else {
                yas::detail::concepts::array::load<yas_flags>(ar, *this);
            }
        }

        template<class Archive>
        void save(Archive &ar) const
        {
            bool compPredict = PredictTransform<T>::use &&
                               (ar.compressionMode() == Predict || (m_exact && ar.compressionMode() != Uncompressed));
            bool compZfp = !compPredict && ar.compressionMode() != Uncompressed && !m_exact;
            bool compress = compPredict || compZfp;
            //std::cerr << "ar.compressed()=" << compress << std::endl;
            if (compPredict) {
                ar &compress;
                ar & false;
                std::vector<T> diff;
                diff.reserve(size());
                std::transform(m_begin, m_end, std::back_inserter(diff), CompressStream());
                yas::detail::concepts::array::save<yas_flags>(ar, diff);
            } else if (compZfp) {
#ifdef HAVE_ZFP
                ZfpParameters param;
                param.mode = ar.compressionMode();
                param.rate = ar.zfpRate();
                param.precision = ar.zfpPrecision();
                param.accuracy = ar.zfpAccuracy();
                //std::cerr << "trying to compresss " << std::endl;
                buffer compressed;
                Index dim[3];
                for (int c = 1; c < 3; ++c)
                    dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
                if (compressZfp<zfp_type_map<T>::value>(compressed, static_cast<const void *>(m_begin), dim,
                                                        sizeof(*m_begin), param)) {
                    ar &compress;
                    ar & true;
                    ar &m_dim[0] & m_dim[1] & m_dim[2];
                    ar &compressed;
                } else {
                    std::cerr << "compression failed" << std::endl;
                    compZfp = false;
                    compress = false;
                }
#else
                compZfp = false;
                compress = false;
#endif
            }
            if (!compress) {
                ar &compress;
                yas::detail::concepts::array::save<yas_flags>(ar, *this);
            }
        }
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

#ifdef HAVE_ZFP
template<>
bool V_COREEXPORT decompressZfp<zfp_type_none>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_int32>(void *dest, const buffer &compressed,
                                                                const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_int64>(void *dest, const buffer &compressed,
                                                                const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_float>(void *dest, const buffer &compressed,
                                                                const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_double>(void *dest, const buffer &compressed,
                                                                 const Index dim[3]);

template<>
bool V_COREEXPORT compressZfp<zfp_type_none>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                             const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_int32>(buffer &compressed, const void *src, const Index dim[3],
                                                              Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_int64>(buffer &compressed, const void *src, const Index dim[3],
                                                              Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_float>(buffer &compressed, const void *src, const Index dim[3],
                                                              Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_double>(buffer &compressed, const void *src, const Index dim[3],
                                                               Index typeSize, const ZfpParameters &param);
#endif
} // namespace detail
#ifdef HAVE_ZFP
using detail::ZfpParameters;
using detail::compressZfp;
using detail::decompressZfp;
#endif
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
#include <yas/std_types.hpp>
#include <yas/std_traits.hpp>
#include <yas/std_streams.hpp>
#include <yas/boost_types.hpp>

#endif

//#define DEBUG_SERIALIZATION
#ifdef DEBUG_SERIALIZATION
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
