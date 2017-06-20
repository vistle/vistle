#ifndef ARCHIVES_H
#define ARCHIVES_H

#include <functional>
#include <vector>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/archive/detail/oserializer.hpp>
#include <boost/archive/detail/iserializer.hpp>

#include <boost/mpl/vector.hpp>

#include <util/vecstreambuf.h>
//#include "shm.h"
#include "findobjectreferenceoarchive.h"

namespace vistle {

template<class T>
class shm_obj_ref;

class oarchive;
class iarchive;

}

namespace boost {
namespace archive {
extern template class V_COREEXPORT basic_binary_oprimitive<
    vistle::oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;
extern template class V_COREEXPORT basic_binary_iprimitive<
    vistle::iarchive,
    std::istream::char_type, 
    std::istream::traits_type
>;
extern template class V_COREEXPORT detail::common_oarchive<vistle::oarchive>;
} // namespace archive
} // namespace boost

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

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & name;
        ar & is_array;
        ar & size;
    }
};
typedef std::vector<SubArchiveDirectoryEntry> SubArchiveDirectory;

class V_COREEXPORT Saver {
public:
    virtual ~Saver();
    virtual void saveArray(const std::string &name, int type, const void *array) = 0;
    virtual void saveObject(const std::string &name, obj_const_ptr obj) = 0;
};

class V_COREEXPORT oarchive: public boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> {

    typedef boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> Base;
public:
    oarchive(std::ostream &os, unsigned int flags=0);
    oarchive(std::streambuf &bsb, unsigned int flags=0);
    ~oarchive();

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


class V_COREEXPORT Fetcher {
public:
    virtual ~Fetcher();
    virtual void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) = 0;
    virtual void requestObject(const std::string &name, const std::function<void()> &completeCallback) = 0;
};

class V_COREEXPORT iarchive: public boost::archive::binary_iarchive_impl<iarchive, std::istream::char_type, std::istream::traits_type> {

    typedef boost::archive::binary_iarchive_impl<iarchive, std::istream::char_type, std::istream::traits_type> Base;
public:
    iarchive(std::istream &is, unsigned int flags=0);
    iarchive(std::streambuf &bsb, unsigned int flags=0);
    ~iarchive();

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
    ObjectData *m_currentObject;
    std::function<void()> m_completer;
};



typedef boost::mpl::vector<
   iarchive
      > InputArchives;

typedef boost::mpl::vector<
   oarchive
      > OutputArchives;

} // namespace vistle


BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::oarchive)
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::iarchive)

#endif
