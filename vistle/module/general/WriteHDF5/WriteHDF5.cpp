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

#include <core/message.h>
#include <core/vec.h>
#include <core/unstr.h>
#include <core/findobjectreferenceoarchive.h>
#include <core/placeholder.h>

#include "hdf5.h"

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
const int WriteHDF5::s_writeVersion = 0;
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
   m_fileName = addStringParameter("File Name", "Name of File that will be written to", "");
   m_portDescriptions.push_back(addStringParameter("Port 0 Description", "Descrition will appear as tooltip on read", ""));

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
    std::string newPortDescriptionName = "Port " + std::to_string(m_numPorts) + " Description";

    if (from->getName() == lastPortName || to->getName() == lastPortName) {
        createInputPort(newPortName);

        m_portDescriptions.push_back(addStringParameter(newPortDescriptionName.c_str(), "Descrition will appear as tooltip on read", ""));

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

    // error check incoming filename
    if (!prepare_fileNameCheck()) {
        m_doNotWrite = true;
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

    // create basic vistle HDF5 Groups: index
    groupId = H5Gcreate2(m_fileId, "/file", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: data
    groupId = H5Gcreate2(m_fileId, "/object", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: data
    groupId = H5Gcreate2(m_fileId, "/index", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: grid
    groupId = H5Gcreate2(m_fileId, "/array", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create dummy object
    fileSpaceId = H5Screate_simple(1, HDF5Const::DummyObject::dims.data(), NULL);
    dataSetId = H5Dcreate(m_fileId, HDF5Const::DummyObject::name.c_str(), HDF5Const::DummyObject::type, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);

    // create version number
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId = H5Dcreate(m_fileId, "/file/version", H5T_NATIVE_INT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);

    // store number of ports
    const std::string numPortsName("/file/numPorts");
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId = H5Dcreate(m_fileId, numPortsName.c_str(), H5T_NATIVE_UINT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5write(m_isRootNode, numPortsName.c_str(), &m_numPorts, m_fileId, oneDims, H5T_NATIVE_UINT);

    // create folders for ports in index
    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string name = "/index/" + std::to_string(i);
        groupId = H5Gcreate2(m_fileId, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Gclose(groupId);
        util_checkStatus(status);
    }

    // initialize first tracker layer
    m_indexTracker = IndexTracker(m_numPorts);


    // save port descriptions
    groupId = H5Gcreate2(m_fileId, "/file/ports", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
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

#ifndef HIDE_REFERENCE_WARNINGS
    bool unresolvedReferencesExistOnNode = false;

    for (unsigned i = 0; i < m_objectReferenceVector.size(); i++) {
        if (m_objectSet.find(m_objectReferenceVector[i]) == m_objectSet.end()
                && m_objectReferenceVector[i] != FindObjectReferenceOArchive::nullObjectReferenceName) {
            unresolvedReferencesExistOnNode = true;
            break;
        }
    }

    // check and send warning message for unresolved references
    if (m_isRootNode) {
        bool unresolvedReferencesExistInComm;

        boost::mpi::reduce(comm(), unresolvedReferencesExistOnNode, unresolvedReferencesExistInComm, boost::mpi::maximum<bool>(), 0);

        if (unresolvedReferencesExistInComm) {
            sendInfo("Warning: some object references are unresolved.");
        }
    } else {
        boost::mpi::reduce(comm(), unresolvedReferencesExistOnNode, boost::mpi::maximum<bool>(), 0);
    }
#endif

    return Module::reduce(timestep);
}

// COMPUTE FUNCTION
// * calls iterates through ports and calls compute_writePort for each of them
//-------------------------------------------------------------------------
bool WriteHDF5::compute() {

    if (m_doNotWrite) {
        return true;
    }

    for (unsigned originPortNumber = 0; originPortNumber < m_numPorts; originPortNumber++) {
        compute_writeForPort(originPortNumber);
    }

    return true;
}

// COMPUTE UTILITY FUNCTION - WRITE PORT DATA
// * function procedure:
// * - serailized incoming object data into a ShmArchive
// * - transmits information regarding object data sizes to all nodes
// * - nodes reserve space for all objects independently
// * - nodes write data into the HDF5 file collectively. If a object is
// *   not present on the node, null data is written to a dummy object so that
// *   the call is still made collectively
//-------------------------------------------------------------------------
void WriteHDF5::compute_writeForPort(unsigned originPortNumber) {
    Object::const_ptr obj = nullptr;
    FindObjectReferenceOArchive archive;
    m_hasObject = false;


    // ----- OBTAIN OBJECT FROM A PORT ----- //
    std::string portName = "data" + std::to_string(originPortNumber) + "_in";

    // acquire input data object
    obj = expect<Object>(portName);

    // save if available
    if (obj) {
        m_hasObject = true;
        obj->save(archive);
    }

    // check if all nodes have no object on port
    bool doesAtLeastOnePortHaveObject;
    boost::mpi::all_reduce(comm(), m_hasObject, doesAtLeastOnePortHaveObject, boost::mpi::maximum<bool>());

    // skip port if so
    if (!doesAtLeastOnePortHaveObject) {
        return;
    }



    // ----- TRANSMIT OBJECT DATA DIMENSIONS TO ALL NODES FOR RESERVATION OF FILE DATASPACE ----- //

    std::vector<ReservationInfo> reservationInfoGatherVector;
    ReservationInfo reservationInfo;
    MetaToArrayArchive metaToArrayArchive;

    if (m_hasObject) {

        // obtain object metadata info
        boost::serialization::serialize_adl(metaToArrayArchive, const_cast<Meta &>(obj->meta()), ::boost::serialization::version< Object >::value);

        // fill i reservation info
        reservationInfo.isValid = true;
        reservationInfo.name = obj->getName();
        reservationInfo.block = obj->getBlock();
        reservationInfo.timestep = obj->getTimestep();
        reservationInfo.origin = originPortNumber;

        // populate reservationInfo shm vector
        for (unsigned i = 0; i < archive.getVector().size(); i++) {
            if (archive.getVector()[i].referenceType == ReferenceType::ShmVector) {
                ShmVectorReserver reserver(archive.getVector()[i].referenceName, archive.getVector()[i].nvpName, &reservationInfo);
                boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorReserver>(reserver));

            } else if (archive.getVector()[i].referenceType == ReferenceType::ObjectReference) {
                if (archive.getVector()[i].referenceName != FindObjectReferenceOArchive::nullObjectReferenceName)
                    reservationInfo.referenceVector.push_back(ReservationInfoReferenceEntry(archive.getVector()[i].referenceName, archive.getVector()[i].nvpName));
            }
        }

    } else {
        reservationInfo.isValid = false;
    }

    // transmit all reservation information
    boost::mpi::all_gather(comm(), reservationInfo, reservationInfoGatherVector);

    // sort for proper collective io ordering
    std::sort(reservationInfoGatherVector.begin(), reservationInfoGatherVector.end());

    // ----- RESERVE SPACE WITHIN THE HDF5 FILE ----- //

    // open file
    herr_t status;
    hid_t groupId;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hsize_t metaDims[] = {WriteHDF5::s_numMetaMembers - HDF5Const::numExclusiveMetaMembers};
    hsize_t oneDims[] = {1};
    hsize_t dims[] = {0};
    double * metaData = nullptr;
    int * typeData = nullptr;
    std::string writeName;
    int typeValue;

    bool isNewObject = false;

    unsigned maxVectorSize;


    // reserve space for object
    for (unsigned i = 0; i < reservationInfoGatherVector.size(); i++) {
        if (reservationInfoGatherVector[i].isValid) {
            std::string objectGroupName = "/object/" + reservationInfoGatherVector[i].name;

            // ----- create object group ----- //
            if (m_objectSet.find(reservationInfoGatherVector[i].name) == m_objectSet.end()) {
                m_objectSet.insert(reservationInfoGatherVector[i].name);
                isNewObject = true;

                groupId = H5Gcreate2(m_fileId, objectGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

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
                for (unsigned j = 0; j < reservationInfoGatherVector[i].referenceVector.size(); j++) {
                    std::string referenceName = reservationInfoGatherVector[i].referenceVector[j].name;;
                    std::string referencePath;

                    if (reservationInfoGatherVector[i].referenceVector[j].referenceType == ReferenceType::ShmVector) {
                        referencePath = "/array/" + referenceName;

                        if (m_arrayMap.find(referenceName) == m_arrayMap.end()) {
                            m_arrayMap.insert({{referenceName, false}});

                            // create array dataset
                            dims[0] = reservationInfoGatherVector[i].referenceVector[j].size;
                            fileSpaceId = H5Screate_simple(1, dims, NULL);
                            dataSetId = H5Dcreate(m_fileId, referencePath.c_str(), reservationInfoGatherVector[i].referenceVector[j].type, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                            H5Sclose(fileSpaceId);
                            H5Dclose(dataSetId);
                        }
                    } else if (reservationInfoGatherVector[i].referenceVector[j].referenceType == ReferenceType::ObjectReference) {
                        referencePath = "/object/" + referenceName;
                    }

                    std::string linkGroupName = objectGroupName + "/" + reservationInfoGatherVector[i].referenceVector[j].nvpTag;
                    groupId = H5Gcreate2(m_fileId, linkGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

                    status = H5Lcreate_soft(referencePath.c_str(), groupId, referenceName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
                    util_checkStatus(status);

                    status = H5Gclose(groupId);
                    util_checkStatus(status);
                }
            }




            // ----- create index entry for object ----- //
            unsigned currOrigin = reservationInfoGatherVector[i].origin;
            int currTimestep = reservationInfoGatherVector[i].timestep;
            int currBlock = reservationInfoGatherVector[i].block;


            // create timestep group
            std::string groupName = "/index/" + std::to_string(currOrigin) + "/" + std::to_string(currTimestep);
            if (m_indexTracker[currOrigin].find(currTimestep) == m_indexTracker[currOrigin].end()) {
                m_indexTracker[currOrigin][currTimestep];

                groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Gclose(groupId);
                util_checkStatus(status);
            }



            // create block group
            groupName += "/" + std::to_string(currBlock);
            if (m_indexTracker[currOrigin][currTimestep].find(currBlock) == m_indexTracker[currOrigin][currTimestep].end()) {
                m_indexTracker[currOrigin][currTimestep][currBlock] = 0;

                groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Gclose(groupId);
                util_checkStatus(status);
            }

            // create variants group
            groupName += "/" + std::to_string(m_indexTracker[currOrigin][currOrigin][currBlock]);
            groupId = H5Gcreate2(m_fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            m_indexTracker[currOrigin][currTimestep][currBlock]++;


            // link index entry to object
            status = H5Lcreate_soft(objectGroupName.c_str(), groupId, reservationInfoGatherVector[i].name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
            util_checkStatus(status);


            // close group
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }
    }

    // record object references for reference validity check in reduce function
    for (unsigned i = 0; i < archive.getVector().size(); i++) {
        if (archive.getVector()[i].referenceType == ReferenceType::ObjectReference) {
            m_objectReferenceVector.push_back(archive.getVector()[i].referenceName);
        }
    }


    // ----- WRITE DATA TO THE HDF5 FILE ----- //


    // write meta info
    if (isNewObject) {
        if (m_hasObject) {
            writeName = "/object/" + obj->getName() + "/meta";
            metaData = metaToArrayArchive.getDataPtr();
        }

        util_HDF5write(m_hasObject, writeName, metaData, m_fileId, metaDims, H5T_NATIVE_DOUBLE);


        // write type info
        if (m_hasObject) {
            writeName = "/object/" + obj->getName() + "/type";
            typeValue = obj->getType();
            typeData = &typeValue;
        }

        util_HDF5write(m_hasObject, writeName, typeData, m_fileId, oneDims, H5T_NATIVE_INT);

    }

    // reduce vector size information for collective calls
    boost::mpi::all_reduce(comm(), (unsigned) archive.getVector().size(), maxVectorSize, boost::mpi::maximum<unsigned>());

    // write array info
    for (unsigned i = 0; i < maxVectorSize; i++) {
        if (archive.getVector().size() > i
                && archive.getVector()[i].referenceType == ReferenceType::ShmVector
                && m_arrayMap[archive.getVector()[i].referenceName] == false) {
            // this node has an object to be written
            m_arrayMap[archive.getVector()[i].referenceName] = true;

            ShmVectorWriter shmVectorWriter(archive.getVector()[i].referenceName, m_fileId, comm());
            boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorWriter>(shmVectorWriter));
        } else {
             // this node does not have an object to be written
             ShmVectorWriter shmVectorWriter(m_fileId, comm());
             shmVectorWriter.writeDummy();

        }

    }

   return;
}

// PREPARE UTILITY HELPER FUNCTION - CHECK FOR VALID INPUT FILENAME
//-------------------------------------------------------------------------
bool WriteHDF5::prepare_fileNameCheck() {
    std::string fileName = m_fileName->getValue();
    boost::filesystem::path path(fileName);
    bool isDirectory;
    bool doesExist;

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
        if (H5Fis_hdf5(m_fileName->getValue().c_str()) > 0) {
            if (m_isRootNode) {
                sendInfo("HDF5 file already exists: Overwriting");
            }
            return true;
        } else {
            if (m_isRootNode) {
                sendInfo("A non-HDF5 file already exists with this name: write aborted");
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
