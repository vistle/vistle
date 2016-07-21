//-------------------------------------------------------------------------
// WRITE HDF5 CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------


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
#include <core/shmvectoroarchive.h>
#include <core/placeholder.h>

#include "hdf5.h"

#include "WriteHDF5.h"

BOOST_SERIALIZATION_REGISTER_ARCHIVE(ShmVectorOArchive)

using namespace vistle;

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(WriteHDF5)


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

   // policies
   setReducePolicy(message::ReducePolicy::OverAll);
   setSchedulingPolicy(message::SchedulingPolicy::LazyGang);

   // variable setup
   m_isRootNode = (comm().rank() == 0);
   m_numPorts = 1;

   // obtain meta member count
   PlaceHolder::ptr tempObject(new PlaceHolder("", Meta(), Object::Type::UNKNOWN));
   MemberCounterArchive memberCounter;
   boost::serialization::serialize_adl(memberCounter, const_cast<Meta &>(tempObject->meta()), ::boost::serialization::version< Meta >::value);
   numMetaMembers = memberCounter.getCount();
}

// DESTRUCTOR
//-------------------------------------------------------------------------
WriteHDF5::~WriteHDF5() {

}

// PARAMETER CHANGED FUNCTION
//-------------------------------------------------------------------------
bool WriteHDF5::parameterChanged(const Parameter * p) {

    return true;
}

// PREPARE FUNCTION
// * creates basic groups within the HDF5 file and instantiates the dummy object used
// * by nodes with null data for writing collectively
//-------------------------------------------------------------------------
bool WriteHDF5::prepare() {
    herr_t status;
    hid_t fileId;
    hid_t filePropertyListId;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hid_t groupId;
    hsize_t dims[] = {1};

    // clear reused variables
    m_arrayMap.clear();
    m_objectSet.clear();


    // Set up file access property list with parallel I/O access
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);

    if (H5Fis_hdf5(m_fileName->getValue().c_str()) > 0 && m_isRootNode) {
        sendInfo("File already exists: Overwriting");
    }

    // create new file
    fileId = H5Fcreate(m_fileName->getValue().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, filePropertyListId);

    // create basic vistle HDF5 Groups: index
    groupId = H5Gcreate2(fileId, "/file", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: data
    groupId = H5Gcreate2(fileId, "/object", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: data
    groupId = H5Gcreate2(fileId, "/index", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: grid
    groupId = H5Gcreate2(fileId, "/array", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(groupId);
    util_checkStatus(status);


    // create dummy object
    fileSpaceId = H5Screate_simple(1, m_dummyObjectDims.data(), NULL);
    dataSetId = H5Dcreate(fileId, m_dummyObjectName.c_str(), m_dummyObjectType, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);

    // store number of ports
    hsize_t oneDims[] = {1};
    const std::string numPortsName("/file/numPorts");
    fileSpaceId = H5Screate_simple(1, oneDims, NULL);
    dataSetId = H5Dcreate(fileId, numPortsName.c_str(), H5T_NATIVE_UINT, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
    util_HDF5write(m_isRootNode, numPortsName.c_str(), &m_numPorts, fileId, oneDims, H5T_NATIVE_UINT);

    // create folders for ports
    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string name = "/index/" + std::to_string(i);
        groupId = H5Gcreate2(fileId, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Gclose(groupId);
        util_checkStatus(status);
    }

    // initialize first tracker layer
    m_indexTracker = IndexTracker(m_numPorts);


    // close all open h5 entities
    H5Pclose(filePropertyListId);
    H5Fclose(fileId);


    return Module::prepare();
}


// REDUCE FUNCTION
//-------------------------------------------------------------------------
bool WriteHDF5::reduce(int timestep) {


    return Module::reduce(timestep);
}

// COMPUTE FUNCTION
// * function procedure:
// * - serailized incoming object data into a ShmArchive
// * - transmits information regarding object data sizes to all nodes
// * - nodes reserve space for all objects independently
// * - nodes write data into the HDF5 file collectively. If a object is
// *   not present on the node, null data is written to a dummy object so that
// *   the call is still made collectively
//-------------------------------------------------------------------------
bool WriteHDF5::compute() {
    Object::const_ptr obj = nullptr;
    ShmVectorOArchive archive;
    m_hasObject = false;
    unsigned originPortNumber = 0;


    // ----- OBTAIN FIRST OBJECT FROM A PORT ----- //

    for (/*defined above*/; originPortNumber < m_numPorts; originPortNumber++) {
        std::string portName = "data" + std::to_string(originPortNumber) + "_in";

        // acquire input data object
        obj = expect<Object>(portName);

        // save if available
        if (obj) {
            m_hasObject = true;
            obj->save(archive);

            break;
        }
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
            ShmVectorReserver reserver(archive.getVector()[i].arrayName, archive.getVector()[i].nvpName, &reservationInfo);
            boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorReserver>(reserver));
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
    hid_t fileId;
    hid_t filePropertyListId;

    hid_t groupId;
    hid_t dataSetId;
    hid_t fileSpaceId;
    hsize_t metaDims[] = {WriteHDF5::numMetaMembers - MetaToArrayArchive::numExclusiveMembers};
    hsize_t oneDims[] = {1};
    hsize_t dims[] = {0};
    double * metaData = nullptr;
    int * typeData = nullptr;
    std::string writeName;
    int typeValue;

    bool isNewObject = false;

    unsigned maxVectorSize;

    // Set up file access property list with parallel I/O access and open
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);
    fileId = H5Fopen(m_fileName->getValue().c_str(), H5F_ACC_RDWR, filePropertyListId);

    // reserve space for object
    for (unsigned i = 0; i < reservationInfoGatherVector.size(); i++) {
        if (reservationInfoGatherVector[i].isValid) {
            std::string objectGroupName = "/object/" + reservationInfoGatherVector[i].name;

            // ----- create object group ----- //
            if (m_objectSet.find(reservationInfoGatherVector[i].name) == m_objectSet.end()) {
                m_objectSet.insert(reservationInfoGatherVector[i].name);
                isNewObject = true;

                groupId = H5Gcreate2(fileId, objectGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

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

                // handle shmVectors and shmVector links
                for (unsigned j = 0; j < reservationInfoGatherVector[i].shmVectors.size(); j++) {
                    std::string arrayName = reservationInfoGatherVector[i].shmVectors[j].name;
                    std::string arrayPath = "/array/" + arrayName;

                    if (m_arrayMap.find(arrayName) == m_arrayMap.end()) {
                        m_arrayMap.insert({{arrayName, false}});

                        // create array dataset
                        dims[0] = reservationInfoGatherVector[i].shmVectors[j].size;
                        fileSpaceId = H5Screate_simple(1, dims, NULL);
                        dataSetId = H5Dcreate(fileId, arrayPath.c_str(), reservationInfoGatherVector[i].shmVectors[j].type, fileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                        H5Sclose(fileSpaceId);
                        H5Dclose(dataSetId);
                    }

                    std::string linkGroupName = objectGroupName + "/" + reservationInfoGatherVector[i].shmVectors[j].nvpTag;
                    groupId = H5Gcreate2(fileId, linkGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

                    status = H5Lcreate_soft(arrayPath.c_str(), groupId, arrayName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
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

                groupId = H5Gcreate2(fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Gclose(groupId);
                util_checkStatus(status);
            }



            // create block group
            groupName += "/" + std::to_string(currBlock);
            if (m_indexTracker[currOrigin][currTimestep].find(currBlock) == m_indexTracker[currOrigin][currTimestep].end()) {
                m_indexTracker[currOrigin][currTimestep][currBlock] = 0;

                groupId = H5Gcreate2(fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Gclose(groupId);
                util_checkStatus(status);
            }

            // create variants group
            groupName += "/" + std::to_string(m_indexTracker[currOrigin][currOrigin][currBlock]);
            groupId = H5Gcreate2(fileId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            m_indexTracker[currOrigin][currTimestep][currBlock]++;


            // link index entry to object
            status = H5Lcreate_soft(objectGroupName.c_str(), groupId, reservationInfoGatherVector[i].name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
            util_checkStatus(status);


            // close group
            status = H5Gclose(groupId);
            util_checkStatus(status);
        }
    }


    // ----- WRITE DATA TO THE HDF5 FILE ----- //


    // write meta info
    if (m_hasObject && isNewObject) {
        writeName = "/object/" + obj->getName() + "/meta";
        metaData = metaToArrayArchive.getDataPtr();
    }

    util_HDF5write(m_hasObject, writeName, metaData, fileId, metaDims, H5T_NATIVE_DOUBLE);


    // write type info
    if (m_hasObject && isNewObject) {
        writeName = "/object/" + obj->getName() + "/type";
        typeValue = obj->getType();
        typeData = &typeValue;
    }

    util_HDF5write(m_hasObject, writeName, typeData, fileId, oneDims, H5T_NATIVE_INT);



    // reduce vector size information for collective calls
    boost::mpi::all_reduce(comm(), (unsigned) archive.getVector().size(), maxVectorSize, boost::mpi::maximum<unsigned>());

    // write array info
    for (unsigned i = 0; i < maxVectorSize; i++) {
        if (archive.getVector().size() > i && m_arrayMap[archive.getVector()[i].arrayName] == false) {
            // this node has an object to be written
            m_arrayMap[archive.getVector()[i].arrayName] = true;

            ShmVectorWriter shmVectorWriter(archive.getVector()[i].arrayName, fileId, comm());
            boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ShmVectorWriter>(shmVectorWriter));
        } else {
             // this node does not have an object to be written
             ShmVectorWriter shmVectorWriter(fileId, comm());
             shmVectorWriter.writeDummy();

        }

    }


    // close all open h5 entities
    H5Pclose(filePropertyListId);
    H5Fclose(fileId);

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
    const std::string writeName = isWriter ? name : m_dummyObjectName;
    const hid_t writeType = isWriter ? dataType : m_dummyObjectType;
    const hsize_t * writeDims = isWriter ? dims : m_dummyObjectDims.data();
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
}

// GENERIC UTILITY HELPER FUNCTION - VERIFY HERR_T STATUS
//-------------------------------------------------------------------------
void WriteHDF5::util_checkStatus(herr_t status) {

    if (status != 0) {
//        sendInfo("Error: status: %i", status);
    }

    return;
}
