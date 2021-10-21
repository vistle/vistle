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

#include <vistle/module/module.h>

#include <vistle/core/findobjectreferenceoarchive.h>
#include <vistle/core/shm.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>

#include "hdf5.h"

#define HDF5Const WriteHDF5Const
#include "HDF5Objects.h"


//-------------------------------------------------------------------------
// WRITE HDF5 CLASS DECLARATION
//-------------------------------------------------------------------------
class WriteHDF5: public vistle::Module {
public:
    friend class boost::serialization::access;
    friend struct ShmVectorWriterOrganized;

    WriteHDF5(const std::string &name, int moduleID, mpi::communicator comm);
    ~WriteHDF5();

private:
    // typedefs
    typedef vistle::FindObjectReferenceOArchive::ReferenceType ReferenceType;
    typedef std::unordered_map<int, std::vector<std::pair<hid_t, std::string>>> ObjTypeToDataMap;
    typedef std::vector<std::pair<int, std::vector<std::pair<hid_t, std::string>>>> ObjTypeToDataTransferVector;

    // IndexTracker: maps: timestep (map) -> block (map) -> portNumber (map) -> variants (unsigned)
    // maps are needed for block and timestep because they can have values of -1
    // a more performant IndexTracker could be achieved with vectors, but I chose to do it this way for reasons of code clarity
    typedef std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, unsigned>>> IndexTracker;

    // structs/functors
    // used in organized mode:
    struct ObjectWriteInfoReferenceEntry;
    struct ObjectWriteInfo;
    struct IndexWriteInfo;

    template<class T>
    struct VectorConcatenator;

    struct ShmVectorReserver;
    struct ShmVectorWriterOrganized;

    // used in performant mode:
    struct WriteObjectContainer;
    struct ShmVectorInfoGetter;

    class DataArrayBase;
    template<class T>
    class DataArray;
    struct DataArrayAllocator;
    struct ShmVectorWriterPerformant;
    class DataArrayContainer;
    class DataArrayAppender;
    struct DataArraySizeFinder;

    struct OffsetContainer;

    // used in all write modes:
    class MetaToArrayArchive;


    // overridden functions
    virtual void connectionRemoved(const vistle::Port *from, const vistle::Port *to) override;
    virtual void connectionAdded(const vistle::Port *from, const vistle::Port *to) override;
    bool parameterChanged(const int senderId, const std::string &name,
                          const vistle::message::SetParameter &msg) override;
    virtual bool prepare() override;
    virtual bool compute() override;
    virtual bool reduce(int timestep) override;

    // private helper functions
    bool prepare_fileNameCheck();
    void compute_organized();
    void compute_performant();
    void compute_organized_addObjectToWrite(vistle::Object::const_ptr obj, unsigned originPortNumber,
                                            unsigned &numNodeObjects, unsigned &numNodeArrays,
                                            std::vector<vistle::Object::const_ptr> &objPtrVector,
                                            std::vector<vistle::FindObjectReferenceOArchive> &objRefArchiveVector,
                                            std::vector<MetaToArrayArchive> &metaToArrayArchiveVector,
                                            std::vector<ObjectWriteInfo> &objectWriteVector,
                                            std::vector<IndexWriteInfo> &indexWriteVector);
    void compute_performant_addObjectToWrite(vistle::Object::const_ptr obj, unsigned originPortNumber);
    void reduce_organized();
    void reduce_performant();
    void util_checkId(hid_t group, const std::string &info) const;
    static void util_checkStatus(herr_t status);
    static void util_HDF5WriteOrganized(bool isWriter, std::string name, const void *data, hid_t fileId, hsize_t *dims,
                                        hid_t dataType);
    static void util_HDF5WriteOrganized(hid_t fileId);

    template<class T>
    void util_HDF5WritePerformant(const char writeName[], unsigned rank, hsize_t *nodeDims, hsize_t *nodeOffset,
                                  hid_t type, const T *data);
    template<class T>
    static void util_HDF5WritePerformant(hid_t fileId, const boost::mpi::communicator &comm, std::string &writeName,
                                         unsigned rank, hsize_t *nodeDims, hsize_t *nodeOffset, hid_t type,
                                         const T *data);

    // private member variables
    vistle::StringParameter *m_fileName;
    vistle::IntParameter *m_writeMode;
    vistle::IntParameter *m_overwrite;
    std::vector<vistle::StringParameter *> m_portDescriptions;
    unsigned m_numPorts;

    hid_t m_fileId;
    bool m_isRootNode;
    bool m_doNotWrite;
    bool m_unresolvedReferencesExist;
    std::unordered_map<std::string, bool>
        m_arrayMap; //< existence determines whether space has been allocated in file, bool describes whether array has actually been written
    std::unordered_set<std::string> m_objectSet; //< existence determines whether entry has been written yet
    std::vector<std::string> m_metaNvpTags;
    IndexTracker m_indexVariantTracker; //< used to keep track of the number of variants in each timestep/block/port

    std::vector<WriteObjectContainer>
        m_objContainerVector; //< used to sort and store objects until the compute function is called when in performant mode
    ObjTypeToDataMap m_objTypeToDataMap;
    unsigned
        m_objectDataArraySize; //< modified within compute to store the total size that the object_data array will need

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
    : name(_name), nvpTag(_nvpTag), referenceType(ReferenceType::ShmVector), type(_type), size(_size)
    {}
    ObjectWriteInfoReferenceEntry(std::string _name, std::string _nvpTag)
    : name(_name), nvpTag(_nvpTag), referenceType(ReferenceType::ObjectReference)
    {}

    // serialization method for passing over mpi
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version);
};

// OBJECT WRITE RESERVATION INFO STRUCT
// * stores data needed for reservation of space within the HDF5 file
//-------------------------------------------------------------------------
struct WriteHDF5::ObjectWriteInfo {
    std::string name;
    std::vector<WriteHDF5::ObjectWriteInfoReferenceEntry> referenceVector;

    ObjectWriteInfo() {}
    ObjectWriteInfo(std::string _name): name(_name) {}

    // < overload for std::sort
    bool operator<(const ObjectWriteInfo &other) const;

    // serialization method for passing over mpi
    template<class Archive>
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
    : name(_name), block(_block), timestep(_timestep), origin(_origin)
    {}

    // < overload for std::sort
    bool operator<(const IndexWriteInfo &other) const;

    // serialization method for passing over mpi
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version);
};


// VECTOR CONCATENATOR FUNCTOR
// * used in mpi reduce call
//-------------------------------------------------------------------------
template<class T>
struct WriteHDF5::VectorConcatenator {
    std::vector<T> &operator()(std::vector<T> &vec1, const std::vector<T> &vec2) const
    {
        vec1.insert(vec1.end(), vec2.begin(), vec2.end());
        return vec1;
    }
};

// META TO ARRAY ARCHIVE
// * used to convert metadata members to an array of doubles for storage
// * into the HDF5 file
// * the last element in the array is the object type
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
    void register_type(const T * = NULL)
    {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}

    MetaToArrayArchive(int objectType);

    // << operators
    template<class T>
    MetaToArrayArchive &operator<<(T const &t);
    template<class T>
    MetaToArrayArchive &operator<<(const boost::serialization::nvp<T> &t);
    template<class U>
    MetaToArrayArchive &operator<<(const boost::serialization::nvp<const boost::serialization::array_wrapper<U>> &t);

    // the & operator
    template<class T>
    MetaToArrayArchive &operator&(const T &t);

    // get functions
    double *getDataPtr() { return m_array.data(); }
};

// SHM VECTOR RESERVER FUNCTOR
// * used to construct ObjectWriteInfoReferenceEntry entries within the shmVectors
// * member of ObjectWriteInfo
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorReserver {
    std::string name;
    std::string nvpTag;
    WriteHDF5::ObjectWriteInfo *reservationInfo;

    ShmVectorReserver(std::string _name, std::string _nvpTag, WriteHDF5::ObjectWriteInfo *_reservationInfo)
    : name(_name), nvpTag(_nvpTag), reservationInfo(_reservationInfo)
    {}

    template<typename T>
    void operator()(T);
};

// SHM VECTOR WRITER FUNCTOR
// * calls util_HDF5Write for a shmVector
// * needed in order to iterate over all possible shm VectorTypes
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorWriterOrganized {
    std::string name;
    hid_t fileId;
    const boost::mpi::communicator &commPtr;

    ShmVectorWriterOrganized(hid_t _fileId, const boost::mpi::communicator &_commPtr)
    : fileId(_fileId), commPtr(_commPtr)
    {}
    ShmVectorWriterOrganized(std::string _name, hid_t _fileId, const boost::mpi::communicator &_commPtr)
    : name(_name), fileId(_fileId), commPtr(_commPtr)
    {}

    template<typename T>
    void operator()(T);

    template<typename T>
    void writeRecursive(const vistle::ShmVector<T> &vec, long double totalWriteSize, unsigned writeSize,
                        unsigned writeIndex);

    void writeDummy();
};

// WRITE OBJECT CONTAINER
// * used to store then sort object pointer vectors into block -> timestep -> port order
//-------------------------------------------------------------------------
struct WriteHDF5::WriteObjectContainer {
    vistle::Object::const_ptr obj;
    unsigned port;
    unsigned order;
    bool isDuplicate;

    vistle::FindObjectReferenceOArchive objRefArchive;
    WriteHDF5::MetaToArrayArchive metaToArrayArchive;

    WriteObjectContainer(vistle::Object::const_ptr _obj, unsigned _port, unsigned _order, bool _isDuplicate)
    : obj(_obj)
    , port(_port)
    , order(_order)
    , isDuplicate(_isDuplicate)
    , objRefArchive(vistle::FindObjectReferenceOArchive())
    , metaToArrayArchive(WriteHDF5::MetaToArrayArchive(obj->getType()))
    {
        if (!isDuplicate) {
            obj->save(objRefArchive);
            boost::serialization::serialize_adl(metaToArrayArchive, const_cast<vistle::Meta &>(obj->meta()),
                                                ::boost::serialization::version<vistle::Object>::value);
        }
    }

    bool operator<(const WriteObjectContainer &other) const
    {
        if (obj->getBlock() != other.obj->getBlock()) {
            return (obj->getBlock() < other.obj->getBlock());
        }
        if (obj->getTimestep() != other.obj->getTimestep()) {
            return (obj->getTimestep() < other.obj->getTimestep());
        }
        if (port != other.port) {
            return (port < other.port);
        }
        return (order < other.order);
    }
};

// SHM VECTOR INFO GETTER
// * obtains information about a shmvector of a given name
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorInfoGetter {
    std::string name;
    hid_t type;
    unsigned size;

    ShmVectorInfoGetter(std::string _name): name(_name), type(0), size(0) {}

    template<typename T>
    void operator()(T)
    {
        const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

        if (vec) {
            size = vec->size();

            auto typeIter = WriteHDF5::s_nativeTypeMap.find(typeid(T));
            if (typeIter != WriteHDF5::s_nativeTypeMap.end()) {
                type = typeIter->second;
            }
        }
    }
};


// DATA ARRAY ENTRY BASE
// * base class for DataArray Entries - needed in order to store pointers to andy DataArray type
// * and to be able to dynamic_cast correctly based on a type
//-------------------------------------------------------------------------
class WriteHDF5::DataArrayBase {
public:
    // virtual destructor need for class to be registered as polymorphic
    // polymorphic characteristic used in dynamic_cast
    virtual ~DataArrayBase() {}
};


// DATA ARRAY ENTRY
// * stores variable type arrays
//-------------------------------------------------------------------------
template<class T>
class WriteHDF5::DataArray: public WriteHDF5::DataArrayBase {
public:
    std::vector<T> array;
};

// DATA ARRAY ALLOCATOR
// * creates a new dataArray of the given type and inserts it into the DataArrayMap
//-------------------------------------------------------------------------
struct WriteHDF5::DataArrayAllocator {
    typedef std::unordered_map<std::type_index, WriteHDF5::DataArrayBase *> DataArrayMap;
    DataArrayMap *dataArraysPtr;

    DataArrayAllocator(DataArrayMap *_dataArraysPtr): dataArraysPtr(_dataArraysPtr) {}

    template<typename T>
    void operator()(T)
    {
        dataArraysPtr->insert(
            std::make_pair<std::type_index, WriteHDF5::DataArrayBase *>(typeid(T), new WriteHDF5::DataArray<T>));
    }
};

// SHM VECTOR WRITER - PERFORMANT
// * obtains the DataArray with the corresponding type
// * writes said array to file
//-------------------------------------------------------------------------
struct WriteHDF5::ShmVectorWriterPerformant {
    WriteHDF5::DataArrayBase *basePtr;
    hid_t fileId;
    const boost::mpi::communicator &comm;


    ShmVectorWriterPerformant(WriteHDF5::DataArrayBase *_basePtr, hid_t _fileId, const boost::mpi::communicator &_comm)
    : basePtr(_basePtr), fileId(_fileId), comm(_comm)
    {}

    template<typename T>
    void operator()(T)
    {
        WriteHDF5::DataArray<T> *dataArrayPtr = dynamic_cast<WriteHDF5::DataArray<T> *>(basePtr);

        if (dataArrayPtr) {
            auto nativeTypeMapIter = WriteHDF5::s_nativeTypeMap.find(typeid(T));
            hsize_t offset[] = {0};
            hsize_t dims[] = {dataArrayPtr->array.size()};
            std::string writeName;

            // handle those VectorTypes unsupported currently. print warning
            if (nativeTypeMapIter == WriteHDF5::s_nativeTypeMap.end()) {
                std::cerr << "Warning: type " << typeid(T).name() << "is unsupported in the HDF5 Typemap" << std::endl;
                return;
            }

            writeName = "/array/" + std::to_string(nativeTypeMapIter->second);

            // transmit offset to required node
            if (comm.size() == 1) {
                offset[0] = 0;
            } else {
                if (comm.rank() == 0) {
                    offset[0] = 0;
                    comm.send(comm.rank() + 1, 0, dataArrayPtr->array.size());

                } else if (comm.rank() == comm.size() - 1) {
                    comm.recv(comm.rank() - 1, 0, offset[0]);

                } else {
                    comm.recv(comm.rank() - 1, 0, offset[0]);
                    comm.send(comm.rank() + 1, 0, offset[0] + dataArrayPtr->array.size());
                }
                comm.barrier();
            }

            WriteHDF5::util_HDF5WritePerformant(fileId, comm, writeName, 1, dims, offset, nativeTypeMapIter->second,
                                                dataArrayPtr->array.data());
        }
    }
};

// DATA ARRAY SIZE FINDER
// * obtains the size of a DataArray by type
//-------------------------------------------------------------------------
struct WriteHDF5::DataArraySizeFinder {
    WriteHDF5::DataArrayBase *basePtr;
    std::vector<std::pair<hid_t, unsigned>> sizeVector;

    DataArraySizeFinder(WriteHDF5::DataArrayBase *_basePtr): basePtr(_basePtr) {}

    template<typename T>
    void operator()(T)
    {
        WriteHDF5::DataArray<T> *dataArrayPtr = dynamic_cast<WriteHDF5::DataArray<T> *>(basePtr);

        if (dataArrayPtr) {
            auto typeIter = WriteHDF5::s_nativeTypeMap.find(typeid(T));

            if (typeIter != WriteHDF5::s_nativeTypeMap.end()) {
                sizeVector.push_back(std::make_pair(typeIter->second, dataArrayPtr->array.size()));
            }
        }
    }
};

// DATA ARRAY CONTAINER CLASS
// * used to store the data arrays if each type that will be written into the hdf5 file
//-------------------------------------------------------------------------
class WriteHDF5::DataArrayContainer {
private:
    std::unordered_map<std::type_index, WriteHDF5::DataArrayBase *> m_dataArrays;

public:
    // constructor
    DataArrayContainer()
    {
        // create entries into the m_dataArrays such that they will be in the same order when iterated over on every node
        WriteHDF5::DataArrayAllocator allocator(&m_dataArrays);
        boost::mpl::for_each<vistle::VectorTypes>(boost::reference_wrapper<WriteHDF5::DataArrayAllocator>(allocator));
    }

    // destructor
    ~DataArrayContainer()
    {
        for (auto it: m_dataArrays) {
            delete it.second;
        }
    }

    // append data of variable type to the array corresponding to that type
    template<class T>
    unsigned append(const T *data, unsigned size)
    {
        if (WriteHDF5::s_nativeTypeMap.find(typeid(T)) != WriteHDF5::s_nativeTypeMap.end()) {
            auto dataArrayIter = m_dataArrays.find(typeid(T));

            if (dataArrayIter != m_dataArrays.end()) {
                std::vector<T> *arrayPtr = &(static_cast<WriteHDF5::DataArray<T> *>(dataArrayIter->second))->array;

                arrayPtr->reserve(size);

                // append
                for (unsigned i = 0; i < size; i++) {
                    arrayPtr->push_back(*(data + i));
                }

                return arrayPtr->size();

            } else {
                // if vistle::VectorTypes does not contain the type T, but the function call requires it
                assert("m_dataArrays not built properly" == NULL);
                return 0;
            }

        } else {
            // the native typemap does not contain the necessary type - please add it if needed
            assert("hdf5 type not found" == NULL);
            return 0;
        }
    }

    // get the size of and array by type
    template<class T>
    unsigned size(T)
    {
        auto dataArrayIter = m_dataArrays.find(typeid(T));

        if (dataArrayIter != m_dataArrays.end()) {
            return (static_cast<WriteHDF5::DataArray<T> *>(dataArrayIter->second))->array.size();

        } else {
            // if vistle::VectorTypes does not contain the type T, but the function call requires it
            assert("m_dataArrays not built properly" == NULL);
            return 0;
        }
    }

    // return a sizevector relating HDF5 type mapping values to the size of each vector
    std::vector<std::pair<hid_t, unsigned>> getSizeVector()
    {
        for (auto it: m_dataArrays) {
            WriteHDF5::DataArraySizeFinder sizeFinder(it.second);
            boost::mpl::for_each<vistle::VectorTypes>(
                boost::reference_wrapper<WriteHDF5::DataArraySizeFinder>(sizeFinder));

            return sizeFinder.sizeVector;
        }
        return std::vector<std::pair<hid_t, unsigned>>();
    }

    // writes all data arrays to file
    void writeToFile(hid_t fileId, const boost::mpi::communicator &comm)
    {
        for (auto it: m_dataArrays) {
            WriteHDF5::ShmVectorWriterPerformant writer(it.second, fileId, comm);
            boost::mpl::for_each<vistle::VectorTypes>(
                boost::reference_wrapper<WriteHDF5::ShmVectorWriterPerformant>(writer));
        }
    }
};


// DATA ARRAY APPENDER
// * appends a ShmVector with a given name to the data array of the corresponding type
//-------------------------------------------------------------------------
class WriteHDF5::DataArrayAppender {
private:
    WriteHDF5::DataArrayContainer *m_containerPtr;
    std::string m_name;
    unsigned m_appendArraySize;
    unsigned m_newDataArraySize;

public:
    DataArrayAppender(WriteHDF5::DataArrayContainer *_containerPtr, std::string _name)
    : m_containerPtr(_containerPtr), m_name(_name)
    {}

    template<typename T>
    void operator()(T)
    {
        const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(m_name);

        if (vec) {
            m_appendArraySize = vec->size();
            m_newDataArraySize = m_containerPtr->append(vec->data(), m_appendArraySize);
        }
    }

    // get/set functions
    unsigned getAppendArraySize() const { return m_appendArraySize; }
    unsigned getNewDataArraySize() const { return m_newDataArraySize; }
};

// OFFSET CONTAINER STRUCT
// * contains all offset information that needs to be passed between nodes
// * kept as a struct so that it can be passed in a single set of mpi communications
//-------------------------------------------------------------------------
struct WriteHDF5::OffsetContainer {
    unsigned object;
    unsigned objectData;

    unsigned block;
    unsigned timestep;
    unsigned port;
    unsigned portObjectList;

    std::vector<std::pair<hid_t, unsigned>> dataArrays;

    OffsetContainer(std::vector<std::pair<hid_t, unsigned>> templateArray)
    : object(0), objectData(0), block(0), timestep(0), port(0), portObjectList(0), dataArrays(templateArray)
    {
        for (unsigned i = 0; i < dataArrays.size(); i++) {
            dataArrays[i].second = 0;
        }
    }

    // serialization method for passing over mpi
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &object;
        ar &objectData;
        ar &block;
        ar &timestep;
        ar &port;
        ar &portObjectList;
        ar &dataArrays;
    }

    OffsetContainer operator+(const OffsetContainer &rhs)
    {
        OffsetContainer container(dataArrays);
        container.object = object + rhs.object;
        container.objectData = objectData + rhs.objectData;
        container.block = block + rhs.block;
        container.timestep = timestep + rhs.timestep;
        container.port = port + rhs.port;
        container.portObjectList = portObjectList + rhs.portObjectList;

        for (unsigned i = 0; i < dataArrays.size(); i++) {
            container.dataArrays[i].second = dataArrays[i].second + rhs.dataArrays[i].second;
        }

        return container;
    }
};


//-------------------------------------------------------------------------
// WRITE HDF5 STRUCT/FUNCTOR FUNCTION DEFINITIONS
//-------------------------------------------------------------------------

// GENERIC UTILITY HELPER FUNCTION - WRITE DATA TO HDF5 ABSTRACTION - PERFORMANT
// * non-static version
//-------------------------------------------------------------------------
template<class T>
void WriteHDF5::util_HDF5WritePerformant(const char writeName[], unsigned rank, hsize_t *nodeDims, hsize_t *nodeOffset,
                                         hid_t type, const T *data)
{
    std::string writeNameString(writeName);
    util_HDF5WritePerformant(m_fileId, comm(), writeNameString, rank, nodeDims, nodeOffset, type, data);
    return;
}

// GENERIC UTILITY HELPER FUNCTION - WRITE DATA TO HDF5 ABSTRACTION - PERFORMANT
//-------------------------------------------------------------------------
template<class T>
void WriteHDF5::util_HDF5WritePerformant(hid_t fileId, const boost::mpi::communicator &comm, std::string &writeName,
                                         unsigned rank, hsize_t *nodeDims, hsize_t *nodeOffset, hid_t type,
                                         const T *data)
{
    std::vector<hsize_t> dims;
    std::vector<hsize_t> offset;
    std::vector<hsize_t> totalDims(rank);
    herr_t status;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t memSpaceId;
    hid_t writeId;

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

    // create dataset
    fileSpaceId = H5Screate_simple(rank, totalDims.data(), NULL);
    dataSetId = H5Dcreate(fileId, writeName.c_str(), type, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);

    // set up parallel write
    writeId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);

    // obtain total write size
    long double totalWriteSize = 1;
    for (unsigned i = 0; i < rank; i++) {
        totalWriteSize *= totalDims[i];
    }

    // obtain divisions and perform a set of necessary writes in order to keep under the 2gb MPIO limit
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
        status = H5Dwrite(dataSetId, type, memSpaceId, fileSpaceId, writeId, data + dataOffset);
        util_checkStatus(status);

        // release resources
        H5Sclose(fileSpaceId);
        H5Sclose(memSpaceId);
    }

    H5Dclose(dataSetId);
    H5Pclose(writeId);
}

// RESERVATION INFO SHM ENTRY - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template<class Archive>
void WriteHDF5::ObjectWriteInfoReferenceEntry::serialize(Archive &ar, const unsigned int version)
{
    ar &name;
    ar &nvpTag;
    ar &referenceType;
    ar &type;
    ar &size;
}

// OBJECT WRITE RESERVATION INFO - < OPERATOR
// * needed for std::sort of ObjectWriteInfo
//-------------------------------------------------------------------------
bool WriteHDF5::ObjectWriteInfo::operator<(const ObjectWriteInfo &other) const
{
    return (name < other.name);
}

// INDEX WRITE RESERVATION INFO - < OPERATOR
// * needed for std::sort of IndexWriteInfo
//-------------------------------------------------------------------------
bool WriteHDF5::IndexWriteInfo::operator<(const IndexWriteInfo &other) const
{
    return (name < other.name);
}

// OBJECT WRITE RESERVATION INFO - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template<class Archive>
void WriteHDF5::ObjectWriteInfo::serialize(Archive &ar, const unsigned int version)
{
    ar &name;
    ar &referenceVector;
}

// INDEX WRITE RESERVATION INFO - SERIALIZE
// * needed for passing struct over mpi
//-------------------------------------------------------------------------
template<class Archive>
void WriteHDF5::IndexWriteInfo::serialize(Archive &ar, const unsigned int version)
{
    ar &name;
    ar &block;
    ar &timestep;
    ar &origin;
}

// SHM VECTOR RESERVER - () OPERATOR
// * manifests construction of ObjectWriteInfoReferenceEntry entries when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorReserver::operator()(T)
{
    const vistle::ShmVector<T> &vec = vistle::Shm::the().getArrayFromName<T>(name);

    if (vec) {
        auto nativeTypeMapIter = WriteHDF5::s_nativeTypeMap.find(typeid(T));

        // check whether the needed vector type is not supported within the nativeTypeMap
        assert(nativeTypeMapIter != WriteHDF5::s_nativeTypeMap.end());

        // store reservation info
        reservationInfo->referenceVector.push_back(
            ObjectWriteInfoReferenceEntry(name, nvpTag, nativeTypeMapIter->second, vec->size()));
    }
}

// SHM VECTOR WRITER - () OPERATOR
// * facilitates writing of shmVector data to the HDF5 file when GetArrayFromName types match
//-------------------------------------------------------------------------
template<typename T>
void WriteHDF5::ShmVectorWriterOrganized::operator()(T)
{
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
void WriteHDF5::ShmVectorWriterOrganized::writeRecursive(const vistle::ShmVector<T> &vec, long double totalWriteSize,
                                                         unsigned writeSize, unsigned writeIndex)
{
    if (totalWriteSize / HDF5Const::numBytesInGb > HDF5Const::mpiReadWriteLimitGb) {
        writeRecursive(vec, totalWriteSize / 2, std::floor(writeSize / 2), writeIndex);
        writeRecursive(vec, totalWriteSize / 2, std::ceil(writeSize / 2), writeIndex + std::floor(writeSize / 2));

    } else {
        if (writeSize == 0) {
            WriteHDF5::util_HDF5WriteOrganized(fileId);

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

            // check whether the needed vector type is not supported within the nativeTypeMap
            assert(nativeTypeMapIter != WriteHDF5::s_nativeTypeMap.end());

            // set up parallel write
            writeId = H5Pcreate(H5P_DATASET_XFER);
            H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);


            // allocate data spaces
            dataSetId = H5Dopen(fileId, writeName.c_str(), H5P_DEFAULT);
            fileSpaceId = H5Dget_space(dataSetId);
            memSpaceId = H5Screate_simple(1, dims, NULL);
            H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, offset, NULL, dims, NULL);


            // write
            status = H5Dwrite(dataSetId, nativeTypeMapIter->second, memSpaceId, fileSpaceId, writeId,
                              vec->data() + writeIndex);
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
void WriteHDF5::ShmVectorWriterOrganized::writeDummy()
{
    long double totalWriteSize; // in bytes
    long double zero = 0;

    boost::mpi::all_reduce(commPtr, zero, totalWriteSize, std::plus<long double>());

    // handle synchronisation of collective writes when over 2gb mpio limit
    for (unsigned i = 0;
         i < (unsigned)std::ceil(totalWriteSize / (HDF5Const::numBytesInGb * HDF5Const::mpiReadWriteLimitGb)); i++) {
        WriteHDF5::util_HDF5WriteOrganized(fileId);
    }
}

// META TO ARRAY CONSTRUCTOR
//-------------------------------------------------------------------------
WriteHDF5::MetaToArrayArchive::MetaToArrayArchive(int objectType): m_insertIndex(0)
{
    m_array.resize(WriteHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers);
    m_array.back() = objectType;
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive &WriteHDF5::MetaToArrayArchive::operator<<(T const &t)
{
    // do nothing - this archive assumes all members that need to be saved are stored as name-value pairs

    return *this;
}

// META TO ARRAY - << OPERATOR: NVP
// * saves Meta member value into appropriate variable
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive &WriteHDF5::MetaToArrayArchive::operator<<(const boost::serialization::nvp<T> &t)
{
    m_array[m_insertIndex] = (double)t.const_value();
    m_insertIndex++;

    return *this;
}

template<class U>
WriteHDF5::MetaToArrayArchive &WriteHDF5::MetaToArrayArchive::operator<<(
    const boost::serialization::nvp<const boost::serialization::array_wrapper<U>> &t)
{
    for (auto i = 0; i < t.const_value().count(); ++i) {
        m_array[m_insertIndex] = t.const_value().address()[i];
        m_insertIndex++;
    }

    return *this;
}

// META TO ARRAY - << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
WriteHDF5::MetaToArrayArchive &WriteHDF5::MetaToArrayArchive::operator&(T const &t)
{
    return *this << t;
}


#endif /* WRITEHDF5_H */
