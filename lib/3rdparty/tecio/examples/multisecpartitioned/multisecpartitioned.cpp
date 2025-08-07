#include <cassert>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


/*
 * If TECIOMPI is #defined, this program may be executed with mpiexec with up to 2 MPI ranks
 * (processes). In that case, it must be linked with the MPI version of TECIO.
 */
#if defined TECIOMPI
#include <mpi.h>
#endif

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
    int32_t const                           fileFormat;
    int32_t const                           fileType;
    int32_t const                           defaultVarType;
    std::string const                       datasetTitle;
    int32_t const                           numPartitions;
    std::vector<int32_t>                    partitionOwners; // [partition]
    std::string const                       zoneTitle;
    std::string const                       varNameList; // space separated variable names for the dataset
    double const                            solutionTime;
    int32_t const                           strandID;
    int32_t const                           numFaceConnections;
    int32_t const                           faceNeighborMode;
    int32_t const                           shareConnectivityFromZone;
    std::vector<int64_t> const              numNodes;         // [partition]: number of nodes per partition (must include number of ghost nodes)
    std::vector<int64_t> const              numGhostNodes;    // [partition]: number of referenced nodes in each partition
    std::vector<std::vector<int64_t>>       numGhostCells;    // [partition][section]: number of overlapping cells from other partitions that should not be written by this partition
    std::vector<std::vector<int32_t>>       ghostNodes;       // [partition][numGhostNodes[partition]]: nodes within each partition that are referenced from another partition
    std::vector<std::vector<int32_t>>       gnPartitions;     // [partition][numGhostNodes[partition]]: referent partition for each referenced node
    std::vector<std::vector<int32_t>>       gnPartitionNodes; // [partition][numGhostNodes[partition]]: referent partition node for each referenced node
    std::vector<std::vector<int32_t>>       ghostCells;       // [partition][sum(numGhostCells[partition])]: overlapping cells of each section (back to back) for each partitions that should not be written by this partition
    int32_t const                           numSections;
    std::vector<int32_t> const              cellShapes;   // [section]
    std::vector<int32_t> const              gridOrders;   // [section]
    std::vector<int32_t> const              basisFns;     // [section]
    std::vector<std::vector<int64_t>> const numElems;     // [partition][section]
    std::vector<int64_t> const              nodesPerElem; // [section]
    std::vector<int32_t> const              varTypes;          // [var]
    std::vector<int32_t> const              shareVarFromZone;  // [var]
    std::vector<int32_t> const              varValueLocations; // [var]
    std::vector<int32_t> const              passiveVarList;    // [var]
    int32_t const                           numNLVars;
    std::vector<std::vector<std::vector<float>>> nlFieldData; // [partition][var][node]
    std::vector<std::vector<std::vector<float>>> ccFieldData; // [partition][section][cell]
    // nodal dimensions
    int32_t const                     iDim;
    int32_t const                     jDim;
    int32_t const                     kDim;
    int32_t const                     ijDim;
    // cell dimensions
    int32_t const                     iCellDim;
    int32_t const                     jCellDim;
    int32_t const                     kCellDim;
    std::vector<std::vector<std::vector<int32_t>>> connectivity; // [partition][section][nodesPerElem[section]*numElems[partition][section]]

    /**
     * Generates the fabricated data for the example. This example contains 2 K-rows of 4x4
     * hexahedrons in a rectilinear pattern stacked on top of one another with another K-row of 2x2
     * hexahedrons centered at the top and bottom. The top and bottom 4 corners of the mesh are fitted
     * with pyramids to chamfer between the outer corners of the inner 4x4 mesh to the outer 2x2 mesh.
     * Similarly two prisms on each top and bottom edge between the pyramids chamfer the side of the
     * 4x4 to the 2x2 meshes.
     */
    explicit GeneratedData(
        #if defined TECIOMPI
        int32_t commSize
        #endif
        )
        : fileFormat{FILEFORMAT_SZL}
        , fileType{FILETYPE_FULL}
        , defaultVarType{static_cast<int32_t>(FieldDataType_Float)}
        , datasetTitle{"Multi-sectioned FE-mixed zone example using the classic and new TecIO APIs"}
        , numPartitions{2}
        , partitionOwners(numPartitions)
        , zoneTitle{"Multi-sectioned FE-mixed zone"}
        , varNameList{"X Y Z NL CC"}
        , solutionTime{0.0}
        , strandID{0}
        , numFaceConnections{0}
        , faceNeighborMode{0}
        , shareConnectivityFromZone{0}
        , numNodes{{
              84, // partition 1 needs 84 nodes
              34, // partition 2 needs 9 nodes plus 25 shared with partition 1 (see ghost nodes below)
          }}
        , numGhostNodes{{
              0,  // partition 1 owns all the shared nodes at the interface between the two partitions
              25, // partition 2 references 25 nodes from partition 1 at the interface between the two
          }}
        , numGhostCells{ // this example doesn't have any overlapping from other partitions
            { 0, 0, 0 }, // partition 1 ghost cell counts per section
            { 0, 0, 0 }  // partition 2 ghost cell counts per section
          }
        , ghostNodes(numPartitions)
        , gnPartitions(numPartitions)
        , gnPartitionNodes(numPartitions)
        , ghostCells(numPartitions)
        , numSections{3}
        , cellShapes{{
              4, // Hexahedron for section 1
              5, // Pyramid for section 2
              6, // Prism for section 3
          }}
        // all 3 sections have grid order 1 and use Lagrangian basis functions
        , gridOrders(numSections, 1)
        , basisFns(numSections, 0/*Lagrangian*/)
        , numElems{
            { 36, 4, 8 }, // partition 1: 36 hexahedrons, 4 pyramids, and 8 prisms
            {  4, 4, 8 }  // partition 2:  4 hexahedrons, 4 pyramids, and 8 prisms
          }
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
        , nlFieldData(numPartitions)
        , ccFieldData(numPartitions)
        // this toy generation code is not very flexible and will not work with different ijk dimensions
        , iDim{5}
        , jDim{5}
        , kDim{5}
        , ijDim{iDim*jDim}
        , iCellDim{iDim-1}
        , jCellDim{jDim-1}
        , kCellDim{kDim-1}
        , connectivity(numPartitions)
    {
        for (int32_t ptn{0}; ptn < numPartitions; ++ptn)
        {
            // setup the partition owners based on the size of the group associated with the MPI communicator
            #if defined TECIOMPI
            partitionOwners[ptn] = ptn % commSize;
            #endif

            // in our simple example the last partition shares its first 'ijDim' number of nodes with the first partition
            assert((ptn == 0 && numGhostNodes[ptn] == static_cast<size_t>(0)) ||
                   (ptn == 1 && numGhostNodes[ptn] == static_cast<size_t>(ijDim)));
            ghostNodes[ptn].resize(numGhostNodes[ptn]);
            std::iota(ghostNodes[ptn].begin(), ghostNodes[ptn].end(), 1);

            nlFieldData[ptn].resize(numNLVars);
            ccFieldData[ptn].resize(numSections);
            connectivity[ptn].resize(numSections);
        }

        /*
         * Generate the nodal field data.
         */
        int32_t node{0};
        for (int32_t k{0}; k < kDim; ++k)
        {
            /*
             * The first and last row of nodes has one less i and j on all boundaries for
             * the prism, and pyramids to neck down from a 4x4 wall to a 2x2 wall.
             */
            bool const isReducedKPlane{k == 0 || k == kCellDim};
            bool const isLastKPlaneOfFirstPtn{k == kCellDim-1};
            int32_t const ptn{k == kCellDim ? 1 : 0};
            int32_t const ijAdjust{isReducedKPlane ? 1 : 0};
            for (int32_t j{0+ijAdjust}; j < jDim-ijAdjust; ++j)
            {
                for (int32_t i{0+ijAdjust}; i < iDim-ijAdjust; ++i)
                {
                    nlFieldData[ptn][0].push_back(static_cast<float>(i));                // X
                    nlFieldData[ptn][1].push_back(static_cast<float>(j));                // Y
                    nlFieldData[ptn][2].push_back(static_cast<float>(k));                // Z
                    nlFieldData[ptn][3].push_back(static_cast<float>(i*i + j*j + k*k));  // NL

                    if (isLastKPlaneOfFirstPtn)
                    {
                        assert(ptn == 0);

                        // duplicate the shared nodes for the next partition
                        nlFieldData[ptn+1][0].push_back(nlFieldData[ptn][0].back());
                        nlFieldData[ptn+1][1].push_back(nlFieldData[ptn][1].back());
                        nlFieldData[ptn+1][2].push_back(nlFieldData[ptn][2].back());
                        nlFieldData[ptn+1][3].push_back(nlFieldData[ptn][3].back());

                        // the next partition shared the last row of nodes from partition 1
                        gnPartitions[ptn+1].push_back(1);
                        gnPartitionNodes[ptn+1].push_back(node+1);
                    }
                    ++node;
                }
            }
        }

        /*
         * Generate cell centered field data for section 1 of the first partition
         * and section 1 through 3 for the second partition.
         */
        // generate the cell centered field data for section 1 of the first and second partitions: hexahedrons
        for (int32_t k{0}; k < kCellDim; ++k) // -1: don't include the last k-plane
        {
            bool const isReducedKCellPlane{k == 0 || k == kCellDim-1};
            int32_t const ptn{k == kCellDim-1 ? 1 : 0};
            int32_t const ijAdjust{isReducedKCellPlane ? 1 : 0}; // not including cell on edges
            for (int32_t j{0+ijAdjust}; j < jCellDim-ijAdjust; ++j) // -1: don't include the last j-plane
            {
                for (int32_t i{0+ijAdjust}; i < iCellDim-ijAdjust; ++i) // -1: don't include the last i-plane
                {
                    ccFieldData[ptn][0].push_back(static_cast<float>(i*i + j*j + k*k));
                }
            }
        }

        // generate the cell centered field data for section 2: pyramids
        ccFieldData[0][1].push_back(static_cast<float>(0*0 + 0*0 + 0*0));
        ccFieldData[0][1].push_back(static_cast<float>(3*3 + 0*0 + 0*0));
        ccFieldData[0][1].push_back(static_cast<float>(0*0 + 3*3 + 0*0));
        ccFieldData[0][1].push_back(static_cast<float>(3*3 + 3*3 + 0*0));

        ccFieldData[1][1].push_back(static_cast<float>(0*0 + 0*0 + 3*3));
        ccFieldData[1][1].push_back(static_cast<float>(3*3 + 0*0 + 3*3));
        ccFieldData[1][1].push_back(static_cast<float>(0*0 + 3*3 + 3*3));
        ccFieldData[1][1].push_back(static_cast<float>(3*3 + 3*3 + 3*3));

        // generate the cell centered field data for section 3: prisms
        ccFieldData[0][2].push_back(static_cast<float>(1*1 + 0*0 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(2*2 + 0*0 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(0*0 + 1*1 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(3*3 + 1*1 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(0*0 + 2*2 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(3*3 + 2*2 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(1*1 + 3*3 + 0*0));
        ccFieldData[0][2].push_back(static_cast<float>(2*2 + 3*3 + 0*0));

        ccFieldData[1][2].push_back(static_cast<float>(1*1 + 0*0 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(2*2 + 0*0 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(0*0 + 1*1 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(3*3 + 1*1 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(0*0 + 2*2 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(3*3 + 2*2 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(1*1 + 3*3 + 3*3));
        ccFieldData[1][2].push_back(static_cast<float>(2*2 + 3*3 + 3*3));

        /*
         * Generate connectivity both partitions.
         */
        // generate the 32 bit connectivity for section 1 of the first and second partitions: hexahedrons
        for (int32_t k{0}; k < kCellDim; ++k) // skip k-plane
        {
            bool const isReducedKCellPlane{k == 0 || k == kCellDim-1};
            int32_t const ptn{k == kCellDim-1 ? 1 : 0};
            int32_t const kCellPlane{ptn == 1 ? 0 : k};
            int32_t const ijCellAdjust{isReducedKCellPlane ? 1 : 0};
            int32_t const kOffset{ ptn == 1 ? 0 : (k > 0 ? (iDim - 2) * (jDim - 2) + (k > 1 ? (k - 1) * ijDim : 0) : 0) };
            int32_t const kp1Offset{ ptn == 1 ? ijDim : ((iDim - 2) * (jDim - 2) + k * ijDim) };
            // The first k-plane has iDim-2 nodes in the i-direction for partition 0, and iDim for partition 1.
            // The remaining k-planes have iDim nodes in the i-direction for partition 0 and iDim-2 for partition 1.
            int32_t const kPlaneIDim = ptn == kCellPlane ? (iDim - 2 * ijCellAdjust) : iDim;
            int32_t const kp1PlaneIDim = ptn == 0 ? iDim : (iDim - 2 * ijCellAdjust);
            for (int32_t j{0+ijCellAdjust}; j < jCellDim-ijCellAdjust; ++j) // skip j-plane
            {
                int32_t jOffset  { (ptn == 0 ? j - ijCellAdjust : j) * kPlaneIDim };
                int32_t jp1Offset{ (ptn == 1 ? j - ijCellAdjust : j) * kp1PlaneIDim }; // j offset in the k+1 plane
                for (int32_t i{0+ijCellAdjust}; i < iCellDim-ijCellAdjust; ++i) // skip i-plane
                {
                    int32_t const iOffset{ ptn == 1 ? i : i - ijCellAdjust };
                    int32_t const ip1Offset{ ptn == 1 ? i - ijCellAdjust : i }; // i offset in the k+1 plane
                    // in our simple example the last partition shares its first 'ijDim' number of nodes with the first partition
                    appendValues(connectivity[ptn][0], {{
                        1 + iOffset   + jOffset                  + kOffset,
                        2 + iOffset   + jOffset                  + kOffset,
                        2 + iOffset   + jOffset   + kPlaneIDim   + kOffset,
                        1 + iOffset   + jOffset   + kPlaneIDim   + kOffset,
                        1 + ip1Offset + jp1Offset                + kp1Offset,
                        2 + ip1Offset + jp1Offset                + kp1Offset,
                        2 + ip1Offset + jp1Offset + kp1PlaneIDim + kp1Offset,
                        1 + ip1Offset + jp1Offset + kp1PlaneIDim + kp1Offset
                    }});
                }
            }
        }

        // generate the connectivity for section 2: pyramids
        appendValues(connectivity[0][1], {{ 11, 10, 15, 16,1}});
        appendValues(connectivity[0][1], {{ 14, 13,18, 19,3}});
        appendValues(connectivity[0][1], {{26,25,30,31,7}});
        appendValues(connectivity[0][1], {{29,28,33,34,9}});

        appendValues(connectivity[1][1], {{ 1, 2, 7, 6,26}});
        appendValues(connectivity[1][1], {{ 4, 5,10, 9,28}});
        appendValues(connectivity[1][1], {{16,17,22,21,32}});
        appendValues(connectivity[1][1], {{19,20,25,24,34}});

        // generate the connectivity for section 3: prisms
        appendValues(connectivity[0][2], {{ 1, 16,11, 2, 17,12}});
        appendValues(connectivity[0][2], {{ 2, 17,12, 3, 18,13}});
        appendValues(connectivity[0][2], {{ 1, 15,16,4,20,21}});
        appendValues(connectivity[0][2], {{4, 20,21,7,25,26}});
        appendValues(connectivity[0][2], {{3, 18, 19, 6,23,24}});
        appendValues(connectivity[0][2], {{6,23,24,9,28,29}});
        appendValues(connectivity[0][2], {{7,31,26,8,32,27}});
        appendValues(connectivity[0][2], {{8,32,27,9,33,28}});

        appendValues(connectivity[1][2], {{ 2, 7,26, 3, 8,27}});
        appendValues(connectivity[1][2], {{ 3, 8,27, 4, 9,28}});
        appendValues(connectivity[1][2], {{ 7, 6,26,12,11,29}});
        appendValues(connectivity[1][2], {{10, 9,28,15,14,31}});
        appendValues(connectivity[1][2], {{12,11,29,17,16,32}});
        appendValues(connectivity[1][2], {{15,14,31,20,19,34}});
        appendValues(connectivity[1][2], {{17,22,32,18,23,33}});
        appendValues(connectivity[1][2], {{18,23,33,19,24,34}});
    }
};
}

namespace {
/**
 * Example of how to create a multi-sectioned, multi-partitioned FE-mixed zone using the new TecIO
 * MPI API. The resulting file, multisecpartitioned_newAPI.szplt, is identical to that generated by
 * the example created using the old TecIO MPI API.
 * @throws std::exception or derivative for any error
 */
void generateOutputUsingNewAPI(
    #if defined TECIOMPI
    MPI_Comm&            mpiComm,
    int32_t              commRank,
    #endif
    GeneratedData const& gen)
{
    int32_t mainRank{0};

    // create an output file handle for writing
    void* outputFileHandle{nullptr};
    if (tecFileWriterOpen("multisecpartitioned_newAPI.szplt",
            gen.datasetTitle.c_str(), gen.varNameList.c_str(),
            gen.fileFormat, gen.fileType, gen.defaultVarType,
            nullptr/*gridFileHandle*/, &outputFileHandle) != 0)
        throw std::runtime_error("failed to open file for writing");

    #if defined TECIOMPI
    // initialize an MPI communicator for each rank
    if (tecMPIInitialize(outputFileHandle, mpiComm, mainRank) != 0)
        throw std::runtime_error("failed to create an MPI communicator for each rank");
    #endif

    // create the multi-sectioned FE-mixed zone to hold the data

    // compute the number of nodes in the zone
    int64_t const numNodes{
        // sum the number of nodes in each partition (which include ghost nodes)
        std::accumulate(gen.numNodes.begin(), gen.numNodes.end(), static_cast<int64_t>(0)) -
        // and subtract the shared ghost nodes from each partition
        std::accumulate(gen.numGhostNodes.begin(), gen.numGhostNodes.end(), static_cast<int64_t>(0))
    };

    // similarly, compute the number of cells in the zone
    std::vector<int64_t> numElems(gen.numSections, static_cast<int64_t>(0));
    for (int32_t ptn{0}; ptn < gen.numPartitions; ++ptn)
        for (int32_t sec{0}; sec < gen.numSections; ++sec)
            numElems[sec] += gen.numElems[ptn][sec] - gen.numGhostCells[ptn][sec];

    int32_t outputZone{0};
    if (tecZoneCreateFEMixed(outputFileHandle, gen.zoneTitle.c_str(), numNodes, gen.numSections,
            gen.cellShapes.data(), gen.gridOrders.data(), gen.basisFns.data(), numElems.data(),
            gen.varTypes.data(), gen.shareVarFromZone.data(), gen.varValueLocations.data(),
            gen.passiveVarList.data(), gen.shareConnectivityFromZone, gen.numFaceConnections,
            gen.faceNeighborMode, &outputZone) != 0)
        throw std::runtime_error("failed to create FE-mixed zone");

    #if defined TECIOMPI
    // prepare output partitions
    if (tecZoneMapPartitionsToMPIRanks(outputFileHandle,
            outputZone, gen.numPartitions, gen.partitionOwners.data()) != 0)
        throw std::runtime_error("failed to prepare output partitions for zone");
    #endif

    for (int32_t ptn{0}; ptn < gen.numPartitions; ++ptn)
    {
        #if defined TECIOMPI
        if (commRank != gen.partitionOwners[ptn])
            continue; // only process partitions owned by this rank
        #endif

        if (tecFEMixedPartitionCreate32(outputFileHandle, outputZone, ptn+1,
                gen.numNodes[ptn], gen.numElems[ptn].data(),
                gen.numGhostNodes[ptn], gen.ghostNodes[ptn].data(),
                gen.gnPartitions[ptn].data(), gen.gnPartitionNodes[ptn].data(),
                gen.numGhostCells[ptn].data(), gen.ghostCells[ptn].data()) != 0)
            throw std::runtime_error("failed to prepare add partition for zone");

        // send the nodal field data to the zone
        for (int32_t var{0}; var < gen.numNLVars; ++var)
        {
            assert(gen.nlFieldData[ptn][var].size() == static_cast<size_t>(gen.numNodes[ptn]));
            if (tecZoneVarWriteFloatValues(outputFileHandle, outputZone,
                    var+1, ptn+1, gen.numNodes[ptn], gen.nlFieldData[ptn][var].data()) != 0)
                throw std::runtime_error("failed to write nodal variable data");
        }

        // send the cell centered field data and connectivity to the zone section by section
        for (int32_t section{0}; section < gen.numSections; ++section)
        {
            // send the cell centered field data to the zone for this section
            assert(gen.ccFieldData[ptn][section].size() == static_cast<size_t>(gen.numElems[ptn][section]));
            auto const ccVar{gen.numNLVars+1};
            if (tecZoneVarWriteFloatValues(outputFileHandle, outputZone,
                    ccVar, ptn+1, gen.numElems[ptn][section], gen.ccFieldData[ptn][section].data()) != 0)
                throw std::runtime_error("failed to write cell centered variable data");

            // send the connectivity to the zone for this section
            assert(gen.connectivity[ptn][section].size() == static_cast<size_t>(gen.numElems[ptn][section]*gen.nodesPerElem[section]));
            if (tecZoneNodeMapWrite32(outputFileHandle, outputZone, ptn+1,
                    1/*nodesAreOneBased*/, gen.numElems[ptn][section]*gen.nodesPerElem[section],
                    gen.connectivity[ptn][section].data()) != 0)
                throw std::runtime_error("failed to write connectivity data");
        }
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
 * Example of how to create a multi-sectioned, multi-partitioned FE-mixed zone using the classic
 * TecIO MPI API. The resulting file, multisecpartitioned_classicAPI.szplt, is identical to that
 * generated by the example created using the new TecIO MPI API.
 * @throws std::exception or derivative for any error
 */
void generateOutputUsingClassicAPI(
    #if defined TECIOMPI
    MPI_Comm&            mpiComm,
    int32_t              commRank,
    #endif
    GeneratedData const& gen)
{
    int32_t mainRank{0};

    // create an output file for writing; TECIO retains the file handle state
    int32_t outputDebugInfo{0}; // 0: no additional logging output, 1: yes, log to standard output
    int32_t varDataIsDouble{0}; // 0: float, 1: double
    if (TECINI142(gen.datasetTitle.c_str(), gen.varNameList.c_str(),
            "multisecpartitioned_classicAPI.szplt", "."/*scratchDir*/,
            &gen.fileFormat, &gen.fileType, &outputDebugInfo, &varDataIsDouble) != 0)
        throw std::runtime_error("failed to open file for writing");

    #if defined TECIOMPI
    // initialize an MPI communicator for each rank
    if (TECMPIINIT142(&mpiComm, &mainRank) != 0)
        throw std::runtime_error("failed to create an MPI communicator for each rank");
    #endif

    // create the multi-sectioned FE-mixed zone to hold the data

    // compute the number of nodes in the zone
    int64_t const numNodes{
        // sum the number of nodes in each partition (which include ghost nodes)
        std::accumulate(gen.numNodes.begin(), gen.numNodes.end(), static_cast<int64_t>(0)) -
        // and subtract the shared ghost nodes from each partition
        std::accumulate(gen.numGhostNodes.begin(), gen.numGhostNodes.end(), static_cast<int64_t>(0))
    };

    // similarly, compute the number of cells in the zone
    std::vector<int64_t> numElems(gen.numSections, static_cast<int64_t>(0));
    for (int32_t ptn{0}; ptn < gen.numPartitions; ++ptn)
        for (int32_t sec{0}; sec < gen.numSections; ++sec)
            numElems[sec] += gen.numElems[ptn][sec] - gen.numGhostCells[ptn][sec];

    if (TECZNEFEMIXED142(gen.zoneTitle.c_str(), &numNodes, &gen.numSections,
            gen.cellShapes.data(), gen.gridOrders.data(), gen.basisFns.data(),
            numElems.data(), &gen.solutionTime, &gen.strandID,
            &gen.numFaceConnections, &gen.faceNeighborMode, gen.passiveVarList.data(),
            gen.varValueLocations.data(), gen.shareVarFromZone.data(),
            &gen.shareConnectivityFromZone) != 0)
        throw std::runtime_error("failed to create FE-mixed zone");

    #if defined TECIOMPI
    // prepare output partitions
    if (TECZNEMAP142(&gen.numPartitions, gen.partitionOwners.data()) != 0)
        throw std::runtime_error("failed to prepare output partitions for zone");
    #endif

    for (int32_t ptn{0}; ptn < gen.numPartitions; ++ptn)
    {
        #if defined TECIOMPI
        if (commRank != gen.partitionOwners[ptn])
            continue; // only process partitions owned by this rank
        #endif

        int32_t const partition{ptn+1}; // ones based partition number needed for TECIO
        if (TECFEMIXEDPTN142(&partition, &gen.numNodes[ptn], gen.numElems[ptn].data(),
                &gen.numGhostNodes[ptn], gen.ghostNodes[ptn].data(),
                gen.gnPartitions[ptn].data(), gen.gnPartitionNodes[ptn].data(),
                gen.numGhostCells[ptn].data(), gen.ghostCells[ptn].data()) != 0)
            throw std::runtime_error("failed to prepare add partition for zone");

        // send the nodal field data to the zone
        for (int32_t var{0}; var < gen.numNLVars; ++var)
        {
            assert(gen.nlFieldData[ptn][var].size() == static_cast<size_t>(gen.numNodes[ptn]));
            /*
             * The classic TECIO API will allow you to deliver block sizes who's value count fits within
             * a signed 32 bit integer, around 2 billion. If you have more than ~2 billion values you
             * can deliver them by making repeated calls to TECDAT142, or TECNODE142, delivering the
             * values in chunk sizes under ~2 billion values. In our case we can deliver each variable's
             * values in their entirety with a single call.
             */
            assert(gen.nlFieldData[ptn][var].size() < std::numeric_limits<int32_t>::max());
            int32_t const numNLFieldValues{static_cast<int32_t>(gen.numNodes[ptn])};
            if (TECDAT142(&numNLFieldValues, gen.nlFieldData[ptn][var].data(), &varDataIsDouble) != 0)
                throw std::runtime_error("failed to write nodal variable data");
        }

        // send the cell centered field data and connectivity to the zone section by section
        for (int32_t section{0}; section < gen.numSections; ++section)
        {
            // send the cell centered field data to the zone for this section
            assert(gen.ccFieldData[ptn][section].size() == static_cast<size_t>(gen.numElems[ptn][section]));
            if (gen.numElems[ptn][section] == 0)
                continue; // no cells to deliver for this section in the partition
            auto const ccVar{gen.numNLVars+1};
            assert(gen.ccFieldData[ptn][section].size() < std::numeric_limits<int32_t>::max()); // see 32 bit note above
            int32_t const numCCFieldValues{static_cast<int32_t>(gen.numElems[ptn][section])};
            if (TECDAT142(&numCCFieldValues, gen.ccFieldData[ptn][section].data(), &varDataIsDouble) != 0)
                throw std::runtime_error("failed to write cell centered variable data");

            // send the connectivity to the zone for this section
            assert(gen.connectivity[ptn][section].size() == static_cast<size_t>(gen.numElems[ptn][section]*gen.nodesPerElem[section]));
            assert(gen.connectivity[ptn][section].size() < std::numeric_limits<int32_t>::max()); // see 32 bit note above
            int32_t const numNodeValues{static_cast<int32_t>(gen.numElems[ptn][section]*gen.nodesPerElem[section])};
            if (TECNODE142(&numNodeValues, gen.connectivity[ptn][section].data()) != 0)
                throw std::runtime_error("failed to write connectivity data");
        }
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

        #if defined TECIOMPI
        // initialize MPI
        if (MPI_Init(&argc, &argv) != 0)
            throw std::runtime_error("failed to initialize MPI");

        MPI_Comm mpiComm{MPI_COMM_WORLD};
        int32_t commRank{0};
        int32_t commSize{0};
        MPI_Comm_size(mpiComm, &commSize);
        MPI_Comm_rank(mpiComm, &commRank);
        if (commSize == 0)
            throw std::runtime_error("failed to get the size of the group associated with the MPI communicator");
        #endif

        // generate example output using the classic and new TecIO APIs
        GeneratedData gen
            #if defined TECIOMPI
            (commSize)
            #endif
            ;
        generateOutputUsingNewAPI(
            #if defined TECIOMPI
            mpiComm, commRank,
            #endif
            gen);
        generateOutputUsingClassicAPI(
            #if defined TECIOMPI
            mpiComm, commRank,
            #endif
            gen);

        #if defined TECIOMPI
        if (MPI_Finalize() != 0)
            throw std::runtime_error("failed to finalize MPI");
        #endif

        return 0;
    }
    catch (std::exception const& ex)
    {
        std::cerr<<argv[0]<<": "<<ex.what()<<std::endl;
        return 1;
    }
}
