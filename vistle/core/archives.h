#ifndef ARCHIVES_H
#define ARCHIVES_H

#include <functional>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/archive/detail/oserializer.hpp>
#include <boost/archive/detail/iserializer.hpp>

#include <boost/mpl/vector.hpp>

#include "shm.h"
#include "object.h"

namespace vistle {

class Object;
struct ObjectData;
typedef boost::shared_ptr<Object> obj_ptr;
typedef boost::shared_ptr<const Object> obj_const_ptr;

class oarchive: public boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> {

    typedef boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> Base;
public:
    oarchive(std::ostream &os, unsigned int flags=0)
       : boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type>(os, flags)
    {}
    oarchive(std::streambuf &bsb, unsigned int flags=0)
       : boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type>(bsb, flags)
    {}

};

class Fetcher {
public:
    virtual ~Fetcher();
    virtual void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) = 0;
    virtual void requestObject(const std::string &name, const std::function<void()> &completeCallback) = 0;
};

class iarchive: public boost::archive::binary_iarchive_impl<iarchive, std::istream::char_type, std::istream::traits_type> {

    typedef boost::archive::binary_iarchive_impl<iarchive, std::istream::char_type, std::istream::traits_type> Base;
public:
    iarchive(std::istream &is, unsigned int flags=0);
    iarchive(std::streambuf &bsb, unsigned int flags=0);

    void setFetcher(boost::shared_ptr<Fetcher> fetcher);
    void setCurrentObject(ObjectData *data);
    ObjectData *currentObject() const;

    template<typename T>
    ShmVector<T> *getArray(const std::string &name, const std::function<void()> &completeCallback) const {
        return static_cast<ShmVector<T> *>(getArrayPointer(name, ShmVector<T>::typeId(), completeCallback));
    }
    obj_const_ptr getObject(const std::string &name) const;

    void setObjectCompletionHandler(const std::function<void()> &completer);
    const std::function<void()> &objectCompletionHandler() const;

private:
    void *getArrayPointer(const std::string &name, int type, const std::function<void()> &completeCallback) const;
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
