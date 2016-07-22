//-------------------------------------------------------------------------
// WRITE HDF5 H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef WRITEHDF5_H
#define WRITEHDF5_H

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
// WRITE HDF5 CLASS DECLARATION
//-------------------------------------------------------------------------
class WriteHDF5 : public vistle::Module {
 public:
    friend class boost::serialization::access;
    friend struct ShmVectorWriter;

   WriteHDF5(const std::string &shmname, const std::string &name, int moduleID);
   ~WriteHDF5();

 private:
   // typedefs
   // IndexTracker: maps: origin (vector) -> timestep (map) -> block (map) -> variants (unsigned)
   typedef std::vector<std::unordered_map<int, std::unordered_map<int, unsigned>>> IndexTracker;

   // structs/functors
   struct ReservationInfoShmEntry;
   struct ReservationInfo;

   struct ShmVectorReserver;
   struct ShmVectorWriter;

   class MemberCounterArchive;
   class MetaToArrayArchive;

   // overriden functions
   virtual bool parameterChanged(const vistle::Parameter * p) override;
   virtual bool prepare() override;
   virtual bool compute() override;
   virtual bool reduce(int timestep) override;

   // private helper functions
   static void util_checkStatus(herr_t status);
   static void util_HDF5write(bool isWriter, std::string name, const void * data, hid_t fileId, hsize_t * dims, hid_t dataType);
   static void util_HDF5write(hid_t fileId);



   // private member variables
   vistle::StringParameter *m_fileName;
   unsigned m_numPorts;

   bool m_isRootNode;
   bool m_hasObject;
   std::unordered_map<std::string, bool> m_arrayMap;
   std::unordered_set<std::string> m_objectSet;
   IndexTracker m_indexTracker;


   // private member constants
   static const hid_t m_dummyObjectType;
   static const std::vector<hsize_t> m_dummyObjectDims;
   static const std::string m_dummyObjectName;

public:
   static unsigned numMetaMembers;
   static const std::unordered_map<std::type_index, hid_t> nativeTypeMap;

};



// WRITE HDF5 STATIC MEMBER OUT OF CLASS INITIALIZATION
//-------------------------------------------------------------------------
const hid_t WriteHDF5::m_dummyObjectType = H5T_NATIVE_INT;
const std::vector<hsize_t> WriteHDF5::m_dummyObjectDims = {1};
const std::string WriteHDF5::m_dummyObjectName = "/file/dummy";

unsigned WriteHDF5::numMetaMembers = 0;

const std::unordered_map<std::type_index, hid_t> WriteHDF5::nativeTypeMap = {
      { typeid(int), H5T_NATIVE_INT },
      { typeid(unsigned int), H5T_NATIVE_UINT },

      { typeid(char), H5T_NATIVE_CHAR },
      { typeid(unsigned char), H5T_NATIVE_UCHAR },

      { typeid(short), H5T_NATIVE_SHORT },
      { typeid(unsigned short), H5T_NATIVE_USHORT },

      { typeid(long), H5T_NATIVE_LONG },
      { typeid(unsigned long), H5T_NATIVE_ULONG },
      { typeid(long long), H5T_NATIVE_LLONG },
      { typeid(unsigned long long), H5T_NATIVE_ULLONG },

      { typeid(float), H5T_NATIVE_FLOAT },
      { typeid(double), H5T_NATIVE_DOUBLE },
      { typeid(long double), H5T_NATIVE_LDOUBLE }

       // to implement? these are other accepted types
       //        H5T_NATIVE_B8
       //        H5T_NATIVE_B16
       //        H5T_NATIVE_B32
       //        H5T_NATIVE_B64

       //        H5T_NATIVE_OPAQUE
       //        H5T_NATIVE_HADDR
       //        H5T_NATIVE_HSIZE
       //        H5T_NATIVE_HSSIZE
       //        H5T_NATIVE_HERR
       //        H5T_NATIVE_HBOOL

};

//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR DEFINITIONS
//-------------------------------------------------------------------------

// RESERVATION INFO SHM ENTRY STRUCT
// * stores ShmVector data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::ReservationInfoShmEntry {
    std::string name;
    std::string nvpTag;
    hid_t type;
    unsigned size;


    ReservationInfoShmEntry() {}
    ReservationInfoShmEntry(std::string _name, std::string _nvpTag, hid_t _type, unsigned _size)
        : name(_name), nvpTag(_nvpTag), type(_type), size(_size) {}

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// RESERVATION INFO STRUCT
// * stores data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::ReservationInfo {
    std::string name;
    std::vector<WriteHDF5::ReservationInfoShmEntry> shmVectors;
    int block;
    int timestep;
    unsigned origin;
    bool isValid;

    // < overload for std::sort
    bool operator<(const ReservationInfo &other) const;

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// MEMBER COUNTER ARCHIVE
// * used to count members within ObjectMeta
//-------------------------------------------------------------------------
class WriteHDF5::MemberCounterArchive {
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

// META TO ARRAY ARCHIVE
// * used to convert metadata members to an array of doubles for storage
// * into the HDF5 file
//-------------------------------------------------------------------------
class WriteHDF5::MetaToArrayArchive {
private:
    std::vector<double> m_array;
    unsigned m_insertIndex;

public:
    static const int numExclusiveMembers;

    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL) {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    MetaToArrayArchive() : m_insertIndex(0) { m_array.reserve(WriteHDF5::numMetaMembers - numExclusiveMembers); }

    // << operators
    template<class T>
    MetaToArrayArchive & operator<<(T const & t);
    template<class T>
    MetaToArrayArchive & operator<<(const boost::serialization::nvp<T> & t);

    // the & operator
    template<class T>
    MetaToArrayArchive & operator&(const T & t);

    // get functions
    double * getDataPtr() { return m_array.data(); }
};

// static memeber out-of-class declaration
const int WriteHDF5::MetaToArrayArchive::numExclusiveMembers = 2;

// SHM VECTOR RESERVER FUNCTOR
// * used to construct ReservationInfoShmEntry entries within the shmVectors
// * member of ReservationInfo
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorReserver {
    std::string name;
    std::string nvpTag;
    ReservationInfo * reservationInfo;

    ShmVectorReserver(std::string _name, std::string _nvpTag, ReservationInfo * _reservationInfo)
        : name(_name), nvpTag(_nvpTag), reservationInfo(_reservationInfo) {}

    template<typename T>
    void operator()(T);
};

// SHM VECTOR WRITER FUNCTOR
// * calls util_HDF5Write for a shmVector
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorWriter {
    std::string name;
    hid_t fileId;
    const boost::mpi::communicator & commPtr;

    // constants
    const long double BYTES_IN_GB = 1073741824;
    const long double MAX_WRITE_GB = 2;

    ShmVectorWriter(hid_t _fileId, const boost::mpi::communicator & _commPtr) : commPtr(_commPtr), fileId(_fileId) {}
    ShmVectorWriter(std::string _name, hid_t _fileId, const boost::mpi::communicator & _commPtr)
        : name(_name), fileId(_fileId), commPtr(_commPtr) {}

    template<typename T>
    void operator()(T);

    template<typename T>
    void writeRecursive(const vistle::ShmVector<T> & vec, long double totalWriteSize, unsigned writeSize, unsigned writeIndex);

    void writeDummy();
};


//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR FUNCTION DEFINITIONS
//-------------------------------------------------------------------------

// RESERVATION INFO SHM ENTRY - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template <class Archive>
void WriteHDF5::ReservationInfoShmEntry::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & nvpTag;
   ar & type;
   ar & size;
}

// RESERVATION INFO - < OPERATOR
// * needed for std::sort of ReservationInfo
//-------------------------------------------------------------------------
bool WriteHDF5::ReservationInfo::operator<(const ReservationInfo &other) const {
    return (name < other.name);
}

// RESERVATION INFO - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template <class Archive>
void WriteHDF5::ReservationInfo::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & shmVectors;
   ar & isValid;
   ar & block;
   ar & timestep;
   ar & origin;
}

// SHM VECTOR RESERVER - () OPERATOR
// * manifests construction of ReservationInfoShmEntry entries when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorReserver::operator()(T) {
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        auto nativeTypeMapIter = WriteHDF5::nativeTypeMap.find(typeid(T));

        // check wether the needed vector type is not supported within the nativeTypeMap
        assert(nativeTypeMapIter != WriteHDF5::nativeTypeMap.end());

        // store reservation info
        reservationInfo->shmVectors.push_back(ReservationInfoShmEntry(name, nvpTag, nativeTypeMapIter->second, vec->size()));
    }
}

// SHM VECTOR WRITER - () OPERATOR
// * facilitates writing of shmVector data to the HDF5 file when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorWriter::operator()(T) {
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        long double totalWriteSize; // in bytes
        long double vecSize = vec->size();
        boost::mpi::all_reduce(commPtr, vecSize, totalWriteSize, std::plus<long double>());

        // write
        writeRecursive(vec, totalWriteSize, vec->size(), 0);
    }
}

// SHM VECTOR WRITER - RECURSIVE WRITES
// * splits up arrays to write them recursively into the hdf5 file
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorWriter::writeRecursive(const vistle::ShmVector<T> & vec, long double totalWriteSize, unsigned writeSize, unsigned writeIndex) {

    if (totalWriteSize / BYTES_IN_GB > MAX_WRITE_GB) {
        writeRecursive(vec, totalWriteSize / 2, std::floor(writeSize / 2), writeIndex);
        writeRecursive(vec, totalWriteSize / 2, std::ceil(writeSize / 2), writeIndex + std::floor(writeSize / 2));

    } else {
        if (writeSize == 0) {
            WriteHDF5::util_HDF5write(fileId);
        } else {
            auto nativeTypeMapIter = WriteHDF5::nativeTypeMap.find(typeid(T));
            std::string writeName = "/array/" + name;
            hsize_t dims[] = {writeSize};
            hsize_t offset[] = {writeIndex};
            herr_t status;
            hid_t dataSetId;
            hid_t fileSpaceId;
            hid_t memSpaceId;
            hid_t writeId;


            // check wether the needed vector type is not supported within the nativeTypeMap
            assert(nativeTypeMapIter != WriteHDF5::nativeTypeMap.end());

            // set up parallel write
            writeId = H5Pcreate(H5P_DATASET_XFER);
            H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);


            // allocate data spaces
            dataSetId = H5Dopen(fileId , writeName.c_str(), H5P_DEFAULT);
            fileSpaceId = H5Dget_space(dataSetId);
            memSpaceId = H5Screate_simple(1, dims, NULL);
            H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, offset, NULL, dims, NULL);


            // write
            status = H5Dwrite(dataSetId, nativeTypeMapIter->second, memSpaceId, fileSpaceId, writeId, vec->data() + writeIndex);
            util_checkStatus(status);

            // release resources
            H5Sclose(fileSpaceId);
            H5Sclose(memSpaceId);
            H5Dclose(dataSetId);
            H5Pclose(writeId);
        }
    }
}

// SHM VECTOR WRITER - WRITE DUMMY
// * handles synchronisation and writing to the dummy object if no ShmVector is available to write
//-------------------------------------------------------------------------
void WriteHDF5::ShmVectorWriter::writeDummy() {
    long double totalWriteSize; // in bytes
    long double zero = 0;

    boost::mpi::all_reduce(commPtr, zero, totalWriteSize, std::plus<long double>());

    // handle synchronisation of collective writes when over 2gb mpio limit
    for (unsigned i = 0; i < (unsigned) std::ceil(totalWriteSize / (BYTES_IN_GB * MAX_WRITE_GB)); i++) {
        WriteHDF5::util_HDF5write(fileId);
    }

}

// MEMBER COUNTER - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MemberCounterArchive & WriteHDF5::MemberCounterArchive::operator&(T const & t) {

    m_counter++;

    return *this;
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive & WriteHDF5::MetaToArrayArchive::operator<<(T const & t) {

    // do nothing - this archive assumes all members that need to be saved are stored as name-value pairs

}

// META TO ARRAY - << OPERATOR: NVP
// * saves Meta member value into appropriate variable
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive & WriteHDF5::MetaToArrayArchive::operator<<(const boost::serialization::nvp<T> & t) {
    std::string memberName(t.name());

    if (memberName != "block" && memberName != "timestep") {
        m_array[m_insertIndex] = (double) t.const_value();
        m_insertIndex++;
    }

    return *this;
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive & WriteHDF5::MetaToArrayArchive::operator&(T const & t) {

    return *this << t;

}


#endif /* WRITEHDF5_H */
