//-------------------------------------------------------------------------
// WRITE HDF5 CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#define NO_PORT_REMOVAL
#define HIDE_REFERENCE_WARNINGS

#include <cfloat>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <hdf5.h>

#include <vistle/core/findobjectreferenceoarchive.h>
#include <vistle/core/placeholder.h>
#include <vistle/util/filesystem.h>
#include <vistle/util/stopwatch.h>

#include "WriteHDF5.h"

using namespace vistle;


//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
BOOST_SERIALIZATION_REGISTER_ARCHIVE(FindObjectReferenceOArchive)

MODULE_MAIN(WriteHDF5)


//-------------------------------------------------------------------------
// WRITE HDF5 STATIC MEMBER OUT OF CLASS INITIALIZATION
//-------------------------------------------------------------------------
unsigned WriteHDF5::s_numMetaMembers = 0;
const std::unordered_map<std::type_index, hid_t> WriteHDF5::s_nativeTypeMap = {
    {typeid(int), H5T_NATIVE_INT},
    {typeid(unsigned int), H5T_NATIVE_UINT},

    {typeid(char), H5T_NATIVE_CHAR},
    {typeid(unsigned char), H5T_NATIVE_UCHAR},

    {typeid(short), H5T_NATIVE_SHORT},
    {typeid(unsigned short), H5T_NATIVE_USHORT},

    {typeid(long), H5T_NATIVE_LONG},
    {typeid(unsigned long), H5T_NATIVE_ULONG},
    {typeid(long long), H5T_NATIVE_LLONG},
    {typeid(unsigned long long), H5T_NATIVE_ULLONG},

    {typeid(float), H5T_NATIVE_FLOAT},
    {typeid(double), H5T_NATIVE_DOUBLE},
    {typeid(long double), H5T_NATIVE_LDOUBLE}

    // to implement? these are other accepted types
    // must implement arrays within DataArrayContainer if so
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
// * intitializes data and records the number of members in the meta object
//-------------------------------------------------------------------------
WriteHDF5::WriteHDF5(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    // create ports
    createInputPort("data0_in");

    // add module parameters
    m_fileName = addStringParameter("file_name", "Name of File that will be written to", "", Parameter::Filename);

    m_writeMode = addIntParameter("write_mode", "Select writing protocol", Performant, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_writeMode, WriteMode);

    m_overwrite = addIntParameter("overwrite", "Write even if output file exists", 0, Parameter::Boolean);

    m_portDescriptions.push_back(
        addStringParameter("port_description_0", "Description will appear as tooltip on read", "port 0"));

    // policies
    setReducePolicy(message::ReducePolicy::OverAll);
    setSchedulingPolicy(message::SchedulingPolicy::Single);

    // variable setup
    m_isRootNode = (this->comm().rank() == 0);
    m_numPorts = 1;

    // obtain meta member count
    PlaceHolder::ptr tempObject(new PlaceHolder("", Meta(), Object::Type::UNKNOWN));
    MemberCounterArchive memberCounter(&m_metaNvpTags);
    boost::serialization::serialize_adl(memberCounter, const_cast<Meta &>(tempObject->meta()),
                                        ::boost::serialization::version<Meta>::value);
    s_numMetaMembers = memberCounter.getCount();
}

// DESTRUCTOR
//-------------------------------------------------------------------------
WriteHDF5::~WriteHDF5()
{}

// CONNECTION REMOVED FUNCTION
//-------------------------------------------------------------------------
void WriteHDF5::connectionRemoved(const Port *from, const Port *to)
{
#ifndef NO_PORT_REMOVAL
    unsigned numNotConnected = 0;

    if (m_numPorts <= 1) {
        return Module::connectionRemoved(from, to);
    }

    for (unsigned i = m_numPorts - 1; i >= 0; i--) {
        std::string portName = "data" + std::to_string(i) + "_in";
        if (!isConnected(portName)) {
            numNotConnected++;
        }
    }

    for (unsigned i = m_numPorts - 1; i > m_numPorts - numNotConnected; i--) {
        std::string portName = "data" + std::to_string(i) + "_in";
        destroyPort(portName);
    }

    m_numPorts -= (numNotConnected - 1);
#endif

    return Module::connectionRemoved(from, to);
}

// CONNECTION ADDED FUNCTION
//-------------------------------------------------------------------------
void WriteHDF5::connectionAdded(const Port *from, const Port *to)
{
    std::string lastPortName = "data" + std::to_string(m_numPorts - 1) + "_in";
    std::string newPortName = "data" + std::to_string(m_numPorts) + "_in";
    std::string newPortDescriptionName = "port_description_" + std::to_string(m_numPorts - 1);
    std::string newPortDescription = "port " + std::to_string(m_numPorts - 1);

    if (from->getName() == lastPortName || to->getName() == lastPortName) {
        createInputPort(newPortName);

        if (m_numPorts != 1) {
            m_portDescriptions.push_back(addStringParameter(
                newPortDescriptionName.c_str(), "Description will appear as tooltip on read", newPortDescription));
        }

        m_numPorts++;
    }

    return Module::connectionAdded(from, to);
}

// PARAMETER CHANGED FUNCTION
// * change scheduling policy based on write mode
//-------------------------------------------------------------------------
bool WriteHDF5::parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg)
{
    if (name == m_writeMode->getName()) {
        if (m_writeMode->getValue() == WriteMode::Performant) {
            setSchedulingPolicy(message::SchedulingPolicy::Single);
        } else if (m_writeMode->getValue() == WriteMode::Organized) {
            setSchedulingPolicy(message::SchedulingPolicy::LazyGang);
        }
    }

    return true;
}

// PREPARE FUNCTION
// * - error checks incoming filename
// * - creates basic groups within the HDF5 file and instantiates the dummy object used
// *   by nodes with null data for writing collectively
// * - writes auxiliary file metadata
//-------------------------------------------------------------------------
bool WriteHDF5::prepare()
{
    herr_t status;
    hid_t filePropertyListId;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t groupId;
    hid_t writeId;
    hsize_t oneDims[] = {1};

    MPI_Info mpiInfo;

    m_doNotWrite = false;
    m_unresolvedReferencesExist = false;
    m_numPorts -= 1; // ignore extra port at end - it will never contain a connection
    m_objectDataArraySize = 0;

    // error check incoming filename
    if (!prepare_fileNameCheck()) {
        m_doNotWrite = true;
        m_doNotWrite = boost::mpi::all_reduce(comm(), m_doNotWrite, std::logical_and<bool>());
        return Module::prepare();
    }

    // clear reused variables
    m_arrayMap.clear();
    m_objectSet.clear();
    m_indexVariantTracker.clear();
    m_objContainerVector.clear();
    m_objTypeToDataMap.clear();

    // Create info to be attached to HDF5 file
    MPI_Info_create(&mpiInfo);

    // Disables ROMIO's data-sieving
    MPI_Info_set(mpiInfo, "romio_ds_read", "disable");
    MPI_Info_set(mpiInfo, "romio_ds_write", "disable");

    // Enable ROMIO's collective buffering
    MPI_Info_set(mpiInfo, "romio_cb_read", "enable");
    MPI_Info_set(mpiInfo, "romio_cb_write", "enable");

    // Set up file access property list with parallel I/O access
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), mpiInfo);

    // free info
    MPI_Info_free(&mpiInfo);

    // create new file
    m_fileId = H5Fcreate(m_fileName->getValue().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, filePropertyListId);
    H5Pclose(filePropertyListId);

    // create basic vistle HDF5 Groups: meta information about this Vistle HDF5 file
    groupId = H5Gcreate2(m_fileId, "/file", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /file");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: objects
    groupId = H5Gcreate2(m_fileId, "/object", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /object");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: references to all the objects received on ports
    groupId = H5Gcreate2(m_fileId, "/index", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /index");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: array data
    groupId = H5Gcreate2(m_fileId, "/array", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /array");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create dummy object
    fileSpaceId = H5Screate_simple(1, HDF5Const::DummyObject::dims.data(), NULL);
    dataSetId = H5Dcreate(m_fileId, HDF5Const::DummyObject::name.c_str(), HDF5Const::DummyObject::type, fileSpaceId,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);

    // create version number
    const std::string versionPath("/file/version");
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId =
        H5Dcreate(m_fileId, versionPath.c_str(), H5T_NATIVE_INT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5WriteOrganized(m_isRootNode, versionPath.c_str(), &HDF5Const::versionNumber, m_fileId, oneDims,
                            H5T_NATIVE_INT);

    // create version number
    const std::string writeModePath("/file/write_mode");
    int writeMode = m_writeMode->getValue();
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId =
        H5Dcreate(m_fileId, writeModePath.c_str(), H5T_NATIVE_INT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5WriteOrganized(m_isRootNode, writeModePath.c_str(), &writeMode, m_fileId, oneDims, H5T_NATIVE_INT);

    // store number of ports
    const std::string numPortsPath("/file/num_ports");
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId =
        H5Dcreate(m_fileId, numPortsPath.c_str(), H5T_NATIVE_UINT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5WriteOrganized(m_isRootNode, numPortsPath.c_str(), &m_numPorts, m_fileId, oneDims, H5T_NATIVE_UINT);

    // save port descriptions
    groupId = H5Gcreate2(m_fileId, "/file/ports", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /file/ports");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string name = "/file/ports/" + std::to_string(i);
        std::string description = m_portDescriptions[i]->getValue();
        hsize_t dims[1];

        // make sure description is not empty
        if (description.size() == 0) {
            description = "Port " + std::to_string(i);
        }

        dims[0] = description.size();

        // store description
        writeId = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);

        fileSpaceId = H5Screate_simple(1, dims, NULL);
        dataSetId =
            H5Dcreate2(m_fileId, name.c_str(), H5T_NATIVE_CHAR, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        status = H5Dwrite(dataSetId, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, writeId, description.data());
        util_checkStatus(status);

        H5Dclose(dataSetId);
        H5Sclose(fileSpaceId);
        H5Pclose(writeId);
    }

    // write meta nvp tags
    groupId = H5Gcreate2(m_fileId, "/file/meta_tags", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /file/meta_tags");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    for (unsigned i = 0; i < m_metaNvpTags.size(); i++) {
        std::string name = "/file/meta_tags/" + std::to_string(i);
        hsize_t dims[] = {m_metaNvpTags[i].size()};

        // store description
        writeId = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);

        fileSpaceId = H5Screate_simple(1, dims, NULL);
        dataSetId =
            H5Dcreate2(m_fileId, name.c_str(), H5T_NATIVE_CHAR, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        status = H5Dwrite(dataSetId, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, writeId, m_metaNvpTags[i].data());
        util_checkStatus(status);

        H5Dclose(dataSetId);
        H5Sclose(fileSpaceId);
        H5Pclose(writeId);
    }

    return Module::prepare();
}


// REDUCE FUNCTION
// * delegate to appropriate reduce utility function
//-------------------------------------------------------------------------
bool WriteHDF5::reduce(int timestep)
{
    if (m_writeMode->getValue() == WriteMode::Organized) {
        reduce_organized();
    } else if (m_writeMode->getValue() == WriteMode::Performant) {
        reduce_performant();
    }

    // close hdf5 file
    H5Fclose(m_fileId);

    //restore portnum value
    m_numPorts += 1;


#ifndef HIDE_REFERENCE_WARNINGS

    // check and send warning message for unresolved references
    if (m_isRootNode) {
        bool unresolvedReferencesExistInComm;

        boost::mpi::reduce(comm(), m_unresolvedReferencesExist, unresolvedReferencesExistInComm,
                           boost::mpi::maximum<bool>(), 0);

        if (unresolvedReferencesExistInComm) {
            sendInfo("Warning: some object references are unresolved.");
        }
    } else {
        boost::mpi::reduce(comm(), m_unresolvedReferencesExist, boost::mpi::maximum<bool>(), 0);
    }
#endif

    return Module::reduce(timestep);
}

// REDUCE UTILITY FUNCTION - ORGANIZED
//-------------------------------------------------------------------------
void WriteHDF5::reduce_organized()
{
    // do nothing
    return;
}

// REDUCE UTILITY FUNCTION - PERFORMANT
// * the following steps are executed:
// * - builds object, object_data, data arrays
// * - builds index
// * - distributes global offsets between nodes
// * - offsets necessary indices to reflect the global index
// * - sends object type data to the root node
// * - root node concatenates all type data and writes it
// * - nodes collectively write all data
//-------------------------------------------------------------------------
void WriteHDF5::reduce_performant()
{
    DataArrayContainer dataArrayContainer;

    std::vector<unsigned> objectArray;
    std::vector<double> objectMetaArray;
    ContiguousMemoryMatrix<unsigned> objectDataArray(m_objectDataArraySize, 2);

    ContiguousMemoryMatrix<int> blockArray(m_objectDataArraySize,
                                           3); //XXX can be optimized - value is greater than needed
    std::vector<int> timestepArray;
    ContiguousMemoryMatrix<unsigned> portArray(m_objectDataArraySize,
                                               2); //XXX can be optimized - value is greater than needed
    std::vector<unsigned> portObjectListArray;

    std::unordered_map<int, std::vector<std::pair<hid_t, std::string>>> objTypeToDataMap;

    std::unordered_map<std::string, unsigned> objectReferenceMap;

    const unsigned numMetaElementsInArray = (WriteHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers + 1);
    hsize_t dims[2];
    hsize_t offset[2];

    // timing metric
    double reduceBeginTime = Clock::time();
    if (m_isRootNode)
        sendInfo("begin reduce");

    // reserve space for all arrays where the size is known/can be estimated
    objectArray.reserve(m_objContainerVector.size());
    objectMetaArray.reserve(m_objContainerVector.size() * numMetaElementsInArray);


    // sorting allows for the assumption that all values appartaining to a particular
    // block/timestep/port will be stored in a contiguous portion of the file. this allows
    // for fewer separate reads to be made.
    // (e.g. if we want to read a whole block, we can do this with one read instead of several)
    std::sort(m_objContainerVector.begin(), m_objContainerVector.end());


    // block order test output
    /*
    if (m_isRootNode)
    for (unsigned i = 0; i < m_objContainerVector.size(); i++) {
        sendInfo("obj: %d, %d, %u, %u",
                 m_objContainerVector[i].ptr->getBlock(),
                 m_objContainerVector[i].ptr->getTimestep(),
                 m_objContainerVector[i].port,
                 m_objContainerVector[i].order
                 );
    }
*/

    // construct object reference map
    // XXX this can be done in compute as well to save time
    unsigned index = 0;
    for (unsigned i = 0; i < m_objContainerVector.size(); i++) {
        if (!m_objContainerVector[i].isDuplicate) {
            objectReferenceMap[m_objContainerVector[i].obj->getName()] = index;
            index++;
        }
    }

    // construct object, object_data, data arrays
    for (unsigned i = 0; i < m_objContainerVector.size(); i++) {
        // build object arrays
        if (!m_objContainerVector[i].isDuplicate) {
            std::vector<std::string> attributeKeyVector = m_objContainerVector[i].obj->getAttributeList();
            unsigned objRefArchiveVecSize = m_objContainerVector[i].objRefArchive.getVector().size();

            // get initial offsets for object array
            if (objectArray.empty()) {
                objectArray.push_back(0);
            } else {
                objectArray.push_back(objRefArchiveVecSize + objectArray.back());
            }

            // build data arrays and object reference related arrays
            // namely: data arrays, array & ref entries within object_data
            for (unsigned j = 0; j < objRefArchiveVecSize; j++) {
                FindObjectReferenceOArchive::ReferenceData *currRefData =
                    &m_objContainerVector[i].objRefArchive.getVector()[j];

                if (currRefData->referenceType == ReferenceType::ShmVector) {
                    // append shmvector data to data arrays
                    DataArrayAppender appender(&dataArrayContainer, currRefData->referenceName);
                    boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<DataArrayAppender>(appender));

                    // append object_data entry
                    unsigned dataArrayIndex = appender.getNewDataArraySize() - appender.getAppendArraySize();
                    objectDataArray.push_back({dataArrayIndex, appender.getAppendArraySize()});

                } else if (currRefData->referenceType == ReferenceType::ObjectReference &&
                           currRefData->referenceName != FindObjectReferenceOArchive::nullObjectReferenceName) {
                    // append object_data entry
                    objectDataArray.push_back(
                        {objectReferenceMap[currRefData->referenceName], HDF5Const::performantReferenceNullVal});
                }
            }

            // append attributes to data arrays and object_data
            for (unsigned j = 0; j < attributeKeyVector.size(); j++) {
                std::string value = m_objContainerVector[i].obj->getAttribute(attributeKeyVector[j]);
                unsigned dataArraySizeAfterKeyAppend;
                unsigned objectDataIndex;

                // append key followed by value within the char data array (both are null terminated)
                dataArraySizeAfterKeyAppend =
                    dataArrayContainer.append(attributeKeyVector[j].c_str(), attributeKeyVector[j].size());
                dataArrayContainer.append(value.c_str(), value.size());

                // append index of key into the object_data array
                // XXX read size fix should occur here. see note 2 within ReadHDF5.cpp XXX comment
                objectDataIndex = dataArraySizeAfterKeyAppend - (attributeKeyVector[j].size() + 1);
                objectDataArray.push_back({objectDataIndex, HDF5Const::performantAttributeNullVal});
            }

            // build meta
            double *metaDataPtr = m_objContainerVector[i].metaToArrayArchive.getDataPtr();
            for (unsigned j = 0; j < WriteHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers; j++) {
                objectMetaArray.push_back(*metaDataPtr);
                metaDataPtr++;
            }
            objectMetaArray.push_back(attributeKeyVector.size());
        }
    }

    // build index
    const unsigned resetFlag = std::numeric_limits<unsigned>::max() - 1;
    unsigned currBlock = resetFlag;
    unsigned currBlockNumEl = 0;

    unsigned currTimestep;

    unsigned currPort;
    unsigned currPortNumEl = 0;

    for (unsigned i = 0; i < m_objContainerVector.size(); i++) {
        // abort if object is only accessed by reference
        if (m_objContainerVector[i].port == HDF5Const::performantReferenceNullVal) {
            continue;
        }

        if (currBlock != m_objContainerVector[i].obj->getBlock()) {
            currTimestep = resetFlag;
            currPort = resetFlag;

            if (i != 0) {
                blockArray.back()[2] = currBlockNumEl;
                currBlockNumEl = 0;
            }

            blockArray.push_back({m_objContainerVector[i].obj->getBlock(), static_cast<int>(timestepArray.size()), 0});

            currBlock = m_objContainerVector[i].obj->getBlock();
        }

        if (currTimestep != m_objContainerVector[i].obj->getTimestep()) {
            currPort = resetFlag;

            timestepArray.push_back(portArray.size());

            if (currTimestep == resetFlag) {
                currBlockNumEl++;
            }

            currTimestep = m_objContainerVector[i].obj->getTimestep();
        }

        if (currPort != m_objContainerVector[i].port) {
            if (i != 0) {
                portArray.back()[1] = currPortNumEl;
                currPortNumEl = 0;
            }

            portArray.push_back({static_cast<unsigned>(portObjectListArray.size()), 0});

            currPort = m_objContainerVector[i].port;
        }
        currPortNumEl++;

        portObjectListArray.push_back(objectReferenceMap[m_objContainerVector[i].obj->getName()]);
    }

    if (!m_objContainerVector.empty()) {
        portArray.back()[1] = currPortNumEl;
        blockArray.back()[2] = currBlockNumEl;
    }

    // distribute offsets between nodes
    std::vector<std::pair<hid_t, unsigned>> dataArraySizeVector = dataArrayContainer.getSizeVector();
    OffsetContainer arrayWriteOffsets(dataArraySizeVector);
    OffsetContainer nodeOffsetSizes(dataArraySizeVector);

    nodeOffsetSizes.object = objectArray.size();
    nodeOffsetSizes.objectData = objectDataArray.size();
    nodeOffsetSizes.block = blockArray.size();
    nodeOffsetSizes.timestep = timestepArray.size();
    nodeOffsetSizes.port = portArray.size();
    nodeOffsetSizes.portObjectList = portObjectListArray.size();
    nodeOffsetSizes.dataArrays = dataArraySizeVector;

    if (size() > 1) {
        if (rank() == 0) {
            comm().send(rank() + 1, 0, nodeOffsetSizes);

        } else if (rank() == size() - 1) {
            comm().recv(rank() - 1, 0, arrayWriteOffsets);

        } else {
            comm().recv(rank() - 1, 0, arrayWriteOffsets);
            comm().send(rank() + 1, 0, arrayWriteOffsets + nodeOffsetSizes);
        }
        comm().barrier();
    }


    // offset array indices
    if (!m_isRootNode) {
        for (unsigned i = 0; i < objectArray.size(); i++) {
            // offset objectData entries
            unsigned currObjType = objectMetaArray[i * numMetaElementsInArray + WriteHDF5::s_numMetaMembers];
            unsigned numRefAndShmVecEntries = m_objTypeToDataMap[currObjType].size();
            unsigned numAttributeEntries = objectMetaArray[i * numMetaElementsInArray + WriteHDF5::s_numMetaMembers +
                                                           HDF5Const::additionalMetaArrayMembers];
            unsigned numDataElements = numAttributeEntries + numRefAndShmVecEntries;

            for (unsigned j = objectArray[i]; j < numDataElements; j++) {
                if (objectDataArray(j, 1) == HDF5Const::performantReferenceNullVal) {
                    objectDataArray(j, 0) += arrayWriteOffsets.object;

                    // XXX read size fix should occur here. see note 2 within ReadHDF5.cpp XXX comment
                } else if (objectDataArray(j, 1) == HDF5Const::performantAttributeNullVal) {
                    for (unsigned k = 0; k < arrayWriteOffsets.dataArrays.size(); k++) {
                        if (arrayWriteOffsets.dataArrays[k].first == s_nativeTypeMap.find(typeid(char))->second) {
                            objectDataArray(j, 0) += arrayWriteOffsets.dataArrays[k].second;
                            break;
                        }
                    }

                } else {
                    for (unsigned k = 0; k < arrayWriteOffsets.dataArrays.size(); k++) {
                        if (arrayWriteOffsets.dataArrays[k].first == m_objTypeToDataMap[currObjType][j].first) {
                            objectDataArray(j, 0) += arrayWriteOffsets.dataArrays[k].second;
                            break;
                        }
                    }
                }
            }

            // offset object
            objectArray[i] += arrayWriteOffsets.objectData;
        }

        // offset block;
        for (unsigned i = 0; i < blockArray.size(); i++) {
            blockArray(i, 1) += arrayWriteOffsets.block;
        }

        // offset timestep;
        for (unsigned i = 0; i < timestepArray.size(); i++) {
            timestepArray[i] += arrayWriteOffsets.timestep;
        }

        // offset port;
        for (unsigned i = 0; i < portArray.size(); i++) {
            portArray(i, 0) += arrayWriteOffsets.port;
        }

        // offset port_object_list;
        for (unsigned i = 0; i < portObjectListArray.size(); i++) {
            portObjectListArray[i] += arrayWriteOffsets.portObjectList;
        }
    }

    // transmit type to data map information
    // XXX compile with boost serilaization for unordered_map to remove need for transfer vectors
    if (size() > 1) {
        if (m_isRootNode) {
            for (unsigned i = 1; i < size(); i++) {
                ObjTypeToDataTransferVector newTypeToDataMap;

                comm().recv(i, 1, newTypeToDataMap);

                for (unsigned j = 0; j < newTypeToDataMap.size(); j++) {
                    if (m_objTypeToDataMap.find(newTypeToDataMap[j].first) == m_objTypeToDataMap.end()) {
                        m_objTypeToDataMap[newTypeToDataMap[j].first] = newTypeToDataMap[j].second;
                    }
                }
            }
        } else {
            ObjTypeToDataTransferVector transferVector;

            for (auto it: m_objTypeToDataMap) {
                transferVector.push_back(std::make_pair(it.first, it.second));
            }

            comm().send(0, 1, transferVector);
        }
    }

    // timing metric
    double writeBeginTime = Clock::time();
    if (m_isRootNode)
        sendInfo("done concatenating - %fs", Clock::time() - reduceBeginTime);

    // create & write type mapping arrays
    if (m_isRootNode) {
        ContiguousMemoryMatrix<int> typeToObjectDataElementInfoArray(m_objTypeToDataMap.size(), 3);
        ContiguousMemoryMatrix<int> objectDataElementInfoArray;
        std::string nvpTagsArray;

        // reserve size for objectDataElementInfoArray
        unsigned numDataElements = 0;
        for (auto it: m_objTypeToDataMap) {
            numDataElements += it.second.size();
        }
        objectDataElementInfoArray.reserve(numDataElements, 2);

        // fill arrays
        for (auto it: m_objTypeToDataMap) {
            std::vector<std::pair<hid_t, std::string>> &dataElementVector = it.second;

            typeToObjectDataElementInfoArray.push_back({it.first, static_cast<int>(objectDataElementInfoArray.size()),
                                                        static_cast<int>(dataElementVector.size())});

            for (unsigned i = 0; i < dataElementVector.size(); i++) {
                objectDataElementInfoArray.push_back(
                    {static_cast<int>(dataElementVector[i].first), static_cast<int>(nvpTagsArray.size())});

                nvpTagsArray += dataElementVector[i].second + '\0';
            }
        }

        // write arrays
        dims[0] = typeToObjectDataElementInfoArray.size();
        dims[1] = 3;
        offset[0] = 0;
        offset[1] = 0;
        util_HDF5WritePerformant("object/type_to_object_element_info", 2, &dims[0], &offset[0], H5T_NATIVE_UINT,
                                 typeToObjectDataElementInfoArray.data());

        dims[0] = objectDataElementInfoArray.size();
        dims[1] = 2;
        offset[0] = 0;
        offset[1] = 0;
        util_HDF5WritePerformant("object/object_element_info", 2, &dims[0], &offset[0], H5T_NATIVE_UINT,
                                 objectDataElementInfoArray.data());

        dims[0] = nvpTagsArray.size();
        offset[0] = 0;
        util_HDF5WritePerformant("object/nvp_tags", 1, &dims[0], &offset[0], H5T_NATIVE_CHAR, nvpTagsArray.data());

    } else {
        dims[0] = 0;
        dims[1] = 3;
        offset[0] = 0;
        offset[1] = 0;
        util_HDF5WritePerformant("object/type_to_object_element_info", 2, &dims[0], &offset[0], H5T_NATIVE_UINT,
                                 (const unsigned *)nullptr);

        dims[1] = 2;
        util_HDF5WritePerformant("object/object_element_info", 2, &dims[0], &offset[0], H5T_NATIVE_UINT,
                                 (const unsigned *)nullptr);

        dims[1] = 1;
        util_HDF5WritePerformant("object/nvp_tags", 1, &dims[0], &offset[0], H5T_NATIVE_CHAR, (const char *)nullptr);
    }

    // write arrays
    dataArrayContainer.writeToFile(m_fileId, comm());

    dims[0] = objectArray.size();
    offset[0] = arrayWriteOffsets.object;
    util_HDF5WritePerformant("object/object", 1, &dims[0], &offset[0], H5T_NATIVE_UINT, objectArray.data());

    dims[0] = objectMetaArray.size();
    offset[0] = arrayWriteOffsets.object * numMetaElementsInArray;
    util_HDF5WritePerformant("object/object_meta", 1, &dims[0], &offset[0], H5T_NATIVE_DOUBLE, objectMetaArray.data());

    dims[0] = objectDataArray.size();
    dims[1] = 2;
    offset[0] = arrayWriteOffsets.objectData;
    offset[1] = 0;
    util_HDF5WritePerformant("object/object_data", 2, &dims[0], &offset[0], H5T_NATIVE_UINT, objectDataArray.data());

    // write in pattern
    dims[0] = blockArray.size();
    dims[1] = 3;
    offset[0] = arrayWriteOffsets.block;
    offset[1] = 0;
    util_HDF5WritePerformant("index/block", 2, &dims[0], &offset[0], H5T_NATIVE_UINT, blockArray.data());

    dims[0] = timestepArray.size();
    offset[0] = arrayWriteOffsets.timestep;
    util_HDF5WritePerformant("index/timestep", 1, &dims[0], &offset[0], H5T_NATIVE_UINT, timestepArray.data());

    dims[0] = portArray.size();
    dims[1] = 2;
    offset[0] = arrayWriteOffsets.port;
    offset[1] = 0;
    util_HDF5WritePerformant("index/port", 2, &dims[0], &offset[0], H5T_NATIVE_UINT, portArray.data());

    dims[0] = portObjectListArray.size();
    offset[0] = arrayWriteOffsets.portObjectList;
    util_HDF5WritePerformant("index/port_object_list", 1, &dims[0], &offset[0], H5T_NATIVE_UINT,
                             portObjectListArray.data());

    // timing metric
    if (m_isRootNode)
        sendInfo("done writing - %fs", Clock::time() - writeBeginTime);
}

// COMPUTE
// * delegates function call to appropriate writing method
//-------------------------------------------------------------------------
bool WriteHDF5::compute()
{
    if (m_writeMode->getValue() == WriteMode::Organized) {
        compute_organized();
    } else if (m_writeMode->getValue() == WriteMode::Performant) {
        compute_performant();
    }

    return true;
}

// COMPUTE - ORGANIZED METHOD
// * function procedure:
// * - serialized incoming object data into a ShmArchive
// * - transmits information regarding object data sizes to all nodes
// * - nodes reserve space for all objects independently
// * - nodes write data into the HDF5 file collectively. If a object is
// *   not present on the node, null data is written to a dummy object so that
// *   the call is still made collectively
//-------------------------------------------------------------------------
void WriteHDF5::compute_organized()
{
    // the following have size = num objects to write on node (== numNodeObjects)
    std::vector<FindObjectReferenceOArchive> objRefArchiveVector;
    std::vector<MetaToArrayArchive> metaToArrayArchiveVector;
    std::vector<Object::const_ptr> objPtrVector; // for object persistence
    std::vector<ObjectWriteInfo> objectWriteVector;
    // the following has size = num objects to write in comm
    std::vector<ObjectWriteInfo> objectWriteGatherVector;

    // the following has size = indices to write on node
    std::vector<IndexWriteInfo> indexWriteVector;
    // the following has size = indices to write in comm
    std::vector<IndexWriteInfo> indexWriteGatherVector;

    unsigned numNodeObjects = 0;
    unsigned numNodeArrays = 0;

    // ----- OBTAIN OBJECT FROM A PORT ----- //

    for (unsigned originPortNumber = 0; originPortNumber < m_numPorts; originPortNumber++) {
        std::string portName = "data" + std::to_string(originPortNumber) + "_in";

        // acquire input data object
        Object::const_ptr obj = expect<Object>(portName);

        // save if available
        if (obj) {
            compute_organized_addObjectToWrite(obj, originPortNumber, numNodeObjects, numNodeArrays, objPtrVector,
                                               objRefArchiveVector, metaToArrayArchiveVector, objectWriteVector,
                                               indexWriteVector);
        }
    }


    // ----- TRANSMIT OBJECT DATA DIMENSIONS TO ALL NODES FOR RESERVATION OF FILE DATASPACE ----- //

    // transmit all reservation information
    boost::mpi::all_reduce(comm(), objectWriteVector, objectWriteGatherVector, VectorConcatenator<ObjectWriteInfo>());
    boost::mpi::all_reduce(comm(), indexWriteVector, indexWriteGatherVector, VectorConcatenator<IndexWriteInfo>());

    // sort for proper collective io ordering
    std::sort(objectWriteGatherVector.begin(), objectWriteGatherVector.end());
    std::sort(indexWriteGatherVector.begin(), indexWriteGatherVector.end());

    // ----- RESERVE SPACE WITHIN THE HDF5 FILE ----- //

    // open file
    herr_t status;
    hid_t groupId;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hsize_t metaDims[] = {WriteHDF5::s_numMetaMembers + HDF5Const::additionalMetaArrayMembers};
    hsize_t dims[] = {0};
    double *metaData = nullptr;
    std::string writeName;


    // reserve space for object
    for (unsigned i = 0; i < objectWriteGatherVector.size(); i++) {
        std::string objectGroupName = "/object/" + objectWriteGatherVector[i].name;

        groupId = H5Gcreate2(m_fileId, objectGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        util_checkId(groupId, "create group " + objectGroupName);

        // create metadata dataset
        fileSpaceId = H5Screate_simple(1, metaDims, NULL);
        dataSetId = H5Dcreate(groupId, "meta", H5T_NATIVE_DOUBLE, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(fileSpaceId);
        H5Dclose(dataSetId);

        // close group
        status = H5Gclose(groupId);
        util_checkStatus(status);

        // handle referenceVector and shmVector links
        for (unsigned j = 0; j < objectWriteGatherVector[i].referenceVector.size(); j++) {
            std::string referenceName = objectWriteGatherVector[i].referenceVector[j].name;
            std::string referencePath;

            if (objectWriteGatherVector[i].referenceVector[j].referenceType == ReferenceType::ShmVector) {
                referencePath = "/array/" + referenceName;

                if (m_arrayMap.find(referenceName) == m_arrayMap.end()) {
                    m_arrayMap.insert({{referenceName, false}});

                    // create array dataset
                    dims[0] = objectWriteGatherVector[i].referenceVector[j].size;
                    fileSpaceId = H5Screate_simple(1, dims, NULL);
                    dataSetId =
                        H5Dcreate(m_fileId, referencePath.c_str(), objectWriteGatherVector[i].referenceVector[j].type,
                                  fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    H5Sclose(fileSpaceId);
                    H5Dclose(dataSetId);
                }
            } else if (objectWriteGatherVector[i].referenceVector[j].referenceType == ReferenceType::ObjectReference) {
                referencePath = "/object/" + referenceName;
            }

            std::string linkGroupName = objectGroupName + "/" + objectWriteGatherVector[i].referenceVector[j].nvpTag;
            groupId = H5Gcreate2(m_fileId, linkGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group " + linkGroupName);

            status = H5Lcreate_soft(referencePath.c_str(), groupId, referenceName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
            util_checkStatus(status);

            status = H5Gclose(groupId);
            util_checkStatus(status);
        }
    }

    // reserve space for index and write
    for (unsigned i = 0; i < indexWriteGatherVector.size(); i++) {
        std::string objectGroupName = "/object/" + objectWriteGatherVector[i].name;
        int currOrigin = indexWriteGatherVector[i].origin;
        int currTimestep = indexWriteGatherVector[i].timestep;
        int currBlock = indexWriteGatherVector[i].block;


        // create timestep group if it does not already exist
        std::string groupName = "/index/t" + std::to_string(currTimestep);
        if (m_indexVariantTracker.find(currTimestep) == m_indexVariantTracker.end()) {
            m_indexVariantTracker[currTimestep];

            groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group " + groupName);
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }

        // create block group if it does not already exist
        groupName += "/b" + std::to_string(currBlock);
        if (m_indexVariantTracker[currTimestep].find(currBlock) == m_indexVariantTracker[currTimestep].end()) {
            m_indexVariantTracker[currTimestep][currBlock];

            groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group " + groupName);
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }

        // create port group if it does not already exist
        groupName += "/p" + std::to_string(currOrigin);
        if (m_indexVariantTracker[currTimestep][currBlock].find(currOrigin) ==
            m_indexVariantTracker[currTimestep][currBlock].end()) {
            m_indexVariantTracker[currTimestep][currBlock][currOrigin] = 0;

            groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group " + groupName);
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }

        // create variants group if it does not already exist
        groupName += "/v" + std::to_string(m_indexVariantTracker[currTimestep][currBlock][currOrigin]);
        groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        util_checkId(groupId, "create group " + groupName);
        m_indexVariantTracker[currTimestep][currBlock][currOrigin]++;


        // link index entry to object
        status = H5Lcreate_soft(objectGroupName.c_str(), groupId, indexWriteGatherVector[i].name.c_str(), H5P_DEFAULT,
                                H5P_DEFAULT);
        util_checkStatus(status);


        // close group
        status = H5Gclose(groupId);
        util_checkStatus(status);
    }


    // ----- WRITE DATA TO THE HDF5 FILE ----- //

    // reduce object number size information for collective calls
    unsigned maxNumCommObjects;
    boost::mpi::all_reduce(comm(), numNodeObjects, maxNumCommObjects, boost::mpi::maximum<unsigned>());

    // write meta info
    for (unsigned i = 0; i < maxNumCommObjects; i++) {
        bool nodeHasObject = (numNodeObjects > i);

        if (nodeHasObject) {
            writeName = "/object/" + objectWriteVector[i].name + "/meta";
            metaData = metaToArrayArchiveVector[i].getDataPtr();
        }

        util_HDF5WriteOrganized(nodeHasObject, writeName, metaData, m_fileId, metaDims, H5T_NATIVE_DOUBLE);
    }

    // reduce vector size information for collective calls
    unsigned maxNumCommArrays;
    boost::mpi::all_reduce(comm(), numNodeArrays, maxNumCommArrays, boost::mpi::maximum<unsigned>());

    unsigned numArraysWritten = 0;

    // write necessary arrays
    for (unsigned i = 0; i < objRefArchiveVector.size(); i++) {
        for (unsigned j = 0; j < objRefArchiveVector[i].getVector().size(); j++) {
            if (objRefArchiveVector[i].getVector()[j].referenceType == ReferenceType::ShmVector &&
                m_arrayMap[objRefArchiveVector[i].getVector()[j].referenceName] == false) {
                // this is an array to be written
                m_arrayMap[objRefArchiveVector[i].getVector()[j].referenceName] = true;

                ShmVectorWriterOrganized shmVectorWriter(objRefArchiveVector[i].getVector()[j].referenceName, m_fileId,
                                                         comm());
                boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorWriterOrganized>(shmVectorWriter));

                numArraysWritten++;
            }
        }
    }

    // pad the rest of the collective calls with dummy writes
    while (numArraysWritten < maxNumCommArrays) {
        ShmVectorWriterOrganized shmVectorWriter(m_fileId, comm());
        shmVectorWriter.writeDummy();
        numArraysWritten++;
    }

    return;
}


// COMPUTE - PERFORMANT METHOD
// * function procedure:
// * - simply collect all incoming objects - write is performed at end
//-------------------------------------------------------------------------
void WriteHDF5::compute_performant()
{
    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string portName = "data" + std::to_string(i) + "_in";

        // acquire input data object
        Object::const_ptr obj = expect<Object>(portName);

        // save if available
        if (obj) {
            compute_performant_addObjectToWrite(obj, i);
        }
    }
}

// COMPUTE HELPER FUNCTION - PERFORMANT - PREPARES AN OBJECT FOR WRITING
// * adds objects and referenced objects into object container
//-------------------------------------------------------------------------
void WriteHDF5::compute_performant_addObjectToWrite(vistle::Object::const_ptr obj, unsigned originPortNumber)
{
    bool isDuplicate = false;

    // XXX optimize this? use unordered_map?
    for (unsigned i = 0; i < m_objContainerVector.size(); i++) {
        if (m_objContainerVector[i].obj->getName() == obj->getName()) {
            // if this object was previously queued for write as a reference, and now it comes in as an
            // object from a port, we overwrite the port number to match its origin port
            if (m_objContainerVector[i].port == HDF5Const::performantReferenceNullVal &&
                originPortNumber != HDF5Const::performantReferenceNullVal) {
                m_objContainerVector[i].port = originPortNumber;
            } else {
                isDuplicate = true;
            }
        }
    }

    // store object
    WriteObjectContainer container(obj, originPortNumber, m_objContainerVector.size(), isDuplicate);
    vistle::FindObjectReferenceOArchive *archive = &container.objRefArchive;

    // calculate and store objectDataArraySize
    if (!isDuplicate) {
        m_objectDataArraySize += archive->getVector().size() + obj->getAttributeList().size();

        auto typeToDataMapIter = m_objTypeToDataMap.find(obj->getType());
        if (typeToDataMapIter == m_objTypeToDataMap.end()) {
            m_objTypeToDataMap[obj->getType()].reserve(archive->getVector().size());
            std::vector<std::pair<hid_t, std::string>> &typeToDataEntry = m_objTypeToDataMap[obj->getType()];

            // build object data needed for mapping
            for (unsigned i = 0; i < archive->getVector().size(); i++) {
                if (archive->getVector()[i].referenceType == ReferenceType::ObjectReference &&
                    archive->getVector()[i].referenceName != FindObjectReferenceOArchive::nullObjectReferenceName) {
                    typeToDataEntry.push_back(
                        std::make_pair(HDF5Const::performantReferenceTypeId, archive->getVector()[i].referenceName));

                } else if (archive->getVector()[i].referenceType == ReferenceType::ShmVector) {
                    ShmVectorInfoGetter infoGetter(archive->getVector()[i].referenceName);
                    boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorInfoGetter>(infoGetter));

                    typeToDataEntry.push_back(std::make_pair(infoGetter.type, archive->getVector()[i].referenceName));
                }
            }
        }

        // queue references
        for (unsigned i = 0; i < archive->getVector().size(); i++) {
            if (archive->getVector()[i].referenceType == ReferenceType::ObjectReference &&
                archive->getVector()[i].referenceName != FindObjectReferenceOArchive::nullObjectReferenceName) {
                Object::const_ptr refObj = vistle::Shm::the().getObjectFromName(archive->getVector()[i].referenceName);

                // setting port number to max uint will order references at the end of all ports
                compute_performant_addObjectToWrite(refObj, HDF5Const::performantReferenceNullVal);
            }
        }
    }

    m_objContainerVector.push_back(container);
}

// COMPUTE HELPER FUNCTION - ORGANIZED - PREPARES AN OBJECT FOR WRITING
// * all objects will be placed into the index write queue unless they are references
// * all objects will be placed into the object write queue unless they have already been written
//-------------------------------------------------------------------------
void WriteHDF5::compute_organized_addObjectToWrite(
    vistle::Object::const_ptr obj, unsigned originPortNumber, unsigned &numNodeObjects, unsigned &numNodeArrays,
    std::vector<vistle::Object::const_ptr> &objPtrVector,
    std::vector<vistle::FindObjectReferenceOArchive> &objRefArchiveVector,
    std::vector<MetaToArrayArchive> &metaToArrayArchiveVector, std::vector<ObjectWriteInfo> &objectWriteVector,
    std::vector<IndexWriteInfo> &indexWriteVector)
{
    bool isReference = (originPortNumber == std::numeric_limits<unsigned>::max());
    unsigned objWriteIndex = numNodeObjects;

    // insert in index if not a reference
    if (!isReference) {
        indexWriteVector.push_back(
            IndexWriteInfo(obj->getName(), obj->getBlock(), obj->getTimestep(), originPortNumber));
    }

    // check if duplicate, do not process if so
    if (m_objectSet.find(obj->getName()) != m_objectSet.end()) {
        return;
    }
    m_objectSet.insert(obj->getName());

    numNodeObjects++;
    objPtrVector.push_back(obj);

    // serialize object into archive
    objRefArchiveVector.push_back(FindObjectReferenceOArchive());
    obj->save(objRefArchiveVector.back());

    // build meta array
    metaToArrayArchiveVector.push_back(MetaToArrayArchive(obj->getType()));
    boost::serialization::serialize_adl(metaToArrayArchiveVector.back(), const_cast<Meta &>(obj->meta()),
                                        ::boost::serialization::version<Object>::value);

    // fill in reservation info
    objectWriteVector.push_back(ObjectWriteInfo(obj->getName()));

    // populate reservationInfo shm vector
    for (unsigned i = 0; i < objRefArchiveVector[objWriteIndex].getVector().size(); i++) {
        if (objRefArchiveVector[objWriteIndex].getVector()[i].referenceType == ReferenceType::ShmVector) {
            ShmVectorReserver reserver(objRefArchiveVector[objWriteIndex].getVector()[i].referenceName,
                                       objRefArchiveVector[objWriteIndex].getVector()[i].nvpName,
                                       &objectWriteVector[objWriteIndex]);
            boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorReserver>(reserver));
            numNodeArrays++;

        } else if (objRefArchiveVector[objWriteIndex].getVector()[i].referenceType == ReferenceType::ObjectReference) {
            if (objRefArchiveVector[objWriteIndex].getVector()[i].referenceName !=
                FindObjectReferenceOArchive::nullObjectReferenceName) {
                objectWriteVector[objWriteIndex].referenceVector.push_back(
                    ObjectWriteInfoReferenceEntry(objRefArchiveVector[objWriteIndex].getVector()[i].referenceName,
                                                  objRefArchiveVector[objWriteIndex].getVector()[i].nvpName));

                // add referenced object to write list if it has not already been written
                Object::const_ptr refObj = vistle::Shm::the().getObjectFromName(
                    objRefArchiveVector[objWriteIndex].getVector()[i].referenceName);

                if (refObj) {
                    compute_organized_addObjectToWrite(refObj, std::numeric_limits<unsigned>::max(), numNodeObjects,
                                                       numNodeArrays, objPtrVector, objRefArchiveVector,
                                                       metaToArrayArchiveVector, objectWriteVector, indexWriteVector);
                } else {
                    m_unresolvedReferencesExist = true;
                }
            }
        }
    }
}


// PREPARE UTILITY HELPER FUNCTION - CHECK FOR VALID INPUT FILENAME
//-------------------------------------------------------------------------
bool WriteHDF5::prepare_fileNameCheck()
{
    std::string fileName = m_fileName->getValue();
    filesystem::path path(fileName);
    bool isDirectory = false;
    bool doesExist = false;

    // check for valid filename size
    if (m_fileName->getValue().size() == 0) {
        return false;
    }

    // setup boost::filesystem
    try {
        isDirectory = filesystem::is_directory(path);
        doesExist = filesystem::exists(path);

    } catch (const filesystem::filesystem_error &error) {
        std::cerr << "filesystem error: " << error.what() << std::endl;
        return false;
    }


    // make sure name is not a directory
    if (isDirectory) {
        if (m_isRootNode) {
            sendInfo("File name cannot be a directory");
        }
        return false;
    }

    // output warning for existing file
    if (doesExist) {
        if (m_overwrite->getValue()) {
            if (H5Fis_hdf5(m_fileName->getValue().c_str()) > 0) {
                if (m_isRootNode) {
                    sendInfo("A HDF5 file already exists: Overwriting");
                }
            } else {
                if (m_isRootNode) {
                    sendInfo("A non-HDF5 file already exists with this name: Overwriting");
                }
            }
            return true;

        } else {
            if (m_isRootNode) {
                sendInfo("A file with this name already exists: Not overwriting");
            }
            return false;
        }
    }

    return true;
}

// GENERIC UTILITY HELPER FUNCTION - WRITE DATA TO HDF5 ABSTRACTION
// * simplification of function call to be used for nodes which do not have objects
// * forces dummy object write within other function call
//-------------------------------------------------------------------------
void WriteHDF5::util_HDF5WriteOrganized(hid_t fileId)
{
    util_HDF5WriteOrganized(false, "", nullptr, fileId, nullptr, 0);
}

// GENERIC UTILITY HELPER FUNCTION - WRITE DATA TO HDF5 ABSTRACTION - ORGANIZED
// * isWriter determines if data will actually be written, otherwise a dummy object is written to
//-------------------------------------------------------------------------
void WriteHDF5::util_HDF5WriteOrganized(bool isWriter, std::string name, const void *data, hid_t fileId, hsize_t *dims,
                                        hid_t dataType)
{
    herr_t status;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t memSpaceId;
    hid_t writeId;

    // assign dummy object values if isWriter is false
    const std::string writeName = isWriter ? name : HDF5Const::DummyObject::name;
    const hid_t writeType = isWriter ? dataType : HDF5Const::DummyObject::type;
    const hsize_t *writeDims = isWriter ? dims : HDF5Const::DummyObject::dims.data();
    const void *writeData = isWriter ? data : nullptr;


    // set up parallel write
    writeId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);


    // allocate data spaces
    dataSetId = H5Dopen(fileId, writeName.c_str(), H5P_DEFAULT);
    fileSpaceId = H5Dget_space(dataSetId);
    memSpaceId = H5Screate_simple(1, writeDims, NULL);

    // cancel write if not a writer
    if (!isWriter) {
        H5Sselect_none(fileSpaceId);
        H5Sselect_none(memSpaceId);
    }

    // write
    status = H5Dwrite(dataSetId, writeType, memSpaceId, fileSpaceId, writeId, writeData);
    util_checkStatus(status);

    // release resources
    H5Sclose(fileSpaceId);
    H5Sclose(memSpaceId);
    H5Dclose(dataSetId);
    H5Pclose(writeId);

    return;
}


// GENERIC UTILITY HELPER FUNCTION - VERIFY HERR_T STATUS
//-------------------------------------------------------------------------
void WriteHDF5::util_checkStatus(herr_t status)
{
    if (status != 0) {
        //        sendInfo("Error: status: %i", status);
    }

    return;
}


void WriteHDF5::util_checkId(hid_t id, const std::string &info) const
{
    if (id < 0) {
        sendInfo("Error: %s", info.c_str());
    }
}
