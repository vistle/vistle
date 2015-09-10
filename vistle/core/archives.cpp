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

#include "archives.h"
#include "message.h"

namespace ba = boost::archive;

namespace boost {
namespace archive {

template class detail::archive_serializer_map<vistle::oarchive>;
template class basic_binary_oprimitive<
    vistle::oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;

template class basic_binary_oarchive<vistle::oarchive> ;
template class binary_oarchive_impl<
    vistle::oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;

// explicitly instantiate for this type of stream
template class detail::archive_serializer_map<vistle::iarchive>;
template class basic_binary_iprimitive<
    vistle::iarchive,
    std::istream::char_type,
    std::istream::traits_type
>;
template class basic_binary_iarchive<vistle::iarchive> ;
template class binary_iarchive_impl<
    vistle::iarchive,
    std::istream::char_type,
    std::istream::traits_type
>;

} // namespace archive
} // namespace boost


namespace vistle {

Fetcher::~Fetcher() {
}

iarchive::iarchive(std::istream &is, unsigned int flags)
: Base(is, flags)
, m_currentObject(nullptr)
{}

iarchive::iarchive(std::streambuf &bsb, unsigned int flags)
: Base(bsb, flags)
, m_currentObject(nullptr)
{}

void iarchive::setFetcher(boost::shared_ptr<Fetcher> fetcher) {
    m_fetcher = fetcher;
}

void iarchive::setCurrentObject(ObjectData *data) {
    m_currentObject = data;
}

ObjectData *iarchive::currentObject() const {
    return m_currentObject;
}

void iarchive::setObjectCompletionHandler(const std::function<void()> &completer) {
    m_completer = completer;
}

const std::function<void()> &iarchive::objectCompletionHandler() const {
    return m_completer;
}

} // namespace vistle
