#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <boost/config.hpp>

#include <boost/serialization/utility.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/split_free.hpp>

#include <boost/serialization/level.hpp>

BOOST_CLASS_IMPLEMENTATION(boost::interprocess::string, boost::serialization::primitive_type)

namespace boost { 
namespace serialization {

template<class Archive, class Type, class Key, class Compare, class Allocator >
inline void save(
    Archive & ar,
    const boost::interprocess::map<Key, Type, Compare, Allocator> &t,
    const unsigned int /* file_version */
){
    boost::serialization::stl::save_collection<
        Archive, 
        boost::interprocess::map<Key, Type, Compare, Allocator> 
    >(ar, t);
}

template<class Archive, class Type, class Key, class Compare, class Allocator >
inline void load(
    Archive & ar,
    boost::interprocess::map<Key, Type, Compare, Allocator> &t,
    const unsigned int /* file_version */
){
    boost::serialization::stl::load_collection<
        Archive,
        boost::interprocess::map<Key, Type, Compare, Allocator>,
        boost::serialization::stl::archive_input_map<
            Archive, boost::interprocess::map<Key, Type, Compare, Allocator> >,
            boost::serialization::stl::no_reserve_imp<boost::interprocess::map<
                Key, Type, Compare, Allocator
            >
        >
    >(ar, t);
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class Type, class Key, class Compare, class Allocator >
inline void serialize(
    Archive & ar,
    boost::interprocess::map<Key, Type, Compare, Allocator> &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

template<class Archive, class U, class Allocator>
inline void save(
    Archive & ar,
    const boost::container::vector<U, Allocator> &t,
    const unsigned int /* file_version */
){
    boost::serialization::stl::save_collection<Archive, boost::container::vector<U, Allocator> >(
        ar, t
    );
}

template<class Archive, class U, class Allocator>
inline void load(
    Archive & ar,
    boost::container::vector<U, Allocator> &t,
    const unsigned int /* file_version */
){
    boost::serialization::stl::load_collection<
        Archive,
        boost::container::vector<U, Allocator>,
        boost::serialization::stl::archive_input_seq<
            Archive, boost::container::vector<U, Allocator> 
        >,
        boost::serialization::stl::reserve_imp<boost::container::vector<U, Allocator> >
    >(ar, t);
}


// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class Type, class Allocator >
inline void serialize(
    Archive & ar,
    boost::container::vector<Type, Allocator> &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

#endif
