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
#include <core/findobjectreferenceoarchive.h>
#include <core/shm.h>

#include "hdf5.h"

#include "../WriteHDF5/HDF5Objects.h"


//-------------------------------------------------------------------------
// WRITE HDF5 CLASS DECLARATION
//-------------------------------------------------------------------------
class ReadHDF5 : public vistle::Module {
 public:
    friend class boost::serialization::access;
    friend struct ShmVectorReader;

   ReadHDF5(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadHDF5();

 private:
   // typedefs
   typedef vistle::FindObjectReferenceOArchive::ReferenceType ReferenceType;

   // structs/functors
   struct LinkIterData;

   struct ReservationInfoShmEntry;
   struct ReservationInfo;

   struct MemberCounter;
   class ArrayToMetaArchive;

   struct ShmVectorReserver;
   struct ShmVectorReader;



   // overriden functions
   virtual bool parameterChanged(const vistle::Parameter *param) override;
   virtual bool prepare() override;
   virtual bool compute() override;
   virtual bool reduce(int timestep) override;

   // private helper functions
   static herr_t prepare_iterateOrigin(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
   static herr_t prepare_iterateTimestep(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
   static herr_t prepare_iterateBlock(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
   static herr_t prepare_iterateVariant(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);

   static herr_t prepare_processObject(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
   static herr_t prepare_processArrayContainer(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
   static herr_t prepare_processArrayLink(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);

   static long double util_readSyncStandby(const boost::mpi::communicator & comm, hid_t fileId);
   static long double util_syncAndGetReadSize(long double readSize, const boost::mpi::communicator & comm);
   void util_checkStatus(herr_t status);
   bool util_checkFile();

   inline static bool util_doesExist(htri_t exist) {
       return exist > 0;
   }



   // private member variables
   vistle::StringParameter *m_fileName;
   unsigned m_numPorts;

   bool m_isRootNode;
   bool m_unresolvedReferencesExist;
   std::unordered_map<std::string, std::string> m_arrayMap;                 //< array name in file -> array name in memory
   std::unordered_map<std::string, std::string> m_objectMap;                //< object name in file -> object name in memory
   std::vector<vistle::Object::ptr> m_objectPersistenceVector;              //< stores pointers to objects so that they are not cleared from memory
   hid_t m_dummyDatasetId;


   // private member constants
   static const hid_t m_dummyObjectType;
   static const std::vector<hsize_t> m_dummyObjectDims;
   static const std::string m_dummyObjectName;

public:
   static unsigned numMetaMembers;
   static const std::unordered_map<std::type_index, hid_t> nativeTypeMap;

};

//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR DEFINITIONS
//-------------------------------------------------------------------------

// LINK ITERATION DATA STRUCT
// * stores data needed for link iteration
//-------------------------------------------------------------------------
struct ReadHDF5::LinkIterData {
    std::string nvpName;
    vistle::FindObjectReferenceOArchive * archive;
    ReadHDF5 * callingModule;
    hid_t fileId;
    std::string groupPath;
    unsigned origin;
    int block;
    int timestep;

    LinkIterData(LinkIterData * other)
        : callingModule(other->callingModule), fileId(other->fileId), block(other->block), timestep(other->timestep) {}
    LinkIterData(ReadHDF5 * _callingModule, hid_t _fileId)
        : archive(nullptr), callingModule(_callingModule), fileId(_fileId) {}

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
struct ReadHDF5::ShmVectorReader {
    typedef std::unordered_map<std::string, std::string> NameMap;

    // members
    vistle::FindObjectReferenceOArchive * archive;
    std::string arrayNameInFile;
    std::string nvpName;
    NameMap & arrayMap;
    hid_t fileId;
    hid_t dummyDatasetId;
    const boost::mpi::communicator & comm;

    // constants
    const long double BYTES_IN_GB = 1073741824;
    const long double MPI_READ_LIMIT_GB = 2;
    long double MAX_READ_GB;

    ShmVectorReader(vistle::FindObjectReferenceOArchive * _archive,  std::string _arrayNameInFile, std::string _nvpName, NameMap & _arrayMap, hid_t _fileId, hid_t _dummyDatasetId, const boost::mpi::communicator & _comm)
        : archive(_archive), arrayNameInFile(_arrayNameInFile), nvpName(_nvpName), arrayMap(_arrayMap), fileId(_fileId), dummyDatasetId(_dummyDatasetId), comm(_comm) {
        // specify read limit while accounting for the max amount of space taken up by metadata reads, which are not split up when reads are too large
        MAX_READ_GB = MPI_READ_LIMIT_GB - (ReadHDF5::numMetaMembers - ReadHDF5::ArrayToMetaArchive::numExclusiveMembers) * sizeof(double) / BYTES_IN_GB;
    }

    template<typename T>
    void operator()(T);

    template<typename T>
    void readRecursive(const vistle::ShmVector<T> & vec, long double totalReadSize, unsigned readSize, unsigned readIndex);

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

}

// SHM VECTOR WRITER - () OPERATOR
// * facilitates writing of shmVector data to the HDF5 file when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void ReadHDF5::ShmVectorReader::operator()(T) {
    const vistle::ShmVector<T> foundArray = vistle::Shm::the().getArrayFromName<T>(archive->getVectorEntryByNvpName(nvpName)->referenceName);

    if (foundArray) {
        auto arrayMapIter = arrayMap.find(arrayNameInFile);
        if (arrayMapIter == arrayMap.end()) {
            // this is a new array
            vistle::ShmVector<T> &newArray = *((vistle::ShmVector<T> *) archive->getVectorEntryByNvpName(nvpName)->ref);
            std::string readName = "/array/" + arrayNameInFile;
            long double commWideReadSize;
            long double nodeCurrentReadSizeBytes;
            unsigned nodeCurrentReadSizeElements;
            unsigned readIndex;
            bool areMultipleReadsNeeded;
            bool isFirstRead = true;

            herr_t status;
            hid_t dataType;
            hid_t dataSetId;
            hid_t dataSpaceId;
            hid_t readId;
            hsize_t dims[] = {0};
            hsize_t maxDims[] = {0};


            arrayMap[arrayNameInFile] = archive->getVectorEntryByNvpName(nvpName)->referenceName;

            while (areMultipleReadsNeeded || isFirstRead) {
                // open dataset and obtain information needed for read
                dataSetId = H5Dopen2(fileId, readName.c_str(), H5P_DEFAULT);
                dataSpaceId = H5Dget_space(dataSetId);
                dataType = H5Dget_type(dataSetId);

                if (isFirstRead) {
                    status = H5Sget_simple_extent_dims(dataSpaceId, dims, maxDims);
                    newArray->resize(dims[0]);
                    nodeCurrentReadSizeElements = dims[0];
                    readIndex = 0;
                    isFirstRead = false;
                } else {
                    nodeCurrentReadSizeElements = newArray->size() - readIndex;
                }


                nodeCurrentReadSizeBytes = nodeCurrentReadSizeElements * sizeof(T);
                commWideReadSize = ReadHDF5::util_syncAndGetReadSize(nodeCurrentReadSizeBytes, comm);


                // perform read
                readId = H5Pcreate(H5P_DATASET_XFER);
                H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

                // read size exceptions and read
                if (nodeCurrentReadSizeElements == 0) {
                    // read size 0
                    double dummyVar;
                    status = H5Dread(dummyDatasetId, m_dummyObjectType, H5S_ALL, H5S_ALL, readId, &dummyVar);
                    areMultipleReadsNeeded = false;
                } else if (commWideReadSize / BYTES_IN_GB > MAX_READ_GB) {
                    // comm-wide read of over the HDF5 parallel read limit
                    hsize_t offset[] = {readIndex};
                    hid_t fileSpaceId;
                    hid_t memSpaceId;


                    nodeCurrentReadSizeBytes = (nodeCurrentReadSizeBytes / commWideReadSize) * MAX_READ_GB * BYTES_IN_GB;
                    nodeCurrentReadSizeElements = std::floor(nodeCurrentReadSizeBytes / sizeof(T));
                    dims[0] = nodeCurrentReadSizeElements;

                    fileSpaceId = H5Dget_space(dataSetId);
                    memSpaceId = H5Screate_simple(1, dims, NULL);
                    H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, offset, NULL, dims, NULL);

                    status = H5Dread(dataSetId, dataType, memSpaceId, fileSpaceId, readId, newArray->data() + readIndex);

                    // release resources
                    H5Sclose(fileSpaceId);
                    H5Sclose(memSpaceId);

                    // prepare for next read, break loop if next read size will be 0
                    readIndex += nodeCurrentReadSizeElements;
                    areMultipleReadsNeeded = !(readIndex == newArray->size() - 1);

                } else {
                    // normal read
                    status = H5Dread(dataSetId, dataType, H5S_ALL, H5S_ALL, readId, newArray->data());
                    areMultipleReadsNeeded = false;
                }


                if (status != 0) {
                    assert("error: in shmVector read" == NULL);
                }

                // release resources
                H5Pclose(readId);
                H5Sclose(dataSpaceId);
                H5Dclose(dataSetId);

                std::cerr << " --- write dims : " << arrayNameInFile << "->" << archive->getVectorEntryByNvpName(nvpName)->referenceName << " - " << dims[0] << std::endl;

            }

        } else {
            // this array already exists in shared memory
            vistle::ShmVector<T> existingArray = vistle::Shm::the().getArrayFromName<T>(arrayMap[arrayNameInFile]);

            // error debug message
            if (!existingArray) {
                assert("error: existing array not found" == NULL);
            }

            // replace current object array with existing array
            *((vistle::ShmVector<T> *) archive->getVectorEntryByNvpName(nvpName)->ref) = existingArray;
        }
    }
}

// SHM VECTOR READER - RECURSIVE READ
// * splits up arrays to read them recursively into the hdf5 file
//-------------------------------------------------------------------------
template<typename T>
void ReadHDF5::ShmVectorReader::readRecursive(const vistle::ShmVector<T> & vec, long double totalReadSize, unsigned readSize, unsigned readIndex) {
    if (totalReadSize / BYTES_IN_GB > MAX_READ_GB) {
        readRecursive(vec, totalReadSize / 2, std::floor(readSize / 2), readIndex);
        readRecursive(vec, totalReadSize / 2, std::ceil(readSize / 2), readIndex + std::floor(readIndex / 2));

    } else {
        std::string readName = "/array/" + arrayNameInFile;
        hsize_t dims[] = {readSize};
        hsize_t offset[] = {readIndex};
        herr_t status;
        hid_t dataType;
        hid_t dataSetId;
        hid_t fileSpaceId;
        hid_t memSpaceId;
        hid_t readId;

        if (comm.rank() == 0)
            std::cerr <<"writing boi" << std::endl;


        // open dataset and obtain information needed for read
        dataSetId = H5Dopen2(fileId, readName.c_str(), H5P_DEFAULT);
        fileSpaceId = H5Dget_space(dataSetId);
        dataType = H5Dget_type(dataSetId);
        memSpaceId = H5Screate_simple(1, dims, NULL);
        H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, offset, NULL, dims, NULL);

        ReadHDF5::util_syncAndGetReadSize(vec->size() * sizeof(T), comm);


        // perform read
        readId = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

        // handle size 0 reads
        if (dims[0] == 0) {

            std::fstream fs;
              fs.open ("log", std::fstream::in | std::fstream::out | std::fstream::app);
              fs << " recurs reading 0\n";
              fs.close();

            double dummyVar;
            status = H5Dread(dummyDatasetId, m_dummyObjectType, H5S_ALL, H5S_ALL, readId, &dummyVar);
        } else {
            status = H5Dread(dataSetId, dataType, memSpaceId, fileSpaceId, readId, vec->data() + readIndex);
        }

        if (status != 0) {
            assert("error: in shmVector read" == NULL);
        }

        // release resources
        H5Sclose(fileSpaceId);
        H5Sclose(memSpaceId);
        H5Pclose(readId);
        H5Dclose(dataSetId);
    }
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive & ReadHDF5::ArrayToMetaArchive::operator<<(T const & t) {

    // do nothing - this archive assumes all members that need to be saved are stored as name-value pairs

    return *this;
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

    return *this;
}

// META TO ARRAY - & OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive & ReadHDF5::ArrayToMetaArchive::operator&(T const & t) {

    return *this << t;

}

#endif /* READHDF5_H */
