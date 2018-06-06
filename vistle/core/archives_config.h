#ifndef VISTLE_ARCHIVES_CONFIG_H
#define VISTLE_ARCHIVES_CONFIG_H

//#define USE_INTROSPECTION_ARCHIVE
#define USE_BOOST_ARCHIVE
#define USE_YAS

#ifdef USE_INTROSPECTION_ARCHIVE
#ifndef USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE
#endif
#endif

#include <cstdlib>
#include <cassert>

namespace vistle {

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
    static ArrayWrapper<T> wrap_array(T *t, S n);

    template<class Archive>
    class StreamType;
};

template<class Base, class Derived, class Archive>
typename archive_helper<typename archive_tag<Archive>::type>::template BaseClass<Base> serialize_base(const Archive &, Derived &d) {
    return archive_helper<typename archive_tag<Archive>::type>::template serialize_base<Base>(d);
}

template<class Archive, class Object>
typename archive_helper<typename archive_tag<Archive>::type>::template NameValuePair<Object> make_nvp(const char *name, Object &d) {
    return archive_helper<typename archive_tag<Archive>::type>::make_nvp(name, d);
}

template<class Archive, class T, class S>
typename archive_helper<typename archive_tag<Archive>::type>::template ArrayWrapper<T> wrap_array(T *t, S n) {
    return archive_helper<typename archive_tag<Archive>::type>::wrap_array(t, n);
}

}
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
struct boost_tag {};
template<>
struct archive_tag<boost_oarchive> {
    typedef boost_tag type;
};
template<>
struct archive_tag<boost_iarchive> {
    typedef boost_tag type;
};

template <>
struct archive_helper<boost_tag> {
    template<class Base>
    using BaseClass = Base &;
    template<class Base, class Derived>
    static BaseClass<Base> serialize_base(Derived &d) {
        return boost::serialization::base_object<Base>(d);
    }

    template<class Object>
    using NameValuePair = const boost::serialization::nvp<Object>;
    template<class Object>
    static NameValuePair<Object> make_nvp(const char *name, Object &obj) {
        return boost::serialization::make_nvp(name, obj);
    }

    template<class T>
    using ArrayWrapper = const boost::serialization::array_wrapper<T>;
    template<class T, class S>
    static ArrayWrapper<T> wrap_array(T *t, S n) {
        return boost::serialization::make_array(t, n);
    }
};
}
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
    template <class Base>
    using BaseClass = Base &;
    template <class Base, class Derived>
    static BaseClass<Base> serialize_base(Derived &d) {
        return yas::base_object<Base>(d);
    }

    template<class Object>
    using NameValuePair = const Object &;
    template<class Object>
    static NameValuePair<Object> make_nvp(const char *name, Object &obj) {
        //return YAS_OBJECT_NVP(name, obj);
        return obj;
    }

    template<class T>
    struct ArrayWrapper {
        typedef T value_type;
        T *m_begin, *m_end;
        ArrayWrapper(T *begin, T *end)
        : m_begin(begin)
        , m_end(end)
        {}
        T *begin() { return m_begin; }
        const T *begin() const { return m_begin; }
        T *end() { return m_end; }
        const T *end() const { return m_end; }
        bool empty() const { return m_end == m_begin; }
        size_t size() const { return m_end - m_begin; }
        T &operator[](size_t idx) { return *(m_begin+idx); }
        const T &operator[](size_t idx) const { return *(m_begin+idx); }
        void push_back(const T &) { assert("not supported" == 0); }
        void resize(std::size_t sz) { if (size() != sz) { assert("not supported" == 0); } }

        template<class Archive>
        void serialize(Archive &ar) const {
            save(ar);
        }
        template<class Archive>
        void serialize(Archive &ar) {
            load(ar);
        }
        template<class Archive>
        void load(Archive &ar) {
            yas::detail::concepts::array::load<ar.yas_flags>(ar, *this);
        }
        template<class Archive>
        void save(Archive &ar) const {
            yas::detail::concepts::array::save<ar.yas_flags>(ar, *this);
        }
    };
    template<class T, class S>
    static ArrayWrapper<T> wrap_array(T *t, S n) {
        return ArrayWrapper<T>(t, t+n);
    }

    template<class Archive>
    using StreamType = typename ArchiveStreamType<Archive>::type;
};
}
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
}

#ifdef USE_BOOST_ARCHIVE
#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/type_traits/is_virtual_base_of.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/archive/archive_exception.hpp>

#define ARCHIVE_ASSUME_ABSTRACT_BOOST(obj) BOOST_SERIALIZATION_ASSUME_ABSTRACT(obj)
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
inline const
boost::serialization::nvp<T> vistle_make_nvp(const char *name, T &t) {
   std::cerr << "<" << name << ">" << std::endl;
   return boost::serialization::make_nvp<T>(name, t);
}

#define V_NAME(name, obj) \
   vistle_make_nvp(name, (obj)); std::cerr << "</" << name << ">" << std::endl;
#else
#if 0
#define V_NAME(name, obj) \
   boost::serialization::make_nvp(name, (obj))
#else
//#define V_NAME(ar, name, obj) vistle::make_nvp(ar, name, obj)
#define V_NAME(ar, name, obj) (obj)
#endif
#endif

#ifdef USE_BOOST_ARCHIVE
#define ARCHIVE_ACCESS_BOOST \
    friend class boost::serialization::access;
#else
#define ARCHIVE_ACCESS_BOOST
#endif
#if defined(USE_BOOST_ARCHIVE)
#define ARCHIVE_FORWARD_SERIALIZE \
    template<class Archive> \
    void serialize(Archive &ar, const unsigned int /* version */) { \
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

#ifdef USE_BOOST_ARCHIVE
#define BOOST_ARCHIVE_ACCESS_SPLIT \
    friend class boost::serialization::access; \
    template<class Archive> \
    void serialize(Archive &ar, const unsigned int version) { \
        boost::serialization::split_member(ar, *this, version); \
    } \
   template<class Archive> \
   void save(Archive &ar, const unsigned int version) const { \
       save(ar); \
   } \
   template<class Archive> \
   void load(Archive &ar, const unsigned int version) { \
       load(ar); \
   }
#else
#define BOOST_ARCHIVE_ACCESS_SPLIT
#endif
#define YAS_ARCHIVE_ACCESS_SPLIT \
    public: \
    template<class Archive> \
    void serialize(Archive &ar) const { \
        save(ar); \
    } \
    template<class Archive> \
    void serialize(Archive &ar) { \
        load(ar); \
    }

#define ARCHIVE_ACCESS_SPLIT public: BOOST_ARCHIVE_ACCESS_SPLIT YAS_ARCHIVE_ACCESS_SPLIT

#ifdef USE_BOOST_ARCHIVE
#define ARCHIVE_REGISTRATION_BOOST(override) \
   virtual void saveToArchive(boost_oarchive &ar) const override; \
   virtual void loadFromArchive(boost_iarchive &ar) override;
#define ARCHIVE_REGISTRATION_BOOST_INLINE \
   void saveToArchive(boost_oarchive &ar) const override { this->serialize(ar); } \
   void loadFromArchive(boost_iarchive &ar) override { this->serialize(ar); }
#define ARCHIVE_REGISTRATION_BOOST_IMPL(ObjType, prefix) \
   prefix \
   void ObjType::saveToArchive(boost_oarchive &ar) const { \
      this->serialize(ar); \
   } \
   prefix \
   void ObjType::loadFromArchive(boost_iarchive &ar) { \
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
   void saveToArchive(yas_oarchive &ar) const override { \
      this->serialize(ar); \
   } \
   void loadFromArchive(yas_iarchive &ar) override { \
      this->serialize(ar); \
   }
#define ARCHIVE_REGISTRATION_YAS_IMPL(ObjType, prefix) \
   prefix \
   void ObjType::saveToArchive(yas_oarchive &ar) const { \
      this->serialize(ar); \
   } \
   prefix \
   void ObjType::loadFromArchive(yas_iarchive &ar) { \
      this->serialize(ar); \
   }
#else
#define ARCHIVE_REGISTRATION_YAS(override)
#define ARCHIVE_REGISTRATION_YAS_INLINE
#define ARCHIVE_REGISTRATION_YAS_IMPL(ObjType, prefix)
#endif
#ifdef USE_INTROSPECTION_ARCHIVE
#define ARCHIVE_REGISTRATION_INTROSPECT \
   void save(FindObjectReferenceOArchive &ar) const override { const_cast<ObjType *>(this)->serialize(ar, 0); }
#else
#define ARCHIVE_REGISTRATION_INTROSPECT
#endif
#define ARCHIVE_REGISTRATION(override) ARCHIVE_REGISTRATION_BOOST(override) ARCHIVE_REGISTRATION_YAS(override) ARCHIVE_REGISTRATION_INTROSPECT
#define ARCHIVE_REGISTRATION_INLINE ARCHIVE_REGISTRATION_BOOST_INLINE ARCHIVE_REGISTRATION_YAS_INLINE ARCHIVE_REGISTRATION_INTROSPECT
#define ARCHIVE_REGISTRATION_IMPL(ObjType,prefix) ARCHIVE_REGISTRATION_BOOST_IMPL(ObjType,prefix) ARCHIVE_REGISTRATION_YAS_IMPL(ObjType,prefix)

#endif
