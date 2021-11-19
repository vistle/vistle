#ifndef VISTLE_ARCHIVES_H
#define VISTLE_ARCHIVES_H

#include "export.h"
#include "archives_config.h"

#include "serialize.h"

#include <functional>
#include <vector>
#include <memory>
#include <iostream>
#include <vistle/util/vecstreambuf.h>
#include <vistle/util/buffer.h>
#include "message.h"

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

#include <vistle/util/vecstreambuf.h>
//#include "findobjectreferenceoarchive.h"
#include "shm_array.h"
#include "object.h"

namespace vistle {

struct CompressionSettings {
    FieldCompressionMode m_compress = Uncompressed;
    double m_zfpRate = 8.;
    int m_zfpPrecision = 8;
    double m_zfpAccuracy = 1e-20;
};

template<class T>
class shm_obj_ref;
} // namespace vistle

#ifdef USE_BOOST_ARCHIVE
namespace boost {
namespace archive {
extern template class basic_binary_oprimitive<vistle::boost_oarchive, std::ostream::char_type,
                                              std::ostream::traits_type>;
extern template class basic_binary_iprimitive<vistle::boost_iarchive, std::istream::char_type,
                                              std::istream::traits_type>;
extern template class detail::common_oarchive<vistle::boost_oarchive>;
} // namespace archive
} // namespace boost
#endif
#endif // VISTLE_ARCHIVES_H

#ifndef ARCHIVES_IMPL_H
#define ARCHIVES_IMPL_H

#include "shm.h"

namespace vistle {

// repeated from core/object.h
class Object;
struct ObjectData;
typedef std::shared_ptr<Object> obj_ptr; // Object::ptr
typedef std::shared_ptr<const Object> obj_const_ptr; // Object::const_ptr
typedef std::function<void(const std::string &name)> ArrayCompletionHandler;
typedef std::function<void(obj_const_ptr)> ObjectCompletionHandler;

struct SubArchiveDirectoryEntry {
    std::string name;
    bool is_array = false;
    size_t size = 0, compressedSize = 0;
    char *data = nullptr;
    std::unique_ptr<buffer> storage;
    message::CompressionMode compression = message::CompressionNone;

    SubArchiveDirectoryEntry(): is_array(false), size(0), data(nullptr) {}
    SubArchiveDirectoryEntry(const std::string &name, bool is_array, size_t size, char *data)
    : name(name), is_array(is_array), size(size), data(data)
    {}

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &name;
        ar &is_array;
        ar &size;
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
class V_COREEXPORT boost_oarchive
: public boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type> {
    typedef boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type>
        Base;

public:
    boost_oarchive(std::streambuf &bsb, unsigned int flags = 0);
    ~boost_oarchive();

    void setCompressionMode(vistle::FieldCompressionMode mode);
    void setCompressionSettings(const CompressionSettings &other);

    void setSaver(std::shared_ptr<Saver> saver);

    template<class T>
    void saveArray(const vistle::ShmVector<T> &t)
    {
        if (m_saver)
            m_saver->saveArray(t.name(), t->type(), &t);
    }

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &t)
    {
        if (m_saver)
            m_saver->saveObject(t.name(), t.getObject());
    }

    std::shared_ptr<Saver> m_saver;
};

#endif

#ifdef USE_YAS
template<class Archive, typename OS, std::size_t F = detail::yas_flags>
struct yas_binary_oarchive: yas::detail::binary_ostream<OS, F>, yas::detail::oarchive_header<F> {
    YAS_NONCOPYABLE(yas_binary_oarchive)
    YAS_MOVABLE(yas_binary_oarchive)

    using stream_type = OS;
    using this_type = Archive;

    yas_binary_oarchive(OS &os): yas::detail::binary_ostream<OS, F>(os), yas::detail::oarchive_header<F>(os) {}

    template<typename T>
    this_type &operator&(const T &v)
    {
        using namespace yas::detail;
        return serializer<type_properties<T>::value, serialization_method<T, this_type>::value, F, T>::save(
            *static_cast<this_type *>(this), v);
    }

    this_type &serialize() { return *static_cast<this_type *>(this); }

    template<typename Head, typename... Tail>
    this_type &serialize(const Head &head, const Tail &...tail)
    {
        return operator&(head).serialize(tail...);
    }

    template<typename... Args>
    this_type &operator()(const Args &...args)
    {
        return serialize(args...);
    }
};

class V_COREEXPORT yas_oarchive: public yas_binary_oarchive<yas_oarchive, vecostreambuf<vistle::buffer>>,
                                 public CompressionSettings {
    typedef vecostreambuf<vistle::buffer> Stream;
    typedef yas_binary_oarchive<yas_oarchive, Stream> Base;
    FieldCompressionMode m_compress = Uncompressed;
    double m_zfpRate = 8.;
    int m_zfpPrecision = 8;
    double m_zfpAccuracy = 1e-20;
    std::shared_ptr<Saver> m_saver;
    Stream &m_os;

public:
    void setCompressionMode(vistle::FieldCompressionMode mode);
    FieldCompressionMode compressionMode() const;

    void setZfpRate(double rate);
    double zfpRate() const;
    void setZfpAccuracy(double rate);
    double zfpAccuracy() const;
    void setZfpPrecision(int precision);
    int zfpPrecision() const;

    void setCompressionSettings(const CompressionSettings &other);

    yas_oarchive(Stream &mo, unsigned int flags = 0);

    void setSaver(std::shared_ptr<Saver> saver);

    template<class T>
    void saveArray(const vistle::ShmVector<T> &t)
    {
        if (m_saver) {
            std::size_t sz = m_os.get_vector().capacity();
            m_os.get_vector().reserve(sz + sizeof(T) * t->size() + 10000);
            m_saver->saveArray(t.name(), t->type(), &t);
        }
    }

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &t)
    {
        if (m_saver)
            m_saver->saveObject(t.name(), t.getObject());
    }
};
#endif


class V_COREEXPORT Fetcher {
public:
    virtual ~Fetcher();
    virtual void requestArray(const std::string &name, int type, const ArrayCompletionHandler &completeCallback) = 0;
    virtual void requestObject(const std::string &name, const ObjectCompletionHandler &completeCallback) = 0;

    virtual bool renameObjects() const;
    virtual std::string translateObjectName(const std::string &name) const;
    virtual std::string translateArrayName(const std::string &name) const;
    virtual void registerObjectNameTranslation(const std::string &arname, const std::string &name);
    virtual void registerArrayNameTranslation(const std::string &arname, const std::string &name);
};


#ifdef USE_BOOST_ARCHIVE
class V_COREEXPORT boost_iarchive
: public boost::archive::binary_iarchive_impl<boost_iarchive, std::istream::char_type, std::istream::traits_type> {
    typedef boost::archive::binary_iarchive_impl<boost_iarchive, std::istream::char_type, std::istream::traits_type>
        Base;

public:
    boost_iarchive(std::streambuf &bsb, unsigned int flags = 0);
    ~boost_iarchive();

    std::string translateObjectName(const std::string &name) const;
    std::string translateArrayName(const std::string &name) const;
    void registerObjectNameTranslation(const std::string &arname, const std::string &name) const;
    void registerArrayNameTranslation(const std::string &arname, const std::string &name) const;

    void setFetcher(std::shared_ptr<Fetcher> fetcher);
    void setCurrentObject(ObjectData *data);
    ObjectData *currentObject() const;
    std::shared_ptr<Fetcher> fetcher() const;

    template<typename T>
    void fetchArray(const std::string &arname, const ArrayCompletionHandler &completeCallback) const
    {
        std::string name = translateArrayName(arname);
        if (!name.empty()) {
            if (auto arr = Shm::the().getArrayFromName<T>(name)) {
                if (completeCallback)
                    completeCallback(name);
                return;
            }
        }
        assert(m_fetcher);
        m_fetcher->requestArray(arname, shm<T>::array::typeId(), completeCallback);
    }

    obj_const_ptr getObject(const std::string &name,
                            const std::function<void(Object::const_ptr)> &completeCallback) const;
    void setObjectCompletionHandler(const std::function<void()> &completer);
    const std::function<void()> &objectCompletionHandler() const;

private:
    bool m_rename = false;
    std::shared_ptr<Fetcher> m_fetcher;
    ObjectData *m_currentObject = nullptr;
    std::function<void()> m_completer;
    std::map<std::string, std::string> m_transObject, m_transArray;
};
#endif

#ifdef USE_YAS
template<class Archive, typename IS, std::size_t F = detail::yas_flags>
struct yas_binary_iarchive: yas::detail::binary_istream<IS, F>, yas::detail::iarchive_header<F> {
    YAS_NONCOPYABLE(yas_binary_iarchive)
    YAS_MOVABLE(yas_binary_iarchive)

    using stream_type = IS;
    using this_type = Archive;

    yas_binary_iarchive(IS &is): yas::detail::binary_istream<IS, F>(is), yas::detail::iarchive_header<F>(is) {}

    template<typename T>
    this_type &operator&(T &&v)
    {
        using namespace yas::detail;
        using real_type = typename std::remove_reference<typename std::remove_const<T>::type>::type;
        return serializer<type_properties<real_type>::value, serialization_method<real_type, this_type>::value, F,
                          real_type>::load(*static_cast<this_type *>(this), v);
    }

    this_type &serialize() { return *static_cast<this_type *>(this); }

    template<typename Head, typename... Tail>
    this_type &serialize(Head &&head, Tail &&...tail)
    {
        return operator&(std::forward<Head>(head)).serialize(std::forward<Tail>(tail)...);
    }

    template<typename... Args>
    this_type &operator()(Args &&...args)
    {
        return serialize(std::forward<Args>(args)...);
    }
};

class V_COREEXPORT yas_iarchive: public yas_binary_iarchive<yas_iarchive, vecistreambuf<vistle::buffer>> {
    typedef vecistreambuf<vistle::buffer> Stream;
    typedef yas_binary_iarchive<yas_iarchive, Stream> Base;

public:
    yas_iarchive(Stream &mi, unsigned int flags = 0);
    ~yas_iarchive();

    std::string translateObjectName(const std::string &name) const;
    std::string translateArrayName(const std::string &name) const;
    void registerObjectNameTranslation(const std::string &arname, const std::string &name) const;
    void registerArrayNameTranslation(const std::string &arname, const std::string &name) const;

    void setFetcher(std::shared_ptr<Fetcher> fetcher);
    void setCurrentObject(ObjectData *data);
    ObjectData *currentObject() const;
    std::shared_ptr<Fetcher> fetcher() const;

    template<typename T>
    void fetchArray(const std::string &arname, const ArrayCompletionHandler &completeCallback) const
    {
        std::string name = translateArrayName(arname);
        if (!name.empty()) {
            if (auto arr = Shm::the().getArrayFromName<T>(name)) {
                if (completeCallback)
                    completeCallback(name);
                return;
            }
        }
        assert(m_fetcher);
        m_fetcher->requestArray(arname, shm<T>::array::typeId(), completeCallback);
    }

    obj_const_ptr getObject(const std::string &name, const ObjectCompletionHandler &completeCallback) const;
    void setObjectCompletionHandler(const std::function<void()> &completer);
    const std::function<void()> &objectCompletionHandler() const;

private:
    std::shared_ptr<Fetcher> m_fetcher;
    ObjectData *m_currentObject = nullptr;
    std::function<void()> m_completer;
    std::map<std::string, std::string> m_transObject, m_transArray;
};
#endif

typedef boost::mpl::vector<
#ifdef USE_BOOST_ARCHIVE
    boost_iarchive
#ifdef USE_YAS
    ,
    yas_iarchive
#endif
#else
#ifdef USE_YAS
    yas_iarchive
#endif
#endif
    >
    InputArchives;

typedef boost::mpl::vector<
#ifdef USE_BOOST_ARCHIVE
    boost_oarchive
#ifdef USE_YAS
    ,
    yas_oarchive
#endif
#else
#ifdef USE_YAS
    yas_oarchive
#endif
#endif
    >
    OutputArchives;

} // namespace vistle


#ifdef USE_BOOST_ARCHIVE
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::boost_oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::boost_oarchive)
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::boost_iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::boost_iarchive)
#endif

#endif // ARCHIVES_IMPL_H
