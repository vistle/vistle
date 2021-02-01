/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for ChEESE tsunami nc-files                   **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Marko Djuric                                                **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  25.01.2021                                                      **
\**************************************************************************/

//vistle
#include "vistle/core/object.h"
#include "vistle/core/parameter.h"
#include "vistle/core/polygons.h"
#include "vistle/core/scalar.h"
#include "vistle/core/vector.h"
/* #include "vistle/module/general/DomainSurface/DomainSurface.h" */
#include "vistle/module/module.h"
#include "vistle/module/reader.h"

//header
#include "ReadTsunami.h"

//std
#include <cstddef>
#include <string>

MODULE_MAIN(ReadTsunami)

using namespace vistle;

ReadTsunami::ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Tsunami files", name, moduleID, comm)
{
    ncDataFile = nullptr;

    // define parameters

    // file-browser
    m_filedir = addStringParameter("file_dir", "NC File directory", "/data/ChEESE/Tsunami/pelicula_eta.nc",
                                   Parameter::Filename);

    // visualise variables
    m_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter", 1.0);
    m_step = addIntParameter("StepWidth", "Timestep step width", 1);
    setParameterRange(m_step, Integer(0), Integer(999999));

    // define ports

    // 2D Surface
    m_surface_out = createOutputPort("surfaceOut", "2D Grid output (Polygons)");
    m_seaSurface_out = createOutputPort("seaSurfaceOut", "2D See floor (Polygons)");
    m_waterSurface_out = createOutputPort("waterSurfaceOut", "2D water surface (Polygons)");
    m_maxHeight = createOutputPort("maxHeight", "Max water height (Float)");

    /* m_size[0] = addIntParameter("size_x", "number of cells per block in x-direction", 10); */
    /* m_size[1] = addIntParameter("size_y", "number of cells per block in y-direction", 10); */
    /* m_size[2] = addIntParameter("size_z", "number of cells per block in z-direction", 10); */

    /* m_blocks[0] = addIntParameter("blocks_x", "number of blocks in x-direction", 3); */
    /* m_blocks[1] = addIntParameter("blocks_y", "number of blocks in y-direction", 3); */
    /* m_blocks[2] = addIntParameter("blocks_z", "number of blocks in z-direction", 3); */

    /* m_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 0); */

    //observer parameters
    observeParameter(m_filedir);
    /* observeParameter(m_blocks[0]); */
    /* observeParameter(m_blocks[1]); */
    /* observeParameter(m_blocks[2]); */
    observeParameter(m_step);

    setParallelizationMode(ParallelizeBlocks);
}

ReadTsunami::~ReadTsunami()
{}

/**
 * Open Nc File and set pointer ncDataFile.
 *
 * @return true if its not empty or cannot be opened.
 */
bool ReadTsunami::openNcFile()
{
    std::string sFileName = m_filedir->getValue();

    if (sFileName.empty()) {
        sendInfo("NetCDF filename is empty!");
        return false;
    } else {
        try {
            ncDataFile = new NcFile(sFileName.c_str(), NcFile::read);
            sendInfo("Reading File: " + sFileName);
        } catch (...) {
            sendInfo("Couldn't open NetCDF file!");
            return false;
        }

        if (ncDataFile->getVarCount() == 0) {
            sendInfo("empty NetCDF file!");
            return false;
        } else {
            return true;
        }
    }
}

bool ReadTsunami::examine(const vistle::Parameter *param)
{
    if (openNcFile()) {
        /* size_t nblocks = m_blocks[0]->getValue() * m_blocks[1]->getValue() * m_blocks[2]->getValue(); */
        /* setPartitions(nblocks); */
        setTimesteps(m_step->getValue());
        return true;
        /* return nblocks > 0; */
    }
    return false;
}

void ReadTsunami::block(Reader::Token &token, Index bx, Index by, Index bz, vistle::Index block,
                        vistle::Index time) const
{
    // get variable name from choice parameters
    /* NcVar var; */

    // read variables from NetCDF-File
    NcVar latvar{ncDataFile->getVar("lat")};
    NcVar lonvar{ncDataFile->getVar("lon")};
    NcVar grid_latvar{ncDataFile->getVar("grid_lat")};
    NcVar grid_lonvar{ncDataFile->getVar("grid_lon")};
    NcVar bathymetryvar{ncDataFile->getVar("bathymetry")};
    NcVar max_height{ncDataFile->getVar("max_height")};
    NcVar eta{ncDataFile->getVar("eta")};

    // dimension from lat and lon variables
    auto surfaceDimX{latvar.getDim(0).getSize()};
    auto surfaceDimY{lonvar.getDim(0).getSize()};

    // num of polygons needed?
    int surfaceNumPoly = (surfaceDimX - 1) * (surfaceDimY - 1);

    // pointer for lat values and coords
    float *latVals = new float[surfaceDimX];
    float *lonVals = new float[surfaceDimY];

    // read in lat var ncdata into float-pointer
    latvar.getVar(latVals);
    lonvar.getVar(lonVals);

    /* Object::ptr surfaceOut; */
    /* Object::ptr warterSurfaceOut; */

    // Now for the 2D variables, we create a surface
    /* Coords::ptr geoSurface; */
    Polygons::ptr surfacePolygons(new Polygons(surfaceNumPoly, surfaceDimX * surfaceDimY, surfaceNumPoly * 4));

    /* geoSurface = surfacePolygons; */

    // fill coords
    int n{0};
    auto sx_coord = surfacePolygons->x().data(), sy_coord = surfacePolygons->y().data(),
         sz_coord = surfacePolygons->z().data();
    for (size_t i = 0; i < surfaceDimX; i++)
        for (size_t j = 0; j < surfaceDimY; j++) {
            sx_coord[n] = latVals[i];
            sy_coord[n] = lonVals[j];
            sz_coord[n] = 0;
        }

    // fill vertices
    n = 0;
    auto surfaceConnectivityList = surfacePolygons->cl().data();
    for (size_t j = 1; j < surfaceDimX; j++)
        for (size_t k = 1; k < surfaceDimY; k++) {
            surfaceConnectivityList[n++] = (j - 1) * surfaceDimY + (k - 1);
            surfaceConnectivityList[n++] = j * surfaceDimY + (k - 1);
            surfaceConnectivityList[n++] = j * surfaceDimY + k;
            surfaceConnectivityList[n++] = (j - 1) * surfaceDimY + k;
        }

    // Fill the polygon list
    auto surfacePolygonList = surfacePolygons->el().data();
    for (auto p = 0; p < surfaceNumPoly; p++)
        surfacePolygonList[p] = p * 4;

    // Delete buffers from grid replication
    delete[] latVals;
    delete[] lonVals;

    // get dim from grid_lon & grid_lat
    int gridLatDimX = grid_latvar.getDim(0).getSize();
    int gridLonDimY = grid_lonvar.getDim(0).getSize();
    latVals = new float[gridLatDimX];
    lonVals = new float[gridLonDimY];

    // depth
    float *depthVals = new float[gridLatDimX * gridLonDimY];

    // set where to stream to data (float pointer)
    grid_latvar.getVar(latVals);
    grid_lonvar.getVar(lonVals);
    bathymetryvar.getVar(depthVals);

    // Now for the 2D variables, we create a surface
    int numPolygons = (gridLatDimX - 1) * (gridLonDimY - 1);
    Polygons::ptr polygonsSeaSurface(new Polygons(numPolygons, gridLatDimX * gridLonDimY, numPolygons * 4));

    // Fill the _coord arrays (memcpy faster?)
    n = 0;
    auto x_coord = polygonsSeaSurface->x().data(), y_coord = polygonsSeaSurface->y().data(),
         z_coord = polygonsSeaSurface->z().data();
    for (int j = 0; j < gridLatDimX; j++)
        for (int k = 0; k < gridLonDimY; k++, n++) {
            x_coord[n] = latVals[j];
            y_coord[n] = lonVals[k];
            z_coord[n] = -depthVals[j * gridLonDimY + k];
        }

    // Fill the vertex list
    n = 0;
    auto seasurfaceConnectivityList = polygonsSeaSurface->cl().data();
    for (int j = 1; j < gridLatDimX; j++)
        for (int k = 1; k < gridLonDimY; k++) {
            seasurfaceConnectivityList[n++] = (j - 1) * gridLonDimY + (k - 1);
            seasurfaceConnectivityList[n++] = j * gridLonDimY + (k - 1);
            seasurfaceConnectivityList[n++] = j * gridLonDimY + k;
            seasurfaceConnectivityList[n++] = (j - 1) * gridLonDimY + k;
        }

    // Fill the polygon list
    auto seaSurfacePolygonList = polygonsSeaSurface->el().data();
    for (int p = 0; p < numPolygons; p++)
        seaSurfacePolygonList[p] = p * 4;

    // Delete buffers from grid replication
    delete[] latVals;
    delete[] lonVals;
    delete[] depthVals;

    // float for read in max_height
    Vec<Scalar>::ptr scalarMaxHeight(new Vec<Scalar>(max_height.getDimCount()));
    vistle::Scalar *maxH{scalarMaxHeight->x().data()};
    max_height.getVar(maxH);

    // read in time
    float *floatData = new float[eta.getDim(0).getSize() * eta.getDim(1).getSize() * eta.getDim(2).getSize()];
    int numTimesteps = eta.getDim(0).getSize();
    eta.getVar(floatData);

    // create Object::ptr pointer for each timestep
    std::string baseName = m_waterSurface_out->getName();
    sendInfo("numTimesteps %d!", numTimesteps);
    int i = 0;

    // get vertical Scale
    float zScale = m_verticalScale->getValue();

    // create watersurface with polygons for each timestep
    for (int t = 0; t < numTimesteps; t += m_step->getValue()) {
        // Now for the 2D variables, we create a surface
        int snumPolygons = (surfaceDimX - 1) * (surfaceDimY - 1);

        Polygons::ptr outSurface(new Polygons(numPolygons, gridLatDimX * gridLonDimY, numPolygons * 4));

        auto x_coord = outSurface->x().data(), y_coord = outSurface->y().data(), z_coord = outSurface->z().data();
        for (auto n = 0; n < surfaceDimX * surfaceDimY; n++) {
            x_coord[n] = sx_coord[n];
            y_coord[n] = sy_coord[n];
            z_coord[n] = floatData[t * surfaceDimX * surfaceDimY + n] * zScale;
        }

        auto vl = outSurface->cl().data();
        for (int j = 0; j < snumPolygons * 4; j++)
            vl[j] = surfaceConnectivityList[j];

        auto pl = outSurface->el().data();
        for (int p = 0; p < snumPolygons; p++)
            pl[p] = surfacePolygonList[p];

        token.addObject(m_surface_out, outSurface);
        i++;
    }

    /* token.addObject(m_surface_out, objs); */
    polygonsSeaSurface->updateInternals();
    token.addObject(m_seaSurface_out->getName(), polygonsSeaSurface);
    /* token.addObject(m_maxHeight, scalarMaxHeight); */

    delete floatData;
    delete maxH;
}

bool ReadTsunami::read(Token &token, int timestep, int blockNum)
{
    constexpr bool m_test{true};
    // get variable name from choice parameters
    /* NcVar var; */

    // read variables from NetCDF-File
    NcVar latvar{ncDataFile->getVar("lat")};
    NcVar lonvar{ncDataFile->getVar("lon")};
    NcVar grid_latvar{ncDataFile->getVar("grid_lat")};
    NcVar grid_lonvar{ncDataFile->getVar("grid_lon")};
    NcVar bathymetryvar{ncDataFile->getVar("bathymetry")};
    NcVar max_height{ncDataFile->getVar("max_height")};
    NcVar eta{ncDataFile->getVar("eta")};

    // dimension from lat and lon variables
    auto surfaceDimX{latvar.getDim(0).getSize()};
    auto surfaceDimY{lonvar.getDim(0).getSize()};

    // num of polygons needed?
    int surfaceNumPoly = (surfaceDimX - 1) * (surfaceDimY - 1);

    // pointer for lat values and coords
    float *latVals = new float[surfaceDimX];
    float *lonVals = new float[surfaceDimY];

    // read in lat var ncdata into float-pointer
    latvar.getVar(latVals);
    lonvar.getVar(lonVals);

    /* Object::ptr surfaceOut; */
    /* Object::ptr warterSurfaceOut; */

    // Now for the 2D variables, we create a surface
    /* Coords::ptr geoSurface; */
    Polygons::ptr surfacePolygons(new Polygons(surfaceNumPoly, surfaceDimX * surfaceDimY, surfaceNumPoly * 4));

    /* geoSurface = surfacePolygons; */

    // fill coords
    int n{0};
    auto sx_coord = surfacePolygons->x().data(), sy_coord = surfacePolygons->y().data(),
         sz_coord = surfacePolygons->z().data();
    for (size_t i = 0; i < surfaceDimX; i++)
        for (size_t j = 0; j < surfaceDimY; j++, n++) {
            sx_coord[n] = latVals[i];
            sy_coord[n] = lonVals[j];
            sz_coord[n] = 0;
        }

    if constexpr (m_test) {
        sendInfo("fill coords for surfacePolygons");
        std::string x = std::to_string(*sx_coord);
        std::string y = std::to_string(*sy_coord);
        std::string z = std::to_string(*sz_coord);
        sendInfo("sx_coord: " + x);
        sendInfo("sy_coord: " + y);
        sendInfo("sz_coord: " + z);
    }

    // fill vertices
    n = 0;
    /* auto surfaceConnectivityList = surfacePolygons->cl().data(); */

    if constexpr (m_test) {
        std::string dimX = std::to_string(surfaceDimX);
        sendInfo("surfaceDimX: " + dimX);
        std::string dimY = std::to_string(surfaceDimY);
        sendInfo("surfaceDimY: " + dimY);
    }

    if constexpr (m_test) {
        std::string numPoly = std::to_string(surfaceNumPoly);
        sendInfo("surfaceNumPoly: " + numPoly);
        sendInfo(std::to_string(surfacePolygons->getSize()));
    }

    n = 0;
    //FIX ME: not accessable
    for (size_t j = 1; j < surfaceDimX; j++)
        for (size_t k = 1; k < surfaceDimY; k++) {
            /* surfaceConnectivityList[n++] = (j - 1) * surfaceDimY + (k - 1); */
            surfacePolygons->cl()[n++] = (j - 1) * surfaceDimY + (k - 1);

            if constexpr (m_test)
                if (n < 100) {
                    sendInfo("(j - 1) * surfaceDimY + (k - 1): " + std::to_string((j - 1) * surfaceDimY + (k - 1)));
                    /* sendInfo("surfaceConnectivityList [" + std::to_string(n) + */
                    /*          "]: " + std::to_string(surfaceConnectivityList[n])); */
                    sendInfo("surfaceConnectivityList [" + std::to_string(n) +
                             "]: " + std::to_string(surfacePolygons->cl()[n]));
                }

            /* surfaceConnectivityList[n++] = j * surfaceDimY + (k - 1); */
            surfacePolygons->cl()[n++] = j * surfaceDimY + (k - 1);

            if constexpr (m_test)
                if (n < 100) {
                    sendInfo("j*surfaceDimY + (k-1): " + std::to_string(j * surfaceDimY + (k - 1)));
                    /* sendInfo("surfaceConnectivityList [" + std::to_string(n) + */
                    /*          "]: " + std::to_string(surfaceConnectivityList[n])); */
                    sendInfo("surfaceConnectivityList [" + std::to_string(n) +
                             "]: " + std::to_string(surfacePolygons->cl()[n]));
                }

            /* surfaceConnectivityList[n++] = j * surfaceDimY + k; */
            surfacePolygons->cl()[n++] = j * surfaceDimY + k;

            if constexpr (m_test)
                if (n < 100) {
                    sendInfo("j * surfaceDimY + k: " + std::to_string(j * surfaceDimY + k));
                    /* sendInfo("surfaceConnectivityList [" + std::to_string(n) + */
                    /*          "]: " + std::to_string(surfaceConnectivityList[n])); */
                    sendInfo("surfaceConnectivityList [" + std::to_string(n) +
                             "]: " + std::to_string(surfacePolygons->cl()[n]));
                }

            /* surfaceConnectivityList[n++] = (j - 1) * surfaceDimY + k; */
            surfacePolygons->cl()[n++] = (j - 1) * surfaceDimY + k;

            if constexpr (m_test)
                if (n < 100) {
                    sendInfo("(j - 1) * surfaceDimY + k: " + std::to_string((j - 1) * surfaceDimY + k));
                    /* sendInfo("surfaceConnectivityList [" + std::to_string(n) + */
                    /*          "]: " + std::to_string(surfaceConnectivityList[n])); */
                    sendInfo("surfaceConnectivityList [" + std::to_string(n) +
                             "]: " + std::to_string(surfacePolygons->cl()[n]));
                }
        }

    /* if constexpr (m_test) { */
    /*     sendInfo("fill vertices for surfacePolygons"); */
    /*     for (int i = 0; i <= n; i++) { */
    /*         std::string vert_str = std::to_string(surfaceConnectivityList[i]); */
    /*         sendInfo("vertices surfaceConnectivityList: " + vert_str); */
    /*     } */
    /* } */

    // Fill the polygon list
    auto surfacePolygonList = surfacePolygons->el().data();

    for (auto p = 0; p < surfaceNumPoly; p++)
        surfacePolygonList[p] = p * 4;

    if constexpr (m_test) {
        std::string surfaceNumPoly_str = std::to_string(surfaceNumPoly);
        sendInfo("surfaceNumPoly: " + surfaceNumPoly);
        sendInfo("fill polygon for surfacePolygons");
        for (int i = 0; i <= n; i++) {
            std::string poly_str = std::to_string(surfacePolygonList[i]);
            sendInfo("vertices surfaceConnectivityList: " + poly_str);
        }
    }

    // Delete buffers from grid replication
    delete[] latVals;
    delete[] lonVals;

    // get dim from grid_lon & grid_lat
    int gridLatDimX = grid_latvar.getDim(0).getSize();
    int gridLonDimY = grid_lonvar.getDim(0).getSize();
    latVals = new float[gridLatDimX];
    lonVals = new float[gridLonDimY];

    // depth
    float *depthVals = new float[gridLatDimX * gridLonDimY];

    // set where to stream to data (float pointer)
    grid_latvar.getVar(latVals);
    grid_lonvar.getVar(lonVals);
    bathymetryvar.getVar(depthVals);

    // Now for the 2D variables, we create a surface
    int numPolygons = (gridLatDimX - 1) * (gridLonDimY - 1);
    Polygons::ptr polygonsSeaSurface(new Polygons(numPolygons, gridLatDimX * gridLonDimY, numPolygons * 4));

    // Fill the _coord arrays (memcpy faster?)
    n = 0;
    auto x_coord = polygonsSeaSurface->x().data(), y_coord = polygonsSeaSurface->y().data(),
         z_coord = polygonsSeaSurface->z().data();
    for (int j = 0; j < gridLatDimX; j++)
        for (int k = 0; k < gridLonDimY; k++, n++) {
            x_coord[n] = latVals[j];
            y_coord[n] = lonVals[k];
            z_coord[n] = -depthVals[j * gridLonDimY + k];
        }

    // Fill the vertex list
    n = 0;
    auto seasurfaceConnectivityList = polygonsSeaSurface->cl().data();
    for (int j = 1; j < gridLatDimX; j++)
        for (int k = 1; k < gridLonDimY; k++) {
            seasurfaceConnectivityList[n++] = (j - 1) * gridLonDimY + (k - 1);
            seasurfaceConnectivityList[n++] = j * gridLonDimY + (k - 1);
            seasurfaceConnectivityList[n++] = j * gridLonDimY + k;
            seasurfaceConnectivityList[n++] = (j - 1) * gridLonDimY + k;
        }

    // Fill the polygon list
    auto seaSurfacePolygonList = polygonsSeaSurface->el().data();
    for (int p = 0; p < numPolygons; p++)
        seaSurfacePolygonList[p] = p * 4;

    // Delete buffers from grid replication
    delete[] latVals;
    delete[] lonVals;
    delete[] depthVals;

    // float for read in max_height
    Vec<Scalar>::ptr scalarMaxHeight(new Vec<Scalar>(max_height.getDimCount()));
    vistle::Scalar *maxH{scalarMaxHeight->x().data()};
    max_height.getVar(maxH);

    // read in time
    float *floatData = new float[eta.getDim(0).getSize() * eta.getDim(1).getSize() * eta.getDim(2).getSize()];

    int numTimesteps = eta.getDim(0).getSize();
    eta.getVar(floatData);

    // create Object::ptr pointer for each timestep
    std::string baseName = m_waterSurface_out->getName();
    sendInfo("numTimesteps %d!", numTimesteps);
    int i = 0;

    // get vertical Scale
    float zScale = m_verticalScale->getValue();

    // create watersurface with polygons for each timestep
    for (int t = 0; t < numTimesteps; t += m_step->getValue()) {
        // Now for the 2D variables, we create a surface
        int snumPolygons = (surfaceDimX - 1) * (surfaceDimY - 1);

        Polygons::ptr outSurface(new Polygons(numPolygons, gridLatDimX * gridLonDimY, numPolygons * 4));

        auto x_coord = outSurface->x().data(), y_coord = outSurface->y().data(), z_coord = outSurface->z().data();
        for (auto n = 0; n < surfaceDimX * surfaceDimY; n++) {
            x_coord[n] = sx_coord[n];
            y_coord[n] = sy_coord[n];
            z_coord[n] = floatData[t * surfaceDimX * surfaceDimY + n] * zScale;
        }

        auto vl = outSurface->cl().data();
        for (int j = 0; j < snumPolygons * 4; j++)
            /* vl[j] = surfaceConnectivityList[j]; */
            vl[j] = surfacePolygons->cl()[j];

        auto pl = outSurface->el().data();
        for (int p = 0; p < snumPolygons; p++)
            pl[p] = surfacePolygonList[p];

        token.addObject(m_surface_out, outSurface);
        i++;
    }

    /* token.addObject(m_surface_out, objs); */
    token.addObject(m_seaSurface_out, polygonsSeaSurface);
    /* token.addObject(m_maxHeight, scalarMaxHeight); */

    delete floatData;
    delete maxH;
    return true;
    /* Index steps = m_step->getValue(); */
    /* if (steps > 0 && timestep < 0) { */
    /*     // don't generate constant data when animation has been requested */
    /*     return true; */
    /* } */

    /* Index blocks[3]; */
    /* for (int i = 0; i < 3; ++i) { */
    /*     blocks[i] = m_blocks[i]->getValue(); */
    /* } */

    /* Index b = blockNum; */
    /* Index bx = b % blocks[0]; */
    /* b /= blocks[0]; */
    /* Index by = b % blocks[1]; */
    /* b /= blocks[1]; */
    /* Index bz = b; */

    /* block(token, bx, by, bz, blockNum, timestep); */

    /* return true; */
}

bool ReadTsunami::finishRead()
{
    /* if (ncDataFile) { */
    /*     delete ncDataFile; */
    /*     ncDataFile = nullptr; */
    /* } */
    return true;
}
