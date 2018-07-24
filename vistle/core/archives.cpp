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

//#include <boost/mpl/for_each.hpp>

#ifdef USE_BOOST_ARCHIVE
namespace ba = boost::archive;

namespace boost {
namespace archive {

template class V_COREEXPORT detail::archive_serializer_map<vistle::boost_oarchive>;
template class detail::common_oarchive<vistle::boost_oarchive>;
template class basic_binary_oprimitive<
    vistle::boost_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;

template class basic_binary_oarchive<vistle::boost_oarchive> ;
template class binary_oarchive_impl<
    vistle::boost_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;


// explicitly instantiate for this type of stream
template class V_COREEXPORT detail::archive_serializer_map<vistle::boost_iarchive>;
template class basic_binary_iprimitive<
    vistle::boost_iarchive,
    std::istream::char_type,
    std::istream::traits_type
>;
template class basic_binary_iarchive<vistle::boost_iarchive> ;
template class binary_iarchive_impl<
    vistle::boost_iarchive,
    std::istream::char_type,
    std::istream::traits_type
>;

} // namespace archive
} // namespace boost
#endif


namespace vistle {

Saver::~Saver() {
}

#ifdef USE_BOOST_ARCHIVE
boost_oarchive::boost_oarchive(std::streambuf &bsb, unsigned int flags)
: boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type>(bsb, flags)
{}

boost_oarchive::~boost_oarchive()
{}

void boost_oarchive::setSaver(std::shared_ptr<Saver> saver) {

    m_saver = saver;
}
#endif


Fetcher::~Fetcher() {
}

#ifdef USE_YAS
void yas_oarchive::setCompressionMode(CompressionMode mode) {
    m_compress = mode;
}

CompressionMode yas_oarchive::compressionMode() const {
    return m_compress;
}

void yas_oarchive::setZfpRate(double rate) {
    m_zfpRate = rate;
}

double yas_oarchive::zfpRate() const {
    return m_zfpRate;
}

void yas_oarchive::setZfpAccuracy(double accuracy) {
    m_zfpAccuracy = accuracy;
}

double yas_oarchive::zfpAccuracy() const {
    return m_zfpAccuracy;
}

void yas_oarchive::setZfpPrecision(int precision) {
    m_zfpPrecision = precision;
}

int yas_oarchive::zfpPrecision() const {
    return m_zfpPrecision;
}

yas_oarchive::yas_oarchive(yas_oarchive::Stream &mo, unsigned int flags)
: yas_oarchive::Base(mo)
{
}

void yas_oarchive::setSaver(std::shared_ptr<Saver> saver) {
    m_saver = saver;
}

yas_iarchive::yas_iarchive(yas_iarchive::Stream &mi, unsigned int flags)
: yas_iarchive::Base(mi)
{
}

void yas_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher) {
    m_fetcher = fetcher;
}

void yas_iarchive::setCurrentObject(ObjectData *data) {
    m_currentObject = data;
}

ObjectData *yas_iarchive::currentObject() const {
    return m_currentObject;
}

void yas_iarchive::setObjectCompletionHandler(const std::function<void ()> &completer) {
    m_completer = completer;
}

const std::function<void ()> &yas_iarchive::objectCompletionHandler() const {
    return m_completer;
}
#endif


#ifdef USE_BOOST_ARCHIVE
boost_iarchive::boost_iarchive(std::streambuf &bsb, unsigned int flags)
: Base(bsb, flags)
, m_currentObject(nullptr)
{}

boost_iarchive::~boost_iarchive()
{}

void boost_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher) {
    m_fetcher = fetcher;
}

void boost_iarchive::setCurrentObject(ObjectData *data) {
    m_currentObject = data;
}

ObjectData *boost_iarchive::currentObject() const {
    return m_currentObject;
}

obj_const_ptr boost_iarchive::getObject(const std::string &name, const std::function<void ()> &completeCallback) const {
    auto obj = Shm::the().getObjectFromName(name);
    if (!obj) {
        assert(m_fetcher);
        m_fetcher->requestObject(name, completeCallback);
        obj = Shm::the().getObjectFromName(name);
    }
    return obj;
}

void boost_iarchive::setObjectCompletionHandler(const std::function<void()> &completer) {
    m_completer = completer;
}

const std::function<void()> &boost_iarchive::objectCompletionHandler() const {
    return m_completer;
}
#endif

#ifdef HAVE_ZFP
template<>
bool decompressZfp<zfp_type_none>(void *dest, const std::vector<char> &compressed, const Index dim[3]) {
    return false;
}

template<>
bool compressZfp<zfp_type_none>(std::vector<char> &compressed, const void *src, const Index dim[3], const ZfpParameters &param) {
   return false;
}
#endif
} // namespace vistle
