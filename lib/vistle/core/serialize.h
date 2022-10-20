#ifndef VISTLE_SERIALIZE_H
#define VISTLE_SERIALIZE_H

#include "archives_config.h"

#include <boost/config.hpp>
#include <boost/version.hpp>

#ifndef NO_SHMEM
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#endif

#ifdef USE_BOOST_ARCHIVE
#include <boost/serialization/utility.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#endif

#ifdef USE_YAS
#include <yas/serialize.hpp>
#include <yas/boost_types.hpp>
#include <yas/std_types.hpp>
#include <yas/object.hpp>
#endif

namespace boost {
namespace serialization {


#ifndef NO_SHMEM
#ifdef USE_BOOST_ARCHIVE
/* serialize boost::container::map */

template<class Archive, class Type, class Key, class Compare, class Allocator>
inline void save(Archive &ar, const boost::interprocess::map<Key, Type, Compare, Allocator> &t,
                 const unsigned int /* file_version */
)
{
    boost::serialization::stl::save_collection<Archive, boost::interprocess::map<Key, Type, Compare, Allocator>>(ar, t);
}

template<class Archive, class Type, class Key, class Compare, class Allocator>
inline void load(Archive &ar, boost::interprocess::map<Key, Type, Compare, Allocator> &t,
                 const unsigned int /* file_version */
)
{
#if BOOST_VERSION >= 105900
    boost::serialization::load_map_collection<Archive, boost::interprocess::map<Key, Type, Compare, Allocator>>(ar, t);
#else
    boost::serialization::stl::load_collection<
        Archive, boost::interprocess::map<Key, Type, Compare, Allocator>,
        boost::serialization::stl::archive_input_map<Archive, boost::interprocess::map<Key, Type, Compare, Allocator>>,
        boost::serialization::stl::no_reserve_imp<boost::interprocess::map<Key, Type, Compare, Allocator>>>(ar, t);
#endif
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class Type, class Key, class Compare, class Allocator>
inline void serialize(Archive &ar, boost::interprocess::map<Key, Type, Compare, Allocator> &t,
                      const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}
#endif
#endif


#ifndef NO_SHMEM
#ifdef USE_BOOST_ARCHIVE
/* serialize boost::container::vector */

template<class Archive, class U, class Allocator>
inline void save(Archive &ar, const boost::container::vector<U, Allocator> &t, const unsigned int /* file_version */
)
{
    size_t count = t.size();
    ar &boost::serialization::make_nvp("count", count);
    auto it = t.begin();
    while (count-- > 0) {
        ar &boost::serialization::make_nvp("item", *it);
        ++it;
    }
}

template<class Archive, class U, class Allocator>
inline void load(Archive &ar, boost::container::vector<U, Allocator> &t, const unsigned int /* file_version */
)
{
    size_t count = 0;
    ar &boost::serialization::make_nvp("count", count);
    t.clear();
    t.reserve(count);
    while (count-- > 0) {
        detail::stack_construct<Archive, U> u(ar, 0);
        ar >> boost::serialization::make_nvp("item", u.reference());
        t.push_back(u.reference());
        ar.reset_object_address(&t.back(), &u.reference());
    }
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class Type, class Allocator>
inline void serialize(Archive &ar, boost::container::vector<Type, Allocator> &t, const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}
#endif
#endif

#ifndef NO_SHMEM
#ifdef USE_BOOST_ARCHIVE
/* serialize boost::container::string */

template<class Archive, class CharT, class Allocator>
inline void save(Archive &ar, const boost::container::basic_string<CharT, std::char_traits<CharT>, Allocator> &t,
                 const unsigned int file_version)
{
    size_t count = t.size();
    ar &boost::serialization::make_nvp("count", count);
    auto it = t.begin();
    while (count-- > 0) {
        ar &boost::serialization::make_nvp("item", *it);
        ++it;
    }
}

template<class Archive, class CharT, class Allocator>
inline void load(Archive &ar, boost::container::basic_string<CharT, std::char_traits<CharT>, Allocator> &t,
                 const unsigned int file_version)
{
    size_t count = 0;
    ar &boost::serialization::make_nvp("count", count);
    t.clear();
    t.reserve(count);
    while (count-- > 0) {
        detail::stack_construct<Archive, CharT> u(ar, 0);
        ar >> boost::serialization::make_nvp("item", u.reference());
        t.push_back(u.reference());
        ar.reset_object_address(&t[t.length() - 1], &u.reference());
    }
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class CharT, class Allocator>
inline void serialize(Archive &ar, boost::container::basic_string<CharT, std::char_traits<CharT>, Allocator> &t,
                      const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}
#endif
#endif

} // namespace serialization
} // namespace boost

#endif
