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

//#include <boost/mpl/for_each.hpp>

#include "archives.h"
#if 0
#include "assert.h"
#include "shm.h"
#include "shm_array.h"
#include "shm_reference.h"
#endif

namespace ba = boost::archive;

namespace boost {
namespace archive {

template class V_COREEXPORT detail::archive_serializer_map<vistle::shallow_oarchive>;
template class basic_binary_oprimitive<
    vistle::shallow_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;

template class basic_binary_oarchive<vistle::shallow_oarchive> ;
template class binary_oarchive_impl<
    vistle::shallow_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
>;

// explicitly instantiate for this type of stream
template class V_COREEXPORT detail::archive_serializer_map<vistle::shallow_iarchive>;
template class basic_binary_iprimitive<
    vistle::shallow_iarchive,
    std::istream::char_type,
    std::istream::traits_type
>;
template class basic_binary_iarchive<vistle::shallow_iarchive> ;
template class binary_iarchive_impl<
    vistle::shallow_iarchive,
    std::istream::char_type,
    std::istream::traits_type
>;

} // namespace archive
} // namespace boost


namespace vistle {

shallow_oarchive::shallow_oarchive(std::ostream &os, unsigned int flags)
: boost::archive::binary_oarchive_impl<shallow_oarchive, std::ostream::char_type, std::ostream::traits_type>(os, flags)
{}

shallow_oarchive::shallow_oarchive(std::streambuf &bsb, unsigned int flags)
: boost::archive::binary_oarchive_impl<shallow_oarchive, std::ostream::char_type, std::ostream::traits_type>(bsb, flags)
{}

shallow_oarchive::~shallow_oarchive()
{}

deep_oarchive::deep_oarchive(std::ostream &os, unsigned int flags)
: shallow_oarchive(os, flags)
{}

deep_oarchive::deep_oarchive(std::streambuf &bsb, unsigned int flags)
: shallow_oarchive(bsb, flags)
{}

deep_oarchive::~deep_oarchive()
{}


Fetcher::~Fetcher() {
}


shallow_iarchive::shallow_iarchive(std::istream &is, unsigned int flags)
: Base(is, flags)
, m_currentObject(nullptr)
{}

shallow_iarchive::shallow_iarchive(std::streambuf &bsb, unsigned int flags)
: Base(bsb, flags)
, m_currentObject(nullptr)
{}

shallow_iarchive::~shallow_iarchive()
{}

void shallow_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher) {
    m_fetcher = fetcher;
}

void shallow_iarchive::setCurrentObject(ObjectData *data) {
    m_currentObject = data;
}

ObjectData *shallow_iarchive::currentObject() const {
    return m_currentObject;
}

void shallow_iarchive::setObjectCompletionHandler(const std::function<void()> &completer) {
    m_completer = completer;
}

const std::function<void()> &shallow_iarchive::objectCompletionHandler() const {
    return m_completer;
}

deep_iarchive::deep_iarchive(std::istream &is, unsigned int flags)
: Base(is, flags)
{}

deep_iarchive::deep_iarchive(std::streambuf &bsb, unsigned int flags)
: Base(bsb, flags)
{}

deep_iarchive::~deep_iarchive()
{}

} // namespace vistle
