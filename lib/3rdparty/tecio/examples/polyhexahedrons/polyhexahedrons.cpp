/*
 * Example C++ program to write a binary data file for Tecplot.
 * This more detailed example does the following:
 *
 *   1.  Opens a datafile called "polyhedral_hexahedrons.plt"
 *   2.  Assigns values for x, y & z and a calculated variable "c" which is x*y*z.
 *   3.  Writes out a polyhedral zone composed of IMAX x JMAX x KMAX hexahedrons (cubes actually)
 *   4.  Closes the datafile.
 */
#include "TECIO.h"
#include <assert.h>
#include <stdio.h>
#define CHECK(b) assert(b)

#define IMAX 10
#define JMAX 10
#define KMAX 10

static INTEGER4 const IElemMax = IMAX; // number of elements across the zone in x-direction
static INTEGER4 const JElemMax = JMAX; // number of elements across the zone in y-direction
static INTEGER4 const KElemMax = KMAX; // number of elements across the zone in z-direction

static INTEGER4 const INodeMax = (IElemMax + 1); // number of nodes across the zone in x-direction
static INTEGER4 const JNodeMax = (JElemMax + 1); // number of nodes across the zone in y-direction
static INTEGER4 const KNodeMax = (KElemMax + 1); // number of nodes across the zone in z-direction

#if !defined NULL
#define NULL 0
#endif

static float getVarValue(
    INTEGER4 var,
    INTEGER4 iNode,
    INTEGER4 jNode,
    INTEGER4 kNode)
{
    if (var == 0) // x
        return float(iNode);
    else if (var == 1) // y
        return float(jNode);
    else if (var == 2) // z
        return float(kNode);
    else if (var == 3) // "c" = x*y*z
        return float(iNode)*float(jNode)*float(kNode);
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
            "Polyhedral Hexahedrons",
            "X Y Z C",
            "polyhedral_hexahedrons.plt",
            ".",
            &fileFormat,
            &fileType,
            &debug,
            &fileIsDouble);
    if (result != 0)
    {
        printf("Polyhexahedrons: error calling TECINI\n");
        return -1;
    }
    INTEGER4 numVars = 4;

    INTEGER4 zoneType = 7;     // FEPolyhedron
    INTEGER4 numNodes = INodeMax*JNodeMax*KNodeMax;  // Number of nodes in the zone
    INTEGER4 numElems = IElemMax*JElemMax*KElemMax; // Number of elements in the zone.
    INTEGER8 numFaces = (INTEGER8)INodeMax*JElemMax*KElemMax + // Number of faces in the zone (numIPlanes*numFacesPerIPlane + numJPlanes*numFacesPerJPlane + numKPlanes*numFacesPerKPlane)
                        (INTEGER8)JNodeMax*IElemMax*KElemMax + // to do the calculation in INTEGER8s because INTEGER4s could overflow
                        (INTEGER8)KNodeMax*IElemMax*JElemMax; 
    INTEGER8 numFaceNodes = 4 * numFaces; // All faces are quads, so 4 nodes per face
    double   solTime = 0.0;
    INTEGER4 strandID = 0;     // Static Zone
    INTEGER4 unused = 0;       // ParentZone is no longer used
    INTEGER4 numBoundaryFaces = 0;
    INTEGER4 numBoundaryConnections = 0;
    INTEGER4 shareConnectivity = 0;

    result = TECPOLYZNE142(
        "Polyhedral Hexahedron Zone",
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
        printf("Polyhexahedrons: error calling TECPOLYZNE\n");
        return -1;
    }

    // write variable values out var by var, calling TECDAT at the end of each k-plane to use less temporary memory
    try
    {
        float* values = new float[INodeMax*JNodeMax];
        for (INTEGER4 var = 0; var < numVars; var++)
        {
            for (INTEGER4 kNode = 0; kNode < KNodeMax; kNode++)
            {
                INTEGER4 count = 0;
                for (INTEGER4 jNode = 0; jNode < JNodeMax; jNode++)
                {
                    for (INTEGER4 iNode = 0; iNode < INodeMax; iNode++)
                    {
                        values[count++] = getVarValue(var, iNode, jNode, kNode);
                    }
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
        printf("Polyhexahedrons: Memory error with variable values\n");
        return -1;
    }

    // now write connectivity, calling TECPOLYFACE at the end of each k-plane to use less temporary memory
    try
    {
        INTEGER4 const maxNumFacesKPlane = 3 * INodeMax*JNodeMax;
        INTEGER4* faceNodes = new INTEGER4[4 * maxNumFacesKPlane];
        INTEGER4* faceLeftElems = new INTEGER4[maxNumFacesKPlane];
        INTEGER4* faceRightElems = new INTEGER4[maxNumFacesKPlane];
        INTEGER4* faceNodeCounts = new INTEGER4[maxNumFacesKPlane];

        INTEGER4 const NO_NEIGHBOR = 0; // 1-based
        INTEGER4 nodeNum = 1; // 1-based
        for (INTEGER4 kNode = 0; kNode < KNodeMax; kNode++)
        {
            INTEGER4 const kElem = kNode < KElemMax ? kNode : KElemMax - 1;

            INTEGER4 faceOffset = 0; // 0-based
            for (INTEGER4 jNode = 0; jNode < JNodeMax; jNode++)
            {
                INTEGER4 const jElem = jNode < JElemMax ? jNode : JElemMax - 1;

                for (INTEGER4 iNode = 0; iNode < INodeMax; iNode++)
                {
                    INTEGER4 const iElem = iNode < IElemMax ? iNode : IElemMax - 1;

                    INTEGER4 const elemNum64 = kElem*JElemMax*IElemMax + jElem*IElemMax + iElem + 1; // 1-based
                    INTEGER4 const elemNum = INTEGER4(elemNum64);
                    CHECK(1 <= elemNum && elemNum <= numElems); // 1-based

                    CHECK(faceOffset < maxNumFacesKPlane);

                    // k-plane faces
                    if (iNode < IElemMax && jNode < JElemMax)
                    {
                        faceNodes[faceOffset * 4 + 0] = nodeNum;
                        faceNodes[faceOffset * 4 + 1] = nodeNum + 1;
                        faceNodes[faceOffset * 4 + 2] = nodeNum + INodeMax + 1;
                        faceNodes[faceOffset * 4 + 3] = nodeNum + INodeMax;
                        if (kNode == 0)
                        {
                            faceLeftElems[faceOffset] = elemNum;
                            faceRightElems[faceOffset] = NO_NEIGHBOR;
                        }
                        else if (kNode < KElemMax)
                        {
                            faceLeftElems[faceOffset] = elemNum;
                            faceRightElems[faceOffset] = elemNum - IElemMax*JElemMax;
                        }
                        else
                        {
                            faceLeftElems[faceOffset] = NO_NEIGHBOR;
                            faceRightElems[faceOffset] = elemNum;
                        }
                        faceNodeCounts[faceOffset] = 4;
                    
                        CHECK(faceNodes[faceOffset * 4 + 0] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 1] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 2] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 3] <= numNodes);
                        CHECK(faceLeftElems[faceOffset] >= 0 && faceLeftElems[faceOffset] <= numElems);
                        CHECK(faceRightElems[faceOffset] >= 0 && faceRightElems[faceOffset] <= numElems);
                        CHECK(faceNodeCounts[faceOffset] == 4);

                        faceOffset++;
                    }

                    // j-plane faces
                    if (iNode < IElemMax && kNode < KElemMax)
                    {
                        faceNodes[faceOffset * 4 + 0] = nodeNum;
                        faceNodes[faceOffset * 4 + 1] = nodeNum + INodeMax*JNodeMax;
                        faceNodes[faceOffset * 4 + 2] = nodeNum + INodeMax*JNodeMax + 1;
                        faceNodes[faceOffset * 4 + 3] = nodeNum + 1;
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
                        faceNodeCounts[faceOffset] = 4;

                        CHECK(faceNodes[faceOffset * 4 + 0] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 1] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 2] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 3] <= numNodes);
                        CHECK(faceLeftElems[faceOffset] >= 0 && faceLeftElems[faceOffset] <= numElems);
                        CHECK(faceRightElems[faceOffset] >= 0 && faceRightElems[faceOffset] <= numElems);
                        CHECK(faceNodeCounts[faceOffset] == 4);

                        faceOffset++;
                    }

                    // i-plane faces
                    if (jNode < JElemMax && kNode < KElemMax)
                    {
                        faceNodes[faceOffset * 4 + 0] = nodeNum;
                        faceNodes[faceOffset * 4 + 1] = nodeNum + INodeMax;
                        faceNodes[faceOffset * 4 + 2] = nodeNum + INodeMax*JNodeMax + INodeMax;
                        faceNodes[faceOffset * 4 + 3] = nodeNum + INodeMax*JNodeMax;
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
                        faceNodeCounts[faceOffset] = 4;

                        CHECK(faceNodes[faceOffset * 4 + 0] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 1] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 2] <= numNodes);
                        CHECK(faceNodes[faceOffset * 4 + 3] <= numNodes);
                        CHECK(faceLeftElems[faceOffset] >= 0 && faceLeftElems[faceOffset] <= numElems);
                        CHECK(faceRightElems[faceOffset] >= 0 && faceRightElems[faceOffset] <= numElems);
                        CHECK(faceNodeCounts[faceOffset] == 4);

                        faceOffset++;
                    }

                    nodeNum++;
                }
            }
            
            INTEGER4 numFaces32 = INTEGER4(faceOffset);
            result = TECPOLYFACE142(
                &numFaces32,
                faceNodeCounts,
                &faceNodes[0],
                &faceLeftElems[0],
                &faceRightElems[0]);

        }
        CHECK(nodeNum == numNodes + 1); // 1-based

        delete [] faceNodes;
        delete [] faceLeftElems;
        delete [] faceRightElems;
        delete [] faceNodeCounts;
    }
    catch (...)
    {
        printf("Polyhexahedrons: Memory error with connectivity\n");
        return -1;
    }

    result = TECEND142();
    if (result != 0)
    {
        printf("Polyhexahedrons: error calling TECEND\n");
        return -1;
    }

    return 0;
}

