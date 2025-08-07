// This example creates a number of IJK-ordered zones.
//
// Output variables are nodal variables X, Y, Z, and cell-centered variables P and C.
// All zones share integer variable C.
// 
// For TecIO-MPI (TECIOMPI #defined), an additional nodal variable, Rank, is added.
// This variable is shared by all zones output on a particular MPI rank.
//
// For brevity, return codes from TecIO routines are ignored here,
// but should be checked in a real application.

#if defined TECIOMPI
#include "mpi.h"
#endif

#include <iostream>
#include <vector>

#include "TECIO.h"

#define XDIM 10
#define YDIM 9
#define ZDIM 8

#define NUM_ZONES 10

#if !defined NULL
#define NULL 0
#endif

//#define IS_DOUBLE

#if IS_DOUBLE
#define VAR_TYPE double
#else
#define VAR_TYPE float
#endif

#if defined TECIOMPI
    #define NUMVARS 6
#else
    #define NUMVARS 5
#endif

#define XVARNUM 1
#define YVARNUM 2
#define ZVARNUM 3
#define PVARNUM 4
#define CVARNUM 5
#define RANKVARNUM 6

int main(int argc, char** argv)
{
    std::string variables("X Y Z P C");
    std::vector<int32_t> shareVarFromZone(NUMVARS, 0); // No variable sharing for first zone output
    std::vector<int32_t> varTypes(NUMVARS, FieldDataType_Float);
    varTypes[CVARNUM - 1] = FieldDataType_Int16;
    #if defined TECIOMPI
        variables += " Rank"; // MPI rank
        varTypes[RANKVARNUM - 1] = FieldDataType_Int32;
        MPI_Init(&argc, &argv);
        MPI_Comm mpiComm = MPI_COMM_WORLD;
        int commSize;
        MPI_Comm_size(mpiComm, &commSize);
        int commRank;
        MPI_Comm_rank(mpiComm, &commRank);
        #if defined _DEBUG
            if (commRank == 0)
            {
                std::cout << "Press return to continue" << std::endl;
                std::cin.get();
            }
        #endif
    #endif

    /*
     * Open the file and write the tecplot datafile
     * header information
     */
    void* fileHandle;
    int32_t res = tecFileWriterOpen(
        "ijkmany.szplt",
        "IJK Ordered Zone",
        variables.c_str(),
        FILEFORMAT_SZL,
        FILETYPE_FULL,
        FieldDataType_Float,
        NULL,
        &fileHandle);

    #if defined TECIOMPI
        INTEGER4 mainRank = 0;
        res = tecMPIInitialize(fileHandle, mpiComm, mainRank);
    #endif

    for (int32_t zn = 0; zn < NUM_ZONES; ++zn)
    {
        int32_t zone;
        if (zn > 0)
            shareVarFromZone[CVARNUM - 1] = 1;

        #if defined TECIOMPI
            int32_t zoneOwner = zn % commSize;
            if (zn >= commSize)
                shareVarFromZone[RANKVARNUM - 1] = zoneOwner + 1;
            if (commRank == zoneOwner || commRank == mainRank)
            {
        #endif

        //  Create the ordered Zone
        std::vector<int32_t> valueLocations(NUMVARS, 1);
        valueLocations[PVARNUM - 1] = 0; // P is cell-centered
        valueLocations[CVARNUM - 1] = 0; // C is cell-centered
        res = tecZoneCreateIJK(
            fileHandle,
            "Ordered Zone",
            XDIM,
            YDIM,
            ZDIM,
            &varTypes[0],
            &shareVarFromZone[0],
            &valueLocations[0],
            NULL, // passiveVarList
            0,    // shareFaceNeighborsFromZone,
            0,    // numFaceConnections,
            0,    // faceNeighborMode,
            &zone);

        #if defined TECIOMPI
            res = tecZoneMapPartitionsToMPIRanks(
                fileHandle,
                zone,
                1, // numPartitions,
                &zoneOwner);
            }
            if (zoneOwner == commRank)
            {
        #endif

        std::vector<float> x(XDIM * YDIM * ZDIM);
        std::vector<float> y(XDIM * YDIM * ZDIM);
        std::vector<float> z(XDIM * YDIM * ZDIM);
        std::vector<float> p((XDIM - 1) * (YDIM - 1) * (ZDIM - 1));
        std::vector<int16_t> c((XDIM - 1) * (YDIM - 1) * (ZDIM - 1));

        // Calculate the zone variables
        for (int i = 0; i < XDIM; ++i)
            for (int j = 0; j < YDIM; ++j)
                for (int k = 0; k < ZDIM; ++k)
                {
                    int index = (k * YDIM + j) * XDIM + i;
                    x[index] = (float)(i + zn * (XDIM - 1));
                    y[index] = (float)j;
                    z[index] = (float)k;
                    if (i < XDIM - 1 && j < YDIM - 1 && k < ZDIM - 1)
                    {
                        int cindex = (k * (YDIM - 1) + j) * (XDIM - 1) + i;
                        p[cindex] = (x[index] + 0.5f) * (y[index] + 0.5f) * (z[index] + 0.5f);
                        c[cindex] = cindex;
                    }
                }

        // Output zone
        res = tecZoneVarWriteFloatValues(fileHandle, zone, XVARNUM, 0, x.size(), &x[0]);
        res = tecZoneVarWriteFloatValues(fileHandle, zone, YVARNUM, 0, y.size(), &y[0]);
        res = tecZoneVarWriteFloatValues(fileHandle, zone, ZVARNUM, 0, z.size(), &z[0]);
        res = tecZoneVarWriteFloatValues(fileHandle, zone, PVARNUM, 0, p.size(), &p[0]);

        if (zn == 0)
            res = tecZoneVarWriteInt16Values(fileHandle, zone, CVARNUM, 0, c.size(), &c[0]);

        #if defined TECIOMPI
                if (zn < commSize)
                {
                    std::vector<int32_t> rank(XDIM * YDIM * ZDIM, commRank);
                    res = tecZoneVarWriteInt32Values(fileHandle, zone, RANKVARNUM, 0, rank.size(), &rank[0]);
                }
            }
        #endif
    }

    res = tecFileWriterClose(&fileHandle);
    #if defined TECIOMPI
        MPI_Finalize();
    #endif

    return 0;
}
