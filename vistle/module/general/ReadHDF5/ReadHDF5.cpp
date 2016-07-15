//-------------------------------------------------------------------------
// READ HDF5 CPP
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
#include <core/object.h>
#include <core/object_impl.h>

#include "hdf5.h"

#include "ReadHDF5.h"

BOOST_SERIALIZATION_REGISTER_ARCHIVE(ShmVectorOArchive)

using namespace vistle;

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(ReadHDF5)


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
bool ReadHDF5::parameterChanged() {
    util_checkFile();
    return true;
}

// PREPARE FUNCTION
//-------------------------------------------------------------------------
bool ReadHDF5::prepare() {
    herr_t status;
    hid_t fileId;
    hid_t filePropertyListId;
    hid_t groupId;
    hid_t readId;

    bool groupExists;

    // check file validity before beginning
    if (!util_checkFile()) {
        return true;
    }


    // create file access property list id
    filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(filePropertyListId, comm(), MPI_INFO_NULL);

    // open file
    fileId = H5Fopen(m_fileName->getValue().c_str(), H5P_DEFAULT, filePropertyListId);


    // o_c = origin counter
    // t_c = timestep counter
    // b_c = block counter
    // v_c = variants counter
    // timesteps and blocks can have values of -1;
    for (unsigned o_c = 0; o_c < m_numPorts; o_c++) {
        groupExists = true;
        for (long int t_c = -1; groupExists || t_c <= 0; t_c++) {
            groupExists = true;
            for (long int b_c = -1; groupExists || b_c <= 0; b_c++) {
                groupExists = true;
                for (unsigned v_c = 0; groupExists; v_c++) {
                    std::string indexName = "/index/"
                            + std::to_string(o_c) + "/"
                            + std::to_string(t_c) + "/"
                            + std::to_string(b_c) + "/"
                            + std::to_string(v_c);

                    groupId = H5Gopen2(fileId, indexName.c_str(), H5P_DEFAULT);

                    // check dataset Validity
                    if (groupId < 0) {
                        // skip rest of read if invalid
                        if (m_isRootNode)
                            sendInfo("not valid: %s", indexName.c_str());
                        groupExists = false;
                        break;
                    }

                    // the group exists
                    groupExists = true;



                    LinkIterData linkIterData(this, fileId, o_c, b_c, t_c);
                    status = H5Literate (groupId, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, prepare_processObject, &linkIterData);
                    util_checkStatus(status);

                    status = H5Gclose(groupId);
                    util_checkStatus(status);
                }
            }
        }
    }


    // sync nodes for collective reads
//    unsigned totalReadingNodes;
//    unsigned isReadingNode = 0;
//    boost::mpi::all_reduce(comm(), isReadingNode, totalReadingNodes, std::plus<unsigned>());

//    while (totalReadingNodes != 0) {

//    }


    // close all open h5 entities
    H5Pclose(filePropertyListId);
    H5Fclose(fileId);

    return Module::prepare();
}

// PREPARE UTILITY FUNCTION - PROCESSES AN OBJECT
// * function falls under the template  H5L_iterate_t as it is a callback from the HDF5 iterate API call
//-------------------------------------------------------------------------
herr_t ReadHDF5::prepare_processObject(hid_t callingGroupId, const char * name, const H5L_info_t * info, void * opData) {
    LinkIterData * linkIterData = (LinkIterData *) opData;
    std::string objectGroup = "/object/" + std::string(name);

    herr_t status;
    hid_t groupId;
    hid_t fileId;
    hid_t dataSetId;
    hid_t readId;

    int objectType;
    double objectMeta[ReadHDF5::numMetaMembers - ArrayToMetaArchive::numExclusiveMembers];
    //ShmVectorOArchive archive;

    // sync nodes for collective read calls
//    unsigned totalReadingNodes;
//    unsigned isReadingNode = 1;
//    boost::mpi::all_reduce(linkIterData->callingModule->comm(), isReadingNode, totalReadingNodes, std::plus<unsigned>());


    // open object group
    groupId = H5Gopen2(linkIterData->fileId, objectGroup.c_str(), H5P_DEFAULT);

    // obtain port number
    readId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(readId, H5FD_MPIO_COLLECTIVE);

    // read type
    dataSetId = H5Dopen2(groupId, "type", H5P_DEFAULT);
    status = H5Dread(dataSetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, readId, &objectType);
    H5Dclose(dataSetId);

    // read meta
    dataSetId = H5Dopen2(groupId, "meta", H5P_DEFAULT);
    status = H5Dread(dataSetId, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, readId, &objectMeta[0]);
    H5Dclose(dataSetId);


    if (linkIterData->callingModule->m_isRootNode) {
        linkIterData->callingModule->sendInfo("iterated: %s, found type: %d", name, objectType);
    }


    Object::ptr returnObject = ObjectTypeRegistry::getType(objectType).createEmpty();

    // construct meta
    ArrayToMetaArchive arrayToMetaArchive(&objectMeta[0], linkIterData->block, linkIterData->timestep);
    boost::serialization::serialize_adl(arrayToMetaArchive, const_cast<Meta &>(returnObject->meta()), ::boost::serialization::version< Meta >::value);


    //returnObject->save(archive);


    // close all open h5 entities
    H5Pclose(readId);


    status = H5Gclose(groupId);
    linkIterData->callingModule->util_checkStatus(status);

    // output object
    linkIterData->callingModule->addObject(std::string("data" + std::to_string(linkIterData->origin) + "_out"), returnObject);

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

// GENERIC UTILITY HELPER FUNCTION - VERIFY HERR_T STATUS
//-------------------------------------------------------------------------
void ReadHDF5::util_checkStatus(herr_t status) {

    if (status != 0) {
        sendInfo("Error: status: %i", status);
    }

    return;
}
