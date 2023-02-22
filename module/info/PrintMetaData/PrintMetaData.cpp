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

#include <vistle/util/hostname.h>

#include <vistle/core/unstr.h>
#include <vistle/core/points.h>
#include <vistle/core/normals.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/spheres.h>
#include <vistle/core/tubes.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/message.h>
#include <vistle/core/index.h>

#include <boost/mpi/collectives.hpp>
#include <boost/serialization/vector.hpp>

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
PrintMetaData::PrintMetaData(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    // create ports
    createInputPort("data_in", "data");

    // add module parameters
    m_param_doPrintTotals = addIntParameter(
        "Print_Totals", "Print the Totals of incoming metadata (i.e. number of: blocks, cells, vertices, etc..)", 1,
        Parameter::Boolean);
    m_param_doPrintMinMax =
        addIntParameter("print_extrema", "Print max/min rank wise values of incoming data", 1, Parameter::Boolean);
    m_param_doPrintMPIInfo = addIntParameter("print_mpi_info", "Print each node MPI rank", 0, Parameter::Boolean);
    m_param_doPrintVerbose =
        addIntParameter("print_verbose", "Prints all data elements on each node", 0, Parameter::Boolean);


    // policies
    setReducePolicy(message::ReducePolicy::OverAll);

    // additional operations
    m_isRootNode = rank() == M_ROOT_NODE;
    util_printMPIInfo("ctor:");
}

// DESTRUCTOR
//-------------------------------------------------------------------------
PrintMetaData::~PrintMetaData()
{}

// PREPARE FUNCTION
//-------------------------------------------------------------------------
bool PrintMetaData::prepare()
{
    m_currentProfile = ObjectProfile();
    m_TotalsProfile = ObjectProfile();
    m_minProfile = ObjectProfile();
    m_maxProfile = ObjectProfile();

    m_isFirstComputeCall = true;
    m_numParsedTimesteps = 0;

    return Module::prepare();
}

// REDUCE FUNCTION
//-------------------------------------------------------------------------
bool PrintMetaData::reduce(int timestep)
{
    // reduce necessary data across all node instances of the module
    if (m_isRootNode) {
        boost::mpi::reduce(comm(), m_currentProfile, m_minProfile, ProfileMinimum<ObjectProfile>(), M_ROOT_NODE);
        boost::mpi::reduce(comm(), m_currentProfile, m_maxProfile, ProfileMaximum<ObjectProfile>(), M_ROOT_NODE);
        boost::mpi::reduce(comm(), m_currentProfile, m_TotalsProfile, std::plus<ObjectProfile>(), M_ROOT_NODE);
    } else {
        boost::mpi::reduce(comm(), m_currentProfile, ProfileMinimum<ObjectProfile>(), M_ROOT_NODE);
        boost::mpi::reduce(comm(), m_currentProfile, ProfileMaximum<ObjectProfile>(), M_ROOT_NODE);
        boost::mpi::reduce(comm(), m_currentProfile, std::plus<ObjectProfile>(), M_ROOT_NODE);
    }

    // reduce type information
    unsigned maxTypeVectorSize;
    boost::mpi::all_reduce(comm(), (unsigned)m_currentProfile.types.size(), maxTypeVectorSize,
                           boost::mpi::maximum<unsigned>());

    m_TotalsProfile.types = std::vector<unsigned>(maxTypeVectorSize, 0);
    m_currentProfile.types.resize(maxTypeVectorSize, 0);
    for (unsigned i = 0; i < maxTypeVectorSize; i++) {
        if (m_isRootNode) {
            boost::mpi::reduce(comm(), m_currentProfile.types[i], m_TotalsProfile.types[i], std::plus<unsigned>(),
                               M_ROOT_NODE);
        } else {
            boost::mpi::reduce(comm(), m_currentProfile.types[i], std::plus<unsigned>(), M_ROOT_NODE);
        }
    }

    // print finalized data and min/max info on root node
    if (m_isRootNode) {
        reduce_printData();
    }

    // print mpi info
    if (m_param_doPrintMPIInfo->getValue()) {
        util_printMPIInfo("reduce:");
    }

    return Module::reduce(timestep);
}

// COMPUTE HELPER FUNCTION - ACQUIRE GENERIC DATA
//-------------------------------------------------------------------------
void PrintMetaData::compute_acquireGenericData(vistle::Object::const_ptr data)
{
    m_dataType = Object::toString(data->getType());

    m_creator = data->meta().creator();
    m_executionCounter = data->meta().executionCounter();
    m_iterationCounter = data->meta().iteration();
    m_numAnimationSteps = data->meta().numAnimationSteps();
    m_numBlocks = data->meta().numBlocks();
    m_numParsedTimesteps =
        (data->getTimestep() + 1 > m_numParsedTimesteps) ? data->getTimestep() + 1 : m_numParsedTimesteps;
    m_numTotalTimesteps = data->meta().numTimesteps();
    m_realTime = data->meta().realTime();
    m_transform = data->meta().transform();

    m_attributesVector = data->getAttributeList();
}

// COMPUTE HELPER FUNCTION - ACQUIRE GRID DATA
// * the if statements in this functions are organized based on which object profile variables they modify
//-------------------------------------------------------------------------
void PrintMetaData::compute_acquireGridData(vistle::Object::const_ptr data)
{
    // obtain object profile information from objects with elements/vertices/types
    if (auto indexed = Indexed::as(data)) {
        m_currentProfile.vertices += indexed->getNumCorners();
        m_currentProfile.elements += indexed->getNumElements();

        // harvest cell type data
        const Index *elPtr = indexed->el() + 1;
        for (unsigned int i = 0; i < indexed->getNumElements(); i++) {
            unsigned numCellVertices = *elPtr - *(elPtr - 1);

            // resize if needed (in case of large polygons)
            if (numCellVertices > m_currentProfile.types.size()) {
                m_currentProfile.types.resize(numCellVertices + 1, 0);
            }

            // obtain ghost cell number if data is an unstructured grid
            if (auto unstrGrid = UnstructuredGrid::as(data)) {
                if (unstrGrid->isGhostCell(i)) {
                    m_currentProfile.ghostCells++;
                }
            }

            m_currentProfile.types[numCellVertices]++;
            elPtr++;
        }
    } else if (auto triangles = Triangles::as(data)) {
        m_currentProfile.vertices += triangles->getNumCorners();
        m_currentProfile.elements += triangles->getNumElements();
    } else if (auto quads = Quads::as(data)) {
        m_currentProfile.vertices += quads->getNumCorners();
        m_currentProfile.elements += quads->getNumElements();
    }

    // Structured Grids
    if (auto structured = StructuredGridBase::as(data)) {
        Index totalDims[3];
        Index nonGhostDims[3];

        for (int i = 0; i < 3; i++) {
            totalDims[i] = structured->getNumDivisions(i) - 1;
            m_currentProfile.structuredGridSize[i] += totalDims[i];

            // obtain non-ghost cells by dimension
            nonGhostDims[i] = totalDims[i] - structured->getNumGhostLayers(i, StructuredGridBase::Top) -
                              structured->getNumGhostLayers(i, StructuredGridBase::Bottom);
        }

        // calculate total ghost cell number
        Index totalElements = totalDims[0] * totalDims[1] * totalDims[2];
        Index nonGhostElements = nonGhostDims[0] * nonGhostDims[1] * nonGhostDims[2];
        m_currentProfile.ghostCells += totalElements - nonGhostElements;


        m_currentProfile.elements += structured->getNumElements();

        if (auto uniform = UniformGrid::as(data)) {
            //iterate and copy min/max coordinates
            for (unsigned i = 0; i < ObjectProfile::NUM_STRUCTURED; i++) {
                m_currentProfile.unifMin[i] =
                    (uniform->min()[i] < m_currentProfile.unifMin[i]) ? uniform->min()[i] : m_currentProfile.unifMin[i];
                m_currentProfile.unifMax[i] =
                    (uniform->max()[i] > m_currentProfile.unifMax[i]) ? uniform->max()[i] : m_currentProfile.unifMax[i];
            }
        }
    }


    // obtain coords object profile information
    if (auto coords = Coords::as(data)) {
        if (coords->normals()) {
            m_currentProfile.normals++;
        }
    }

    //obtain vec object profile information
    if (auto vec = Vec<Scalar, 1>::as(data)) {
        m_currentProfile.vecs[1] += vec->getSize();

    } else if (auto vec = Vec<Scalar, 3>::as(data)) {
        m_currentProfile.vecs[3] += vec->getSize();
    }

    //obtain database object profile information
    if (auto db = DataBase::as(data)) {
        if (db->grid()) {
            m_currentProfile.grids++;
        }
    }

    m_currentProfile.blocks++;
}

// COMPUTE HELPER FUNCTION - PRINT VERBOSE DATA
//-------------------------------------------------------------------------
void PrintMetaData::compute_printVerbose(vistle::Object::const_ptr data)
{
    std::string message;

    // verbose print for indexed data types
    if (auto indexed = Indexed::as(data)) {
        message += "\nel: ";
        for (unsigned i = 0; i < indexed->getNumElements(); i++) {
            message += std::to_string(indexed->el()[i]) + ", ";
        }
        sendInfo(message);
        message.clear();

        message += "\ncl: ";
        for (unsigned i = 0; i < indexed->getNumCorners(); i++) {
            message += std::to_string(indexed->cl()[i]) + ", ";
        }
        sendInfo(message);
        message.clear();
    }


    // verbose print for vec data types
    if (auto vec = Vec<Scalar, 1>::as(data)) {
        message += "\nvec: ";
        for (unsigned i = 0; i < vec->getSize(); i++) {
            message += std::to_string(vec->x()[i]) + ", ";
        }

    } else if (auto vec = Vec<Scalar, 3>::as(data)) {
        message += "\nvecs: ";
        for (unsigned i = 0; i < vec->getSize(); i++) {
            message += "(" + std::to_string(vec->x()[i]) + ",";
            message += std::to_string(vec->y()[i]) + ",";
            message += std::to_string(vec->z()[i]) + "), ";
        }
    }

    sendInfo(message);
}

// REDUCE HELPER FUNCTION - PRINT DATA
//-------------------------------------------------------------------------
void PrintMetaData::reduce_printData()
{
    std::string message;

    if (!m_param_doPrintMinMax->getValue() && !m_param_doPrintTotals->getValue()) {
        // nothing to print here
        return;
    }

    if (m_param_doPrintTotals->getValue()) {
        message = M_HORIZONTAL_RULER + "\nOBJECT METADATA:" + M_HORIZONTAL_RULER;

        // print generic data
        message += "\n\nType: " + m_dataType;

        message += "\n\nObjectData:";
        message += "\n   Creator: " + std::to_string(m_creator);
        message += "\n   Execution Counter: " + std::to_string(m_executionCounter);
        message += "\n   Iteration: " + std::to_string(m_iterationCounter);
        message += "\n   Number of Animation Steps: " + std::to_string(m_numAnimationSteps);
        message += "\n   Number of Blocks: " + std::to_string(m_numBlocks);
        message += "\n   Number of Parsed Time Steps: " + std::to_string(m_numParsedTimesteps);
        message += "\n   Number of Total Time Steps: " + std::to_string(m_numTotalTimesteps);
        message += "\n   Real Time: " + std::to_string(m_realTime);
        if (m_transform == Matrix4::Identity()) {
            message += "\n   Transform: identity";
        } else {
            std::stringstream str;
            str << m_transform;
            message += "\n   Transform: " + str.str();
        }

        message += "\n\nAttributes: ";
        for (unsigned i = 0; i < m_attributesVector.size(); i++) {
            message += "\n   " + std::to_string(i) + ": " + m_attributesVector[i];
        }

        // print data so not to overfill print buffer
        sendInfo("%s", message.c_str());
        message.clear();
    }


    // print object profile header
    message += "\n\nObject Profile: ";

    if (m_param_doPrintTotals->getValue()) {
        message += "Total";
    }

    // formatting
    if (m_param_doPrintMinMax->getValue() && m_param_doPrintTotals->getValue()) {
        message += ", ";
    }

    if (m_param_doPrintMinMax->getValue()) {
        message += "(Min/Max) ";
    }


    //print object profile
    message += reduce_conditionalProfileEntryPrint("\n   Number of Vertices: ", m_TotalsProfile.vertices,
                                                   m_minProfile.vertices, m_maxProfile.vertices);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Elements: ", m_TotalsProfile.elements,
                                                   m_minProfile.elements, m_maxProfile.elements);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Ghost Cells: ", m_TotalsProfile.ghostCells,
                                                   m_minProfile.ghostCells, m_maxProfile.ghostCells);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Blocks: ", m_TotalsProfile.blocks,
                                                   m_minProfile.blocks, m_maxProfile.blocks);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Grids: ", m_TotalsProfile.grids, m_minProfile.grids,
                                                   m_maxProfile.grids);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Normals: ", m_TotalsProfile.normals,
                                                   m_minProfile.normals, m_maxProfile.normals);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Vec 1: ", m_TotalsProfile.vecs[1],
                                                   m_minProfile.vecs[1], m_maxProfile.vecs[1]);
    message += reduce_conditionalProfileEntryPrint("\n   Number of Vec 3: ", m_TotalsProfile.vecs[3],
                                                   m_minProfile.vecs[3], m_maxProfile.vecs[3]);

    // print message that has been built
    sendInfo("%s", message.c_str());
    message.clear();

    // print structured grid information
    message += "\n   Structured Grid Size:";
    if (m_param_doPrintTotals->getValue()) {
        message += " (" + std::to_string(m_TotalsProfile.structuredGridSize[0]) + ", " +
                   std::to_string(m_TotalsProfile.structuredGridSize[1]) + ", " +
                   std::to_string(m_TotalsProfile.structuredGridSize[2]) + ")";
    }

    // formatting
    if (m_param_doPrintMinMax->getValue() && m_param_doPrintTotals->getValue()) {
        message += ",  ";
    }

    if (m_param_doPrintMinMax->getValue()) {
        message += "{(" + std::to_string(m_minProfile.structuredGridSize[0]) + ", " +
                   std::to_string(m_minProfile.structuredGridSize[1]) + ", " +
                   std::to_string(m_minProfile.structuredGridSize[2]) + ")/(" +
                   std::to_string(m_maxProfile.structuredGridSize[0]) + ", " +
                   std::to_string(m_maxProfile.structuredGridSize[1]) + ", " +
                   std::to_string(m_maxProfile.structuredGridSize[2]) + ")} ";
    }


    // print uniform grid data
    if (m_minProfile.unifMin[0] != std::numeric_limits<double>::max() && m_param_doPrintMinMax->getValue()) {
        message += "\n   Uniform Grid Min/Max: (" + std::to_string(m_minProfile.unifMin[0]) + ", " +
                   std::to_string(m_minProfile.unifMin[1]) + ", " + std::to_string(m_minProfile.unifMin[2]) + "), (" +
                   std::to_string(m_maxProfile.unifMax[0]) + ", " + std::to_string(m_maxProfile.unifMax[1]) + ", " +
                   std::to_string(m_maxProfile.unifMax[2]) + ") ";
    }


    message += "\n   Cell Types: ";
    for (unsigned i = 0; i < m_TotalsProfile.types.size(); i++) {
        if (m_TotalsProfile.types[i] != 0) {
            message += "\n      " + std::to_string(i) + " Vertices: " + std::to_string(m_TotalsProfile.types[i]);
        }
    }


    // add end string formatting
    message += M_HORIZONTAL_RULER;

    // print message that has been built
    sendInfo("%s", message.c_str());

    return;
}

// REDUCE HELPER FUNCTION - PRINT DATA
//-------------------------------------------------------------------------
std::string PrintMetaData::reduce_conditionalProfileEntryPrint(std::string message, vistle::Index total,
                                                               vistle::Index min, vistle::Index max)
{
    // initialize return message
    if (m_param_doPrintTotals->getValue()) {
        message += std::to_string(total);
    }

    // append min/max info if desired
    if (m_param_doPrintMinMax->getValue()) {
        // formatting
        if (m_param_doPrintTotals->getValue()) {
            message += ", ";
        }

        message += "(" + std::to_string(min) + "/" + std::to_string(max) + ")";
    }


    return message;
}


// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool PrintMetaData::compute()
{
    // acquire input data object
    Object::const_ptr data = expect<Object>("data_in");

    // assert existence of usable data
    if (!data) {
        sendInfo("Error: Unknown Input Data");
        return true;
    }

    // obtain generic data on first compute call of module
    // this data does not change between blocks, and only needs to be processed once
    if (m_isFirstComputeCall && m_isRootNode) {
        compute_acquireGenericData(data);
        m_isFirstComputeCall = false;
    }

    // acquire grid specific data articles
    compute_acquireGridData(data);

    if (m_param_doPrintVerbose->getValue()) {
        compute_printVerbose(data);
    }

    return true;
}

// UTILITY FUNCTION - PRINTS MPI INFORMATION
// * must be called collectively - mpi barrier used to ensure proper message ordering
//-------------------------------------------------------------------------
void PrintMetaData::util_printMPIInfo(std::string printTag)
{
    std::string message;

    // print header for MPI info
    if (m_isRootNode) {
        message = std::string(M_HORIZONTAL_RULER + "\nMPI INFO:" + M_HORIZONTAL_RULER);

        // print mpi library version on root node
        if (printTag == "ctor:") {
            int len = 0;
            char version[MPI_MAX_LIBRARY_VERSION_STRING];

            MPI_Get_library_version(version, &len);

            message += "\nMPI version: " + std::string(version, len) + "\n";
        }

        sendInfo(message);
    }

    // sync so that header is at top
    comm().barrier();
    usleep(10);

    // print mpi rank and size
    message =
        printTag + "rank " + std::to_string(rank()) + "/" + std::to_string(size()) + " on host " + hostname() + "\n";

    sendInfo(message);
}
