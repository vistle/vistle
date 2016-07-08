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

#include <module/module.h>

#include <core/vec.h>
#include <core/unstr.h>
#include <core/pointeroarchive.h>
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

   // structs/functors
   struct ReservationInfoShmEntry;
   struct ReservationInfo;

   struct MemberCounter;
   struct MetaToArray;

   struct ShmVectorReserver;
   struct ShmVectorWriter;



   // overriden functions
   virtual bool parameterChanged(const vistle::Parameter * p);
   virtual bool prepare();
   virtual bool compute();
   virtual bool reduce(int timestep);

   // private helper functions
   static void util_checkStatus(herr_t status);
   static void util_HDF5write(bool isWriter, std::string name, const void * data, hid_t fileId, hsize_t * dims, hid_t dataType);
   static void util_HDF5write(hid_t fileId);



   // private member variables
   vistle::StringParameter *m_fileName;
   unsigned m_numPorts;

   bool m_isRootNode;
   bool m_hasObject;
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
const hid_t WriteHDF5::m_dummyObjectType = H5T_NATIVE_INT;
const std::vector<hsize_t> WriteHDF5::m_dummyObjectDims = {1};
const std::string WriteHDF5::m_dummyObjectName = "/dummy";

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
struct WriteHDF5::ReservationInfo {
    std::string name;
    std::vector<ReservationInfoShmEntry> shmVectors;
    bool isValid;

    // < overload for std::sort
    bool operator<(const ReservationInfo &other) const;

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// MEMBER COUNTER FUNCTOR
// * used to count members within ObjectMeta
//-------------------------------------------------------------------------
struct WriteHDF5::MemberCounter {
    mutable unsigned counter;

    MemberCounter() : counter(0) {}

    template<class Member>
    void operator()(Member) const { counter++; }
};

// META TO ARRAY FUNCTOR
// * used to convert metadata members to an array of doubles for storage
// * into the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::MetaToArray {
private:
    mutable std::vector<double> array;
    mutable unsigned insertIndex;

public:
    MetaToArray() : insertIndex(0) { array.reserve(WriteHDF5::numMetaMembers); }

    double * getDataPtr() { return array.data(); }

    template<class Member>
    void operator()(const Member &member) const;
};

// SHM VECTOR RESERVER FUNCTOR
// * used to construct ReservationInfoShmEntry entries within the shmVectors
// * member of ReservationInfo
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorReserver {
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
struct WriteHDF5::ShmVectorWriter {
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
void WriteHDF5::ReservationInfoShmEntry::serialize(Archive &ar, const unsigned int version) {
   ar & name;
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
}

// META TO ARRAY - () OPERATOR
// * manifests conversion and storage of members to an array of doubles
//-------------------------------------------------------------------------
template<class Member>
void WriteHDF5::MetaToArray::operator()(const Member &member) const {
     array[insertIndex] = (double) member;
     insertIndex++;
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
        reservationInfo->shmVectors.push_back(ReservationInfoShmEntry(name, nativeTypeMapIter->second, vec->size()));
    }
}

// SHM VECTOR WRITER - () OPERATOR
// * facilitates writing of shmVector data to the HDF5 file when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorWriter::operator()(T) {
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        auto nativeTypeMapIter = WriteHDF5::nativeTypeMap.find(typeid(T));
        std::string writeName = "/array/" + name;
        hsize_t dims[] = {vec->size()};

        // check wether the needed vector type is not supported within the nativeTypeMap
        assert(nativeTypeMapIter != WriteHDF5::nativeTypeMap.end());

        //write
        WriteHDF5::util_HDF5write(true, writeName, vec->data(), fileId, dims, nativeTypeMapIter->second);

    }
}

#endif /* WRITEHDF5_H */
