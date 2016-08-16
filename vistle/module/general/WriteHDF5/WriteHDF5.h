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

#include <boost/filesystem.hpp>
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

#include "HDF5Objects.h"


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
   typedef vistle::FindObjectReferenceOArchive::ReferenceType ReferenceType;

   // IndexTracker: maps: timestep (map) -> block (map) -> portNumber (map) -> variants (unsigned)
   // maps are needed for block and timestep because they can have values of -1
   // a more performant IndexTracker could be achieved with vectors, but I chose to do it this way for reasons of code clarity
   typedef std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, unsigned>>> IndexTracker;

   // structs/functors
   struct ObjectWriteInfoReferenceEntry;
   struct ObjectWriteInfo;
   struct IndexWriteInfo;

   template<class T>
   struct VectorConcatenator;

   struct ShmVectorReserver;
   struct ShmVectorWriter;

   class MetaToArrayArchive;

   // overriden functions
   virtual void connectionRemoved(const vistle::Port *from, const vistle::Port *to) override;
   virtual void connectionAdded(const vistle::Port *from, const vistle::Port *to) override;
   virtual bool prepare() override;
   virtual bool compute() override;
   virtual bool reduce(int timestep) override;

   // private helper functions
   bool prepare_fileNameCheck();
   void compute_addObjectToWrite(vistle::Object::const_ptr obj, unsigned originPortNumber, unsigned &numNodeObjects, unsigned &numNodeArrays,
                                 std::vector<vistle::Object::const_ptr> &objPtrVector,
                                 std::vector<vistle::FindObjectReferenceOArchive> &objRefArchiveVector,
                                 std::vector<MetaToArrayArchive> &metaToArrayArchiveVector,
                                 std::vector<ObjectWriteInfo> &objectWriteVector,
                                 std::vector<IndexWriteInfo> &indexWriteVector);
   void util_checkId(hid_t group, const std::string &info) const;
   static void util_checkStatus(herr_t status);
   static void util_HDF5write(bool isWriter, std::string name, const void * data, hid_t fileId, hsize_t * dims, hid_t dataType);
   static void util_HDF5write(hid_t fileId);



   // private member variables
   vistle::StringParameter *m_fileName;
   vistle::IntParameter *m_overwrite;
   std::vector<vistle::StringParameter *> m_portDescriptions;
   unsigned m_numPorts;

   hid_t m_fileId;
   bool m_isRootNode;
   bool m_doNotWrite;
   bool m_unresolvedReferencesExist;
   std::unordered_map<std::string, bool> m_arrayMap;    //< existence determines wether space has been allocated in file, bool describes wether array has actually been written
   std::unordered_set<std::string> m_objectSet;         //< existence determines wether entry has been written yet
   std::vector<std::string> m_metaNvpTags;
   IndexTracker m_indexVariantTracker;                  //< used to keep track of the number of variants in each timestep/block/port

public:
   static unsigned s_numMetaMembers;
   static const std::unordered_map<std::type_index, hid_t> s_nativeTypeMap;
};


//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR DEFINITIONS
//-------------------------------------------------------------------------

// RESERVATION INFO SHM ENTRY STRUCT
// * stores ShmVector data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::ObjectWriteInfoReferenceEntry {
    typedef vistle::FindObjectReferenceOArchive::ReferenceType ReferenceType;

    std::string name;
    std::string nvpTag;
    ReferenceType referenceType;
    hid_t type;
    unsigned size;


    ObjectWriteInfoReferenceEntry() {}
    ObjectWriteInfoReferenceEntry(std::string _name, std::string _nvpTag, hid_t _type, unsigned _size)
        : name(_name), nvpTag(_nvpTag), referenceType(ReferenceType::ShmVector), type(_type), size(_size) {}
    ObjectWriteInfoReferenceEntry(std::string _name, std::string _nvpTag)
        : name(_name), nvpTag(_nvpTag), referenceType(ReferenceType::ObjectReference) {}

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// OBJECT WRITE RESERVATION INFO STRUCT
// * stores data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::ObjectWriteInfo {
    std::string name;
    std::vector<WriteHDF5::ObjectWriteInfoReferenceEntry> referenceVector;

    ObjectWriteInfo() {}
    ObjectWriteInfo(std::string _name)
        : name(_name) {}

    // < overload for std::sort
    bool operator<(const ObjectWriteInfo &other) const;

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// INDEX WRITERESERVATION INFO STRUCT
// * stores data needed for creation of index within the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::IndexWriteInfo {
    std::string name;
    int block;
    int timestep;
    unsigned origin;

    IndexWriteInfo() {}
    IndexWriteInfo(std::string _name, int _block, int _timestep, unsigned _origin)
        : name(_name), block(_block), timestep(_timestep), origin(_origin) {}

    // < overload for std::sort
    bool operator<(const IndexWriteInfo &other) const;

    // serialization method for passing over mpi
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version);
};



// VECTOR CONCATENATOR FUNCTOR
// * used in mpi reduce call
//-------------------------------------------------------------------------
template<class T>
struct WriteHDF5::VectorConcatenator {
    std::vector<T> & operator()(std::vector<T> &vec1, const std::vector<T> &vec2) const {
        vec1.insert(vec1.end(), vec2.begin(), vec2.end());
        return vec1;
    }
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

    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL) {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    MetaToArrayArchive() : m_insertIndex(0) { m_array.resize(WriteHDF5::s_numMetaMembers); }

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

// SHM VECTOR RESERVER FUNCTOR
// * used to construct ObjectWriteInfoReferenceEntry entries within the shmVectors
// * member of ObjectWriteInfo
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorReserver {
    std::string name;
    std::string nvpTag;
    WriteHDF5::ObjectWriteInfo * reservationInfo;

    ShmVectorReserver(std::string _name, std::string _nvpTag, WriteHDF5::ObjectWriteInfo * _reservationInfo)
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

    ShmVectorWriter(hid_t _fileId, const boost::mpi::communicator & _commPtr)
        : fileId(_fileId), commPtr(_commPtr) {}
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
void WriteHDF5::ObjectWriteInfoReferenceEntry::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & nvpTag;
   ar & referenceType;
   ar & type;
   ar & size;
}

// OBJECT WRITE RESERVATION INFO - < OPERATOR
// * needed for std::sort of ObjectWriteInfo
//-------------------------------------------------------------------------
bool WriteHDF5::ObjectWriteInfo::operator<(const ObjectWriteInfo &other) const {
    return (name < other.name);
}

// INDEX WRITE RESERVATION INFO - < OPERATOR
// * needed for std::sort of IndexWriteInfo
//-------------------------------------------------------------------------
bool WriteHDF5::IndexWriteInfo::operator<(const IndexWriteInfo &other) const {
    return (name < other.name);
}

// OBJECT WRITE RESERVATION INFO - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template <class Archive>
void WriteHDF5::ObjectWriteInfo::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & referenceVector;
}

// INDEX WRITE RESERVATION INFO - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template <class Archive>
void WriteHDF5::IndexWriteInfo::serialize(Archive &ar, const unsigned int version) {
   ar & name;
   ar & block;
   ar & timestep;
   ar & origin;
}

// SHM VECTOR RESERVER - () OPERATOR
// * manifests construction of ObjectWriteInfoReferenceEntry entries when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorReserver::operator()(T) {
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        auto nativeTypeMapIter = WriteHDF5::s_nativeTypeMap.find(typeid(T));

        // check wether the needed vector type is not supported within the nativeTypeMap
        assert(nativeTypeMapIter != WriteHDF5::s_nativeTypeMap.end());

        // store reservation info
        reservationInfo->referenceVector.push_back(ObjectWriteInfoReferenceEntry(name, nvpTag, nativeTypeMapIter->second, vec->size()));
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
        long double vecSize = vec->size() * sizeof(T);
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

    if (totalWriteSize / HDF5Const::numBytesInGb > HDF5Const::mpiReadWriteLimitGb) {
        writeRecursive(vec, totalWriteSize / 2, std::floor(writeSize / 2), writeIndex);
        writeRecursive(vec, totalWriteSize / 2, std::ceil(writeSize / 2), writeIndex + std::floor(writeSize / 2));

    } else {
        if (writeSize == 0) {
            WriteHDF5::util_HDF5write(fileId);

        } else {
            auto nativeTypeMapIter = WriteHDF5::s_nativeTypeMap.find(typeid(T));
            std::string writeName = "/array/" + name;
            hsize_t dims[] = {writeSize};
            hsize_t offset[] = {writeIndex};
            herr_t status;
            hid_t dataSetId;
            hid_t fileSpaceId;
            hid_t memSpaceId;
            hid_t writeId;

            // check wether the needed vector type is not supported within the nativeTypeMap
            assert(nativeTypeMapIter != WriteHDF5::s_nativeTypeMap.end());

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
    for (unsigned i = 0; i < (unsigned) std::ceil(totalWriteSize / (HDF5Const::numBytesInGb * HDF5Const::mpiReadWriteLimitGb)); i++) {
        WriteHDF5::util_HDF5write(fileId);
    }

}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive & WriteHDF5::MetaToArrayArchive::operator<<(T const & t) {

    // do nothing - this archive assumes all members that need to be saved are stored as name-value pairs

    return *this;
}

// META TO ARRAY - << OPERATOR: NVP
// * saves Meta member value into appropriate variable
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive & WriteHDF5::MetaToArrayArchive::operator<<(const boost::serialization::nvp<T> & t) {
    m_array[m_insertIndex] = (double) t.const_value();
    m_insertIndex++;

    return *this;
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive & WriteHDF5::MetaToArrayArchive::operator&(T const & t) {

    return *this << t;

}


#endif /* WRITEHDF5_H */
