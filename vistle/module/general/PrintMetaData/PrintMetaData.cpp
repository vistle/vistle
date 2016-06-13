//-------------------------------------------------------------------------
// PRINT METADATA CPP
// * Prints Meta-Data  about the input object to the Vistle console
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------


#include <sstream>
#include <iomanip>
#include <cfloat>
#include <limits>
#include <string>
#include <vector>

#include <core/unstr.h>
#include <core/points.h>
#include <core/normals.h>
#include <core/texture1d.h>
#include <core/spheres.h>
#include <core/tubes.h>
#include <core/triangles.h>
#include <core/polygons.h>
#include <core/message.h>
#include <core/index.h>

#include <boost/mpi/collectives.hpp>

#include "PrintMetaData.h"

using namespace vistle;

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(PrintMetaData)


//-------------------------------------------------------------------------
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// CONSTRUCTOR
//-------------------------------------------------------------------------
PrintMetaData::PrintMetaData(const std::string &shmname, const std::string &name, int moduleID)
   : Module("PrintMetaData", shmname, name, moduleID) {

   // create ports
   createInputPort("data_in");

   // add module parameters
   // none for now

   // policies
   setReducePolicy(message::ReducePolicy::OverAll);
   setDefaultCacheMode(ObjectCache::CacheDeleteLate);

}

// DESTRUCTOR
//-------------------------------------------------------------------------
PrintMetaData::~PrintMetaData() {

}

// PREPARE FUNCTION
//-------------------------------------------------------------------------
bool PrintMetaData::prepare() {
    m_numCurrElements = 0;
    m_numCurrVertices = 0;
    m_elCurrTypeVector.clear();
    m_isFirstComputeCall = true;

    return Module::prepare();
}

// REDUCE FUNCTION
//-------------------------------------------------------------------------
bool PrintMetaData::reduce(int timestep) {

    // reduce necessary data across all node instances of the module
    if (comm().rank() == ROOT_NODE) {
        boost::mpi::reduce(comm(), m_numCurrElements, m_numTotalElements, std::plus<Index>(), ROOT_NODE);
        boost::mpi::reduce(comm(), m_numCurrVertices, m_numTotalVertices, std::plus<Index>(), ROOT_NODE);
    } else {
        boost::mpi::reduce(comm(), m_numCurrElements, std::plus<Index>(), ROOT_NODE);
        boost::mpi::reduce(comm(), m_numCurrVertices, std::plus<Index>(), ROOT_NODE);
    }


    unsigned maxTypeVectorSize;
    boost::mpi::all_reduce(comm(), (unsigned) m_elCurrTypeVector.size(), maxTypeVectorSize, boost::mpi::maximum<unsigned>());

    m_elTotalTypeVector = std::vector<unsigned>(maxTypeVectorSize, 0);
    m_elCurrTypeVector.resize(maxTypeVectorSize, 0);
    for (unsigned i = 0; i < maxTypeVectorSize; i++) {
        if (comm().rank() == ROOT_NODE) {
            boost::mpi::reduce(comm(), m_elCurrTypeVector[i], m_elTotalTypeVector[i], std::plus<unsigned>(), ROOT_NODE);
        } else {
            boost::mpi::reduce(comm(), m_elCurrTypeVector[i], std::plus<unsigned>(), ROOT_NODE);
        }
    }


    // print finalized data on root node
    if (comm().rank() == ROOT_NODE) {
        reduce_printData();
    }

    return Module::reduce(timestep);
}

// COMPUTE HELPER FUNCTION - ACQUIRE GENERIC DATA
//-------------------------------------------------------------------------
void PrintMetaData::compute_acquireGenericData(vistle::DataBase::const_ptr data) {
    m_dataType = Object::toString(data->getType());

    m_creator = data->meta().creator();
    m_executionCounter = data->meta().executionCounter();
    m_iterationCounter = data->meta().iteration();
    m_numAnimationSteps = data->meta().numAnimationSteps();
    m_numBlocks = data->meta().numBlocks();
    m_numTimesteps = data->meta().numTimesteps();
    m_realTime = data->meta().realTime();

    m_attributesVector = data->getAttributeList();
}

// COMPUTE HELPER FUNCTION - ACQUIRE GRID DATA
//-------------------------------------------------------------------------
void PrintMetaData::compute_acquireGridData(vistle::UnstructuredGrid::const_ptr dataGrid) {
    m_numCurrVertices += dataGrid->getNumVertices();
    m_numCurrElements += dataGrid->getNumElements();

    // harvest cell type data
    const Index * elPtr = dataGrid->el() + 1;
    for (unsigned int i = 0; i < dataGrid->getNumElements(); i++) {
        unsigned numCellVertices = *elPtr - *(elPtr - 1);

        // resize if needed (in case of large polygons)
        if (numCellVertices > m_elCurrTypeVector.size()) {
            m_elCurrTypeVector.resize(numCellVertices + 1, 0);
        }

        m_elCurrTypeVector[numCellVertices]++;
        elPtr++;
    }
}

// REDUCE HELPER FUNCTION - PRINT DATA
//-------------------------------------------------------------------------
void PrintMetaData::reduce_printData() {
     std::string message = "\n-----------------------------------------------------";

     // print generic data
     message += "\n\nType: " + m_dataType;

     message += "\n\nObjectData:";
     message += "\n   Creator: " + std::to_string(m_creator);
     message += "\n   Execution Counter: " + std::to_string(m_executionCounter);
     message += "\n   Iteration: " + std::to_string(m_iterationCounter);
     message += "\n   Number of Animation Steps: " + std::to_string(m_numAnimationSteps);
     message += "\n   Number of Blocks: " + std::to_string(m_numBlocks);
     message += "\n   Number of Time Steps: " + std::to_string(m_numTimesteps);
     message += "\n   Real Time: " + std::to_string(m_realTime);

     message += "\n\nAttributes: ";
     for (unsigned i = 0; i < m_attributesVector.size(); i++) {
         message += "\n   " + std::to_string(i) + ": " + m_attributesVector[i];
     }

     // print grid data
     if (m_isGridAttatched) {
         message += "\n\nGrid Data: ";
         message += "\n   Number of Vertices: " + std::to_string(m_numTotalVertices);
         message += "\n   Number of Cells: " + std::to_string(m_numTotalElements);
         message += "\n   Cell Types: ";

         for (unsigned i = 0; i < m_elTotalTypeVector.size(); i++) {
             if (m_elTotalTypeVector[i] != 0) {
                 message += "\n      " + std::to_string(i) + " Vertices: " + std::to_string(m_elTotalTypeVector[i]);
             }
         }

     } else {
         message += "\n\n No grid is overlayed with this data";
     }

     // print message that has been built
     sendInfo("%s", message.c_str());
}


// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool PrintMetaData::compute() {

    // acquire input data object
    DataBase::const_ptr data = expect<DataBase>("data_in");
    UnstructuredGrid::const_ptr dataGrid = UnstructuredGrid::as(data);

    // check if dataGrid has been obtained properly
    // if not, try interpreting data->grid
    if (!dataGrid) {
       dataGrid = UnstructuredGrid::as(data->grid());
    }

    // assert existence of useable data <- not sure if this will ever get triggered because of the expect function
    if (!data) {
       sendInfo("Error: Unknown Input Data");
       return true;
    }

    // record wether dataGrid is available
    m_isGridAttatched = (bool) dataGrid;

    // obtain generic data on first compute call of module
    // this data does not change between blocks, and only needs to be processed once
    if (m_isFirstComputeCall && comm().rank() == ROOT_NODE) {
        compute_acquireGenericData(data);
        m_isFirstComputeCall = false;
    }

    // acquire grid specific data articles
    if (m_isGridAttatched) {
        compute_acquireGridData(dataGrid);
    }

   return true;
}
