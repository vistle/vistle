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

#include <vistle/core/findobjectreferenceoarchive.h>
#include <vistle/core/object.h>
#include <vistle/core/object_impl.h>
#include <vistle/core/shm.h
#include <vistle/module/module.h>>

#include "hdf5.h"

#define HDF5Const ReadHDF5Const
#include "../WriteHDF5/HDF5Objects.h"


//-------------------------------------------------------------------------
// WRITE HDF5 CLASS DECLARATION
//-------------------------------------------------------------------------
class ReadHDF5: public vistle::Module {
public:
    friend class boost::serialization::access;
    friend struct ShmVectorReader;

    ReadHDF5(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadHDF5();

private:
    // typedefs
    typedef vistle::FindObjectReferenceOArchive::ReferenceType ReferenceType;

    // structs/functors
    struct LinkIterData;

    struct MemberCounter;
    class ArrayToMetaArchive;

    struct ShmVectorReader;


    // overridden functions
    virtual bool changeParameter(const vistle::Parameter *param) override;
    virtual bool prepare() override;
    virtual bool compute() override;
    virtual bool reduce(int timestep) override;

    // private helper functions
    void prepare_organized(hid_t fileId);
    void prepare_performant(hid_t fileId);

    template<class T>
    void prepare_performant_readHDF5(hid_t fileId, const boost::mpi::communicator &comm, const char *readName,
                                     unsigned rank, hsize_t *nodeDims, hsize_t *nodeOffset, T *data);
    std::vector<hsize_t> prepare_performant_getArrayDims(hid_t fileId, char readName[]);

    static herr_t prepare_iterateMeta(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);

    static herr_t prepare_iterateTimestep(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
    static herr_t prepare_iterateBlock(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
    static herr_t prepare_iterateOrigin(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
    static herr_t prepare_iterateVariant(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);

    static herr_t prepare_processObject(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData);
    static herr_t prepare_processArrayContainer(hid_t callingGroupId, const char *name, const H5L_info_t *info,
                                                void *opData);
    static herr_t prepare_processArrayLink(hid_t callingGroupId, const char *name, const H5L_info_t *info,
                                           void *opData);

    static long double util_readSyncStandby(const boost::mpi::communicator &comm, hid_t fileId);
    static long double util_syncAndGetReadSize(long double readSize, const boost::mpi::communicator &comm);
    void util_checkStatus(herr_t status);
    bool util_checkFile();

    inline static bool util_doesExist(htri_t exist) { return exist > 0; }


    // private member variables
    vistle::StringParameter *m_fileName;
    unsigned m_numPorts;
    int m_writeMode;

    bool m_isRootNode;
    bool m_unresolvedReferencesExist;
    std::unordered_map<std::string, unsigned> m_metaNvpMap; //< meta nvp tag -> index in boost::serialization order
    std::unordered_map<std::string, std::string> m_arrayMap; //< array name in file -> array name in memory
    std::unordered_map<std::string, std::string> m_objectMap; //< object name in file -> object name in memory
    std::vector<vistle::Object::ptr>
        m_objectPersistenceVector; //< stores pointers to objects so that they are not cleared from memory
    hid_t m_dummyDatasetId;

public:
    static unsigned s_numMetaMembers;
};

//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR DEFINITIONS
//-------------------------------------------------------------------------

// LINK ITERATION DATA STRUCT
// * stores data needed for link iteration
//-------------------------------------------------------------------------
struct ReadHDF5::LinkIterData {
    std::string nvpName;
    vistle::FindObjectReferenceOArchive *archive;
    ReadHDF5 *callingModule;
    hid_t fileId;
    std::string groupPath;
    unsigned origin;
    int block;
    int timestep;

    LinkIterData(LinkIterData *other)
    : callingModule(other->callingModule), fileId(other->fileId), block(other->block), timestep(other->timestep)
    {}
    LinkIterData(ReadHDF5 *_callingModule, hid_t _fileId)
    : archive(nullptr), callingModule(_callingModule), fileId(_fileId)
    {}
};

// META TO ARRAY ARCHIVE
// * used to convert metadata members from their stored array form to member form
//-------------------------------------------------------------------------
class ReadHDF5::ArrayToMetaArchive {
private:
    std::vector<double> m_array;
    std::unordered_map<std::string, unsigned> *m_nvpMapPtr;

public:
    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL)
    {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    ArrayToMetaArchive(double *_array, std::unordered_map<std::string, unsigned> *_nvpMapPtr): m_nvpMapPtr(_nvpMapPtr)
    {
        m_array.assign(_array, _array + (ReadHDF5::s_numMetaMembers));
    }

    // << operators
    template<class T>
    ArrayToMetaArchive &operator<<(T const &t);
    template<class T>
    ArrayToMetaArchive &operator<<(const boost::serialization::nvp<T> &t);
    template<class U>
    ArrayToMetaArchive &operator<<(const boost::serialization::nvp<const boost::serialization::array_wrapper<U>> &t);

    // the & operator
    template<class T>
    ArrayToMetaArchive &operator&(const T &t);
};

// SHM VECTOR READER FUNCTOR
// * reads a shmVector
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct ReadHDF5::ShmVectorReader {
    typedef std::unordered_map<std::string, std::string> NameMap;

    // members
    vistle::FindObjectReferenceOArchive *archive;
    std::string arrayNameInFile;
    std::string nvpName;
    NameMap &arrayMap;
    hid_t fileId;
    hid_t dummyDatasetId;
    const boost::mpi::communicator &comm;

    long double maxReadSizeGb;

    ShmVectorReader(vistle::FindObjectReferenceOArchive *_archive, std::string _arrayNameInFile, std::string _nvpName,
                    NameMap &_arrayMap, hid_t _fileId, hid_t _dummyDatasetId, const boost::mpi::communicator &_comm)
    : archive(_archive)
    , arrayNameInFile(_arrayNameInFile)
    , nvpName(_nvpName)
    , arrayMap(_arrayMap)
    , fileId(_fileId)
    , dummyDatasetId(_dummyDatasetId)
    , comm(_comm)
    {
        // specify read limit while accounting for the max amount of space taken up by metadata reads, which are not split up when reads are too large
        unsigned totalMetaMembers = ReadHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers;
        maxReadSizeGb = HDF5Const::mpiReadWriteLimitGb - totalMetaMembers * sizeof(double) / HDF5Const::numBytesInGb;
    }

    template<typename T>
    void operator()(T);

    template<typename T>
    void readRecursive(const vistle::ShmVector<T> &vec, long double totalReadSize, unsigned readSize,
                       unsigned readIndex);
};


//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR FUNCTION DEFINITIONS
//-------------------------------------------------------------------------

// GENERIC UTILITY HELPER FUNCTION - READ DATA FROM HDF5 ABSTRACTION - PERFORMANT
//-------------------------------------------------------------------------
template<class T>
void ReadHDF5::prepare_performant_readHDF5(hid_t fileId, const boost::mpi::communicator &comm, const char *readName,
                                           unsigned rank, hsize_t *nodeDims, hsize_t *nodeOffset, T *data)
{
    std::vector<hsize_t> dims;
    std::vector<hsize_t> offset;
    std::vector<hsize_t> totalDims(rank);
    herr_t status;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t memSpaceId;
    hid_t readId;
    hid_t dataType;

    dims.assign(nodeDims, nodeDims + rank);
    offset.assign(nodeOffset, nodeOffset + rank);

    // obtain total size of the array
    boost::mpi::all_reduce(comm, dims[0], totalDims[0], std::plus<hsize_t>());
    for (unsigned i = 1; i < totalDims.size(); i++) {
        totalDims[i] = dims[i];
    }

    // abort write if dataset is empty
    if (totalDims[0] == 0) {
        return;
    }

    // open dataspace
    dataSetId = H5Dopen2(fileId, readName, H5P_DEFAULT);
    dataType = H5Dget_type(dataSetId);

    // set up parallel read
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    long double totalWriteSize = 1;
    for (unsigned i = 0; i < rank; i++) {
        totalWriteSize *= totalDims[i];
    }

    // obtain divisions and perform a set of necessary reads in order to keep under the 2gb MPIO limit
    // limit lies in number of elements collectively within a write, not total write size based off my tests (i.e. 2e8 elements, not 2e8 bytes)
    const long double writeLimit = HDF5Const::mpiReadWriteLimitGb * HDF5Const::numBytesInGb;
    unsigned numWriteDivisions = std::ceil(totalWriteSize / writeLimit);
    unsigned nodeCutoffWriteIndex = std::ceil((double)nodeDims[0] / numWriteDivisions);
    for (unsigned i = 0; i < numWriteDivisions; i++) {
        // handle size zero nodeCutoffWriteIndex to avoid floating point exception with % operator
        if (nodeDims[0] != 0) {
            dims[0] = (i == numWriteDivisions - 1) ? nodeDims[0] % nodeCutoffWriteIndex : nodeCutoffWriteIndex;
            offset[0] = nodeOffset[0] + i * nodeCutoffWriteIndex;

            // handle case where % would evaluate to 0
            if (nodeDims[0] % nodeCutoffWriteIndex == 0) {
                dims[0] = nodeCutoffWriteIndex;
            }

        } else {
            dims[0] = 0;
            offset[0] = 0;
        }

        // allocate data spaces
        memSpaceId = H5Screate_simple(rank, dims.data(), NULL);
        fileSpaceId = H5Dget_space(dataSetId);
        H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, offset.data(), NULL, dims.data(), NULL);

        if (dims[0] == 0 || data == nullptr) {
            H5Sselect_none(fileSpaceId);
            H5Sselect_none(memSpaceId);
        }

        unsigned dataOffset = i * nodeCutoffWriteIndex;
        for (unsigned j = 1; j < rank; j++) {
            dataOffset *= nodeDims[j];
        }

        // write
        status = H5Dread(dataSetId, dataType, memSpaceId, fileSpaceId, readId, data + dataOffset);
        util_checkStatus(status);

        // release resources
        H5Sclose(fileSpaceId);
        H5Sclose(memSpaceId);
    }

    H5Dclose(dataSetId);
    H5Pclose(readId);
}

// SHM VECTOR READER - () OPERATOR
// * facilitates reading of shmVector data to the HDF5 file when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void ReadHDF5::ShmVectorReader::operator()(T)
{
    const vistle::ShmVector<T> foundArray =
        vistle::Shm::the().getArrayFromName<T>(archive->getVectorEntryByNvpName(nvpName)->referenceName);

    if (foundArray) {
        auto arrayMapIter = arrayMap.find(arrayNameInFile);
        if (arrayMapIter == arrayMap.end()) {
            // this is a new array
            vistle::ShmVector<T> &newArray = *((vistle::ShmVector<T> *)archive->getVectorEntryByNvpName(nvpName)->ref);
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
                    status = H5Dread(dummyDatasetId, HDF5Const::DummyObject::type, H5S_ALL, H5S_ALL, readId, &dummyVar);
                    areMultipleReadsNeeded = false;
                } else if (commWideReadSize / HDF5Const::numBytesInGb > maxReadSizeGb) {
                    // comm-wide read of over the HDF5 parallel read limit
                    hsize_t offset[] = {readIndex};
                    hid_t fileSpaceId;
                    hid_t memSpaceId;


                    nodeCurrentReadSizeBytes =
                        (nodeCurrentReadSizeBytes / commWideReadSize) * maxReadSizeGb * HDF5Const::numBytesInGb;
                    nodeCurrentReadSizeElements = std::floor(nodeCurrentReadSizeBytes / sizeof(T));
                    dims[0] = nodeCurrentReadSizeElements;

                    fileSpaceId = H5Dget_space(dataSetId);
                    memSpaceId = H5Screate_simple(1, dims, NULL);
                    H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, offset, NULL, dims, NULL);

                    status =
                        H5Dread(dataSetId, dataType, memSpaceId, fileSpaceId, readId, newArray->data() + readIndex);

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
            }

        } else {
            // this array already exists in shared memory
            vistle::ShmVector<T> existingArray = vistle::Shm::the().getArrayFromName<T>(arrayMap[arrayNameInFile]);

            // error debug message
            if (!existingArray) {
                assert("error: existing array not found" == NULL);
            }

            // replace current object array with existing array
            *((vistle::ShmVector<T> *)archive->getVectorEntryByNvpName(nvpName)->ref) = existingArray;
        }
    }
}

// ARRAY TO META - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive &ReadHDF5::ArrayToMetaArchive::operator<<(T const &t)
{
    // do nothing - this archive assumes all members that need to be saved are stored as name-value pairs

    return *this;
}

// ARRAY TO META - << OPERATOR: NVP
// * saves Meta member value into appropriate variable
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive &ReadHDF5::ArrayToMetaArchive::operator<<(const boost::serialization::nvp<T> &t)
{
    std::string memberName(t.name());
    auto nvpMapIter = m_nvpMapPtr->find(memberName);

    if (nvpMapIter != m_nvpMapPtr->end() && nvpMapIter->second < m_array.size()) {
        t.value() = m_array[nvpMapIter->second];
    }

    return *this;
}

template<class U>
ReadHDF5::ArrayToMetaArchive &ReadHDF5::ArrayToMetaArchive::operator<<(
    const boost::serialization::nvp<const boost::serialization::array_wrapper<U>> &t)
{
    std::string memberName(t.name());
    auto nvpMapIter = m_nvpMapPtr->find(memberName);

    if (nvpMapIter != m_nvpMapPtr->end() && nvpMapIter->second < m_array.size()) {
#if 0
        t.value() = m_array[nvpMapIter->second];
#else
        std::cerr << "not implemented" << std::endl;
#endif
    }

    return *this;
}

// ARRAY TO META - & OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ReadHDF5::ArrayToMetaArchive &ReadHDF5::ArrayToMetaArchive::operator&(T const &t)
{
    return *this << t;
}

#endif /* READHDF5_H */
