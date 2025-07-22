/*
 * Example C++ program to write a binary data file for Tecplot.
 * This more detailed example does the following:
 *
 *   1.  Opens a datafile called "polygonal_quads.plt"
 *   2.  Assigns values for x & y and a calculated variable "c" which is x*y.
 *   3.  Writes out a polygonal zone composed of IMAX x JMAX quadrilaterals (squares actually)
 *   4.  Closes the datafile.
 */
#include "TECIO.h"
#include <assert.h>
#include <stdio.h>
#define CHECK(b) assert(b)

#define IMAX 10
#define JMAX 10

static INTEGER4 const IElemMax = IMAX; // number of elements across the zone in x-direction
static INTEGER4 const JElemMax = JMAX; // number of elements across the zone in y-direction

static INTEGER4 const INodeMax = (IElemMax + 1); // number of nodes across the zone in x-direction
static INTEGER4 const JNodeMax = (JElemMax + 1); // number of nodes across the zone in y-direction

#if !defined NULL
#define NULL 0
#endif

static float getVarValue(
    INTEGER4 var,
    INTEGER4 iNode,
    INTEGER4 jNode)
{
    if (var == 0) // x
        return float(iNode);
    else if (var == 1) // y
        return float(jNode);
    else if (var == 2) // "c" = x*y
        return float(iNode)*float(jNode);
    else
    {
        CHECK(0);
        return 0.0;
    }
}

int main(int, char**)
{
    INTEGER4 debug = 1;
    INTEGER4 fileIsDouble = 0;
    INTEGER4 fileType = 0;
    INTEGER4 fileFormat = 0; // 0 == PLT, 1 == SZPLT; Only PLT is currently supported for poly zones
    INTEGER4 result;

    result = TECINI142(
        "Polygonal Quads",
        "X Y C",
        "polygonal_quads.plt",
        ".",
        &fileFormat,
        &fileType,
        &debug,
        &fileIsDouble);
    if (result != 0)
    {
        printf("Polyquads: error calling TECINI\n");
        return -1;
    }
    INTEGER4 numVars = 3;

    INTEGER4 zoneType = 6; // FEPolygon
    INTEGER4 numNodes = INodeMax*JNodeMax; // Number of nodes in the zone
    INTEGER4 numElems = IElemMax*JElemMax; // Number of elements in the zone.
    INTEGER8 numFaces = (INTEGER8)INodeMax*JElemMax + // Number of faces in the zone (numILines*numFacesPerILine + numJLines*numFacesPerJLine)
                        (INTEGER8)JNodeMax*IElemMax;  // to do the calculation in INTEGER8s because INTEGER4s could overflow
    INTEGER8 numFaceNodes = 2 * numFaces; // All faces are line segments, so 2 nodes per face
    double   solTime = 0.0;
    INTEGER4 strandID = 0;     // Static Zone
    INTEGER4 unused = 0;       // ParentZone is no longer used
    INTEGER4 numBoundaryFaces = 0;
    INTEGER4 numBoundaryConnections = 0;
    INTEGER4 shareConnectivity = 0;

    result = TECPOLYZNE142(
        "Polygonal Quad Zone",
        &zoneType,
        &numNodes,
        &numElems,
        &numFaces,
        &numFaceNodes,
        &solTime,
        &strandID,
        &unused,
        &numBoundaryFaces,
        &numBoundaryConnections,
        NULL,
        NULL,  // All nodal variables
        NULL,
        &shareConnectivity);
    if (result != 0)
    {
        printf("Polyquads: error calling TECPOLYZNE\n");
        return -1;
    }

    // write variable values out var by var, calling TECDAT at the end of each j loop to use less temporary memory
    try
    {
        float* values = new float[INodeMax];
        for (INTEGER4 var = 0; var < numVars; var++)
        {
            for (INTEGER4 jNode = 0; jNode < JNodeMax; jNode++)
            {
                INTEGER4 count = 0;
                for (INTEGER4 iNode = 0; iNode < INodeMax; iNode++)
                {
                    values[count++] = (float)getVarValue(var, iNode, jNode);
                }
                INTEGER4 dIsDouble = 0;
                result = TECDAT142(&count, values, &dIsDouble);
                CHECK(result == 0);
            }
        }
        delete[] values;
    }
    catch (...)
    {
        printf("Polyquads: Memory error with variable values\n");
    }

    // now write connectivity, calling TECPOLYFACE at the end of each j loop to use less temporary memory
    try
    {
        INTEGER4 const maxNumFacesJLine = 2 * INodeMax;
        INTEGER4* faceNodes = new INTEGER4[2 * maxNumFacesJLine]; // two nodes per face
        INTEGER4* faceLeftElems = new INTEGER4[maxNumFacesJLine];
        INTEGER4* faceRightElems = new INTEGER4[maxNumFacesJLine];

        INTEGER4 const NO_NEIGHBOR = 0; // 1-based
        INTEGER4 nodeNum = 1; // 1-based
        for (INTEGER4 jNode = 0; jNode < JNodeMax; jNode++)
        {
            INTEGER4 const jElem = jNode < JElemMax ? jNode : JElemMax - 1;

            INTEGER4 faceOffset = 0; // 0-based
            for (INTEGER4 iNode = 0; iNode < INodeMax; iNode++)
            {
                INTEGER4 const iElem = iNode < IElemMax ? iNode : IElemMax - 1;

                INTEGER4 const elemNum = jElem*IElemMax + iElem + 1; // 1-based
                CHECK(1 <= elemNum && elemNum <= numElems); // 1-based

                CHECK(faceOffset < maxNumFacesJLine);

                // i-varying line faces
                if (iNode < IElemMax)
                {
                    faceNodes[faceOffset * 2 + 0] = nodeNum;
                    faceNodes[faceOffset * 2 + 1] = nodeNum + 1;
                    if (jNode == 0)
                    {
                        faceLeftElems[faceOffset] = elemNum;
                        faceRightElems[faceOffset] = NO_NEIGHBOR;
                    }
                    else if (jNode < JElemMax)
                    {
                        faceLeftElems[faceOffset] = elemNum;
                        faceRightElems[faceOffset] = elemNum - IElemMax;
                    }
                    else
                    {
                        faceLeftElems[faceOffset] = NO_NEIGHBOR;
                        faceRightElems[faceOffset] = elemNum;
                    }

                    CHECK(faceNodes[faceOffset * 2 + 0] <= numNodes);
                    CHECK(faceNodes[faceOffset * 2 + 1] <= numNodes);
                    CHECK(faceLeftElems[faceOffset] >= 0 && faceLeftElems[faceOffset] <= numElems);
                    CHECK(faceRightElems[faceOffset] >= 0 && faceRightElems[faceOffset] <= numElems);

                    faceOffset++;
                }

                // j-varying line faces
                if (jNode < JElemMax)
                {
                    faceNodes[faceOffset * 2 + 0] = nodeNum;
                    faceNodes[faceOffset * 2 + 1] = nodeNum + INodeMax;
                    if (iNode == 0)
                    {
                        faceLeftElems[faceOffset] = elemNum;
                        faceRightElems[faceOffset] = NO_NEIGHBOR;
                    }
                    else if (iNode < IElemMax)
                    {
                        faceLeftElems[faceOffset] = elemNum;
                        faceRightElems[faceOffset] = elemNum - 1;
                    }
                    else
                    {
                        faceLeftElems[faceOffset] = NO_NEIGHBOR;
                        faceRightElems[faceOffset] = elemNum;
                    }

                    CHECK(faceNodes[faceOffset * 2 + 0] <= numNodes);
                    CHECK(faceNodes[faceOffset * 2 + 1] <= numNodes);
                    CHECK(faceLeftElems[faceOffset] >= 0 && faceLeftElems[faceOffset] <= numElems);
                    CHECK(faceRightElems[faceOffset] >= 0 && faceRightElems[faceOffset] <= numElems);

                    faceOffset++;
                }

                nodeNum++;
            }
            
            result = TECPOLYFACE142(
                &faceOffset,
                NULL,
                &faceNodes[0],
                &faceLeftElems[0],
                &faceRightElems[0]);

        }
        CHECK(nodeNum == numNodes + 1); // 1-based

        delete [] faceNodes;
        delete [] faceLeftElems;
        delete [] faceRightElems;
    }
    catch (...)
    {
        printf("Polyquads: Memory error with connectivity\n");
    }

    result = TECEND142();
    if (result != 0)
    {
        printf("Polyquads: error calling TECEND\n");
        return -1;
    }

    return 0;
}

