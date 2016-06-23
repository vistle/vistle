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

#include <core/message.h>
#include <core/vec.h>
#include <core/unstr.h>
#include <core/pointeroarchive.h>

#include "hdf5.h"

#include "WriteHDF5.h"

BOOST_SERIALIZATION_REGISTER_ARCHIVE(PointerOArchive)

using namespace vistle;

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(WriteHDF5)


//-------------------------------------------------------------------------
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// CONSTRUCTOR
//-------------------------------------------------------------------------
WriteHDF5::WriteHDF5(const std::string &shmname, const std::string &name, int moduleID)
   : Module("WriteHDF5", shmname, name, moduleID) {

   // create ports
   createInputPort("data0_in");

   // add module parameters
   m_fileName = addStringParameter("File Name", "Name of File that will be written to", "");

   // policies
   setReducePolicy(message::ReducePolicy::OverAll);

   // variable setup
   m_isRootNode = (comm().rank() == M_ROOT_NODE);
   m_numPorts = 1;

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
//-------------------------------------------------------------------------
bool WriteHDF5::prepare() {
    herr_t status;

    // Set up file access property list with parallel I/O access
    m_filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(m_filePropertyListId, comm(), MPI_INFO_NULL);

    if (H5Fis_hdf5(m_fileName->getValue().c_str()) > 0 && m_isRootNode) {
        sendInfo("File already exists: Overwriting");
    }

    // create new file
    m_fileId = H5Fcreate(m_fileName->getValue().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, m_filePropertyListId);


    // create basic vistle HDF5 Groups: index
    m_groupId_index = H5Gcreate2(m_fileId, "/index", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(m_groupId_index);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: data
    m_groupId_data = H5Gcreate2(m_fileId, "/data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);


//-------------------------------------------------------------------------
    hid_t currentDataSet;
    hid_t currentDataSpace;
    hid_t currentPropertyListId;
    // Create the dataspace for the dataset.
    hsize_t dataDimensions[] = { 5 };
    currentDataSpace = H5Screate_simple(1, dataDimensions, NULL);

    int * d;
    d = (int *) malloc(sizeof(int)*5);
    d[0] = 1;
    d[1] = 3;
    d[2] = 1;
    d[3] = 6;
    d[4] = 1;

    // Create the dataset with default properties.
    currentDataSet = H5Dcreate(m_groupId_data, std::to_string(comm().rank()).c_str(), H5T_NATIVE_INT, currentDataSpace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    // Create property list for an independent dataset write.
    currentPropertyListId = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(currentPropertyListId, H5FD_MPIO_COLLECTIVE);

    // write data
    status = H5Dwrite(currentDataSet, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, currentPropertyListId,  d);

    //close/release respources
    H5Dclose(currentDataSet);
    H5Sclose(currentDataSpace);
    H5Pclose(currentPropertyListId);

//-------------------------------------------------------------------------

    status = H5Gclose(m_groupId_data);
    util_checkStatus(status);

    // create basic vistle HDF5 Groups: grid
    m_groupId_grid = H5Gcreate2(m_fileId, "/grid", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose(m_groupId_grid);
    util_checkStatus(status);


    return Module::prepare();
}


// REDUCE FUNCTION
//-------------------------------------------------------------------------
bool WriteHDF5::reduce(int timestep) {

    // close file property list identifier
    H5Pclose(m_filePropertyListId);

    // close file
    H5Fclose(m_fileId);

    return Module::reduce(timestep);
}

// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool WriteHDF5::compute() {

    for (unsigned i = 0; i < m_numPorts; i++) {
        std::string portName = "data" + std::to_string(i) + "_in";
        std::stringstream ss;
        PointerOArchive archive(ss);

        // acquire input data object
        Object::const_ptr obj = expect<Object>(portName);
        //obj->save(archive);
        obj->getArrays(archive);

        //archive << obj.get();//*UnstructuredGrid::as(data);

        //data->save(archive);

        // debug output
        if (comm().rank() == 0) {
            debug_printArchive(archive);
        }

        //sendInfo(ss.str());



//        compute_store(archive, obj);


    }

   return true;
}

// COMPUTE HELPER FUNCTION - STORE DATA OBJECT INTO HDF5 LIBRARY
//-------------------------------------------------------------------------
void WriteHDF5::compute_store(PointerOArchive & archive, Object::const_ptr data) {
    hid_t currentGroupId;
    hid_t currentDataSet;
    hid_t currentDataSpace;
    hid_t currentPropertyListId;
    herr_t status;

    // open data writing location group
    m_groupId_data = H5Gopen1(m_fileId, "/data");

//    std::string newGroupBlock = data->getBlock() >= 0 ? std::to_string(data->getBlock()) : "0";
//    std::string newGroupTimestep = data->getTimestep() >= 0 ? std::to_string(data->getTimestep()) : "0";
//    std::string newGroupName = "/data/block_" + newGroupBlock + "_" + newGroupTimestep;
//    currentGroupId = H5Gcreate2(m_fileId, newGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);


//    for (unsigned i = 0; i < archive.vector().size(); i++) {

//        // obtain variable type
//        auto nativeTypeMapIterator = m_nativeTypeMap.find(archive.vector()[i].typeInfo);

//        // abort if unidentified
//        if (nativeTypeMapIterator == m_nativeTypeMap.end()) {
//            continue;
//        }

//        sendInfo("%i writing %s", comm().rank(), archive.vector()[i].name.c_str());


//        // Create the dataspace for the dataset.
//        hsize_t dataDimensions[] = { archive.vector()[i].size };
//        currentDataSpace = H5Screate_simple(1, dataDimensions, NULL);

//        // Create the dataset with default properties.
//        currentDataSet = H5Dcreate(m_groupId_data, archive.vector()[i].name.c_str(), nativeTypeMapIterator->second, currentDataSpace,
//                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

//        // Create property list for an independent dataset write.
//        currentPropertyListId = H5Pcreate(H5P_DATASET_XFER);
//        H5Pset_dxpl_mpio(currentPropertyListId, H5FD_MPIO_INDEPENDENT);

//        // write data
//        status = H5Dwrite(currentDataSet, nativeTypeMapIterator->second, H5S_ALL, H5S_ALL, currentPropertyListId, archive.vector()[i].value);

//        //close/release respources
//        H5Dclose(currentDataSet);
//        H5Sclose(currentDataSpace);
//        H5Pclose(currentPropertyListId);

//    }
////-------------------------------------------------------------------------

//        // Create the dataspace for the dataset.
//        hsize_t dataDimensions[] = { 5 };
//        currentDataSpace = H5Screate_simple(1, dataDimensions, NULL);

//        int * d;
//        d = (int *) malloc(sizeof(int)*5);
//        d[0] = 1;
//        d[1] = 3;
//        d[2] = 1;
//        d[3] = 6;
//        d[4] = 1;

//        // Create the dataset with default properties.
//        currentDataSet = H5Dcreate(m_groupId_data, std::to_string(comm().rank()).c_str(), H5T_NATIVE_INT, currentDataSpace,
//                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

//        // Create property list for an independent dataset write.
//        currentPropertyListId = H5Pcreate(H5P_DATASET_XFER);
//        H5Pset_dxpl_mpio(currentPropertyListId, H5FD_MPIO_INDEPENDENT);

//        // write data
//        status = H5Dwrite(currentDataSet, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, currentPropertyListId, d);

//        //close/release respources
//        H5Dclose(currentDataSet);
//        H5Sclose(currentDataSpace);
//        H5Pclose(currentPropertyListId);

////-------------------------------------------------------------------------

//    // close data writing location group
//    status = H5Gclose(currentGroupId);
//    util_checkStatus(status);

    status = H5Gclose(m_groupId_data);
    util_checkStatus(status);


    return;
}

// DEBUG UTILITY HELPER FUNCTION - ENUMERATE ARCHIVE CONTENTS
//-------------------------------------------------------------------------
void WriteHDF5::debug_printArchive(PointerOArchive & archive) {
    sendInfo("vistle object archive found: %u, enum: %u, primitive: %u only: %u shm: %u\n",
             archive.nvpCount, archive.enumCount, archive.primitiveCount, archive.onlyCount, archive.shmCount);

    std::string message;

    for (unsigned i = 0; i < archive.vector().size(); i++) {
        message += archive.vector()[i].name + " ";

        if (archive.vector()[i].isPointer) {
            message += "->";
        }

        auto nativeTypeMapIterator = m_nativeTypeMap.find(archive.vector()[i].typeInfo);

        if (nativeTypeMapIterator != m_nativeTypeMap.end()) {
            message += "primitive";
        } else {
            message += "unknown data type: skipping...";
        }

        if (archive.vector()[i].size > 0) {
            message += "[" + std::to_string(archive.vector()[i].size) + "]";
        }


        sendInfo("%s", message.c_str());
        message.clear();
    }
}

// GENERIC UTILITY HELPER FUNCTION - VERIFY HERR_T STATUS
//-------------------------------------------------------------------------
void WriteHDF5::util_checkStatus(herr_t status) {

    if (status != 0) {
        sendInfo("Error: status: %i", status);
    }

    return;
}
