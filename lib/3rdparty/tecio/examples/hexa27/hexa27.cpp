/*
 * Simple example c program to write a
 * binary datafile for tecplot.  This example
 * does the following:
 *
 *   1.  Open a datafile called "hexa27.szplt"
 *   2.  Assign values for x,y, and p.
 *   3.  Write out a hexahedral (hexa27) zone.
 *   4.  Close the datafile.
 */

// Internal testing flags
// RUNFLAGS:none
// RUNFLAGS:--szl

#include "TECIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef NULL
#define NULL 0
#endif

/* The zone will appear the same as an ordered zone with these dimensions: */
#define XDIM 9
#define YDIM 7
#define ZDIM 5

enum fileType_e { FULL = 0, GRID = 1, SOLUTION = 2 };

int main(int argc, const char* argv[])
{
    float* x, * y, * z, * p, * cc;
    int* connectivity;
    double solTime;
    INTEGER4 debug, i, j, k, dIsDouble, vIsDouble, zoneType, strandID, unused, isBlock, defaultVarType;
    INTEGER4 iCellMax, jCellMax, kCellMax, nFConns, fNMode, shrConn, fileType;
    INTEGER4 nNodes, nCells, connectivityCount, index;
    INTEGER8 numNodes, numCells;
    int valueLocation[] = { 1,1,1,1,0 };

    INTEGER4 fileFormat = 1; // 0 == PLT, 1 == SZPLT - currently only support SZPLT
#if(0)  // Currently only support SZPLT
    if (argc == 2 && strncmp(argv[1], "--szl", 5) == 0)
        fileFormat = 1;
    else
        fileFormat = 0;
#endif

    debug = 1;
    vIsDouble = 0;
    dIsDouble = 0;
    nNodes = XDIM * YDIM * ZDIM;
    nCells = (XDIM - 1) * (YDIM - 1) * (ZDIM - 1);
    int32_t numHexa27Cells = (XDIM - 1) * (YDIM - 1) * (ZDIM - 1) / 8;
    numNodes = XDIM * YDIM * ZDIM;
    numCells = (XDIM - 1) * (YDIM - 1) * (ZDIM - 1);

    zoneType = 5;      /* hexa27 */
    solTime = 360.0;
    strandID = 0;     /* StaticZone */
    unused = 0;     // ParentZone is no longer used
    isBlock = 1;      /* Block */
    iCellMax = 0;
    jCellMax = 0;
    kCellMax = 0;
    nFConns = 0;
    fNMode = 0;
    shrConn = 0;
    fileType = FULL;
    defaultVarType = 1;    // float

    /*
     * Open the file and write the tecplot datafile
     * header information
     */
    void* outputFileHandle = NULL;
    i = tecFileWriterOpen(
        (char*)"hexa27",
        (char*)"SIMPLE DATASET",
        (char*)"x y z p cc",  // NOTE: Make sure and change valueLocation above if this changes.
        fileFormat,
        fileType,
        defaultVarType,
        NULL,             // gridFileHandle
        &outputFileHandle);

    x = (float*)malloc(nNodes * sizeof(float));
    y = (float*)malloc(nNodes * sizeof(float));
    z = (float*)malloc(nNodes * sizeof(float));
    p = (float*)malloc(nNodes * sizeof(float));
    cc = (float*)malloc(nCells * sizeof(float));
    for (k = 0; k < ZDIM; k++)
        for (j = 0; j < YDIM; j++)
            for (i = 0; i < XDIM; i++)
            {
                index = (k * YDIM + j) * XDIM + i;
                x[index] = (float)(i + 1);
                y[index] = (float)(j + 1);
                z[index] = (float)(k + 1);
                p[index] = (float)((i + 1) * (j + 1) * (k + 1));
            }

    for (i = 0; i < numHexa27Cells; ++i)
        cc[i] = (float)i;

    connectivityCount = 27 * numHexa27Cells;
    connectivity = (INTEGER4*)malloc(connectivityCount * sizeof(INTEGER4));

    int32_t cornersIndex = 0;   // Index in connectivity array of the first corner node of the cell
    for (k = 0; k < ZDIM - 1; k += 2)
        for (j = 0; j < YDIM - 1; j += 2)
            for (i = 0; i < XDIM - 1; i += 2)
            {
                // Lower corners
                connectivity[cornersIndex] = (k * YDIM + j) * XDIM + i + 1;
                connectivity[cornersIndex + 1] = connectivity[cornersIndex] + 2;
                connectivity[cornersIndex + 2] = connectivity[cornersIndex] + 2 * XDIM + 2;
                connectivity[cornersIndex + 3] = connectivity[cornersIndex] + 2 * XDIM;
                // Upper corners
                connectivity[cornersIndex + 4] = connectivity[cornersIndex] + 2 * XDIM * YDIM;
                connectivity[cornersIndex + 5] = connectivity[cornersIndex + 1] + 2 * XDIM * YDIM;
                connectivity[cornersIndex + 6] = connectivity[cornersIndex + 2] + 2 * XDIM * YDIM;
                connectivity[cornersIndex + 7] = connectivity[cornersIndex + 3] + 2 * XDIM * YDIM;

                int32_t hoeIndex = cornersIndex + 8;   // Index in connectivity array of the first hoe node of the cell
                // Lower edge nodes
                connectivity[hoeIndex] = connectivity[cornersIndex] + 1;
                connectivity[hoeIndex + 1] = connectivity[cornersIndex] + XDIM + 2;
                connectivity[hoeIndex + 2] = connectivity[cornersIndex] + 2 * XDIM + 1;
                connectivity[hoeIndex + 3] = connectivity[cornersIndex] + XDIM;
                // Ascending edge nodes
                connectivity[hoeIndex + 4] = connectivity[cornersIndex] + XDIM * YDIM;
                connectivity[hoeIndex + 5] = connectivity[cornersIndex + 1] + XDIM * YDIM;
                connectivity[hoeIndex + 6] = connectivity[cornersIndex + 2] + XDIM * YDIM;
                connectivity[hoeIndex + 7] = connectivity[cornersIndex + 3] + XDIM * YDIM;
                // Upper edge nodes
                connectivity[hoeIndex + 8] = connectivity[hoeIndex] + 2 * XDIM * YDIM;
                connectivity[hoeIndex + 9] = connectivity[hoeIndex + 1] + 2 * XDIM * YDIM;
                connectivity[hoeIndex + 10] = connectivity[hoeIndex + 2] + 2 * XDIM * YDIM;
                connectivity[hoeIndex + 11] = connectivity[hoeIndex + 3] + 2 * XDIM * YDIM;
                // Lower face node
                connectivity[hoeIndex + 12] = connectivity[cornersIndex] + XDIM + 1;
                // Vertical face nodes
                connectivity[hoeIndex + 13] = connectivity[hoeIndex] + XDIM * YDIM;
                connectivity[hoeIndex + 14] = connectivity[hoeIndex + 1] + XDIM * YDIM;
                connectivity[hoeIndex + 15] = connectivity[hoeIndex + 2] + XDIM * YDIM;
                connectivity[hoeIndex + 16] = connectivity[hoeIndex + 3] + XDIM * YDIM;
                // Upper face node
                connectivity[hoeIndex + 17] = connectivity[hoeIndex + 12] + 2 * XDIM * YDIM;
                // Cell-center node
                connectivity[hoeIndex + 18] = connectivity[hoeIndex + 12] + XDIM * YDIM;

                cornersIndex += 27;
            }

    /*
     * Write the zone header information.
     */
    int32_t outputZone;
    int32_t cellShapes[] = { 4 };   // HEXA
    int32_t gridOrders[] = { 2 };
    int32_t basisFns[] = { 0 };
    int64_t numElementsPerSection[] = { static_cast<int64_t>(numHexa27Cells) };
    i = tecZoneCreateFEMixed(
        outputFileHandle,
        (char*)"Simple Zone",
        numNodes,
        1,          // numSections,
        cellShapes,
        gridOrders,
        basisFns,
        numElementsPerSection,
        NULL,      // varTypes,
        NULL,      // shareVarFromZone,
        valueLocation,
        NULL,      // passiveVarList,
        shrConn,           //  shareConnectivityFromZone,
        0,                 //  numFaceConnections
        fNMode,
        &outputZone);

    /*
     * Write out the field data.
     */
    i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 1, 0, numNodes, x);
    i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 2, 0, numNodes, y);
    i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 3, 0, numNodes, z);
    i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 4, 0, numNodes, p);
    i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 5, 0, numHexa27Cells, cc);
    free(x);
    free(y);
    free(z);
    free(p);
    free(cc);

    int32_t nodesAreOneBased = 1;
    int64_t connectivityCount64 = connectivityCount;

    i = tecZoneNodeMapWrite32(
        outputFileHandle,
        outputZone,
        0,               // partition
        nodesAreOneBased,
        connectivityCount64,
        connectivity);


    free(connectivity);

    i = tecFileWriterClose(&outputFileHandle);

    return 0;
}
