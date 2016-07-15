//-------------------------------------------------------------------------
// HDF5 OBJECTS H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef HDF5OBJECTS_H
#define HDF5OBJECTS_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/mpl/for_each.hpp>
#include <boost/config.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include <module/module.h>

#include <core/vec.h>
#include <core/unstr.h>
#include <core/shmvectoroarchive.h>
#include <core/shm.h>

#include "hdf5.h"


// MEMBER COUNTER ARCHIVE
// * used to count members within ObjectMeta
//-------------------------------------------------------------------------
class MemberCounterArchive {
private:
    unsigned m_counter;

public:

    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL) {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    MemberCounterArchive() : m_counter(0) {}

    // the & operator
    template<class T>
    MemberCounterArchive & operator&(const T & t);

    // get functions
    unsigned getCount() { return m_counter; }

};



// MEMBER COUNTER - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
MemberCounterArchive & MemberCounterArchive::operator&(T const & t) {

    m_counter++;

    return *this;
}



#endif /* HDF5OBJECTS_H */
