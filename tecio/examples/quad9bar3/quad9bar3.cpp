/*
 * Simple example c program to write a
 * binary datafile for tecplot.  This example
 * does the following:
 *
 *   1.  Open a datafile called "quad9bar3.szplt"
 *   2.  Assign values for x,y, and p.
 *   3.  Write out a quadratic quadrilateral (quad9) zone.
 *   4.  Write out a quadratic bar (bar3) zone.
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

enum fileType_e { FULL = 0, GRID = 1, SOLUTION = 2 };

int main(int argc, const char* argv[])
{
    float* x, * y, * z, * p, * cc;
    int* connectivity;
    double solTime;
    INTEGER4 debug, i, j, dIsDouble, vIsDouble, strandID, unused, isBlock, defaultVarType;
    INTEGER4 nFConns, fNMode, shrConn, fileType;
    INTEGER4 nNodes, connectivityCount, index;

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

    solTime = 360.0;
    strandID = 0;     /* StaticZone */
    unused = 0;     // ParentZone is no longer used
    isBlock = 1;      /* Block */
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
        (char*)"quad9bar3",
        (char*)"SIMPLE DATASET",
        (char*)"x y z p cc",  // NOTE: Make sure and change valueLocation above if this changes.
        fileFormat,
        fileType,
        defaultVarType,
        NULL,             // gridFileHandle
        &outputFileHandle);

    /* Write header, data, and connectivity for the quad9 zone. */
    if (outputFileHandle != NULL)
    {
        nNodes = XDIM * YDIM;
        int32_t numQuad9Cells = (XDIM - 1) * (YDIM - 1) / 4;

        x = (float*)malloc(nNodes * sizeof(float));
        y = (float*)malloc(nNodes * sizeof(float));
        z = (float*)malloc(nNodes * sizeof(float));
        p = (float*)malloc(nNodes * sizeof(float));
        cc = (float*)malloc(numQuad9Cells * sizeof(float));
        for (j = 0; j < YDIM; j++)
            for (i = 0; i < XDIM; i++)
            {
                index =  j * XDIM + i;
                x[index] = (float)(i + 1);
                y[index] = (float)(j + 1);
                z[index] = 0.0;
                p[index] = (float)((i + 1) * (j + 1));
            }

        for (i = 0; i < numQuad9Cells; ++i)
            cc[i] = (float)i;

        connectivityCount = 9 * numQuad9Cells;
        connectivity = (INTEGER4*)malloc(connectivityCount * sizeof(INTEGER4));

        int32_t cornersIndex = 0;   // Index in connectivity array of the first corner node of the cell
        for (j = 0; j < YDIM - 1; j += 2)
            for (i = 0; i < XDIM - 1; i += 2)
            {
                // Corners
                connectivity[cornersIndex] =  j * XDIM + i + 1;
                connectivity[cornersIndex + 1] = connectivity[cornersIndex] + 2;
                connectivity[cornersIndex + 2] = connectivity[cornersIndex] + 2 * XDIM + 2;
                connectivity[cornersIndex + 3] = connectivity[cornersIndex] + 2 * XDIM;

                int32_t hoeIndex = cornersIndex + 4;   // Index in connectivity array of the first hoe node of the cell
                // Lower edge node
                connectivity[hoeIndex] = connectivity[cornersIndex] + 1;
                // Ascending edge node
                connectivity[hoeIndex + 1] = connectivity[cornersIndex] + XDIM + 2;
                // Upper edge nodes
                connectivity[hoeIndex + 2] = connectivity[cornersIndex] + 2 * XDIM + 1;
                // Descending edge node
                connectivity[hoeIndex + 3] = connectivity[cornersIndex] + XDIM;
                // Cell-center node
                connectivity[hoeIndex + 4] = connectivity[cornersIndex] + XDIM + 1;

                cornersIndex += 9;
            }

        /*
         * Write the zone header information.
         */
        int32_t outputZone;
        int64_t numNodes = nNodes;
        int32_t cellShapes[] = { 2 };   // Quad
        int32_t gridOrders[] = { 2 };
        int32_t basisFns[] = { 0 };
        int64_t numElementsPerSection[] = { static_cast<int64_t>(numQuad9Cells) };
        i = tecZoneCreateFEMixed(
            outputFileHandle,
            (char*)"Simple Quad9 Zone",
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
        i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 5, 0, numQuad9Cells, cc);
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
    }


    /* Write header, data, and connectivity for the bar3 zone. */
    if (outputFileHandle != NULL)
    {
        nNodes = XDIM;
        int32_t numBar3Cells = (XDIM - 1) / 2;

        x = (float*)malloc(nNodes * sizeof(float));
        y = (float*)malloc(nNodes * sizeof(float));
        z = (float*)malloc(nNodes * sizeof(float));
        p = (float*)malloc(nNodes * sizeof(float));
        cc = (float*)malloc(numBar3Cells * sizeof(float));

        for (i = 0; i < XDIM; i++)
        {
            x[i] = (float)(i + 1);
            y[i] = (float)(0.1 * (i + 1) * (i + 1));
            z[i] = (float)(0.05 * (i + 1) * (i + 1));
            p[i] = (float)((i + 1));
        }

        for (i = 0; i < numBar3Cells; ++i)
            cc[i] = (float)i;

        connectivityCount = 3 * numBar3Cells;
        connectivity = (INTEGER4*)malloc(connectivityCount * sizeof(INTEGER4));

        int32_t cornersIndex = 0;   // Index in connectivity array of the first corner node of the cell
        for (i = 0; i < XDIM - 1; i += 2)
        {
            // Corners
            connectivity[cornersIndex] = i + 1;
            connectivity[cornersIndex + 1] = connectivity[cornersIndex] + 2;

            int32_t hoeIndex = cornersIndex + 2;   // Index in connectivity array of the first hoe node of the cell
            // Edge node
            connectivity[hoeIndex] = connectivity[cornersIndex] + 1;

            cornersIndex += 3;
        }

        /*
         * Write the zone header information.
         */
        int32_t outputZone;
        int64_t numNodes = nNodes;
        int32_t cellShapes[] = { 0 };   // Bar
        int32_t gridOrders[] = { 2 };
        int32_t basisFns[] = { 0 };
        int64_t numElementsPerSection[] = { static_cast<int64_t>(numBar3Cells) };
        i = tecZoneCreateFEMixed(
            outputFileHandle,
            (char*)"Simple Bar3 Zone",
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
        i = tecZoneVarWriteFloatValues(outputFileHandle, outputZone, 5, 0, numBar3Cells, cc);
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
    }


    i = tecFileWriterClose(&outputFileHandle);

    return 0;
}
