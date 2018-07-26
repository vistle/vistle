#ifndef VISTLE_ARCHIVES_H
#define VISTLE_ARCHIVES_H

#include "export.h"
#include "archives_config.h"

#include "serialize.h"

#include <functional>
#include <vector>
#include <memory>
#include <iostream>
#include <util/vecstreambuf.h>

#ifdef USE_BOOST_ARCHIVE
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/archive/detail/oserializer.hpp>
#include <boost/archive/detail/iserializer.hpp>

#include <boost/archive/basic_binary_oprimitive.hpp>
#include <boost/archive/basic_binary_iprimitive.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#endif
#endif

#ifdef USE_YAS
#include <yas/mem_streams.hpp>
#endif

#include <boost/mpl/vector.hpp>

#include <util/vecstreambuf.h>
//#include "findobjectreferenceoarchive.h"
#include "shm_array.h"
#include "object.h"

namespace vistle {

template<class T>
class shm_obj_ref;
}

#ifdef USE_BOOST_ARCHIVE
namespace boost {
namespace archive {
extern template class V_COREEXPORT basic_binary_oprimitive<
    vistle::boost_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;
extern template class V_COREEXPORT basic_binary_iprimitive<
    vistle::boost_iarchive,
    std::istream::char_type, 
    std::istream::traits_type
>;
extern template class V_COREEXPORT detail::common_oarchive<vistle::boost_oarchive>;
} // namespace archive
} // namespace boost
#endif
#endif // VISTLE_ARCHIVES_H

#ifdef VISTLE_IMPL
#ifndef ARCHIVES_IMPL_H
#define ARCHIVES_IMPL_H

#include "shm.h"

namespace vistle {

class Object;
struct ObjectData;
typedef std::shared_ptr<Object> obj_ptr;
typedef std::shared_ptr<const Object> obj_const_ptr;

struct SubArchiveDirectoryEntry {
    std::string name;
    bool is_array;
    size_t size;
    char *data;

    SubArchiveDirectoryEntry(): is_array(false), size(0), data(nullptr) {}
    SubArchiveDirectoryEntry(const std::string &name, bool is_array, size_t size, char *data)
        : name(name), is_array(is_array), size(size), data(data) {}

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar) {
        ar & name;
        ar & is_array;
        ar & size;
    }
#if 0
    template<class Archive>
    void serialize(Archive &ar, unsigned version) {
        serialize(ar);
    }
#endif
};
typedef std::vector<SubArchiveDirectoryEntry> SubArchiveDirectory;

class V_COREEXPORT Saver {
public:
    virtual ~Saver();
    virtual void saveArray(const std::string &name, int type, const void *array) = 0;
    virtual void saveObject(const std::string &name, obj_const_ptr obj) = 0;
};


#ifdef USE_BOOST_ARCHIVE
class V_COREEXPORT boost_oarchive: public boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type> {

    typedef boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type> Base;
public:
    boost_oarchive(std::streambuf &bsb, unsigned int flags=0);
    ~boost_oarchive();

    void setSaver(std::shared_ptr<Saver> saver);

    template<class T>
    void saveArray(const vistle::ShmVector<T> &t) {
        if (m_saver)
            m_saver->saveArray(t.name(), t->type(), &t);
    }

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &t) {
        if (m_saver)
            m_saver->saveObject(t.name(), t.getObject());
    }

    std::shared_ptr<Saver> m_saver;
};

#endif

#ifdef USE_YAS
template<class Archive, typename OS, std::size_t F = yas_flags>
struct yas_binary_oarchive
    :yas::detail::binary_ostream<OS, F>
    ,yas::detail::oarchive_header<F>
{
    YAS_NONCOPYABLE(yas_binary_oarchive)
    YAS_MOVABLE(yas_binary_oarchive)

    using stream_type = OS;
    using this_type = Archive;

    yas_binary_oarchive(OS &os)
        :yas::detail::binary_ostream<OS, F>(os)
        ,yas::detail::oarchive_header<F>(os)
    {}

    template<typename T>
    this_type& operator& (const T &v) {
        using namespace yas::detail;
        return serializer<
             type_properties<T>::value
            ,serialization_method<T, this_type>::value
            ,F
            ,T
        >::save(*static_cast<this_type *>(this), v);
    }

    this_type& serialize() { return *static_cast<this_type *>(this); }

    template<typename Head, typename... Tail>
    this_type& serialize(const Head &head, const Tail&... tail) {
        return operator& (head).serialize(tail...);
    }

    template<typename... Args>
    this_type& operator()(const Args&... args) {
        return serialize(args...);
    }
};

class V_COREEXPORT yas_oarchive: public yas_binary_oarchive<yas_oarchive, vecostreambuf<char>> {

    typedef vecostreambuf<char> Stream;
    //typedef yas::mem_ostream Stream;
    typedef yas_binary_oarchive<yas_oarchive, Stream> Base;
    std::ostream * m_os = nullptr;
    std::streambuf *m_sbuf = nullptr;
    FieldCompressionMode m_compress = Uncompressed;
    double m_zfpRate = 8.;
    int m_zfpPrecision = 8.;
    double m_zfpAccuracy = 1e-20;

public:
    void setCompressionMode(vistle::FieldCompressionMode mode);
    FieldCompressionMode compressionMode() const;

    void setZfpRate(double rate);
    double zfpRate() const;
    void setZfpAccuracy(double rate);
    double zfpAccuracy() const;
    void setZfpPrecision(int precision);
    int zfpPrecision() const;

    typedef yas::mem_ostream stream_type;
    yas_oarchive(Stream &mo, unsigned int flags=0);

    void setSaver(std::shared_ptr<Saver> saver);

    template<class T>
    void saveArray(const vistle::ShmVector<T> &t) {
        if (m_saver)
            m_saver->saveArray(t.name(), t->type(), &t);
    }

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &t) {
        if (m_saver)
            m_saver->saveObject(t.name(), t.getObject());
    }

    std::shared_ptr<Saver> m_saver;
};
#endif


class V_COREEXPORT Fetcher {
public:
    virtual ~Fetcher();
    virtual void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) = 0;
    virtual void requestObject(const std::string &name, const std::function<void()> &completeCallback) = 0;
};


#ifdef USE_BOOST_ARCHIVE
class V_COREEXPORT boost_iarchive: public boost::archive::binary_iarchive_impl<boost_iarchive, std::istream::char_type, std::istream::traits_type> {

    typedef boost::archive::binary_iarchive_impl<boost_iarchive, std::istream::char_type, std::istream::traits_type> Base;
public:
    boost_iarchive(std::streambuf &bsb, unsigned int flags=0);
    ~boost_iarchive();

    void setFetcher(std::shared_ptr<Fetcher> fetcher);
    void setCurrentObject(ObjectData *data);
    ObjectData *currentObject() const;

    template<typename T>
    ShmVector<T> getArray(const std::string &name, const std::function<void()> &completeCallback) const {
        auto arr = Shm::the().getArrayFromName<T>(name);
        if (!arr) {
            assert(m_fetcher);
            m_fetcher->requestArray(name, shm<T>::array::typeId(), completeCallback);
            arr = Shm::the().getArrayFromName<T>(name);
        }
        return arr;
    }

    obj_const_ptr getObject(const std::string &name, const std::function<void()> &completeCallback) const;

    void setObjectCompletionHandler(const std::function<void()> &completer);
    const std::function<void()> &objectCompletionHandler() const;

private:
    std::shared_ptr<Fetcher> m_fetcher;
    ObjectData *m_currentObject = nullptr;
    std::function<void()> m_completer;
};
#endif

#ifdef USE_YAS
template<class Archive, typename IS, std::size_t F = yas_flags>
struct yas_binary_iarchive
    :yas::detail::binary_istream<IS, F>
    ,yas::detail::iarchive_header<F>
{
    YAS_NONCOPYABLE(yas_binary_iarchive)
    YAS_MOVABLE(yas_binary_iarchive)

    using stream_type = IS;
    using this_type = Archive;

    yas_binary_iarchive(IS &is)
        :yas::detail::binary_istream<IS, F>(is)
        ,yas::detail::iarchive_header<F>(is)
    {}

    template<typename T>
    this_type& operator& (T &&v) {
        using namespace yas::detail;
        using real_type = typename std::remove_reference<
            typename std::remove_const<T>::type
        >::type;
        return serializer<
             type_properties<real_type>::value
            ,serialization_method<real_type, this_type>::value
            ,F
            ,real_type
        >::load(*static_cast<this_type *>(this), v);
    }

    this_type& serialize() { return *static_cast<this_type *>(this); }

    template<typename Head, typename... Tail>
    this_type& serialize(Head &&head, Tail&&... tail) {
        return operator& (std::forward<Head>(head)).serialize(std::forward<Tail>(tail)...);
    }

    template<typename... Args>
    this_type& operator()(Args &&... args) {
        return serialize(std::forward<Args>(args)...);
    }
};

class V_COREEXPORT yas_iarchive: public yas_binary_iarchive<yas_iarchive, vecistreambuf<char>> {

    typedef vecistreambuf<char> Stream;
    typedef yas_binary_iarchive<yas_iarchive, Stream> Base;
public:
    yas_iarchive(Stream &mi, unsigned int flags=0);

    void setFetcher(std::shared_ptr<Fetcher> fetcher);
    void setCurrentObject(ObjectData *data);
    ObjectData *currentObject() const;

    template<typename T>
    ShmVector<T> getArray(const std::string &name, const std::function<void()> &completeCallback) const {
        auto arr = Shm::the().getArrayFromName<T>(name);
        if (!arr) {
            assert(m_fetcher);
            m_fetcher->requestArray(name, shm<T>::array::typeId(), completeCallback);
            arr = Shm::the().getArrayFromName<T>(name);
        }
        return arr;
    }

    obj_const_ptr getObject(const std::string &name, const std::function<void()> &completeCallback) const {
        auto obj = Shm::the().getObjectFromName(name);
        if (!obj) {
            assert(m_fetcher);
            m_fetcher->requestObject(name, completeCallback);
            obj = Shm::the().getObjectFromName(name);
        }
        return obj;
    }

    void setObjectCompletionHandler(const std::function<void()> &completer);
    const std::function<void()> &objectCompletionHandler() const;

private:
    std::shared_ptr<Fetcher> m_fetcher;
    ObjectData *m_currentObject = nullptr;
    std::function<void()> m_completer;
};
#endif

typedef boost::mpl::vector<
#ifdef USE_BOOST_ARCHIVE
   boost_iarchive
#ifdef USE_YAS
   , yas_iarchive
#endif
#else
#ifdef USE_YAS
   yas_iarchive
#endif
#endif
      > InputArchives;

typedef boost::mpl::vector<
#ifdef USE_BOOST_ARCHIVE
   boost_oarchive
#ifdef USE_YAS
   , yas_oarchive
#endif
#else
#ifdef USE_YAS
   yas_oarchive
#endif
#endif
      > OutputArchives;

} // namespace vistle


#ifdef USE_BOOST_ARCHIVE
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::boost_oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::boost_oarchive)
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::boost_iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::boost_iarchive)
#endif

#endif // ARCHIVES_IMPL_H
#endif // VISTLE_IMPL
