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

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/int.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/type_traits/is_enum.hpp>

#include <vistle/core/findobjectreferenceoarchive.h>
#include <vistle/core/shm.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/module/module.h>

#include "hdf5.h"

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------

DEFINE_ENUM_WITH_STRING_CONVERSIONS(WriteMode, (Organized)(Performant))

//-------------------------------------------------------------------------
// CLASS DECLARATIONS
//-------------------------------------------------------------------------

// CONTAINER FOR CONSTANTS SHARED ACROSS HDF5 MODULES
//-------------------------------------------------------------------------
struct HDF5Const {
    struct DummyObject;

    static const long double numBytesInGb;
    static const long double mpiReadWriteLimitGb;

    static const int additionalMetaArrayMembers;

    static const int versionNumber;

    static const unsigned performantReferenceNullVal;
    static const unsigned performantAttributeNullVal;
    static const hid_t performantReferenceTypeId;
};

// cout-of-class init for static members
const long double HDF5Const::numBytesInGb = 1073741824;
const long double HDF5Const::mpiReadWriteLimitGb = 2;

const int HDF5Const::additionalMetaArrayMembers = 1;

const int HDF5Const::versionNumber = 1;

const unsigned HDF5Const::performantReferenceNullVal =
    std::numeric_limits<unsigned>::max(); // < must not = resetFlag in WriteHDF5.cpp
const unsigned HDF5Const::performantAttributeNullVal = std::numeric_limits<unsigned>::max() - 1;
const hid_t HDF5Const::performantReferenceTypeId = std::numeric_limits<hid_t>::max();

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
    std::vector<std::string> *m_nvpTagVectorPtr;

public:
    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL)
    {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    MemberCounterArchive(std::vector<std::string> *_nvpTagVectorPtr = nullptr)
    : m_counter(0), m_nvpTagVectorPtr(_nvpTagVectorPtr)
    {}

    // the & operator
    template<class T>
    MemberCounterArchive &operator&(const boost::serialization::nvp<T> &t)
    {
        m_counter++;

        if (m_nvpTagVectorPtr != nullptr) {
            if (t.name() != "block" && t.name() != "timestep") {
                m_nvpTagVectorPtr->push_back(t.name());
            }
        }

        return *this;
    }

    // get functions
    unsigned getCount() { return m_counter; }
};

// CONTIGUOUS MEMORY ARRAY
// * a wrapper for a 2d array
// * stored data in a contiguous memory buffes so that it can be used with the HDF5 library
// * interfaces similar to a std::vector
//-------------------------------------------------------------------------
template<class T>
class ContiguousMemoryMatrix {
private:
    T *m_data;
    unsigned m_size[2];
    unsigned m_pushIndex;

    const double m_capacityIncreaseFactor = 1.2;

public:
    // constructors
    ContiguousMemoryMatrix(): m_data(nullptr), m_pushIndex(0)
    {
        m_size[0] = 0;
        m_size[1] = 0;
    }

    ContiguousMemoryMatrix(unsigned x, unsigned y): m_data(nullptr) { reserve(x, y); }

    // destructor
    ~ContiguousMemoryMatrix()
    {
        if (m_data) {
            delete[] m_data;
        }
    }

    // resizes the array and copies data over
    void reserve(unsigned x, unsigned y)
    {
        T *newData = new T[x * y];

        if (m_data) {
            for (unsigned i = 0; (i < m_size[0] * m_size[1]) && (i < x * y); i++) {
                newData[i] = m_data[i];
            }

            delete[] m_data;
        }

        // create new matrix
        m_pushIndex = 0;
        m_size[0] = x;
        m_size[1] = y;

        m_data = newData;
    }

    // push_back methods, they also resize the array if needed
    void push_back(std::vector<T> values)
    {
        push_back(values.data(), values.size());
        return;
    }

    void push_back(T values[])
    {
        push_back(values, m_size[1]);
        return;
    }

    void push_back(T values[], unsigned size)
    {
        // bound check
        if (size > m_size[1]) {
            assert("segmentation fault - out of range in y dimension" == NULL);
            return;
        }

        // increase capacity if needed
        if (m_pushIndex == m_size[0]) {
            reserve(m_size[0] * m_capacityIncreaseFactor, m_size[1]);
        }

        // copy data into array
        for (unsigned i = 0; i < size; i++) {
            m_data[index(m_pushIndex, i)] = values[i];
        }
        m_pushIndex++;
    }

    // obtain a reference to the back element group of the matrix
    T *back() { return &m_data[index(m_pushIndex - 1, 0)]; }

    // index into the matrix
    T &operator()(unsigned x, unsigned y) { return m_data[index(x, y)]; }

    T *operator[](unsigned x) { return &m_data[index(x, 0)]; }

    // obtain matrix data
    T *data() { return m_data; }
    unsigned size() { return m_pushIndex; }

    // obtain 1 dimensional index from 2 dimensional index
    unsigned index(unsigned x, unsigned y) { return x * m_size[1] + y; }
};


#endif /* HDF5OBJECTS_H */
