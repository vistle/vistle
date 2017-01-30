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
#include "shm.h"
#include "findobjectreferenceoarchive.h"

namespace vistle {

template<class T>
class shm_obj_ref;

class shallow_oarchive;
class shallow_iarchive;

}

namespace boost {
namespace archive {
extern template class V_COREEXPORT basic_binary_oprimitive<
    vistle::shallow_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;
extern template class V_COREEXPORT basic_binary_iprimitive<
    vistle::shallow_iarchive,
    std::istream::char_type, 
    std::istream::traits_type
>;
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

class V_COREEXPORT shallow_oarchive: public boost::archive::binary_oarchive_impl<shallow_oarchive, std::ostream::char_type, std::ostream::traits_type> {

    typedef boost::archive::binary_oarchive_impl<shallow_oarchive, std::ostream::char_type, std::ostream::traits_type> Base;
public:
    shallow_oarchive(std::ostream &os, unsigned int flags=0);
    shallow_oarchive(std::streambuf &bsb, unsigned int flags=0);
    ~shallow_oarchive();

    void setSaver(std::shared_ptr<Saver> saver);

    template<class T>
    void saveArray(const vistle::ShmVector<T> &t) {
        std::cerr << "shallow_oarchive: serializing array " << t.name() << std::endl;
        if (m_saver)
            m_saver->saveArray(t.name(), t->type(), &t);
    }

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &t) {
        std::cerr << "shallow_oarchive: serializing object " << t.name() << std::endl;
        if (m_saver)
            m_saver->saveObject(t.name(), t.getObject());
    }

    std::shared_ptr<Saver> m_saver;
};

#if 0
class V_COREEXPORT deep_oarchive: public shallow_oarchive {

    typedef shallow_oarchive Base;
    void move_subarchives(deep_oarchive &other) {
        for (auto &p: other.objects)
            objects.insert(std::move(p));
        other.objects.clear();
        for (auto &p: other.arrays)
            arrays.insert(std::move(p));
        other.arrays.clear();
    }

public:
    deep_oarchive(std::ostream &os, unsigned int flags=0);
    deep_oarchive(std::streambuf &bsb, unsigned int flags=0);
    ~deep_oarchive();
    std::map<std::string, std::vector<char>> objects;
    std::map<std::string, std::vector<char>> arrays;

    template<class T>
    void saveArray(const vistle::ShmVector<T> &t) {
        vecostreambuf<char> vb;
        deep_oarchive ar(vb);
        ar & *t;
        std::cerr << "deep_oarchive: serializing array " << t->name() << std::endl;
        ar.arrays.emplace(t->name(), vb.get_vector());
        move_subarchives(ar);
    }

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &t) {
        vecostreambuf<char> vb;
        deep_oarchive ar(vb);
        ar & *t;
        std::cerr << "deep_oarchive: serializing object " << t->name() << std::endl;
        ar.arrays.emplace(t->name(), vb.get_vector());
        move_subarchives(ar);
    }

};
#endif


class V_COREEXPORT Fetcher {
public:
    virtual ~Fetcher();
    virtual void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) = 0;
    virtual void requestObject(const std::string &name, const std::function<void()> &completeCallback) = 0;
};

class V_COREEXPORT shallow_iarchive: public boost::archive::binary_iarchive_impl<shallow_iarchive, std::istream::char_type, std::istream::traits_type> {

    typedef boost::archive::binary_iarchive_impl<shallow_iarchive, std::istream::char_type, std::istream::traits_type> Base;
public:
    shallow_iarchive(std::istream &is, unsigned int flags=0);
    shallow_iarchive(std::streambuf &bsb, unsigned int flags=0);
    ~shallow_iarchive();

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

#if 0
class V_COREEXPORT deep_iarchive: public shallow_iarchive {

    typedef shallow_iarchive Base;
public:
    deep_iarchive(std::istream &is, unsigned int flags=0);
    deep_iarchive(std::streambuf &bsb, unsigned int flags=0);
    ~deep_iarchive();

    template<class T>
    deep_iarchive &operator>>(vistle::ShmVector<T> &t) {
        *this >> *t;
        return *this;
    }

    template<class T>
    deep_iarchive &operator>>(vistle::shm_obj_ref<T> &t) {
        *this >> *t;
        return *this;
    }
};
#endif



typedef boost::mpl::vector<
   shallow_iarchive
      > InputArchives;

typedef boost::mpl::vector<
   shallow_oarchive
      > OutputArchives;

} // namespace vistle


BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::shallow_oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::shallow_oarchive)
#if 0
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::deep_oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::deep_oarchive)
#endif
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::shallow_iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::shallow_iarchive)
#if 0
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::deep_iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::deep_iarchive)
#endif

#endif
