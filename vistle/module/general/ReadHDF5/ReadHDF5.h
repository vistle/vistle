//-------------------------------------------------------------------------
// READ HDF5 H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef READHDF5_H
#define READHDF5_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/mpl/for_each.hpp>

#include <module/module.h>

#include <core/object.h>
#include <core/object_impl.h>
#include <core/shmvectoroarchive.h>
#include <core/shm.h>

#include "hdf5.h"

#include "../WriteHDF5/HDF5Objects.h"


//-------------------------------------------------------------------------
// WRITE HDF5 CLASS DECLARATION
//-------------------------------------------------------------------------
class ReadHDF5 : public vistle::Module {
 public:
    friend class boost::serialization::access;
    friend struct ShmVectorWriter;

   ReadHDF5(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadHDF5();

 private:

   // structs/functors
   struct LinkIterData;

   struct ReservationInfoShmEntry;
   struct ReservationInfo;

   struct MemberCounter;
   class ArrayToMetaArchive;

   struct ShmVectorReserver;
   struct ShmVectorWriter;



   // overriden functions
   virtual bool parameterChanged();
   virtual bool prepare();
   virtual bool compute();
   virtual bool reduce(int timestep);

   // private helper functions
   static herr_t prepare_processObject(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
   void util_readSync(bool isReader);
   void util_checkStatus(herr_t status);
   bool util_checkFile();



   // private member variables
   vistle::StringParameter *m_fileName;
   unsigned m_numPorts;

   bool m_isRootNode;
   std::unordered_set<std::string> m_arraySet;


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
const hid_t ReadHDF5::m_dummyObjectType = H5T_NATIVE_INT;
const std::vector<hsize_t> ReadHDF5::m_dummyObjectDims = {1};
const std::string ReadHDF5::m_dummyObjectName = "/dummy";

unsigned ReadHDF5::numMetaMembers = 0;

const std::unordered_map<std::type_index, hid_t> ReadHDF5::nativeTypeMap = {
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

// LINK ITERATION DATA STRUCT
// * stores data needed for link iteration
//-------------------------------------------------------------------------
struct ReadHDF5::LinkIterData {
    ReadHDF5 * callingModule;
    hid_t fileId;
    unsigned origin;
    int block;
    int timestep;

    LinkIterData(ReadHDF5 * _callingModule, hid_t _fileId, unsigned _origin, int _block, int _timestep)
        : callingModule(_callingModule), fileId(_fileId), origin(_origin), block(_block), timestep(_timestep) {}

};

// RESERVATION INFO SHM ENTRY STRUCT
// * stores ShmVector data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct ReadHDF5::ReservationInfoShmEntry {
    std::string name;
    hid_t type;
    unsigned size;

    ReservationInfoShmEntry() {}
    ReservationInfoShmEntry(std::string _name, hid_t _type, unsigned _size)
        : name(_name), type(_type), size(_size) {}

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// RESERVATION INFO STRUCT
// * stores data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct ReadHDF5::ReservationInfo {
    std::string name;
    std::vector<ReservationInfoShmEntry> shmVectors;
    bool isValid;

    // < overload for std::sort
    bool operator<(const ReservationInfo &other) const;

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};


// META TO ARRAY ARCHIVE
// * used to convert metadata members to an array of doubles for storage
// * into the HDF5 file
//-------------------------------------------------------------------------
class ReadHDF5::ArrayToMetaArchive {
private:
    std::vector<double> m_array;
    unsigned m_insertIndex;
    int m_block;
    int m_timestep;

public:
    static const int numExclusiveMembers;

    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL) {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    ArrayToMetaArchive(double * _array, int _block, int _timestep) : m_insertIndex(0), m_block(_block), m_timestep(_timestep) {
        m_array.assign(_array, _array + (ReadHDF5::numMetaMembers - numExclusiveMembers));
    }

    // << operators
    template<class T>
    ArrayToMetaArchive & operator<<(T const & t);
    template<class T>
    ArrayToMetaArchive & operator<<(const boost::serialization::nvp<T> & t);

    // the & operator
    template<class T>
    ArrayToMetaArchive & operator&(const T & t);
};

// static memeber out-of-class declaration
const int ReadHDF5::ArrayToMetaArchive::numExclusiveMembers = 2;

// SHM VECTOR RESERVER FUNCTOR
// * used to construct ReservationInfoShmEntry entries within the shmVectors
// * member of ReservationInfo
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct ReadHDF5::ShmVectorReserver {
    std::string name;
    ReservationInfo * reservationInfo;

    ShmVectorReserver(std::string _name, ReservationInfo * _reservationInfo) : name(_name), reservationInfo(_reservationInfo) {}

    template<typename T>
    void operator()(T);
};

// SHM VECTOR WRITER FUNCTOR
// * calls util_HDF5Write for a shmVector
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct ReadHDF5::ShmVectorWriter {
    std::string name;
    hid_t fileId;

    ShmVectorWriter(std::string _name, hid_t _fileId)
        : name(_name), fileId(_fileId) {}

    template<typename T>
    void operator()(T);
};


//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR FUNCTION DEFINITIONS
//-------------------------------------------------------------------------

// RESERVATION INFO SHM ENTRY - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template <class Archive>
void ReadHDF5::ReservationInfoShmEntry::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & type;
   ar & size;
}

// RESERVATION INFO - < OPERATOR
// * needed for std::sort of ReservationInfo
//-------------------------------------------------------------------------
bool ReadHDF5::ReservationInfo::operator<(const ReservationInfo &other) const {
    return (name < other.name);
}

// RESERVATION INFO - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template <class Archive>
void ReadHDF5::ReservationInfo::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & shmVectors;
   ar & isValid;
}

// SHM VECTOR RESERVER - () OPERATOR
// * manifests construction of ReservationInfoShmEntry entries when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void ReadHDF5::ShmVectorReserver::operator()(T) {
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        auto nativeTypeMapIter = ReadHDF5::nativeTypeMap.find(typeid(T));

        // check wether the needed vector type is not supported within the nativeTypeMap
        assert(nativeTypeMapIter != ReadHDF5::nativeTypeMap.end());

        // store reservation info
        reservationInfo->shmVectors.push_back(ReservationInfoShmEntry(name, nativeTypeMapIter->second, vec->size()));
    }
}

// SHM VECTOR WRITER - () OPERATOR
// * facilitates writing of shmVector data to the HDF5 file when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void ReadHDF5::ShmVectorWriter::operator()(T) {
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        auto nativeTypeMapIter = ReadHDF5::nativeTypeMap.find(typeid(T));
        std::string writeName = "/array/" + name;
        hsize_t dims[] = {vec->size()};

        // check wether the needed vector type is not supported within the nativeTypeMap
        assert(nativeTypeMapIter != ReadHDF5::nativeTypeMap.end());

        //write
        //ReadHDF5::util_HDF5write(true, writeName, vec->data(), fileId, dims, nativeTypeMapIter->second);

    }
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive & ReadHDF5::ArrayToMetaArchive::operator<<(T const & t) {

    // do nothing - this archive assumes all members that need to be saved are stored as name-value pairs

}

// META TO ARRAY - << OPERATOR: NVP
// * saves Meta member value into appropriate variable
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive & ReadHDF5::ArrayToMetaArchive::operator<<(const boost::serialization::nvp<T> & t) {
    std::string memberName(t.name());

    if (memberName == "block") {
        t.value() = m_block;

    } else if (memberName == "timestep") {
        t.value() = m_timestep;

    } else {
        t.value() = m_array[m_insertIndex];
        m_insertIndex++;
    }

    std::cerr << " --- " << t.name() << " - " << t.value() << " --- " << std::endl;

    return *this;
}

// META TO ARRAY - & OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive & ReadHDF5::ArrayToMetaArchive::operator&(T const & t) {

    return *this << t;

}

#endif /* READHDF5_H */
