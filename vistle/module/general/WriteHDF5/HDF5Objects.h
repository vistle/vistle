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
#include <core/findobjectreferenceoarchive.h>
#include <core/shm.h>

#include "hdf5.h"

//-------------------------------------------------------------------------
// CLASS DECLARATIONS
//-------------------------------------------------------------------------

// CONTAINER FOR CONSTANTS SHARED ACROSS HDF5 MODULES
//-------------------------------------------------------------------------
struct HDF5Const {
    struct DummyObject;

    static const long double numBytesInGb;
    static const long double mpiReadWriteLimitGb;

    static const unsigned numExclusiveMetaMembers;
};

// cout-of-class init for static members
const long double HDF5Const::numBytesInGb = 1073741824;
const long double HDF5Const::mpiReadWriteLimitGb = 2;
const unsigned HDF5Const::numExclusiveMetaMembers = 2;


// CONTAINER FOR HDF5 DUMMY OBJECT CONSTANTS
//-------------------------------------------------------------------------
struct HDF5Const::DummyObject {
    static const hid_t type;
    static const std::vector<hsize_t> dims;
    static const std::string name;
};

// cout-of-class init for static members
const hid_t HDF5Const::DummyObject::type = H5T_NATIVE_INT;
const std::vector<hsize_t> HDF5Const::DummyObject::dims = {1};
const std::string HDF5Const::DummyObject::name = "/file/dummy";




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
