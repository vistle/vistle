//-------------------------------------------------------------------------
// READ HDF5 CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------


#include <cfloat>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <vistle/core/findobjectreferenceoarchive.h>
#include <vistle/core/message.h>
#include <vistle/core/object.h>
#include <vistle/core/object_impl.h>
#include <vistle/core/placeholder.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/util/filesystem.h>

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
unsigned ReadHDF5::s_numMetaMembers = 0;

//-------------------------------------------------------------------------
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// CONSTRUCTOR
//-------------------------------------------------------------------------
ReadHDF5::ReadHDF5(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    // add module parameters
    m_fileName = addStringParameter("file_name", "Name of File that will read from", "", Parameter::ExistingFilename);

    // policies
    setReducePolicy(message::ReducePolicy::OverAll);

    // variable setup
    m_isRootNode = (this->comm().rank() == 0);
    m_numPorts = 0;

    // obtain meta member count
    PlaceHolder::ptr tempObject(new PlaceHolder("", Meta(), Object::Type::UNKNOWN));
    MemberCounterArchive memberCounter;
    boost::serialization::serialize_adl(memberCounter, const_cast<Meta &>(tempObject->meta()),
                                        ::boost::serialization::version<Meta>::value);
    s_numMetaMembers = memberCounter.getCount();
}

// DESTRUCTOR
//-------------------------------------------------------------------------
ReadHDF5::~ReadHDF5()
{}

// PARAMETER CHANGED FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::changeParameter(const vistle::Parameter *param)
{
    util_checkFile();

    return true;
}

// PREPARE FUNCTION
// * executes commands common to both write modes
// * delegates processing of file to appropriate prepare method based on writeMode
//-------------------------------------------------------------------------
bool ReadHDF5::prepare()
{
    bool unresolvedReferencesExistInComm;
    MPI_Info mpiInfo;
    hid_t fileId;
    hid_t filePropertyListId;

    hid_t dataSetId;
    hid_t readId;
    int fileVersion;


    // check file validity before beginning
    if (!util_checkFile()) {
        Module::prepare();
    }

    // clear persisitent variables
    m_arrayMap.clear();
    m_objectMap.clear();
    m_unresolvedReferencesExist = false;

    // create specialized mpi info object
    MPI_Info_create(&mpiInfo);

    // Disables ROMIO's data-sieving
    MPI_Info_set(mpiInfo, "romio_ds_read", "disable");
    MPI_Info_set(mpiInfo, "romio_ds_write", "disable");

    // Enable ROMIO's collective buffering
    MPI_Info_set(mpiInfo, "romio_cb_read", "enable");
    MPI_Info_set(mpiInfo, "romio_cb_write", "enable");

    // create file access property list id
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), mpiInfo);

    // free resources
    MPI_Info_free(&mpiInfo);

    // open file
    fileId = H5Fopen(m_fileName->getValue().c_str(), H5P_DEFAULT, filePropertyListId);
    H5Pclose(filePropertyListId);


    // read file version and print
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);
    dataSetId = H5Dopen2(fileId, "/file/version", H5P_DEFAULT);

    H5Dread(dataSetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, readId, &fileVersion);

    H5Dclose(dataSetId);
    H5Pclose(readId);

    if (m_isRootNode) {
        sendInfo("Reading File: %s \n     Vistle HDF5 Version - File: %d / Application: %d",
                 m_fileName->getValue().c_str(), fileVersion, HDF5Const::versionNumber);
    }


    // delegate prepare
    if (m_writeMode == WriteMode::Organized) {
        prepare_organized(fileId);
    } else if (m_writeMode == WriteMode::Performant) {
        prepare_performant(fileId);
    }


    // check and send warning message for unresolved references
    if (m_isRootNode) {
        boost::mpi::reduce(comm(), m_unresolvedReferencesExist, unresolvedReferencesExistInComm,
                           boost::mpi::maximum<bool>(), 0);

        if (unresolvedReferencesExistInComm) {
            sendInfo("Warning: some object references are unresolved.");
        }
    } else {
        boost::mpi::reduce(comm(), m_unresolvedReferencesExist, boost::mpi::maximum<bool>(), 0);
    }


    // release references to objects so that they can be deleted when not needed anymore
    m_objectPersistenceVector.clear();

    // close file
    H5Fclose(fileId);


    return Module::prepare();
}

// PREPARE FUNCTION - PERFORMANT
// * handles:
// * - reading in of indices and object type information
// * - iteratest through index and constructs output object
//-------------------------------------------------------------------------
void ReadHDF5::prepare_performant(hid_t fileId)
{
    ContiguousMemoryMatrix<unsigned> blockArray;
    std::vector<unsigned> timestepArray;
    ContiguousMemoryMatrix<unsigned> portArray;
    std::vector<unsigned> portObjectListArray;

    std::vector<unsigned> objectArray;

    ContiguousMemoryMatrix<unsigned> typeToObjectDataElementInfoArray;
    ContiguousMemoryMatrix<unsigned> objectDataElementInfoArray;
    std::vector<char> nvpTagsArray;

    std::unordered_map<int, std::vector<std::pair<hid_t, std::string>>> objTypeToDataMap;

    std::unordered_map<std::string, unsigned> objectReferenceMap;

    const unsigned numMetaElementsInArray = (ReadHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers + 1);
    std::vector<hsize_t> dims;
    std::vector<hsize_t> offset;

    // obtain array sizes and allocate memory
    dims = prepare_performant_getArrayDims(fileId, "index/block");
    blockArray.reserve(dims[0], dims[1]);

    dims = prepare_performant_getArrayDims(fileId, "index/timestep");
    timestepArray.reserve(dims[0]);

    dims = prepare_performant_getArrayDims(fileId, "index/port");
    portArray.reserve(dims[0], dims[1]);

    dims = prepare_performant_getArrayDims(fileId, "index/port_object_list");
    portObjectListArray.reserve(dims[0]);

    dims = prepare_performant_getArrayDims(fileId, "object/object");
    objectArray.reserve(dims[0]);

    dims = prepare_performant_getArrayDims(fileId, "object/type_to_object_element_info");
    typeToObjectDataElementInfoArray.reserve(dims[0], dims[1]);

    dims = prepare_performant_getArrayDims(fileId, "object/object_element_info");
    objectDataElementInfoArray.reserve(dims[0], dims[1]);

    dims = prepare_performant_getArrayDims(fileId, "object/nvpTags");
    nvpTagsArray.reserve(dims[0]);


    // XXX this is where I left off:
    // read in data for all above arrays
    // implemented but have not tested all reads
    dims[0] = blockArray.size();
    dims[1] = 3;
    offset[0] = 0;
    offset[1] = 0;
    prepare_performant_readHDF5(fileId, comm(), "index/block", 2, dims.data(), offset.data(), blockArray.data());

    dims[0] = timestepArray.size();
    offset[0] = 0;
    prepare_performant_readHDF5(fileId, comm(), "index/timstep", 1, dims.data(), offset.data(), timestepArray.data());

    dims[0] = portArray.size();
    dims[1] = 2;
    offset[0] = 0;
    offset[1] = 0;
    prepare_performant_readHDF5(fileId, comm(), "index/port", 2, dims.data(), offset.data(), portArray.data());

    dims[0] = portObjectListArray.size();
    offset[0] = 0;
    prepare_performant_readHDF5(fileId, comm(), "index/port_object_list", 1, dims.data(), offset.data(),
                                portObjectListArray.data());

    dims[0] = objectArray.size();
    offset[0] = 0;
    prepare_performant_readHDF5(fileId, comm(), "object/object", 1, dims.data(), offset.data(), objectArray.data());

    dims[0] = typeToObjectDataElementInfoArray.size();
    dims[1] = 3;
    offset[0] = 0;
    offset[1] = 0;
    prepare_performant_readHDF5(fileId, comm(), "object/type_to_object_element_info", 2, dims.data(), offset.data(),
                                typeToObjectDataElementInfoArray.data());

    dims[0] = objectDataElementInfoArray.size();
    dims[1] = 2;
    offset[0] = 0;
    offset[1] = 0;
    prepare_performant_readHDF5(fileId, comm(), "object/object_element_info", 2, dims.data(), offset.data(),
                                objectDataElementInfoArray.data());

    dims[0] = nvpTagsArray.size();
    offset[0] = 0;
    prepare_performant_readHDF5(fileId, comm(), "object/nvp_tags", 1, dims.data(), offset.data(), nvpTagsArray.data());


    // parse index
    // ib, it, ip, ipol : iterators for block, timestep, port and port_object_list (respectively)
    for (unsigned ib = 0; ib < blockArray.size(); ib++) {
        if (blockArray(ib, 0) % rank() != 0) {
            continue;
        }

        for (unsigned it = blockArray(ib, 1); it < blockArray(ib, 2); it++) {
            for (unsigned ip = timestepArray[it]; ip < m_numPorts; ip++) {
                for (unsigned ipol = portArray(ip, 0); ipol < portArray(ip, 1); ipol++) {
                    // XXX this is where I left off:
                    // parse object: objectArray[portObjectListArray[ipol]]
                    //
                    // To Implement:
                    // - read object_data entry
                    // - read object_meta entry
                    // - create empty object based off type (2nd last entry within object_meta array that was loaded)
                    // - fill in metaData values (first $(totalMetaSize) - 2 entries in the retrieved metadata array)
                    // - iterate through object_data elements: (select one of the following based on objectDataElementInfoArray(n, 0), described in documentation)
                    //   - for ShmVectors: (this area will be similar to the existing prepare_processArrayLink, but a different ShmVectorReader is needed)
                    //     - obtain appropriate nvp tags based on objectDataElementInfoArray(n, 1) (described in documentation)
                    //     - call getVectorEntryByNvpName() with nvp tag on archive of new object to obtain each FindObjectOArchive::ReferenceData entry.
                    //       a value of NULL means that the current object does not contain the nvp tag, meaning that the object structure has changed
                    //       since the file was written - abort read on this condition.
                    //     - create a struct to use with mpl::for_each that retrieves the new object's ShmVector with its appropriate type,
                    //       resizes the array, then reads in the data.
                    //   - for Object References:
                    //     - check if object has already been written (uniquely identify objects based on their index in the objectArray)
                    //     - if so, link the reference, else, queue the referenced object to be built at this point
                    //   - for attributes:
                    //     - obtain number of attributes (last entry within object_meta array that was loaded)
                    //     - read attributes *see notes
                    //     - fill in new object attributes
                    // - Add the new object to its host port, pray everything works!
                    //
                    // Notes:
                    // * every time a read is performed, a size and offset must be provided. See documentation for description of how to find each
                    // * There is an error in the design of the performant file system. There is currently no way of finding the total read size needed
                    //   for the reading of attributes. This can be added in the second column of the object_data array (currently it is set to
                    //   HDF5Const::performantAttributeNullVal). However, some changes will need to be made where the global offsets are stored into the
                    //   HDF5 file. See lines 647 and 523 of WriteHDF5.cpp.
                }
            }
        }
    }
}

// PREPARE UTILITY FUNCTION - OBTAIN ARRAY DIMENSIONS
//-------------------------------------------------------------------------
std::vector<hsize_t> ReadHDF5::prepare_performant_getArrayDims(hid_t fileId, char readName[])
{
    hid_t dataSetId;
    hid_t dataSpaceId;
    std::vector<hsize_t> dims(2);

    dataSetId = H5Dopen2(fileId, readName, H5P_DEFAULT);
    dataSpaceId = H5Dget_space(dataSetId);

    H5Sget_simple_extent_dims(dataSpaceId, dims.data(), NULL);

    H5Sclose(dataSpaceId);
    H5Dclose(dataSetId);

    return dims;
}

// PREPARE FUNCTION - ORGANIZED
// * handles:
// * - constructing the nvp map
// * - iterating over index and loading objects from the HDF5 file
//-------------------------------------------------------------------------
void ReadHDF5::prepare_organized(hid_t fileId)
{
    herr_t status;


    // open dummy dataset id for read size 0 sync
    m_dummyDatasetId = H5Dopen2(fileId, HDF5Const::DummyObject::name.c_str(), H5P_DEFAULT);

    // prepare data object that is passed to the functions
    LinkIterData linkIterData(this, fileId);

    // construct nvp map
    status = H5Literate_by_name(fileId, "/file/meta_tags", H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateMeta,
                                &linkIterData, H5P_DEFAULT);
    util_checkStatus(status);

    // parse index to find objects
    status = H5Literate_by_name(fileId, "/index", H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateTimestep,
                                &linkIterData, H5P_DEFAULT);
    util_checkStatus(status);


    // sync nodes for collective reads
    while (util_readSyncStandby(comm(), fileId) != 0.0) { /*wait for all nodes to finish reading*/
    }

    // close all open h5 entities
    H5Dclose(m_dummyDatasetId);


    return;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR META NVP TAGS
// * constructs the metadata index to nvp map
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateMeta(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    hid_t dataSetId;
    hid_t dataSpaceId;
    hid_t readId;
    hsize_t dims[1];
    hsize_t maxDims[1];

    LinkIterData *linkIterData = (LinkIterData *)opData;

    std::string groupPath = "/file/meta_tags/" + std::string(name);
    std::vector<char> nvpTag;

    // obtain nvp tag
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    dataSetId = H5Dopen2(linkIterData->fileId, groupPath.c_str(), H5P_DEFAULT);
    dataSpaceId = H5Dget_space(dataSetId);
    H5Sget_simple_extent_dims(dataSpaceId, dims, maxDims);

    nvpTag.resize(dims[0] + 1);
    nvpTag.back() = '\0';

    H5Dread(dataSetId, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, readId, nvpTag.data());

    H5Sclose(dataSpaceId);
    H5Dclose(dataSetId);
    H5Pclose(readId);


    // add to map
    auto insertionPair =
        std::make_pair<std::string, unsigned>(std::string(nvpTag.data()), std::stoul(std::string(name)));
    linkIterData->callingModule->m_metaNvpMap.insert(insertionPair);

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR TIMESTEP
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateTimestep(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    LinkIterData *linkIterData = (LinkIterData *)opData;

    linkIterData->timestep = std::stoi(std::string(name + 1));

    H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateBlock, linkIterData,
                       H5P_DEFAULT);

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR BLOCK
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateBlock(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    LinkIterData *linkIterData = (LinkIterData *)opData;
    int blockNum = std::stoi(std::string(name + 1));

    // only continue for correct blocks on each node
    if (blockNum % linkIterData->callingModule->size() == linkIterData->callingModule->rank() ||
        (linkIterData->callingModule->m_isRootNode && blockNum == -1)) {
        linkIterData->block = blockNum;

        H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_INC, NULL, prepare_iterateOrigin, linkIterData,
                           H5P_DEFAULT);
    }

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR ORIGIN
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateOrigin(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    LinkIterData *linkIterData = (LinkIterData *)opData;
    std::string originPortName = "data" + std::string(name + 1) + "_out";

    linkIterData->origin = std::stoul(std::string(name + 1));

    // only iterate over connected ports
    // references will be handled when needed
    if (linkIterData->callingModule->isConnected(originPortName)) {
        H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_iterateVariant,
                           linkIterData, H5P_DEFAULT);
    }

    return 0;
}

// PREPARE UTILITY FUNCTION - ITERATE CALLBACK FOR VARIANT
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_iterateVariant(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_processObject, opData,
                       H5P_DEFAULT);

    return 0;
}

// PREPARE UTILITY FUNCTION - PROCESSES AN OBJECT
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
// * object creation and attachment to port happens here
// * object metadata reading also occurs here
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processObject(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    LinkIterData *linkIterData = (LinkIterData *)opData;
    std::string objectGroup = "/object/" + std::string(name);
    std::string typeGroup = objectGroup + "/type";
    std::string metaGroup = objectGroup + "/meta";

    herr_t status;
    hid_t dataSetId;
    hid_t readId;

    int objectType;
    std::vector<double> objectMeta(ReadHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers);
    FindObjectReferenceOArchive archive;

    // return if object has already been constructed
    if (linkIterData->callingModule->m_objectMap.find(std::string(name)) !=
        linkIterData->callingModule->m_objectMap.end()) {
        return 0;
    }

    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    // read meta and type
    dataSetId = H5Dopen2(linkIterData->fileId, metaGroup.c_str(), H5P_DEFAULT);
    util_syncAndGetReadSize(sizeof(double) * objectMeta.size(), linkIterData->callingModule->comm());
    status = H5Dread(dataSetId, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, readId, objectMeta.data());
    H5Dclose(dataSetId);

    // extract object type from object meta
    objectType = objectMeta.back();
    objectMeta.pop_back();


    // create empty object by type
    Object::ptr returnObject = ObjectTypeRegistry::getType(objectType).createEmpty();

    // record in file name to in memory name relation
    linkIterData->callingModule->m_objectMap[std::string(name)] = returnObject->getName();

    // construct meta
    ArrayToMetaArchive arrayToMetaArchive(objectMeta.data(), &linkIterData->callingModule->m_metaNvpMap);
    boost::serialization::serialize_adl(arrayToMetaArchive, const_cast<Meta &>(returnObject->meta()),
                                        ::boost::serialization::version<Meta>::value);

    // serialize object and prepare for array iteration
    returnObject->save(archive);
    linkIterData->archive = &archive;


    // iterate through array links
    status = H5Literate_by_name(linkIterData->fileId, objectGroup.c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                                prepare_processArrayContainer, linkIterData, H5P_DEFAULT);
    linkIterData->callingModule->util_checkStatus(status);

    // close all open h5 entities
    H5Pclose(readId);

    // output object as long as it is not a pure reference
    linkIterData->callingModule->m_objectPersistenceVector.push_back(returnObject);
    if (linkIterData->origin != std::numeric_limits<unsigned>::max()) {
        linkIterData->callingModule->updateMeta(returnObject);
        linkIterData->callingModule->addObject(std::string("data" + std::to_string(linkIterData->origin) + "_out"),
                                               returnObject);
    }
    return 0;
}

// PREPARE UTILITY FUNCTION - PROCESSES AN ARRAY CONTAINER
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
// * function processes the group within the object which contains either a ShmVector or an object reference
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processArrayContainer(hid_t callingGroupId, const char *name, const H5L_info_t *info,
                                               void *opData)
{
    LinkIterData *linkIterData = (LinkIterData *)opData;
    H5O_info_t objectInfo;
    herr_t status;

    // only iterate through array links, not object meta datasets
    H5Oget_info_by_name(callingGroupId, name, &objectInfo, H5P_DEFAULT);
    if (objectInfo.type != H5O_TYPE_GROUP) {
        return 0;
    }

    // check if current version of object contains the correct array name
    if (linkIterData->archive->getVectorEntryByNvpName(name) == nullptr) {
        return 0;
    }

    linkIterData->nvpName = name;

    // use Literate to access ShmVector/object reference being linked
    status = H5Literate_by_name(callingGroupId, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_processArrayLink,
                                linkIterData, H5P_DEFAULT);
    linkIterData->callingModule->util_checkStatus(status);

    return 0;
}

// PREPARE UTILITY FUNCTION - PROCESSES AN ARRAY LINK
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
// * determines whether the link points to a reference object or a ShmVector
// * - if reference object, makes sure the object being referenced has already been loaded from the file,
// *   then resolves reference
// * - if ShmVector, loads and attaches the target array. If the array is already present in memory, the load step is skipped
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processArrayLink(hid_t callingGroupId, const char *name, const H5L_info_t *info, void *opData)
{
    LinkIterData *linkIterData = (LinkIterData *)opData;

    if (linkIterData->archive->getVectorEntryByNvpName(linkIterData->nvpName)->referenceType ==
        ReferenceType::ObjectReference) {
        void *objectReference = linkIterData->archive->getVectorEntryByNvpName(linkIterData->nvpName)->ref;
        shm_obj_ref<Object> *objectReferencePtr = (shm_obj_ref<Object> *)objectReference;
        auto objectMapIter = linkIterData->callingModule->m_objectMap.find(std::string(name));

        // check if reference object has been constructed, if not, construct it
        if (objectMapIter == linkIterData->callingModule->m_objectMap.end()) {
            std::string referenceObjectName = "/object/" + std::string(name);
            htri_t referenceObjectExists = H5Lexists(linkIterData->fileId, referenceObjectName.c_str(), H5P_DEFAULT);

            if (util_doesExist(referenceObjectExists)) {
                LinkIterData referenceOpData(linkIterData);
                referenceOpData.origin = std::numeric_limits<unsigned>::max();

                // obtain origin - there are other ways to complete this processing step - think about which is best
                std::string originObjectName;
                htri_t originObjectExists;
                bool objectFound = false;

                for (unsigned i = 0; i < linkIterData->callingModule->m_numPorts && !objectFound; i++) {
                    originObjectName = "/index/t" + std::to_string(linkIterData->timestep) + "/b" +
                                       std::to_string(linkIterData->block) + "/p" + std::to_string(i);

                    originObjectExists = H5Lexists(linkIterData->fileId, originObjectName.c_str(), H5P_DEFAULT);
                    if (!util_doesExist(originObjectExists)) {
                        continue;
                    }

                    for (unsigned j = 0; /* exit conditions handled within */; j++) {
                        std::string variantGroupName = originObjectName + "/v" + std::to_string(j);
                        htri_t variantGroupExists =
                            H5Lexists(linkIterData->fileId, variantGroupName.c_str(), H5P_DEFAULT);

                        // exit condition
                        if (!util_doesExist(variantGroupExists)) {
                            break;
                        }

                        // check if variant object references match
                        variantGroupName += "/" + std::string(name);
                        variantGroupExists = H5Lexists(linkIterData->fileId, variantGroupName.c_str(), H5P_DEFAULT);

                        if (util_doesExist(variantGroupExists)) {
                            referenceOpData.origin = i;
                            objectFound = true;
                            break;
                        }
                    }
                }

                // only name and referenceOpData are needed within the function
                prepare_processObject(0, name, nullptr, &referenceOpData);

                // error message
                if (linkIterData->callingModule->m_objectMap.find(std::string(name)) ==
                    linkIterData->callingModule->m_objectMap.end()) {
                    linkIterData->callingModule->sendInfo("Error: reference object out of order creation failed");
                }

                // resolve reference
                *objectReferencePtr =
                    vistle::Shm::the().getObjectFromName(linkIterData->callingModule->m_objectMap[std::string(name)]);
            } else {
                // mark for unresolved references
                linkIterData->callingModule->m_unresolvedReferencesExist = true;
            }

        } else {
            // resolve reference
            *objectReferencePtr = vistle::Shm::the().getObjectFromName(objectMapIter->second);
        }

    } else if (linkIterData->archive->getVectorEntryByNvpName(linkIterData->nvpName)->referenceType ==
               ReferenceType::ShmVector) {
        ShmVectorReader reader(linkIterData->archive, std::string(name), linkIterData->nvpName,
                               linkIterData->callingModule->m_arrayMap, linkIterData->fileId,
                               linkIterData->callingModule->m_dummyDatasetId, linkIterData->callingModule->comm());

        boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorReader>(reader));
    }

    return 0;
}

// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::compute()
{
    // do nothing
    return true;
}


// REDUCE FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::reduce(int timestep)
{
    return Module::reduce(timestep);
}

// GENERIC UTILITY HELPER FUNCTION - CHECK THE VALIDITY OF THE FILENAME PARAMETER
// * also updates m_numPorts and handles port creation/deletion
//-------------------------------------------------------------------------
bool ReadHDF5::util_checkFile()
{
    herr_t status;
    hid_t fileId;
    hid_t filePropertyListId;
    hid_t dataSetId;
    hid_t dataSpaceId;
    hid_t readId;
    hsize_t dims[1];
    hsize_t maxDims[1];

    unsigned numNewPorts;
    std::string fileName = m_fileName->getValue();

    filesystem::path path(fileName);
    bool isDirectory;
    bool doesExist;

    // name size check
    if (fileName.size() == 0) {
        if (m_isRootNode) {
            sendInfo("File name too short");
        }
        return false;
    }

    // setup boost::filesystem
    try {
        isDirectory = filesystem::is_directory(path);
        doesExist = filesystem::exists(path);

    } catch (const filesystem::filesystem_error &error) {
        std::cerr << "filesystem error - directory: " << error.what() << std::endl;
        return false;
    }


    // check existence of file to remove error logs
    if (!doesExist) {
        if (m_isRootNode) {
            sendInfo("File does not exist");
        }
        return false;
    }

    // make sure name is not a directory - this causes H5Fopen to freeze
    if (isDirectory) {
        if (m_isRootNode) {
            sendInfo("File name cannot be a directory");
        }
        return false;
    }


    // Set up file access property list with parallel I/O access
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);

    // open file
    fileId = H5Fopen(fileName.c_str(), H5P_DEFAULT, filePropertyListId);

    // check if file exists
    if (fileId < 0) {
        if (m_isRootNode) {
            sendInfo("HDF5 file could not be opened");
        }

        return false;
    } else {
        if (m_isRootNode) {
            sendInfo("HDF5 file found");
        }
    }


    // obtain port number
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    dataSetId = H5Dopen2(fileId, "/file/num_ports", H5P_DEFAULT);
    status = H5Dread(dataSetId, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, readId, &numNewPorts);
    if (status != 0) {
        sendInfo("File format not recognized");
        return false;
    }

    H5Dclose(dataSetId);

    dataSetId = H5Dopen2(fileId, "/file/write_mode", H5P_DEFAULT);
    status = H5Dread(dataSetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, readId, &m_writeMode);
    if (status != 0) {
        sendInfo("File format not recognized");
        return false;
    }

    H5Dclose(dataSetId);


    // handle ports
    if (numNewPorts > m_numPorts) {
        for (unsigned i = m_numPorts; i < numNewPorts; i++) {
            std::string portName = "data" + std::to_string(i) + "_out";
            std::string portDescriptionPath = "/file/ports/" + std::to_string(i);
            std::vector<char> portDescription;

            // obtain port description
            dataSetId = H5Dopen2(fileId, portDescriptionPath.c_str(), H5P_DEFAULT);
            dataSpaceId = H5Dget_space(dataSetId);
            H5Sget_simple_extent_dims(dataSpaceId, dims, maxDims);

            portDescription.resize(dims[0] + 1);
            portDescription.back() = '\0';

            status = H5Dread(dataSetId, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, readId, portDescription.data());

            H5Sclose(dataSpaceId);
            H5Dclose(dataSetId);

            // create output port
            createOutputPort(portName, std::string(portDescription.data()));
        }
    } else {
        for (unsigned i = numNewPorts; i < m_numPorts; i++) {
            std::string portName = "data" + std::to_string(i) + "_out";
            destroyPort(portName);
        }
    }

    m_numPorts = numNewPorts;


    // close all open h5 entities
    H5Pclose(readId);
    H5Pclose(filePropertyListId);
    H5Fclose(fileId);

    return true;
}

// GENERIC UTILITY HELPER FUNCTION - SYNCHRONISE COLLECTIVE READS NULL READ VERSION
// * returns the total read size across the comm in bytes
//-------------------------------------------------------------------------
long double ReadHDF5::util_readSyncStandby(const boost::mpi::communicator &comm, hid_t fileId)
{
    hid_t dataSetId;
    hid_t readId;
    int readDummy;
    long double totalReadSize;
    long double readSize = 0;


    dataSetId = H5Dopen2(fileId, HDF5Const::DummyObject::name.c_str(), H5P_DEFAULT);

    boost::mpi::all_reduce(comm, readSize, totalReadSize, std::plus<long double>());

    // obtain port number
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    // perform write followed by read
    H5Dread(dataSetId, HDF5Const::DummyObject::type, H5S_ALL, H5S_ALL, readId, &readDummy);

    // release resources
    H5Pclose(readId);
    H5Dclose(dataSetId);


    return totalReadSize;
}

// GENERIC UTILITY HELPER FUNCTION - SYNCHRONISE COLLECTIVE READS
// * returns whether or not all nodes are done
//-------------------------------------------------------------------------
long double ReadHDF5::util_syncAndGetReadSize(long double readSize, const boost::mpi::communicator &comm)
{
    long double totalReadSize;

    boost::mpi::all_reduce(comm, readSize, totalReadSize, std::plus<long double>());

    return totalReadSize;
}

// GENERIC UTILITY HELPER FUNCTION - VERIFY HERR_T STATUS
//-------------------------------------------------------------------------
void ReadHDF5::util_checkStatus(herr_t status)
{
    if (status != 0) {
        sendInfo("Error: status: %i", status);
    }

    return;
}
