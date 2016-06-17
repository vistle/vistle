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
#include <core/vec_impl.h>
#include <core/unstr.h>

#include "hdf5.h"

#include "WriteHDF5.h"
#include "vistleObject_oarchive.h"

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

    // Set up file access property list with parallel I/O access
    m_filePropertyListId = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(m_filePropertyListId, comm(), MPI_INFO_NULL);

    if (H5Fis_hdf5(m_fileName->getValue().c_str()) > 0 && m_isRootNode) {
        sendInfo("File already exists: Overwriting");
    }

    // create new file
    m_fileId = H5Fcreate(m_fileName->getValue().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, m_filePropertyListId);


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
        vistleObject_oarchive archive;

        // acquire input data object
        DataBase::const_ptr data = expect<DataBase>(portName);

        archive << *data;

        sendInfo("found %u", archive.getVector().size());

    }

   return true;
}

// COMPUTE HELPER FUNCTION - STORE DATA OBJECT INTO HDF5 LIBRARY
//-------------------------------------------------------------------------
void WriteHDF5::compute_store(vistle::UnstructuredGrid::const_ptr dataGrid, vistle::DataBase::const_ptr data) {

}


