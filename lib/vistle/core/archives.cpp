/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// binary_oarchive.cpp:
// binary_iarchive.cpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <istream>

#include "archives_config.h"
#include "archives_impl.h"
#include "archives_compress.h"
#include "archives_compress_sz3.h"
#include "archives.h"
#include "object.h"

#include "shmvector.h"

#ifdef USE_BOOST_ARCHIVE
#define BOOST_ARCHIVE_SOURCE
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/detail/archive_serializer_map.hpp>

// explicitly instantiate for this type of binary stream
#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>
#endif

//#define COMP_DEBUG

//#include <boost/mpl/for_each.hpp>

#ifdef USE_BOOST_ARCHIVE
namespace ba = boost::archive;

namespace boost {
namespace archive {

template class V_COREEXPORT detail::archive_serializer_map<vistle::boost_oarchive>;
template class detail::common_oarchive<vistle::boost_oarchive>;
template class basic_binary_oprimitive<vistle::boost_oarchive, std::ostream::char_type, std::ostream::traits_type>;

template class basic_binary_oarchive<vistle::boost_oarchive>;
template class binary_oarchive_impl<vistle::boost_oarchive, std::ostream::char_type, std::ostream::traits_type>;


// explicitly instantiate for this type of stream
template class V_COREEXPORT detail::archive_serializer_map<vistle::boost_iarchive>;
template class basic_binary_iprimitive<vistle::boost_iarchive, std::istream::char_type, std::istream::traits_type>;
template class basic_binary_iarchive<vistle::boost_iarchive>;
template class binary_iarchive_impl<vistle::boost_iarchive, std::istream::char_type, std::istream::traits_type>;

} // namespace archive
} // namespace boost
#endif


namespace vistle {

Saver::~Saver()
{}

#ifdef USE_BOOST_ARCHIVE
boost_oarchive::boost_oarchive(std::streambuf &bsb, unsigned int flags)
: boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type>(bsb, flags)
{}

boost_oarchive::~boost_oarchive()
{}

void boost_oarchive::setCompressionSettings(const CompressionSettings &other)
{}

void boost_oarchive::setCompressionMode(FieldCompressionMode mode)
{}

void boost_oarchive::setSaver(std::shared_ptr<Saver> saver)
{
    m_saver = saver;
}
#endif


Fetcher::~Fetcher()
{}

bool Fetcher::renameObjects() const
{
    return false;
}

std::string Fetcher::translateObjectName(const std::string &name) const
{
    assert(!renameObjects());
    return name;
}

std::string Fetcher::translateArrayName(const std::string &name) const
{
    assert(!renameObjects());
    return name;
}

void Fetcher::registerObjectNameTranslation(const std::string &arname, const std::string &name)
{
    assert(!renameObjects());
}

void Fetcher::registerArrayNameTranslation(const std::string &arname, const std::string &name)
{
    assert(!renameObjects());
}

#ifdef USE_YAS
void yas_oarchive::setCompressionSettings(const CompressionSettings &other)
{
    m_compress = other;
}

const CompressionSettings &yas_oarchive::compressionSettings() const
{
    return m_compress;
}

yas_oarchive::yas_oarchive(yas_oarchive::Stream &mo, unsigned int flags): yas_oarchive::Base(mo), m_os(mo)
{}

void yas_oarchive::setSaver(std::shared_ptr<Saver> saver)
{
    m_saver = saver;
}

yas_iarchive::yas_iarchive(yas_iarchive::Stream &mi, unsigned int flags): yas_iarchive::Base(mi)
{}

yas_iarchive::~yas_iarchive()
{
    setCurrentObject(nullptr);
}

std::string yas_iarchive::translateObjectName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateObjectName(name);
}

std::string yas_iarchive::translateArrayName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateArrayName(name);
}

void yas_iarchive::registerObjectNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerObjectNameTranslation(arname, name);
}

void yas_iarchive::registerArrayNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerArrayNameTranslation(arname, name);
}

void yas_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher)
{
    m_fetcher = fetcher;
}

void yas_iarchive::setCurrentObject(ObjectData *data)
{
    if (data)
        data->ref();
    if (m_currentObject)
        m_currentObject->unref();
    m_currentObject = data;
}

ObjectData *yas_iarchive::currentObject() const
{
    return m_currentObject;
}

std::shared_ptr<Fetcher> yas_iarchive::fetcher() const
{
    return m_fetcher;
}

void yas_iarchive::setObjectCompletionHandler(const std::function<void()> &completer)
{
    m_completer = completer;
}

const std::function<void()> &yas_iarchive::objectCompletionHandler() const
{
    return m_completer;
}

obj_const_ptr yas_iarchive::getObject(const std::string &arname, const ObjectCompletionHandler &completeCallback) const
{
    std::string name = arname;
    if (m_fetcher)
        name = m_fetcher->translateObjectName(arname);
    //std::cerr << "yas_iarchive::getObject(arname=" << arname << ", name=" << name << ")" << std::endl;
    auto obj = Shm::the().getObjectFromName(name);
    if (!obj) {
        assert(m_fetcher);
        m_fetcher->requestObject(arname, completeCallback);
        obj = Shm::the().getObjectFromName(name);
    }
    return obj;
}
#endif


#ifdef USE_BOOST_ARCHIVE
boost_iarchive::boost_iarchive(std::streambuf &bsb, unsigned int flags): Base(bsb, flags), m_currentObject(nullptr)
{}

boost_iarchive::~boost_iarchive()
{
    setCurrentObject(nullptr);
}

std::string boost_iarchive::translateObjectName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateObjectName(name);
}

std::string boost_iarchive::translateArrayName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateArrayName(name);
}

void boost_iarchive::registerObjectNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerObjectNameTranslation(arname, name);
}

void boost_iarchive::registerArrayNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerArrayNameTranslation(arname, name);
}

void boost_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher)
{
    m_fetcher = fetcher;
}

void boost_iarchive::setCurrentObject(ObjectData *data)
{
    if (data)
        data->ref();
    if (m_currentObject)
        m_currentObject->unref();
    m_currentObject = data;
}

ObjectData *boost_iarchive::currentObject() const
{
    return m_currentObject;
}

std::shared_ptr<Fetcher> boost_iarchive::fetcher() const
{
    return m_fetcher;
}

obj_const_ptr boost_iarchive::getObject(const std::string &name, const ObjectCompletionHandler &completeCallback) const
{
    auto obj = Shm::the().getObjectFromName(name);
    if (!obj) {
        assert(m_fetcher);
        m_fetcher->requestObject(name, completeCallback);
        obj = Shm::the().getObjectFromName(name);
    }
    return obj;
}

void boost_iarchive::setObjectCompletionHandler(const std::function<void()> &completer)
{
    m_completer = completer;
}

const std::function<void()> &boost_iarchive::objectCompletionHandler() const
{
    return m_completer;
}
#endif
} // namespace vistle
