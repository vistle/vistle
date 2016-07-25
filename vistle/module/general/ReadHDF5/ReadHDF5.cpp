//-------------------------------------------------------------------------
// READ HDF5 CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------


#include <sstream>
#include <fstream>
#include <iomanip>
#include <cfloat>
#include <limits>
#include <string>
#include <vector>
#include <ctime>

#include <core/message.h>
#include <core/vec.h>
#include <core/unstr.h>
#include <core/findobjectreferenceoarchive.h>
#include <core/placeholder.h>
#include <core/object.h>
#include <core/object_impl.h>

#include "hdf5.h"

#include "ReadHDF5.h"

using namespace vistle;

BOOST_SERIALIZATION_REGISTER_ARCHIVE(FindObjectReferenceOArchive)

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(ReadHDF5)


//-------------------------------------------------------------------------
// WRITE HDF5 STATIC MEMBER OUT OF CLASS INITIALIZATION
//-------------------------------------------------------------------------
const hid_t ReadHDF5::m_dummyObjectType = H5T_NATIVE_INT;
const std::vector<hsize_t> ReadHDF5::m_dummyObjectDims = {1};
const std::string ReadHDF5::m_dummyObjectName = "/file/dummy";

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
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// CONSTRUCTOR
//-------------------------------------------------------------------------
ReadHDF5::ReadHDF5(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadHDF5", shmname, name, moduleID) {

    // create ports
    // add support for multiple ports
    //util_checkFile();
    createOutputPort("data0_out");

   // add module parameters
   m_fileName = addStringParameter("File Name", "Name of File that will read from", "");

   // policies
   setReducePolicy(message::ReducePolicy::OverAll);

   // variable setup
   m_isRootNode = (comm().rank() == 0);

   // obtain meta member count
   PlaceHolder::ptr tempObject(new PlaceHolder("", Meta(), Object::Type::UNKNOWN));
   MemberCounterArchive memberCounter;
   boost::serialization::serialize_adl(memberCounter, const_cast<Meta &>(tempObject->meta()), ::boost::serialization::version< Meta >::value);
   numMetaMembers = memberCounter.getCount();


}

// DESTRUCTOR
//-------------------------------------------------------------------------
ReadHDF5::~ReadHDF5() {

}

// PARAMETER CHANGED FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::parameterChanged(const vistle::Parameter *param) {
    util_checkFile();
    return true;
}

// PREPARE FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::prepare() {
    herr_t status;
    hid_t fileId;
    hid_t filePropertyListId;

    htri_t oGroupFound;
    htri_t tGroupFound;
    htri_t bGroupFound;
    htri_t vGroupFound;
    std::string oIndexName;
    std::string tIndexName;
    std::string bIndexName;
    std::string vIndexName;


    // check file validity before beginning
    if (!util_checkFile()) {
        return true;
    }

    // clear persisitent variables
    m_arrayMap.clear();

    // create file access property list id
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);

    // open file
    fileId = H5Fopen(m_fileName->getValue().c_str(), H5P_DEFAULT, filePropertyListId);

    // open dummy dataset id for read size 0 sync
    m_dummyDatasetId = H5Dopen2(fileId, m_dummyObjectName.c_str(), H5P_DEFAULT);


    // parse index to find objects
    LinkIterData linkIterData(this, fileId);
    status = H5Literate_by_name(fileId, "/index", H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateOrigin, &linkIterData, H5P_DEFAULT);
    util_checkStatus(status);


    // sync nodes for collective reads
    while (util_readSyncStandby(comm(), fileId) != 0.0) { /*wait for all nodes to finish reading*/ }

    // close all open h5 entities
    H5Dclose(m_dummyDatasetId);
    H5Pclose(filePropertyListId);
    H5Fclose(fileId);



    // resolve object references
    bool unresolvedReferencesExistInComm;
    bool unresolvedReferencesExistOnNode = false;
    for (unsigned i = 0; i < m_objectReferenceVector.size(); i++) {
        auto objectMapIter = m_objectMap.find(m_objectReferenceVector[i].second);

        if (objectMapIter == m_objectMap.end()) {
            unresolvedReferencesExistOnNode = true;
        } else {
            shm_obj_ref<Object> * objectReferencePtr = (shm_obj_ref<Object> *) m_objectReferenceVector[i].first;
            *objectReferencePtr = vistle::Shm::the().getObjectFromName(objectMapIter->second);
        }
    }

    // check and send warning message for unresolved references
    if (m_isRootNode) {
        boost::mpi::reduce(comm(), unresolvedReferencesExistOnNode, unresolvedReferencesExistInComm, boost::mpi::maximum<bool>(), 0);

        if (unresolvedReferencesExistInComm) {
            sendInfo("Warning: some object references are unresolved.");
        }
    } else {
        boost::mpi::reduce(comm(), unresolvedReferencesExistOnNode, boost::mpi::maximum<bool>(), 0);
    }



    m_objectPersistenceVector.clear();

    return Module::prepare();
}


// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR ORIGIN
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateOrigin(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;

    linkIterData->origin = std::stoul(std::string(name));

    H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateTimestep, linkIterData, H5P_DEFAULT);

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR TIMESTEP
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateTimestep(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;

    linkIterData->timestep = std::stoi(std::string(name));

    H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateBlock, linkIterData, H5P_DEFAULT);

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR BLOCK
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateBlock(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;
    int blockNum = std::stoi(std::string(name));

    // only continue for correct blocks on each node
    if (blockNum % linkIterData->callingModule->size() == linkIterData->callingModule->rank()
            || linkIterData->callingModule->m_isRootNode && blockNum == -1) {

        linkIterData->block = blockNum;

        H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_INC, NULL, prepare_iterateVariant, linkIterData, H5P_DEFAULT);
    }

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR VARIANT
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateVariant(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;

    H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_processObject, linkIterData, H5P_DEFAULT);

    return 0;
}

// PREPARE UTILITY FUNCTION - PROCESSES AN OBJECT
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processObject(hid_t callingGroupId, const char * name, const H5L_info_t * info, void * opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;
    std::string objectGroup = "/object/" + std::string(name);
    std::string typeGroup = objectGroup + "/type";
    std::string metaGroup = objectGroup + "/meta";

    herr_t status;
    hid_t dataSetId;
    hid_t readId;

    int objectType;
    std::vector<double> objectMeta(ReadHDF5::numMetaMembers - ArrayToMetaArchive::numExclusiveMembers);
    FindObjectReferenceOArchive archive;


    // obtain port number
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    // read type
    dataSetId = H5Dopen2(linkIterData->fileId, typeGroup.c_str(), H5P_DEFAULT);
    util_syncAndGetReadSize(sizeof(int), linkIterData->callingModule->comm());
    status = H5Dread(dataSetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, readId, &objectType);
    H5Dclose(dataSetId);

    // read meta
    dataSetId = H5Dopen2(linkIterData->fileId, metaGroup.c_str(), H5P_DEFAULT);
    util_syncAndGetReadSize(sizeof(double) * numMetaMembers, linkIterData->callingModule->comm());
    status = H5Dread(dataSetId, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, readId, objectMeta.data());
    H5Dclose(dataSetId);


    // create empty object by type
    Object::ptr returnObject = ObjectTypeRegistry::getType(objectType).createEmpty();

    // record in file name to in memory name relation
    linkIterData->callingModule->m_objectMap[std::string(name)] = returnObject->getName();

    // construct meta
    ArrayToMetaArchive arrayToMetaArchive(objectMeta.data(), linkIterData->block, linkIterData->timestep);
    boost::serialization::serialize_adl(arrayToMetaArchive, const_cast<Meta &>(returnObject->meta()), ::boost::serialization::version< Meta >::value);

    // serialize object and prepare for array iteration
    returnObject->save(archive);
    linkIterData->archive = &archive;


    // iterate through array links
    status = H5Literate_by_name(linkIterData->fileId, objectGroup.c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_processArrayContainer, linkIterData, H5P_DEFAULT);
    linkIterData->callingModule->util_checkStatus(status);

    // close all open h5 entities
    H5Pclose(readId);

    // output object
    linkIterData->callingModule->m_objectPersistenceVector.push_back(returnObject);
    linkIterData->callingModule->addObject(std::string("data" + std::to_string(linkIterData->origin) + "_out"), returnObject);

    return 0;
}

// PREPARE UTILITY FUNCTION - PROCESSES AN ARRAY CONTAINER
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processArrayContainer(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;
    H5O_info_t objectInfo;
    herr_t status;

    // only iterate through array links
    H5Oget_info_by_name(callingGroupId, name, &objectInfo, H5P_DEFAULT);
    if (objectInfo.type != H5O_TYPE_GROUP) {
        return 0;
    }

    // debug output
    //linkIterData->callingModule->sendInfo("iteratedg: %s", name);

    // check if current version of object contains the correct array name
    if (linkIterData->archive->getVectorEntryByNvpName(name) == nullptr) {
        return 0;
    }

    linkIterData->nvpName = name;

    status = H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_processArrayLink, linkIterData, H5P_DEFAULT);
    linkIterData->callingModule->util_checkStatus(status);

    return 0;
}

// PREPARE UTILITY FUNCTION - PROCESSES AN ARRAY LINK
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processArrayLink(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;

    //linkIterData->callingModule->sendInfo("iteratedl: %s", name);

    if (linkIterData->archive->getVectorEntryByNvpName(linkIterData->nvpName)->referenceType == ReferenceType::ObjectReference) {
        void * objectReference = linkIterData->archive->getVectorEntryByNvpName(linkIterData->nvpName)->ref;

        linkIterData->callingModule->m_objectReferenceVector.push_back(std::make_pair(objectReference, std::string(name)));

    } else if (linkIterData->archive->getVectorEntryByNvpName(linkIterData->nvpName)->referenceType == ReferenceType::ShmVector) {
        ShmVectorReader reader(
                    linkIterData->archive,
                    std::string(name),
                    linkIterData->nvpName,
                    linkIterData->callingModule->m_arrayMap,
                    linkIterData->fileId,
                    linkIterData->callingModule->m_dummyDatasetId,
                    linkIterData->callingModule->comm()
                    );

        boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorReader>(reader));
    }

    return 0;
}

// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::compute() {
    // do nothing
    return true;
}


// REDUCE FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::reduce(int timestep) {

    return Module::reduce(timestep);
}

// GENERIC UTILITY HELPER FUNCTION - CHECK THE VALIDITY OF THE FILENAME PARAMETER
//-------------------------------------------------------------------------
bool ReadHDF5::util_checkFile() {
    herr_t status;
    hid_t fileId;
    hid_t filePropertyListId;
    hid_t dataSetId;
    hid_t readId;

    // Set up file access property list with parallel I/O access
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);

    // open file
    fileId = H5Fopen(m_fileName->getValue().c_str(), H5P_DEFAULT, filePropertyListId);

    // check if file exists
    if (fileId < 0) {
        if (m_isRootNode) {
            sendInfo("File not found");
        }

        return false;
    }


    // obtain port number
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    dataSetId = H5Dopen2(fileId, "/file/numPorts", H5P_DEFAULT);
    status = H5Dread(dataSetId, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, readId, &m_numPorts);
    if (status != 0) {
        sendInfo("File format not recognized");
        return false;
    }

    // add support for multiple ports
//    for (unsigned i = 0; i < m_numPorts; i++) {
//        std::string portName = "data" + std::to_string(i) + "_out";
//        createOutputPort(portName);
//    }


    // close all open h5 entities
    H5Pclose(readId);
    H5Dclose(dataSetId);

    H5Pclose(filePropertyListId);
    H5Fclose(fileId);

    return true;
}

// GENERIC UTILITY HELPER FUNCTION - SYNCHRONISE COLLECTIVE READS NULL READ VERSION
// * returns wether or not all nodes are done
//-------------------------------------------------------------------------
long double ReadHDF5::util_readSyncStandby(const boost::mpi::communicator & comm, hid_t fileId) {
    hid_t dataSetId;
    hid_t readId;
    int readDummy;
    long double totalReadSize;
    long double readSize = 0;


    dataSetId = H5Dopen2(fileId, m_dummyObjectName.c_str(), H5P_DEFAULT);

    boost::mpi::all_reduce(comm, readSize, totalReadSize, std::plus<long double>());

    // obtain port number
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    // perform write followed by read
    H5Dread(dataSetId, m_dummyObjectType, H5S_ALL, H5S_ALL, readId, &readDummy);

    // release resources
    H5Pclose(readId);
    H5Dclose(dataSetId);


    return totalReadSize;
}

// GENERIC UTILITY HELPER FUNCTION - SYNCHRONISE COLLECTIVE READS
// * returns wether or not all nodes are done
//-------------------------------------------------------------------------
long double ReadHDF5::util_syncAndGetReadSize(long double readSize, const boost::mpi::communicator & comm) {
    long double totalReadSize;

    boost::mpi::all_reduce(comm, readSize, totalReadSize, std::plus<long double>());

    return totalReadSize;
}

// GENERIC UTILITY HELPER FUNCTION - VERIFY HERR_T STATUS
//-------------------------------------------------------------------------
void ReadHDF5::util_checkStatus(herr_t status) {

    if (status != 0) {
        sendInfo("Error: status: %i", status);
    }

    return;
}
