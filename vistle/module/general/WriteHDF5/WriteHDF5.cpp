//-------------------------------------------------------------------------
// WRITE HDF5 CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#define NO_PORT_REMOVAL
#define HIDE_REFERENCE_WARNINGS

#include <sstream>
#include <iomanip>
#include <cfloat>
#include <limits>
#include <string>
#include <vector>
#include <ctime>

#include <hdf5.h>

#include <core/findobjectreferenceoarchive.h>
#include <core/placeholder.h>

#include "WriteHDF5.h"

using namespace vistle;

BOOST_SERIALIZATION_REGISTER_ARCHIVE(FindObjectReferenceOArchive)

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(WriteHDF5)

//-------------------------------------------------------------------------
// WRITE HDF5 STATIC MEMBER OUT OF CLASS INITIALIZATION
//-------------------------------------------------------------------------
unsigned WriteHDF5::s_numMetaMembers = 0;
const std::unordered_map<std::type_index, hid_t> WriteHDF5::s_nativeTypeMap = {
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
// * intitializes data and records the number of members in the meta object
//-------------------------------------------------------------------------
WriteHDF5::WriteHDF5(const std::string &shmname, const std::string &name, int moduleID)
   : Module("WriteHDF5", shmname, name, moduleID) {

   // create ports
   createInputPort("data0_in");

   // add module parameters
   m_fileName = addStringParameter("file_name", "Name of File that will be written to", "");
   m_overwrite = addIntParameter("overwrite", "write even if output file exists", 0, Parameter::Boolean);
   m_portDescriptions.push_back(addStringParameter("port_description_0", "Description will appear as tooltip on read", "port 0"));

   // policies
   setReducePolicy(message::ReducePolicy::OverAll);
   setSchedulingPolicy(message::SchedulingPolicy::LazyGang);

   // variable setup
   m_isRootNode = (comm().rank() == 0);
   m_numPorts = 1;

   // obtain meta member count
   PlaceHolder::ptr tempObject(new PlaceHolder("", Meta(), Object::Type::UNKNOWN));
   MemberCounterArchive memberCounter(&m_metaNvpTags);
   boost::serialization::serialize_adl(memberCounter, const_cast<Meta &>(tempObject->meta()), ::boost::serialization::version< Meta >::value);
   s_numMetaMembers = memberCounter.getCount();
}

// DESTRUCTOR
//-------------------------------------------------------------------------
WriteHDF5::~WriteHDF5() {

}

// CONNECTION REMOVED FUNCTION
//-------------------------------------------------------------------------
void WriteHDF5::connectionRemoved(const Port *from, const Port *to) {

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
void WriteHDF5::connectionAdded(const Port *from, const Port *to) {
    std::string lastPortName = "data" + std::to_string(m_numPorts - 1) + "_in";
    std::string newPortName = "data" + std::to_string(m_numPorts) + "_in";
    std::string newPortDescriptionName = "port_description_" + std::to_string(m_numPorts - 1);
    std::string newPortDescription = "port " + std::to_string(m_numPorts - 1);

    if (from->getName() == lastPortName || to->getName() == lastPortName) {
        createInputPort(newPortName);

        if (m_numPorts != 1) {
            m_portDescriptions.push_back(addStringParameter(newPortDescriptionName.c_str(), "Description will appear as tooltip on read", newPortDescription));
        }

        m_numPorts++;
    }

    return Module::connectionAdded(from, to);
}

// PREPARE FUNCTION
// * - error checks incoming filename
// * - creates basic groups within the HDF5 file and instantiates the dummy object used
// *   by nodes with null data for writing collectively
// * - writes auxiliary file metadata
//-------------------------------------------------------------------------
bool WriteHDF5::prepare() {
    herr_t status;
    hid_t filePropertyListId;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t groupId;
    hid_t writeId;
    hsize_t oneDims[] = {1};

    m_doNotWrite = false;
    m_unresolvedReferencesExist = false;
    m_numPorts -=1; // ignore extra port at end - it will never contain a connection

    // error check incoming filename
    if (!prepare_fileNameCheck()) {
        m_doNotWrite = true;
        m_doNotWrite = boost::mpi::all_reduce(comm(), m_doNotWrite, std::logical_and<bool>());
        return Module::prepare();
    }

    // clear reused variables
    m_arrayMap.clear();
    m_objectSet.clear();

    // Set up file access property list with parallel I/O access
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);

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
    dataSetId = H5Dcreate(m_fileId, HDF5Const::DummyObject::name.c_str(), HDF5Const::DummyObject::type, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);

    // create version number
    const std::string versionPath("/file/version");
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId = H5Dcreate(m_fileId, versionPath.c_str(), H5T_NATIVE_INT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5write(m_isRootNode, versionPath.c_str(), &HDF5Const::versionNumber, m_fileId, oneDims, H5T_NATIVE_INT);

    // store number of ports
    const std::string numPortsPath("/file/numPorts");
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId = H5Dcreate(m_fileId, numPortsPath.c_str(), H5T_NATIVE_UINT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5write(m_isRootNode, numPortsPath.c_str(), &m_numPorts, m_fileId, oneDims, H5T_NATIVE_UINT);

    // create folders for ports in index
    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string name = "/index/p" + std::to_string(i);
        groupId = H5Gcreate2(m_fileId, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        util_checkId(groupId, "create group "+name);
        status = H5Gclose(groupId);
        util_checkStatus(status);
    }

    // initialize first tracker layer
    m_indexTracker = IndexTracker(m_numPorts);


    // save port descriptions
    groupId = H5Gcreate2(m_fileId, "/file/ports", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /file/ports");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string name = "/file/ports/" + std::to_string(i);
        std::string description = m_portDescriptions[i]->getValue();
        hsize_t dims[1];

        // make sure description isnt empty
        if (description.size() == 0) {
            description = "Port " + std::to_string(i);
        }

        dims[0] = description.size();

        // store description
        writeId = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);

        fileSpaceId = H5Screate_simple(1, dims, NULL);
        dataSetId = H5Dcreate2(m_fileId, name.c_str(), H5T_NATIVE_CHAR, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        status = H5Dwrite(dataSetId, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, writeId, description.data());
        util_checkStatus(status);

        H5Dclose(dataSetId);
        H5Sclose(fileSpaceId);
        H5Pclose(writeId);
    }

    // write meta nvp tags
    groupId = H5Gcreate2(m_fileId, "/file/metaTags", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    util_checkId(groupId, "create group /file/metaTags");
    status = H5Gclose(groupId);
    util_checkStatus(status);

    for (unsigned i = 0; i < m_metaNvpTags.size(); i++) {
        std::string name = "/file/metaTags/" + std::to_string(i);
        hsize_t dims[] = {m_metaNvpTags[i].size()};

        // store description
        writeId = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(writeId, H5FD_MPIO_COLLECTIVE);

        fileSpaceId = H5Screate_simple(1, dims, NULL);
        dataSetId = H5Dcreate2(m_fileId, name.c_str(), H5T_NATIVE_CHAR, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        status = H5Dwrite(dataSetId, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, writeId, m_metaNvpTags[i].data());
        util_checkStatus(status);

        H5Dclose(dataSetId);
        H5Sclose(fileSpaceId);
        H5Pclose(writeId);
    }

    return Module::prepare();
}


// REDUCE FUNCTION
// * check for unresolved reference
//-------------------------------------------------------------------------
bool WriteHDF5::reduce(int timestep) {

    // close hdf5 file
    H5Fclose(m_fileId);

    //restore portnum value
    m_numPorts += 1;


#ifndef HIDE_REFERENCE_WARNINGS

    // check and send warning message for unresolved references
    if (m_isRootNode) {
        bool unresolvedReferencesExistInComm;

        boost::mpi::reduce(comm(), m_unresolvedReferencesExist, unresolvedReferencesExistInComm, boost::mpi::maximum<bool>(), 0);

        if (unresolvedReferencesExistInComm) {
            sendInfo("Warning: some object references are unresolved.");
        }
    } else {
        boost::mpi::reduce(comm(), m_unresolvedReferencesExist, boost::mpi::maximum<bool>(), 0);
    }
#endif

    return Module::reduce(timestep);
}

// COMPUTE
// * function procedure:
// * - serailized incoming object data into a ShmArchive
// * - transmits information regarding object data sizes to all nodes
// * - nodes reserve space for all objects independently
// * - nodes write data into the HDF5 file collectively. If a object is
// *   not present on the node, null data is written to a dummy object so that
// *   the call is still made collectively
//-------------------------------------------------------------------------
bool WriteHDF5::compute() {
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
            compute_addObjectToWrite(obj, originPortNumber, numNodeObjects, numNodeArrays, objPtrVector,
                                     objRefArchiveVector, metaToArrayArchiveVector, objectWriteVector, indexWriteVector);
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
    hsize_t metaDims[] = {WriteHDF5::s_numMetaMembers};
    hsize_t oneDims[] = {1};
    hsize_t dims[] = {0};
    double * metaData = nullptr;
    int typeValue;
    int * typeData = nullptr;
    std::string writeName;


    // reserve space for object
    for (unsigned i = 0; i < objectWriteGatherVector.size(); i++) {
        std::string objectGroupName = "/object/" + objectWriteGatherVector[i].name;

        groupId = H5Gcreate2(m_fileId, objectGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        util_checkId(groupId, "create group "+objectGroupName);

        // create metadata dataset
        fileSpaceId = H5Screate_simple(1, metaDims, NULL);
        dataSetId = H5Dcreate(groupId, "meta", H5T_NATIVE_DOUBLE, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(fileSpaceId);
        H5Dclose(dataSetId);

        // create type dataset
        fileSpaceId = H5Screate_simple(1, oneDims, NULL);
        dataSetId = H5Dcreate(groupId, "type", H5T_NATIVE_INT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(fileSpaceId);
        H5Dclose(dataSetId);

        // close group
        status = H5Gclose(groupId);
        util_checkStatus(status);

        // handle referenceVector and shmVector links
        for (unsigned j = 0; j < objectWriteGatherVector[i].referenceVector.size(); j++) {
            std::string referenceName = objectWriteGatherVector[i].referenceVector[j].name;;
            std::string referencePath;

            if (objectWriteGatherVector[i].referenceVector[j].referenceType == ReferenceType::ShmVector) {
                referencePath = "/array/" + referenceName;

                if (m_arrayMap.find(referenceName) == m_arrayMap.end()) {
                    m_arrayMap.insert({{referenceName, false}});

                    // create array dataset
                    dims[0] = objectWriteGatherVector[i].referenceVector[j].size;
                    fileSpaceId = H5Screate_simple(1, dims, NULL);
                    dataSetId = H5Dcreate(m_fileId, referencePath.c_str(), objectWriteGatherVector[i].referenceVector[j].type, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    H5Sclose(fileSpaceId);
                    H5Dclose(dataSetId);
                }
            } else if (objectWriteGatherVector[i].referenceVector[j].referenceType == ReferenceType::ObjectReference) {
                referencePath = "/object/" + referenceName;
            }

            std::string linkGroupName = objectGroupName + "/" + objectWriteGatherVector[i].referenceVector[j].nvpTag;
            groupId = H5Gcreate2(m_fileId, linkGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group "+linkGroupName);

            status = H5Lcreate_soft(referencePath.c_str(), groupId, referenceName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
            util_checkStatus(status);

            status = H5Gclose(groupId);
            util_checkStatus(status);
        }

    }

    // reserve space for index and write
    for (unsigned i = 0; i < indexWriteGatherVector.size(); i++) {
        std::string objectGroupName = "/object/" + objectWriteGatherVector[i].name;
        unsigned currOrigin = indexWriteGatherVector[i].origin;
        int currTimestep = indexWriteGatherVector[i].timestep;
        int currBlock = indexWriteGatherVector[i].block;


        // create timestep group
        std::string groupName = "/index/p" + std::to_string(currOrigin) + "/t" + std::to_string(currTimestep);
        if (m_indexTracker[currOrigin].find(currTimestep) == m_indexTracker[currOrigin].end()) {
            m_indexTracker[currOrigin][currTimestep];

            groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group "+groupName);
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }



        // create block group
        groupName += "/b" + std::to_string(currBlock);
        if (m_indexTracker[currOrigin][currTimestep].find(currBlock) == m_indexTracker[currOrigin][currTimestep].end()) {
            m_indexTracker[currOrigin][currTimestep][currBlock] = 0;

            groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            util_checkId(groupId, "create group "+groupName);
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }

        // create variants group
        groupName += "/o" + std::to_string(m_indexTracker[currOrigin][currTimestep][currBlock]);
        groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        util_checkId(groupId, "create group "+groupName);
        m_indexTracker[currOrigin][currTimestep][currBlock]++;


        // link index entry to object
        status = H5Lcreate_soft(objectGroupName.c_str(), groupId, indexWriteGatherVector[i].name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
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

        util_HDF5write(nodeHasObject, writeName, metaData, m_fileId, metaDims, H5T_NATIVE_DOUBLE);

        // write type info
        if (nodeHasObject) {
            writeName = "/object/" + objectWriteVector[i].name + "/type";
            typeValue = objPtrVector[i]->getType();
            typeData = &typeValue;
        }

        util_HDF5write(nodeHasObject, writeName, typeData, m_fileId, oneDims, H5T_NATIVE_INT);
    }

    // reduce vector size information for collective calls
    unsigned maxNumCommArrays;
    boost::mpi::all_reduce(comm(), numNodeArrays, maxNumCommArrays, boost::mpi::maximum<unsigned>());

    unsigned numArraysWritten = 0;

    // write neccessary arrays
    for (unsigned i = 0; i < objRefArchiveVector.size(); i++) {
        for (unsigned j = 0; j < objRefArchiveVector[i].getVector().size(); j++) {
            if (objRefArchiveVector[i].getVector()[j].referenceType == ReferenceType::ShmVector
                    && m_arrayMap[objRefArchiveVector[i].getVector()[j].referenceName] == false) {
                // this is an array to be written
                m_arrayMap[objRefArchiveVector[i].getVector()[j].referenceName] = true;

                ShmVectorWriter shmVectorWriter(objRefArchiveVector[i].getVector()[j].referenceName, m_fileId, comm());
                boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorWriter>(shmVectorWriter));

                numArraysWritten++;
            }
        }
    }

    // pad the rest of the collective calls with dummy writes
    while (numArraysWritten < maxNumCommArrays) {
        ShmVectorWriter shmVectorWriter(m_fileId, comm());
        shmVectorWriter.writeDummy();
        numArraysWritten++;
    }

   return true;
}

// COMPUTE HELPER FUNCTION - PREPARES AN OBJECT FOR WRITING
// * all objects will be placed into the index write queue unless they are references
// * all objects will be placed into the object write queue unless they have already been written
//-------------------------------------------------------------------------
void WriteHDF5::compute_addObjectToWrite(vistle::Object::const_ptr obj, unsigned originPortNumber, unsigned &numNodeObjects, unsigned &numNodeArrays,
                              std::vector<vistle::Object::const_ptr> &objPtrVector,
                              std::vector<vistle::FindObjectReferenceOArchive> &objRefArchiveVector,
                              std::vector<MetaToArrayArchive> &metaToArrayArchiveVector,
                              std::vector<ObjectWriteInfo> &objectWriteVector,
                              std::vector<IndexWriteInfo> &indexWriteVector) {
    bool isReference = (originPortNumber == std::numeric_limits<unsigned>::max());
    unsigned objWriteIndex = numNodeObjects;

    // insert in index if not a reference
    if (!isReference) {
        indexWriteVector.push_back(IndexWriteInfo(obj->getName(), obj->getBlock(), obj->getTimestep(), originPortNumber));
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
    metaToArrayArchiveVector.push_back(MetaToArrayArchive());
    boost::serialization::serialize_adl(metaToArrayArchiveVector.back(), const_cast<Meta &>(obj->meta()), ::boost::serialization::version< Object >::value);

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
            if (objRefArchiveVector[objWriteIndex].getVector()[i].referenceName != FindObjectReferenceOArchive::nullObjectReferenceName) {
                objectWriteVector[objWriteIndex].referenceVector.push_back(ObjectWriteInfoReferenceEntry(objRefArchiveVector[objWriteIndex].getVector()[i].referenceName,
                                                                                                 objRefArchiveVector[objWriteIndex].getVector()[i].nvpName));

                // add referenced object to write list if it has not already been written
                Object::const_ptr refObj = vistle::Shm::the().getObjectFromName(objRefArchiveVector[objWriteIndex].getVector()[i].referenceName);

                if (refObj) {
                    compute_addObjectToWrite(refObj, std::numeric_limits<unsigned>::max(), numNodeObjects, numNodeArrays, objPtrVector,
                                             objRefArchiveVector, metaToArrayArchiveVector, objectWriteVector, indexWriteVector);
                } else {
                    m_unresolvedReferencesExist = true;
                }
            }
        }
    }

}

// PREPARE UTILITY HELPER FUNCTION - CHECK FOR VALID INPUT FILENAME
//-------------------------------------------------------------------------
bool WriteHDF5::prepare_fileNameCheck() {
    std::string fileName = m_fileName->getValue();
    boost::filesystem::path path(fileName);
    bool isDirectory = false;
    bool doesExist = false;

    // check for valid filename size
    if (m_fileName->getValue().size() == 0) {
        return false;
    }

    // setup boost::filesystem
    try {
        isDirectory = boost::filesystem::is_directory(path);
        doesExist = boost::filesystem::exists(path);

    } catch (const boost::filesystem::filesystem_error &error) {
        std::cerr << "filesystem error: " << error.what() << std::endl;
        return false;
    }


    // make sure name isnt a directory
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
void WriteHDF5::util_HDF5write(hid_t fileId) {
       util_HDF5write(false, "", nullptr, fileId, nullptr, 0);
   }

// GENERIC UTILITY HELPER FUNCTION - WRITE DATA TO HDF5 ABSTRACTION
// * isWriter determines if data will actually be written, otherwise a dummy object is written to
//-------------------------------------------------------------------------
void WriteHDF5::util_HDF5write(bool isWriter, std::string name, const void * data, hid_t fileId, hsize_t * dims, hid_t dataType) {
    herr_t status;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t memSpaceId;
    hid_t writeId;

    // assign dummy object values if isWriter is false
    const std::string writeName = isWriter ? name : HDF5Const::DummyObject::name;
    const hid_t writeType = isWriter ? dataType : HDF5Const::DummyObject::type;
    const hsize_t * writeDims = isWriter ? dims : HDF5Const::DummyObject::dims.data();
    const void * writeData = isWriter ? data : nullptr;


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
void WriteHDF5::util_checkStatus(herr_t status) {

    if (status != 0) {
//        sendInfo("Error: status: %i", status);
    }

    return;
}


void WriteHDF5::util_checkId(hid_t id, const std::string &info) const {

    if (id < 0) {
        sendInfo("Error: %s", info.c_str());
    }
}
