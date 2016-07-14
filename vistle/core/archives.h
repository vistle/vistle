#ifndef ARCHIVES_H
#define ARCHIVES_H

#include <functional>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/archive/detail/oserializer.hpp>
#include <boost/archive/detail/iserializer.hpp>

#include <boost/mpl/vector.hpp>

#include "shm.h"

namespace vistle {
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
} // namespace archive
} // namespace boost

namespace vistle {

class Object;
struct ObjectData;
typedef boost::shared_ptr<Object> obj_ptr;
typedef boost::shared_ptr<const Object> obj_const_ptr;

class V_COREEXPORT oarchive: public boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> {

    typedef boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> Base;
public:
    oarchive(std::ostream &os, unsigned int flags=0);
    oarchive(std::streambuf &bsb, unsigned int flags=0);
    ~oarchive();

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

    void setFetcher(boost::shared_ptr<Fetcher> fetcher);
    void setCurrentObject(ObjectData *data);
    ObjectData *currentObject() const;

    template<typename T>
    ShmVector<T> getArray(const std::string &name, const std::function<void()> &completeCallback) const {
        auto arr = Shm::the().getArrayFromName<T>(name);
        if (!arr) {
            assert(m_fetcher);
            m_fetcher->requestArray(name, shm<T>::array::typeId(), completeCallback);
        }
        return arr;
    }
    obj_const_ptr getObject(const std::string &name, const std::function<void()> &completeCallback) const {
        auto obj = Shm::the().getObjectFromName(name);
        if (!obj) {
            assert(m_fetcher);
            m_fetcher->requestObject(name, completeCallback);
        }
        return obj;
    }

    void setObjectCompletionHandler(const std::function<void()> &completer);
    const std::function<void()> &objectCompletionHandler() const;

private:
    //void *getArrayPointer(const std::string &name, int type, const std::function<void()> &completeCallback) const;
    boost::shared_ptr<Fetcher> m_fetcher;
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
