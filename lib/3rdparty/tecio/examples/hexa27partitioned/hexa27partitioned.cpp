/*
 * Example C++ program to write a partitioned binary grid and solution
 * datafiles for tecplot. This example does the following:
 *
 *   1.  Open a datafile called "hexa27partitioned.szplt"
 *       and "hexa27partitioned_solution.szplt"
 *   2.  Assign values for x, y, z to the grid and p to the solution.
 *   3.  Write out a hexahedral (hexa27) zone in 3 partitions.
 *   4.  Close the datafiles
 *
 * If TECIOMPI is #defined, this program may be executed with mpiexec with
 * up to 3 MPI ranks (processes). In this case, it must be linked
 * with the MPI version of TECIO.
 */
#define INCLUDEGHOSTCELLS

#if defined TECIOMPI
#include <mpi.h>
#endif

#include "TECIO.h"
#include <assert.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>

// for testing
#include <iostream>
using namespace std;

// internal testing
// RUNFLAGS:none
// RUNFLAGS:--num-solution-files 5

#ifndef NULL
#define NULL 0
#endif

/* The zone will appear the same as an ordered zone with these dimensions for corner nodes: */
#define XDIM 23
#define YDIM 19
#define ZDIM 15

enum fileType_e { FULL = 0, GRID = 1, SOLUTION = 2 };

/**
 * Open the file and write the tecplot datafile header information.
 */
static INTEGER4 initializeFile(
    #if defined TECIOMPI
    MPI_Comm mpiComm,
    #endif
    int      fOffset,
    int      numSolutionFiles,
    int      numPartitions,
    double   solTime,
    void*    gridFileHandle,
    void**   outputFileHandle)
{
    INTEGER4 returnValue = 0;

    INTEGER4 fileFormat = 1; /* 1 == SZPLT (cannot output partitioned zones to PLT) */
    INTEGER4 debug = 1;
    INTEGER4 vIsDouble = 0;

    if (returnValue == 0)
    {
        INTEGER4 dataFileType;
        std::ostringstream fileName;
        char const* baseFileName = numPartitions > 0 ? "hexa27partitioned" : "hexa27";
        char const* varNames;
        if (numSolutionFiles == 0)
        {
            dataFileType = FULL;
            fileName << baseFileName << ".szplt";
            varNames = "x y z p";
        }
        else if (fOffset == 0)
        {
            dataFileType = GRID;
            fileName << baseFileName << "_grid.szplt";
            varNames = "x y z";
        }
        else
        {
            dataFileType = SOLUTION;
            if (numSolutionFiles == 1)
                fileName << baseFileName << "_solution.szplt";
            else
                fileName << baseFileName << "_solution_" << std::setfill('0') << std::setw(4) << static_cast<int>(std::ceil(solTime)) << ".szplt";
            varNames = "p";
        }

        INTEGER4 defaultVarType = 1;    // float

        returnValue = tecFileWriterOpen(
            fileName.str().c_str(),
            (char*)"SIMPLE DATASET",
            varNames,  // NOTE: Make sure and change valueLocation above if this changes.
            fileFormat,
            dataFileType,
            defaultVarType,
            gridFileHandle,
            outputFileHandle);
    }

    #if defined TECIOMPI
    {
        if (returnValue == 0)
        {
            INTEGER4 mainRank = 0;
            returnValue = tecMPIInitialize(
                *outputFileHandle,
                mpiComm,
                mainRank);
        }
    }
    #endif

    return returnValue;
}

/**
 */
static INTEGER4 createZones(
    int             fOffset,
    INTEGER4        numPartitions,
    INTEGER4 const* partitionOwners,
    double          solTime,
    int32_t*        outputZone,
    void*           outputFileHandle)
{
    INTEGER4 returnValue = 0;

    INTEGER4 zoneType  = 5;      /* hexa27 */
    INTEGER4 nNodes = XDIM * YDIM * ZDIM; /* Overall zone dimensions for HEXA27 */
    INTEGER4 nCells = (XDIM - 1) * (YDIM - 1) * (ZDIM - 1) / 8;
    int32_t cellShapes[] = { 4 };   // HEXA
    int32_t gridOrders[] = { 2 };
    int32_t basisFns[] = { 0 };
    int64_t numElementsPerSection[] = { nCells };

    INTEGER4 nFaces    = 6;         /* Not used */
    INTEGER4 iCellMax  = 0;
    INTEGER4 jCellMax  = 0;
    INTEGER4 kCellMax  = 0;
    INTEGER4 strandID  = 1;
    INTEGER4 unused    = 0;      // ParentZone is no longer used
    INTEGER4 isBlock   = 1;      /* Block */
    INTEGER4 nFConns   = 0;
    INTEGER4 fNMode    = 0;
    INTEGER4 valueLocations[4] = {1, 1, 1, 0}; /* 1 = Nodal, 0 = Cell-Centered */
    INTEGER4 shrConn   = 0;

    std::ostringstream zoneTitle;
    char const* baseTitle = numPartitions > 0 ? "Partitioned Zone" : "Zone";
    zoneTitle << baseTitle << " Time=" << static_cast<int>(std::ceil(solTime));
    int const baseVarOffset = fOffset == 0 ? 0 : 3;

    if (returnValue == 0)
        returnValue = tecZoneCreateFEMixed(
            outputFileHandle,
            zoneTitle.str().c_str(),
            nNodes,
            1,          // numSections,
            cellShapes,
            gridOrders,
            basisFns,
            numElementsPerSection,
            NULL,      // varTypes,
            NULL,      // shareVarFromZone,
            &valueLocations[baseVarOffset],
            NULL,      // passiveVarList,
            shrConn,           //  shareConnectivityFromZone,
            nFaces,                 //  numFaceConnections
            fNMode,
            outputZone);

    if (returnValue == 0)
        returnValue = tecZoneSetUnsteadyOptions(
            outputFileHandle,
            *outputZone,
            solTime,
            strandID);

    if (returnValue == 0)
    {
        #if defined TECIOMPI
        {
            /* Output partitions */
            if (numPartitions > 0 && returnValue == 0)
                returnValue = tecZoneMapPartitionsToMPIRanks(
                    outputFileHandle,
                    *outputZone, // Number may change due to communication with the main output rank
                    numPartitions,
                    partitionOwners);
        }
        #endif
    }

    return returnValue;
}

/**
 */
static void appendGhostItems(
    INTEGER4* nGhosts, INTEGER4** ghosts, INTEGER4** gPartitions, INTEGER4** gPGhosts, INTEGER4 ptn,
    INTEGER4 gIDim, INTEGER4 gJDim,
    INTEGER4 gIStart, INTEGER4 gIEnd, INTEGER4 gJStart, INTEGER4 gJEnd, INTEGER4 gKStart, INTEGER4 gKEnd,
    INTEGER4 oIDim, INTEGER4 oJDim,
    INTEGER4 oIStart, INTEGER4 oIEnd, INTEGER4 oJStart, INTEGER4 oJEnd, INTEGER4 oKStart, INTEGER4 oKEnd)
{
    INTEGER4 localNumGhosts = (gIEnd - gIStart) * (gJEnd - gJStart) * (gKEnd - gKStart);
    INTEGER4 gOffset = *nGhosts;
    *nGhosts += localNumGhosts;
    *ghosts = (INTEGER4*)realloc(*ghosts, *nGhosts * sizeof(INTEGER4));
    if (gPartitions)
        *gPartitions = (INTEGER4*)realloc(*gPartitions, *nGhosts * sizeof(INTEGER4));
    if (gPGhosts)
        *gPGhosts = (INTEGER4*)realloc(*gPGhosts, *nGhosts * sizeof(INTEGER4));

    for(INTEGER4 i = gIStart; i < gIEnd; ++i)
    for(INTEGER4 j = gJStart; j < gJEnd; ++j)
    for(INTEGER4 k = gKStart; k < gKEnd; ++k)
    {
        (*ghosts)[gOffset] = (k * gJDim + j) * gIDim + i + 1;
        if (gPartitions)
            (*gPartitions)[gOffset] = ptn;
        if (gPGhosts)
        {
            INTEGER4 oI = i - gIStart + oIStart;
            INTEGER4 oJ = j - gJStart + oJStart;
            INTEGER4 oK = k - gKStart + oKStart;
            (*gPGhosts)[gOffset] = (oK * oJDim + oJ) * oIDim + oI + 1;
        }
        ++gOffset;
    }
}


/**
 */
static void GatherGhostNodesAndCells(
    INTEGER4* nGNodes, INTEGER4** ghostNodes, INTEGER4** gNPartitions, INTEGER4** gNPNodes,
    INTEGER4* nGCells, INTEGER4** ghostCells, int* iDim, int* jDim, int* kDim, int* jMin, int* jMax)
{
    /*
     * Assemble lists of ghost nodes and cells--nodes and cells near partition boundaries that
     * coincide with those "owned" by neighboring partitions. For each set of coincident nodes
     * or cells, exactly one partition must own the node or cell, and all other involved partitions
     * must report it as a ghost node or cell.
     *
     * Arbitrarily, we say that the first partition owns any nodes that do not overlay the interior
     * of neighboring partitions. That is, it owns any nodes that its "real" (non-ghost) cells use.
     * So only a single layer of nodes and cells--on its IMax boundary--are ghosts, and are owned
     * by the second and third partitions. We use the same logic to assign ownership for nodes
     * shared by partitions 2 and 3.
     */
    /* No ghost nodes or cells for partition 1 (it owns all it's nodes and cells). */
    nGNodes[0] = 0;
    ghostNodes[0] = NULL;
    gNPartitions[0] = NULL;
    gNPNodes[0] = NULL;
    nGCells[0] = 0;
    ghostCells[0] = NULL;

    nGNodes[1] = 0;
    ghostNodes[1] = NULL;
    gNPartitions[1] = NULL;
    gNPNodes[1] = NULL;
    nGCells[1] = 0;            // No ghost cells
    ghostCells[1] = NULL;
#if defined INCLUDEGHOSTCELLS
    /* Nodes owned by the first partition. */
    appendGhostItems(
        &nGNodes[1], &ghostNodes[1], &gNPartitions[1], &gNPNodes[1], 1,
        iDim[1], jDim[1],                                  /* I- and J-dimensions */
        0, 3, 0, jDim[1], 0, kDim[1],                      /* local index ranges */
        iDim[0], jDim[0],                                  /* I- and J-dimensions */
        iDim[0] - 3, iDim[0], 0, jDim[1], 0, kDim[0]);        /* local index ranges */
    /* Cells owned by the first partition. */
    int iCellDim0 = iDim[0] / 2;
    int jCellDim0 = jDim[0] / 2;
    int kCellDim0 = kDim[0] / 2;
    int iCellDim1 = iDim[1] / 2;
    int jCellDim1 = jDim[1] / 2;
    int kCellDim1 = kDim[1] / 2;
    appendGhostItems(
        &nGCells[1], &ghostCells[1], NULL, NULL, 1,
        iCellDim1, jCellDim1,                                  /* I- and J-dimensions */
        0, 1, 0, jCellDim1, 0, kCellDim1,                      /* local index ranges */
        iCellDim0, jCellDim0,                                  /* I- and J-dimensions */
        iCellDim0 - 1, iCellDim0, 0, jCellDim1, 0, kCellDim0); /* local index ranges */
#else
    /* Nodes owned by the first partition. */
    appendGhostItems(
        &nGNodes[1], &ghostNodes[1], &gNPartitions[1], &gNPNodes[1], 1,
        iDim[1], jDim[1],                                  /* I- and J-dimensions */
        0, 1, 0, jDim[1], 0, kDim[1],                      /* local index ranges */
        iDim[0], jDim[0],                                  /* I- and J-dimensions */
        iDim[0] - 1, iDim[0], 0, jDim[1], 0, kDim[0]);        /* local index ranges */
#endif

    nGNodes[2] = 0;
    ghostNodes[2] = NULL;
    gNPartitions[2] = NULL;
    gNPNodes[2] = NULL;
    nGCells[2] = 0;            // No ghost cells
    ghostCells[2] = NULL;
    /* Nodes owned by the first partition */
    appendGhostItems(
        &nGNodes[2], &ghostNodes[2], &gNPartitions[2], &gNPNodes[2], 1,
        iDim[2], jDim[2],                                        /* I- and J-dimensions */
        0, 1, 0, jDim[2], 0, kDim[2],                            /* local index ranges */
        iDim[0], jDim[0],                                        /* I- and J-dimensions */
        iDim[0] - 1, iDim[0], jMin[2], jMax[2], 0, kDim[0]);     /* local index ranges */
    /* Nodes owned by the second partition. */
#if defined INCLUDEGHOSTCELLS
    appendGhostItems(
        &nGNodes[2], &ghostNodes[2], &gNPartitions[2], &gNPNodes[2], 2,
        iDim[2], jDim[2],                                  /* I- and J-dimensions */
        1, iDim[2], 0, 1, 0, kDim[2],                      /* local index ranges */
        iDim[1], jDim[1],                                  /* I- and J-dimensions */
        3, iDim[1], jDim[1] - 1, jDim[1], 0, kDim[1]);     /* local index ranges */
#else
    appendGhostItems(
        &nGNodes[2], &ghostNodes[2], &gNPartitions[2], &gNPNodes[2], 2,
        iDim[2], jDim[2],                                  /* I- and J-dimensions */
        1, iDim[2], 0, 1, 0, kDim[2],                      /* local index ranges */
        iDim[1], jDim[1],                                  /* I- and J-dimensions */
        1, iDim[1], jDim[1] - 1, jDim[1], 0, kDim[1]);     /* local index ranges */
#endif
}

/**
 */
static INTEGER4 createData(
    #if defined TECIOMPI
    int             commRank,
    #endif
    int             fOffset,
    int             numSolutionFiles,
    INTEGER4        numPartitions,
    INTEGER4 const* partitionOwners,
    double          solTime,
    int             outputZone,
    void*           outputFileHandle)
{
    INTEGER4 returnValue = 0;

    // Add aux data to solution files; for MPI, only the main output rank may do this.
    if (numSolutionFiles == 0 || fOffset > 0)
    {
        std::ostringstream auxDataValue;
        auxDataValue << fOffset;
        #if defined TECIOMPI
            if (commRank == 0)
        #endif
        // returnValue = TECZAUXSTR142("TimeStep", auxDataValue.str().c_str());
        returnValue = tecZoneAddAuxData(
                outputFileHandle,
                outputZone,
                "TimeStep",
                auxDataValue.str().c_str());
    }

    /*
     * If the file isn't partitioned, we still allocate out array resources as if there was a single
     * partition as it makes the code between partitioned and non-partitioned the same.
     */
    INTEGER4 const effectiveNumPartitions = numPartitions > 0 ? numPartitions : 1;

    /*
     * Divide the zone into number of partitions, identified by the index ranges
     * of an equivalent unpartitioned ordered zone.
     */
    int* iMin = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* iMax = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* jMin = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* jMax = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* kMin = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* kMax = (int*)malloc(effectiveNumPartitions * sizeof(int));

    assert(numPartitions == 0 || numPartitions == 3); /* ...because this example only handles on or the other */
    if (numPartitions > 0)
    {
        /* Partition 1 node range, which will include one layer of ghost cells on the IMAX boundary: */
        iMin[0] = 0; /* We use zero-based indices here because it simplifies index arithmetic in C. */
        iMax[0] = XDIM / 2 + 2; // XDIM / 2 + 4; // XDIM / 2 + 2;
        jMin[0] = 0;
        jMax[0] = YDIM;
        kMin[0] = 0;
        kMax[0] = ZDIM;

        /* Partition 2; ghost cells on IMIN and JMAX boundaries: */
        iMin[1] = iMax[0] - 1; // iMax[0] - 5; // iMax[0] - 3;
        iMax[1] = XDIM;
        jMin[1] = jMin[0];
        jMax[1] = YDIM / 2 + 2;
        kMin[1] = kMin[0];
        kMax[1] = kMax[0];

        /* Partition 3; ghost cells on IMIN and JMIN boundaries: */
        iMin[2] = iMin[1];
        iMax[2] = iMax[1];
        jMin[2] = jMax[1] - 1; // jMax[1] - 3;
        jMax[2] = YDIM;
        kMin[2] = kMin[1];
        kMax[2] = kMax[1];
#if defined INCLUDEGHOSTCELLS
        iMin[1] -= 2;
#endif
    }
    else
    {
        iMin[0] = 0;
        iMax[0] = XDIM;
        jMin[0] = 0;
        jMax[0] = YDIM;
        kMin[0] = 0;
        kMax[0] = ZDIM;
    }

    /* Local partition dimensions (of equivalent ordered zones) */
    int* iDim = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* jDim = (int*)malloc(effectiveNumPartitions * sizeof(int));
    int* kDim = (int*)malloc(effectiveNumPartitions * sizeof(int));
    for(int ii = 0; ii < effectiveNumPartitions; ++ii)
    {
        iDim[ii] = iMax[ii] - iMin[ii];
        jDim[ii] = jMax[ii] - jMin[ii];
        kDim[ii] = kMax[ii] - kMin[ii];
    }

    /* Calculate variable and connectivity values for partitions. */
    int** connectivity = (int**)malloc(effectiveNumPartitions * sizeof(int*));
    float** x = (float**)malloc(effectiveNumPartitions * sizeof(float*));
    float** y = (float**)malloc(effectiveNumPartitions * sizeof(float*));
    float** z = (float**)malloc(effectiveNumPartitions * sizeof(float*));
    float** p = (float**)malloc(effectiveNumPartitions * sizeof(float*));

    /* Partition node and cell counts, including ghost items */
    INTEGER4* pNNodes = (INTEGER4*)malloc(effectiveNumPartitions * sizeof(INTEGER4));
    INTEGER4* pNCells = (INTEGER4*)malloc(effectiveNumPartitions * sizeof(INTEGER4));

    for (INTEGER4 ptn = 0; ptn < effectiveNumPartitions; ++ptn)
    {
        pNNodes[ptn] = iDim[ptn] * jDim[ptn] * kDim[ptn];
        if (fOffset == 0)
        {
            /* create grid variables */
            x[ptn] = (float*)malloc(pNNodes[ptn] * sizeof(float));
            y[ptn] = (float*)malloc(pNNodes[ptn] * sizeof(float));
            z[ptn] = (float*)malloc(pNNodes[ptn] * sizeof(float));
            for (int k = 0; k < kDim[ptn]; ++k)
            for (int j = 0; j < jDim[ptn]; ++j)
            for (int i = 0; i < iDim[ptn]; ++i)
            {
                int index = (k * jDim[ptn] + j) * iDim[ptn] + i;

                x[ptn][index] = (float)(i + iMin[ptn] + 1);
                y[ptn][index] = (float)(j + jMin[ptn] + 1);
                z[ptn][index] = (float)(k + kMin[ptn] + 1);
            }
        }
        else
        {
            x[ptn] = 0;
            y[ptn] = 0;
            z[ptn] = 0;
        }

        /* p (cell-centered) and connectivity */
        pNCells[ptn] = (iDim[ptn] - 1) * (jDim[ptn] - 1) * (kDim[ptn] - 1) / 8;
        p[ptn] = (float*)malloc(pNCells[ptn] * sizeof(float));

        if (fOffset == 0)
        {
            int connectivityCount = 27 * pNCells[ptn];
            connectivity[ptn] = (INTEGER4*)malloc(connectivityCount * sizeof(INTEGER4));
        }
        else
        {
            connectivity[ptn] = 0;
        }

        int32_t cornersIndex = 0;   // Index in connectivity array of the first corner node of the cell
        int32_t cellIndex = 0;
        for (int k = 0; k < kDim[ptn] - 1; k += 2)
        for (int j = 0; j < jDim[ptn] - 1; j += 2)
        for (int i = 0; i < iDim[ptn] - 1; i += 2)
        {
            int kc = k / 2;
            int jc = j / 2;
            int ic = i / 2;
            int iGhostCellBuffer = 0;
#if defined INCLUDEGHOSTCELLS
            if (ptn == 1)
               iGhostCellBuffer = -1;
#endif
            p[ptn][cellIndex] = (float)(((i + iMin[ptn]) / 2 + 1 + iGhostCellBuffer) * ((j + jMin[ptn]) / 2 + 1) * ((k + kMin[ptn]) / 2 + 1) + solTime);  // Is this correct?

            if (fOffset == 0)
            {
                // Lower corners
                connectivity[ptn][cornersIndex] = (k * jDim[ptn] + j) * iDim[ptn] + i + 1;
                connectivity[ptn][cornersIndex + 1] = connectivity[ptn][cornersIndex] + 2;
                connectivity[ptn][cornersIndex + 2] = connectivity[ptn][cornersIndex] + 2 * iDim[ptn] + 2;
                connectivity[ptn][cornersIndex + 3] = connectivity[ptn][cornersIndex] + 2 * iDim[ptn];
                // Upper corners
                connectivity[ptn][cornersIndex + 4] = connectivity[ptn][cornersIndex] + 2 * iDim[ptn] * jDim[ptn];
                connectivity[ptn][cornersIndex + 5] = connectivity[ptn][cornersIndex + 1] + 2 * iDim[ptn] * jDim[ptn];
                connectivity[ptn][cornersIndex + 6] = connectivity[ptn][cornersIndex + 2] + 2 * iDim[ptn] * jDim[ptn];
                connectivity[ptn][cornersIndex + 7] = connectivity[ptn][cornersIndex + 3] + 2 * iDim[ptn] * jDim[ptn];

                int32_t hoeIndex = cornersIndex + 8;   // Index in connectivity array of the first hoe node of the cell
                // Lower edge nodes
                connectivity[ptn][hoeIndex] = connectivity[ptn][cornersIndex] + 1;
                connectivity[ptn][hoeIndex + 1] = connectivity[ptn][cornersIndex] + iDim[ptn] + 2;
                connectivity[ptn][hoeIndex + 2] = connectivity[ptn][cornersIndex] + 2 * iDim[ptn] + 1;
                connectivity[ptn][hoeIndex + 3] = connectivity[ptn][cornersIndex] + iDim[ptn];
                // Ascending edge nodes
                connectivity[ptn][hoeIndex + 4] = connectivity[ptn][cornersIndex] + iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 5] = connectivity[ptn][cornersIndex + 1] + iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 6] = connectivity[ptn][cornersIndex + 2] + iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 7] = connectivity[ptn][cornersIndex + 3] + iDim[ptn] * jDim[ptn];
                // Upper edge nodes
                connectivity[ptn][hoeIndex + 8] = connectivity[ptn][hoeIndex] + 2 * iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 9] = connectivity[ptn][hoeIndex + 1] + 2 * iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 10] = connectivity[ptn][hoeIndex + 2] + 2 * iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 11] = connectivity[ptn][hoeIndex + 3] + 2 * iDim[ptn] * jDim[ptn];
                // Lower face node
                connectivity[ptn][hoeIndex + 12] = connectivity[ptn][cornersIndex] + iDim[ptn] + 1;
                // Vertical face nodes
                connectivity[ptn][hoeIndex + 13] = connectivity[ptn][hoeIndex] + iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 14] = connectivity[ptn][hoeIndex + 1] + iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 15] = connectivity[ptn][hoeIndex + 2] + iDim[ptn] * jDim[ptn];
                connectivity[ptn][hoeIndex + 16] = connectivity[ptn][hoeIndex + 3] + iDim[ptn] * jDim[ptn];
                // Upper face node
                connectivity[ptn][hoeIndex + 17] = connectivity[ptn][hoeIndex + 12] + 2 * iDim[ptn] * jDim[ptn];
                // Cell-center node
                connectivity[ptn][hoeIndex + 18] = connectivity[ptn][hoeIndex + 12] + iDim[ptn] * jDim[ptn];

                cornersIndex += 27;
            }
            cellIndex++;
        }
    }



    INTEGER4*  nGNodes      = NULL;
    INTEGER4*  nGCells      = NULL;
    INTEGER4** ghostNodes   = NULL;
    INTEGER4** gNPartitions = NULL;
    INTEGER4** gNPNodes     = NULL;
    INTEGER4** ghostCells   = NULL;

    if (numPartitions > 0)
    {
        /* Partition ghost node and ghost cell counts */
        nGNodes      = (INTEGER4*)malloc(numPartitions * sizeof(INTEGER4));
        nGCells      = (INTEGER4*)malloc(numPartitions * sizeof(INTEGER4));

        ghostNodes   = (INTEGER4**)malloc(numPartitions * sizeof(INTEGER4*));
        gNPartitions = (INTEGER4**)malloc(numPartitions * sizeof(INTEGER4*));
        gNPNodes     = (INTEGER4**)malloc(numPartitions * sizeof(INTEGER4*));
        ghostCells   = (INTEGER4**)malloc(numPartitions * sizeof(INTEGER4*));

        GatherGhostNodesAndCells(nGNodes, ghostNodes, gNPartitions, gNPNodes, nGCells, ghostCells, iDim, jDim, kDim, jMin, jMax);
    }

    INTEGER4 dIsDouble = 0;
    for(INTEGER4 ptn = 1; ptn <= effectiveNumPartitions; ++ptn)
    {
        #if defined TECIOMPI
        if (numPartitions == 0 || partitionOwners[ptn - 1] == commRank)
        #endif
        {
            if (numPartitions > 0)
            {
                if (returnValue == 0)
                    returnValue = tecFEPartitionCreate32(
                        outputFileHandle,
                        outputZone,
                        ptn,
                        pNNodes[ptn - 1],
                        pNCells[ptn - 1],
                        nGNodes[ptn - 1],
                        ghostNodes[ptn - 1],
                        gNPartitions[ptn - 1],
                        gNPNodes[ptn - 1],
                        nGCells[ptn - 1],
                        ghostCells[ptn - 1]);


                free(ghostNodes[ptn - 1]);
                free(gNPartitions[ptn - 1]);
                free(gNPNodes[ptn - 1]);
                free(ghostCells[ptn - 1]);
            }

            if (fOffset == 0)
            {
                /* write out the grid field data */
                if (returnValue == 0)
                    returnValue = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 1, ptn, pNNodes[ptn - 1], x[ptn - 1]);
                if (returnValue == 0)
                    returnValue = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 2, ptn, pNNodes[ptn - 1], y[ptn - 1]);
                if (returnValue == 0)
                    returnValue = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 3, ptn, pNNodes[ptn - 1], z[ptn - 1]);
            }

            if (fOffset != 0 || numSolutionFiles == 0)
            {
                /* write out the solution variable */
                int32_t const solutionVar = numSolutionFiles > 0
                    ? 1  // solution variable is written to the solution file
                    : 4; // solution variable is being written to the grid file
                if (returnValue == 0)
                    returnValue = tecZoneVarWriteFloatValues(outputFileHandle, outputZone,
                        solutionVar, ptn, pNCells[ptn - 1], p[ptn - 1]);
            }

            if (fOffset == 0)
            {
                /* write out the connectivity */ 
                INTEGER4 connectivityCount = 27 * pNCells[ptn - 1];

                int32_t nodesAreOneBased = 1;
                int64_t connectivityCount64 = connectivityCount;
                if (returnValue == 0)
                    returnValue = tecZoneNodeMapWrite32(
                        outputFileHandle,
                        outputZone,
                        ptn,               // partition
                        nodesAreOneBased,
                        connectivityCount64,
                        connectivity[ptn - 1]);
            }

            if (fOffset == 0)
            {
                free(x[ptn - 1]);
                free(y[ptn - 1]);
                free(z[ptn - 1]);
                free(connectivity[ptn - 1]);
            }
            free(p[ptn - 1]);
        }
    }

    free(iMin);
    free(iMax);
    free(jMin);
    free(jMax);
    free(kMin);
    free(kMax);

    free(iDim);
    free(jDim);
    free(kDim);

    free(connectivity);
    free(x);
    free(y);
    free(z);
    free(p);

    free(pNNodes);
    free(pNCells);

    free(nGNodes);
    free(nGCells);
    free(ghostNodes);
    free(gNPartitions);
    free(gNPNodes);
    free(ghostCells);

    return returnValue;
}

/**
 */
static INTEGER4 finalizeFile(void* outputFileHandle)
{
    return tecFileWriterClose(&outputFileHandle);
}

/**
 */
int main(int argc, char** argv)
{
    int returnValue = 0;

    #if defined TECIOMPI
    {
        MPI_Init(&argc, &argv);
    }
    #endif

    char const* NUM_SOLUTION_FILES_FLAG = "--num-solution-files";
    char* endptr = 0;
    int numSolutionFiles = 3;
    if (argc == 3 && (strcmp(argv[1], NUM_SOLUTION_FILES_FLAG) == 0 &&
                      (strtol(argv[2], &endptr, 10) >= 0 && *endptr == 0)))
    {
        numSolutionFiles = static_cast<int>(strtol(argv[2], &endptr, 10));
    }
    else if (argc != 1)
    {
        returnValue = 1;
        fprintf(stderr, "%s: Expected \"%s <count>, where <count> is zero or greater.\n",
                argv[0], NUM_SOLUTION_FILES_FLAG);
    }

    /*
     * Open the file(s) and write the tecplot datafile header information.
     */
    INTEGER4 const numPartitions = 3; /* 0: non-partitioned file; 3: partitioned */
    assert(numPartitions == 0 || numPartitions == 3); /* ...because this example only handles on or the other */
    INTEGER4* partitionOwners = NULL;
    if (numPartitions > 0)
        partitionOwners = (INTEGER4*)malloc(numPartitions * sizeof(INTEGER4));

    #if defined TECIOMPI
    MPI_Comm mpiComm = MPI_COMM_WORLD;
    int commRank = 0;

    if (returnValue == 0)
    {
        int commSize = 0;
        MPI_Comm_size(mpiComm, &commSize);
        MPI_Comm_rank(mpiComm, &commRank);
        if (commSize == 0)
        {
            returnValue = 1; /* error */
        }
        else
        {
            /* create partition owners */
            for(int ptn = 0; ptn < numPartitions; ++ptn)
                partitionOwners[ptn] = (ptn % commSize);
        }
    }
    #endif

    int const numOutputFiles = 1 + numSolutionFiles;
    void* gridFileHandle = NULL;
    for (int fOffset = 0; returnValue == 0 && fOffset < numOutputFiles; ++fOffset)
    {
        double const solTime = static_cast<double>(360 + (fOffset == 0 ? 0 : fOffset-1));

        void* outputFileHandle = NULL;

        if (returnValue == 0)
            returnValue = initializeFile(
                #if defined TECIOMPI
                mpiComm,
                #endif
                fOffset, numSolutionFiles, numPartitions, solTime, gridFileHandle, &outputFileHandle);

        if (numSolutionFiles > 0 && fOffset == 0)
            gridFileHandle = outputFileHandle;

        /*
         * Create zones.
         */
        int32_t outputZone{ 0 };
        if (returnValue == 0)
            returnValue = createZones(fOffset, numPartitions, partitionOwners, solTime, &outputZone, outputFileHandle);

        /*
         * Create the connectivity and variable data.
         */
        if (returnValue == 0)
            returnValue = createData(
                #if defined TECIOMPI
                commRank,
                #endif
                fOffset, numSolutionFiles, numPartitions, partitionOwners, solTime, outputZone, outputFileHandle);

        if (returnValue == 0 && gridFileHandle != outputFileHandle)
            returnValue = finalizeFile(outputFileHandle);
    }

    if (returnValue == 0 && gridFileHandle)
        returnValue = finalizeFile(gridFileHandle);

    #if defined TECIOMPI
    {
        MPI_Finalize();
    }
    #endif


    free(partitionOwners);

    return returnValue;
}
