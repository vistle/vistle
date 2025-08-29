#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TECIO.h"


namespace {
/**
 */
template <typename T>
void appendValues(
    std::vector<T>& destVector,
    std::vector<T> const& sourceVector)
{
    destVector.insert(destVector.end(), sourceVector.begin(), sourceVector.end());
}
}

namespace {
/**
 * Structure to hold all the generated data for this example. The method to generate the data itself
 * isn't particularly interesting nor instructive, it is merely used to show how to fill out arrays
 * passed to the various TECIO functions.
 */
struct GeneratedData
{
    int32_t const                     fileFormat;
    int32_t const                     fileType;
    int32_t const                     defaultVarType;
    std::string const                 datasetTitle;
    std::string const                 zoneTitle;
    std::string const                 varNameList; // space separated variable names for the dataset
    double const                      solutionTime;
    int32_t const                     strandID;
    int32_t const                     numFaceConnections;
    int32_t const                     faceNeighborMode;
    int32_t const                     shareConnectivityFromZone;
    int64_t const                     numNodes;
    int32_t const                     numSections;
    std::vector<int32_t> const        cellShapes;
    std::vector<int32_t> const        gridOrders;
    std::vector<int32_t> const        basisFns;
    std::vector<int64_t> const        numElems;
    std::vector<int64_t> const        nodesPerElem;
    std::vector<int32_t> const        varTypes;
    std::vector<int32_t> const        shareVarFromZone;
    std::vector<int32_t> const        varValueLocations;
    std::vector<int32_t> const        passiveVarList;
    int32_t const                     numNLVars;
    std::vector<std::vector<float>>   nlFieldData;
    std::vector<std::vector<float>>   ccFieldData;
    // nodal dimensions
    int32_t const                     iDim;
    int32_t const                     jDim;
    int32_t const                     kDim;
    int32_t const                     ijDim;
    // cell dimensions
    int32_t const                     iCellDim;
    int32_t const                     jCellDim;
    int32_t const                     kCellDim;
    std::vector<std::vector<int32_t>> connectivity;

    /**
     * Generates the fabricated data for the example. This example contains 2 K rows of 4x4
     * hexahedrons in a rectilinear pattern stacked on top of one another with a third K row of 2x2
     * hexahedrons centered at the top. The top 4 corners of the mesh are fitted with pyramids to
     * chamfer between the outer corners of the lower 4x4 mesh to the upper 2x2 mesh. Similarly two
     * prism's on each top edge between the pyramids chamfer the side of the 4x4 to the top of the
     * 2x2 mesh.
     */
    GeneratedData()
        : fileFormat{FILEFORMAT_SZL}
        , fileType{FILETYPE_FULL}
        , defaultVarType{static_cast<int32_t>(FieldDataType_Float)}
        , datasetTitle{"Multi-sectioned FE-mixed zone example using the classic and new TecIO APIs"}
        , zoneTitle{"Multi-sectioned FE-mixed zone"}
        , varNameList{"X Y Z NL CC"}
        , solutionTime{0.0}
        , strandID{0}
        , numFaceConnections{0}
        , faceNeighborMode{0}
        , shareConnectivityFromZone{0}
        , numNodes{84}
        , numSections{3}
        , cellShapes{{
              4, // Hexahedron for section 1
              5, // Pyramid for section 2
              6, // Prism for section 3
          }}
        // all 3 sections have grid order 1 and use Lagrangian basis functions
        , gridOrders(numSections, 1)
        , basisFns(numSections, 0/*Lagrangian*/)
        , numElems{{ 36, 4, 8 }} // 36 hexahedrons, 4 pyramids, and 8 prisms
        , nodesPerElem{{ 8, 5, 6 }}
        , varTypes{{
              static_cast<int32_t>(FieldDataType_Float), // X
              static_cast<int32_t>(FieldDataType_Float), // Y
              static_cast<int32_t>(FieldDataType_Float), // Z
              static_cast<int32_t>(FieldDataType_Float), // NL
              static_cast<int32_t>(FieldDataType_Float)  // CC
          }}
        , shareVarFromZone{{ 0, 0, 0, 0, 0 }} // no sharing for any variables
        , varValueLocations{{
              1, // nodal value location for X
              1, // nodal value location for Y
              1, // nodal value location for Z
              1, // nodal value location for NL
              0  // cell centered value location for CC
          }}
        , passiveVarList{{ 0, 0, 0, 0, 0 }} // no passive variables
        , numNLVars{4} // 4 NL variables in this example; X, Y, Z, and NL
        , nlFieldData(numNLVars)
        , ccFieldData(numSections)
        // this toy generation code is not very flexible and will not work with different ijk dimensions
        , iDim{5}
        , jDim{5}
        , kDim{4}
        , ijDim{iDim*jDim}
        , iCellDim{iDim-1}
        , jCellDim{jDim-1}
        , kCellDim{kDim-1}
        , connectivity(numSections)
    {
        /*
         * Generate the nodal field data.
         */
        for (int32_t k{0}; k < kDim; ++k)
        {
            /*
             * The last row of nodes has one less i and j on all boundaries for
             * the prism, and pyramids to neck down from a 4x4 wall to a 2x2 wall.
             */
            int32_t const ijAdjust{k == kCellDim ? 1 : 0};
            for (int32_t j{0+ijAdjust}; j < jDim-ijAdjust; ++j)
            {
                for (int32_t i{0+ijAdjust}; i < iDim-ijAdjust; ++i)
                {
                    nlFieldData[0].push_back(static_cast<float>(i));                // X
                    nlFieldData[1].push_back(static_cast<float>(j));                // Y
                    nlFieldData[2].push_back(static_cast<float>(k));                // Z
                    nlFieldData[3].push_back(static_cast<float>(i*i + j*j + k*k));  // NL
                }
            }
        }

        /*
         * Generate cell centered field data for section 1 through 3.
         */
        // generate the cell centered field data for section 1: hexahedrons
        for (int32_t k{0}; k < kCellDim; ++k) // -1: don't include the last k-plane
        {
            int32_t const ijAdjust{k == kCellDim-1 ? 1 : 0}; // not including cell on edges
            for (int32_t j{0+ijAdjust}; j < jCellDim-ijAdjust; ++j) // -1: don't include the last j-plane
            {
                for (int32_t i{0+ijAdjust}; i < iCellDim-ijAdjust; ++i) // -1: don't include the last i-plane
                {
                    ccFieldData[0].push_back(static_cast<float>(i*i + j*j + k*k));
                }
            }
        }

        // generate the cell centered field data for section 2: pyramids
        ccFieldData[1].push_back(static_cast<float>(0*0 + 0*0 + 2*2));
        ccFieldData[1].push_back(static_cast<float>(3*3 + 0*0 + 2*2));
        ccFieldData[1].push_back(static_cast<float>(0*0 + 3*3 + 2*2));
        ccFieldData[1].push_back(static_cast<float>(3*3 + 3*3 + 2*2));

        // generate the cell centered field data for section 3: prisms
        ccFieldData[2].push_back(static_cast<float>(1*1 + 0*0 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(2*2 + 0*0 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(0*0 + 1*1 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(3*3 + 1*1 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(0*0 + 2*2 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(3*3 + 2*2 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(1*1 + 3*3 + 2*2));
        ccFieldData[2].push_back(static_cast<float>(2*2 + 3*3 + 2*2));

        /*
         * Generate connectivity for section 1 through 3.
         */
        // generate the 32 bit connectivity for each section
        for (int32_t k{0}; k < kCellDim; ++k) // skip k-plane
        {
            int32_t const ijCellAdjust{k == kCellDim-1 ? 1 : 0};
            for (int32_t j{0+ijCellAdjust}; j < jCellDim-ijCellAdjust; ++j) // skip j-plane
            {
                for (int32_t i{0+ijCellAdjust}; i < iCellDim-ijCellAdjust; ++i) // skip i-plane
                {
                    appendValues(connectivity[0], {{
                        (i+1) + (j+0)*iDim + (k+0)*ijDim,
                        (i+2) + (j+0)*iDim + (k+0)*ijDim,
                        (i+2) + (j+1)*iDim + (k+0)*ijDim,
                        (i+1) + (j+1)*iDim + (k+0)*ijDim,
                        (i+1-ijCellAdjust) + (j+0-ijCellAdjust)*(iDim-2*ijCellAdjust) + (k+1)*ijDim,
                        (i+2-ijCellAdjust) + (j+0-ijCellAdjust)*(iDim-2*ijCellAdjust) + (k+1)*ijDim,
                        (i+2-ijCellAdjust) + (j+1-ijCellAdjust)*(iDim-2*ijCellAdjust) + (k+1)*ijDim,
                        (i+1-ijCellAdjust) + (j+1-ijCellAdjust)*(iDim-2*ijCellAdjust) + (k+1)*ijDim
                    }});
                }
            }
        }

        // generate the connectivity for section 2: pyramids
        appendValues(connectivity[1], {{51,52,57,56,76}});
        appendValues(connectivity[1], {{54,55,60,59,78}});
        appendValues(connectivity[1], {{66,67,72,71,82}});
        appendValues(connectivity[1], {{69,70,75,74,84}});

        // generate the connectivity for section 3: prisms
        appendValues(connectivity[2], {{52,57,76,53,58,77}});
        appendValues(connectivity[2], {{53,58,77,54,59,78}});
        appendValues(connectivity[2], {{57,56,76,62,61,79}});
        appendValues(connectivity[2], {{60,59,78,65,64,81}});
        appendValues(connectivity[2], {{62,61,79,67,66,82}});
        appendValues(connectivity[2], {{65,64,81,70,69,84}});
        appendValues(connectivity[2], {{67,72,82,68,73,83}});
        appendValues(connectivity[2], {{68,73,83,69,74,84}});
    }
};
}

namespace {
/**
 * Example of how to create a multi-sectioned FE-mixed zone using the new TecIO API. The resulting
 * file, multiplesection_newAPI.szplt, is identical to that generated by the example created using
 * the old TecIO API.
 * @throws std::exception or derivative for any error
 */
void generateOutputUsingNewAPI(GeneratedData const& gen)
{
    // create an output file handle for writing
    void* outputFileHandle{nullptr};
    if (tecFileWriterOpen("multiplesection_newAPI.szplt",
            gen.datasetTitle.c_str(), gen.varNameList.c_str(),
            gen.fileFormat, gen.fileType, gen.defaultVarType,
            nullptr/*gridFileHandle*/, &outputFileHandle) != 0)
        throw std::runtime_error("failed to open file for writing");

    // create the multi-sectioned FE-mixed zone to hold the data
    int32_t outputZone{0};
    if (tecZoneCreateFEMixed(outputFileHandle, gen.zoneTitle.c_str(), gen.numNodes, gen.numSections,
            gen.cellShapes.data(), gen.gridOrders.data(), gen.basisFns.data(), gen.numElems.data(),
            gen.varTypes.data(), gen.shareVarFromZone.data(), gen.varValueLocations.data(),
            gen.passiveVarList.data(), gen.shareConnectivityFromZone, gen.numFaceConnections,
            gen.faceNeighborMode, &outputZone) != 0)
        throw std::runtime_error("failed to create FE-mixed zone");

    // send the nodal field data to the zone
    for (int32_t var{0}; var < gen.numNLVars; ++var)
    {
        assert(gen.nlFieldData[var].size() == static_cast<size_t>(gen.numNodes));
        if (tecZoneVarWriteFloatValues(outputFileHandle, outputZone,
                var+1, 0/*partition*/, gen.numNodes, gen.nlFieldData[var].data()) != 0)
            throw std::runtime_error("failed to write nodal variable data");
    }

    // send the cell centered field data and connectivity to the zone section by section
    for (int32_t section{0}; section < gen.numSections; ++section)
    {
        // send the cell centered field data to the zone for this section
        assert(gen.ccFieldData[section].size() == static_cast<size_t>(gen.numElems[section]));
        auto const ccVar{gen.numNLVars+1};
        if (tecZoneVarWriteFloatValues(outputFileHandle, outputZone,
                ccVar, 0/*partition*/, gen.numElems[section], gen.ccFieldData[section].data()) != 0)
            throw std::runtime_error("failed to write cell centered variable data");

        // send the connectivity to the zone for this section
        assert(gen.connectivity[section].size() == static_cast<size_t>(gen.numElems[section]*gen.nodesPerElem[section]));
        if (tecZoneNodeMapWrite32(outputFileHandle, outputZone, 0/*partition*/,
                1/*nodesAreOneBased*/, gen.numElems[section]*gen.nodesPerElem[section],
                gen.connectivity[section].data()) != 0)
            throw std::runtime_error("failed to write connectivity data");
    }

    /*
     * Write the file to disk.
     */
    if (tecFileWriterClose(&outputFileHandle) != 0)
        throw std::runtime_error("failed to generate file");
}
}

namespace {
/**
 * Example of how to create a multi-sectioned FE-mixed zone using the classic TecIO API. The
 * resulting file, multiplesection_classicAPI.szplt, is identical to that generated by the example
 * created using the new TecIO API.
 * @throws std::exception or derivative for any error
 */
void generateOutputUsingClassicAPI(GeneratedData const& gen)
{
    // create an output file for writing; TECIO retains the file handle state
    int32_t outputDebugInfo{0}; // 0: no additional logging output, 1: yes, log to standard output
    int32_t varDataIsDouble{0}; // 0: float, 1: double
    if (TECINI142(gen.datasetTitle.c_str(), gen.varNameList.c_str(),
            "multiplesection_classicAPI.szplt", "."/*scratchDir*/,
            &gen.fileFormat, &gen.fileType, &outputDebugInfo, &varDataIsDouble) != 0)
        throw std::runtime_error("failed to open file for writing");

    // create the multi-sectioned FE-mixed zone to hold the data
    if (TECZNEFEMIXED142(gen.zoneTitle.c_str(), &gen.numNodes, &gen.numSections,
            gen.cellShapes.data(), gen.gridOrders.data(), gen.basisFns.data(),
            gen.numElems.data(), &gen.solutionTime, &gen.strandID,
            &gen.numFaceConnections, &gen.faceNeighborMode, gen.passiveVarList.data(),
            gen.varValueLocations.data(), gen.shareVarFromZone.data(),
            &gen.shareConnectivityFromZone) != 0)
        throw std::runtime_error("failed to create FE-mixed zone");

    // send the nodal field data to the zone
    for (int32_t var{0}; var < gen.numNLVars; ++var)
    {
        assert(gen.nlFieldData[var].size() == static_cast<size_t>(gen.numNodes));
        /*
         * The classic TECIO API will allow you to deliver block sizes who's value count fits within
         * a signed 32 bit integer, around 2 billion. If you have more than ~2 billion values you
         * can deliver them by making repeated calls to TECDAT142, or TECNODE142, delivering the
         * values in chunk sizes under ~2 billion values. In our case we can deliver each variable's
         * values in their entirety with a single call.
         */
        assert(gen.nlFieldData[var].size() < std::numeric_limits<int32_t>::max());
        int32_t const numNLFieldValues{static_cast<int32_t>(gen.numNodes)};
        if (TECDAT142(&numNLFieldValues, gen.nlFieldData[var].data(), &varDataIsDouble) != 0)
            throw std::runtime_error("failed to write nodal variable data");
    }

    // send the cell centered field data and connectivity to the zone section by section
    for (int32_t section{0}; section < gen.numSections; ++section)
    {
        // send the cell centered field data to the zone for this section
        assert(gen.ccFieldData[section].size() == static_cast<size_t>(gen.numElems[section]));
        auto const ccVar{gen.numNLVars+1};
        assert(gen.ccFieldData[section].size() < std::numeric_limits<int32_t>::max()); // see 32 bit note above
        int32_t const numCCFieldValues{static_cast<int32_t>(gen.numElems[section])};
        if (TECDAT142(&numCCFieldValues, gen.ccFieldData[section].data(), &varDataIsDouble) != 0)
            throw std::runtime_error("failed to write cell centered variable data");

        // send the connectivity to the zone for this section
        assert(gen.connectivity[section].size() == static_cast<size_t>(gen.numElems[section]*gen.nodesPerElem[section]));
        assert(gen.connectivity[section].size() < std::numeric_limits<int32_t>::max()); // see 32 bit note above
        int32_t const numNodeValues{static_cast<int32_t>(gen.numElems[section]*gen.nodesPerElem[section])};
        if (TECNODE142(&numNodeValues, gen.connectivity[section].data()) != 0)
            throw std::runtime_error("failed to write connectivity data");
    }

    /*
     * Write the file to disk.
     */
    if (TECEND142() != 0)
        throw std::runtime_error("failed to generate file");
}
}

/**
 */
int main(
    int    argc,
    char** argv)
{
    try
    {
        // verify command line options (this example has none)
        if (argc > 1)
        {
            std::ostringstream os;
            os<<"invalid command line options:";
            for (int arg{1}; arg < argc; ++arg)
                os<<" "<<argv[arg];
            throw std::invalid_argument(os.str());
        }

        // generate example output using the classic and new TecIO APIs
        GeneratedData gen;
        generateOutputUsingNewAPI(gen);
        generateOutputUsingClassicAPI(gen);

        return 0;
    }
    catch (std::exception const& ex)
    {
        std::cerr<<argv[0]<<": "<<ex.what()<<std::endl;
        return 1;
    }
}
