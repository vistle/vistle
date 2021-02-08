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

//header
#include "ReadTsunami.h"

//vistle
#include "vistle/core/database.h"
#include "vistle/core/index.h"
#include "vistle/core/object.h"
#include "vistle/core/parameter.h"
#include "vistle/core/scalar.h"
#include "vistle/core/vector.h"
#include "vistle/module/module.h"

//std
#include <algorithm>
#include <string>

using namespace vistle;
using namespace netCDF;

MODULE_MAIN(ReadTsunami)

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
    /* setParameterRange(m_step, Integer(0), Integer(999999)); */

    // define ports

    // 2D Surface
    m_seaSurface_out = createOutputPort("surfaceOut", "2D Grid output (Polygons)");
    m_groundSurface_out = createOutputPort("seaSurfaceOut", "2D See floor (Polygons)");
    /* m_waterSurface_out = createOutputPort("waterSurfaceOut", "2D water surface (Polygons)"); */
    m_maxHeight = createOutputPort("maxHeight", "Max water height (Float)");

    m_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 0);

    //observer parameters
    observeParameter(m_filedir);

    /* setParallelizationMode(ParallelizeBlocks); */
    vistle::Reader::setParallelizationMode(Serial);
    initTupList();
}

ReadTsunami::~ReadTsunami()
{
    delete[] etaPtr;
    delete[] scalarMaxHeight;
}

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

/**
  * Initialize vector t_NcVar with NcVar pointers. (needed for checkValidNcVar)
  */
void ReadTsunami::initTupList()
{
    t_NcVar.push_back(&latvar);
    t_NcVar.push_back(&lonvar);
    t_NcVar.push_back(&grid_latvar);
    t_NcVar.push_back(&grid_lonvar);
    t_NcVar.push_back(&bathymetryvar);
    t_NcVar.push_back(&max_height);
    t_NcVar.push_back(&eta);
}

/**
  * Called when any of the reader parameter changing.
  *
  * @param Parameter that got changed.
  * @return true if all essential parameters could be initialized.
  */
bool ReadTsunami::examine(const vistle::Parameter *param)
{
    if (!param || param == m_filedir) {
        if (!initNcData())
            return false;
        sendInfo("Examine!");
        initHelperVariables();
    }

    setTimesteps(eta.getDim(0).getSize());
    setPartitions(1);
    return true;
}

/**
  * Set 2D coordinates for given polygon.
  *
  * @poly: Pointer on Polygon.
  * @dimX: Dimension of coordinates in x.
  * @dimY: Dimension of coordinates in y.
  * @coords: Vector which contains coordinates.
  */
void ReadTsunami::fillCoords2DimPoly(Polygons::ptr poly, const size_t &dimX, const size_t &dimY,
                                     const std::vector<float *> &coords)
{
    int n = 0;
    auto sx_coord = poly->x().data(), sy_coord = poly->y().data(), sz_coord = poly->z().data();

    for (size_t i = 0; i < dimX; i++)
        for (size_t j = 0; j < dimY; j++, n++) {
            sx_coord[n] = coords.at(0)[i];
            sy_coord[n] = coords.at(1)[j];
            sz_coord[n] = 0;
        }
}

/**
  * Set 3D coordinates for given polygon.
  *
  * @poly: Pointer on Polygon.
  * @dimX: Dimension of coordinates in x.
  * @dimY: Dimension of coordinates in y.
  * @dimZ: Dimension of coordinates in z.
  * @coords: Vector which contains coordinates.
  */
void ReadTsunami::fillCoords3DimPoly(Polygons::ptr poly, const size_t &dimX, const size_t &dimY, const size_t &dimZ,

                                     const std::vector<float *> &coords)
{
    int n = 0;
    auto x_coord = poly->x().data(), y_coord = poly->y().data(), z_coord = poly->z().data();
    for (size_t i = 0; i < dimX; i++)
        for (size_t j = 0; j < dimY; j++)
            for (size_t k = 0; k < dimZ; k++, n++) {
                x_coord[n] = coords.at(0)[i];
                y_coord[n] = coords.at(1)[j];
                z_coord[n] = coords.at(2)[k];
            }
}

/**
  * Set the connectivitylist for given polygon for 2 dimensions and 4 Corners.
  *
  * @poly: Pointer on Polygon.
  * @dimX: Dimension of vertice list in x direction.
  * @dimY: Dimension of vertice list in y direction.
  */
void ReadTsunami::fillConnectList2DimPoly(Polygons::ptr poly, const size_t &dimX, const size_t &dimY)
{
    int n = 0;
    auto verticeConnectivityList = poly->cl().data();
    for (size_t j = 1; j < dimX; j++)
        for (size_t k = 1; k < dimY; k++) {
            verticeConnectivityList[n++] = (j - 1) * dimY + (k - 1);
            verticeConnectivityList[n++] = j * dimY + (k - 1);
            verticeConnectivityList[n++] = j * dimY + k;
            verticeConnectivityList[n++] = (j - 1) * dimY + k;
        }
}

/**
  * Set which vertices represent a polygon.
  *
  * @poly: Pointer on Polygon.
  * @numPoly: number of polygons.
  * @numCorner: number of corners.
  */
void ReadTsunami::fillPolyList(Polygons::ptr poly, const size_t &numPoly, const size_t &numCorner)
{
    auto polyList = poly->el().data();
    for (size_t p = 0; p <= numPoly; p++)
        polyList[p] = p * 4;
}

/**
 * Generate surface from polygons.
 *
 * @numElem Number of polygons that will be used to great the surface.
 * @numCorner Number of all corners.
 * @numVertices Number of all different corners.
 * @dimension dimension in x,y,z.
 * @coords coordinates for polygons.
 * @return vistle::Polygons::ptr
 */
Polygons::ptr ReadTsunami::generateSurface(const size_t &numElem, const size_t &numCorner, const size_t &numVertices,
                                           const std::vector<size_t> &dimension, const std::vector<float *> &coords)
{
    Polygons::ptr surface(new Polygons(numElem, numCorner, numVertices));

    size_t surfaceDimX = dimension.at(0);
    size_t surfaceDimY = dimension.at(1);
    size_t surfaceDimZ = dimension.at(2);

    if (surfaceDimZ == 0) {
        // fill coords 2D
        fillCoords2DimPoly(surface, surfaceDimX, surfaceDimY, coords);
    } else {
        // fill coords 3D
        fillCoords3DimPoly(surface, surfaceDimX, surfaceDimY, surfaceDimZ, coords);
    }

    // fill vertices
    fillConnectList2DimPoly(surface, surfaceDimX, surfaceDimY);

    // fill the polygon list
    fillPolyList(surface, numElem, 4);

    return surface;
}

/**
  * Computing per block. //TODO: implement later
  */
void ReadTsunami::block(Reader::Token &token, Index bx, Index by, Index bz, vistle::Index block,
                        vistle::Index time) const
{}

/**
  * Check if NcVars in t_NcVar are not null.
  *
  * @return: true if one of the variables are null.
  */
bool ReadTsunami::checkValidNcVar()
{
    for (NcVar *var: t_NcVar) {
        if (var->isNull()) {
            sendInfo("Invalid parameters! Look into method initNcData.");
            return false;
        }
    }
    return true;
}

/**
  * Init NcVar data.
  *
  * @return true if parameters are valid and could be initialized.
  */
bool ReadTsunami::initNcData()
{
    if (openNcFile()) {
        // read variables from NetCDF-File
        latvar = ncDataFile->getVar("lat");
        lonvar = ncDataFile->getVar("lon");
        grid_latvar = ncDataFile->getVar("grid_lat");
        grid_lonvar = ncDataFile->getVar("grid_lon");
        bathymetryvar = ncDataFile->getVar("bathymetry");
        max_height = ncDataFile->getVar("max_height");
        eta = ncDataFile->getVar("eta");

        return checkValidNcVar();
    }
    return false;
}

void ReadTsunami::initHelperVariables()
{
    // read max_height
    scalarMaxHeight = new float[max_height.getDim(0).getSize() * max_height.getDim(1).getSize()];
    max_height.getVar(scalarMaxHeight);

    // read in eta
    etaPtr = new float[eta.getDim(0).getSize() * eta.getDim(1).getSize() * eta.getDim(2).getSize()];
    eta.getVar(etaPtr);

    // get vertical Scale
    zScale = m_verticalScale->getValue();
}

/**
  * Called for each timestep or block based on parallelizationMode.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  * @block parallelization block.
  * @return true if all data is set and valid.
  */
bool ReadTsunami::read(Token &token, int timestep, int block)
{
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

    sendInfo("timestep: " + std::to_string(timestep));

    // dimension from lat and lon variables
    size_t surfaceDimX{latvar.getDim(0).getSize()};
    size_t surfaceDimY{lonvar.getDim(0).getSize()};
    size_t surfaceDimZ{0};

    // get dim from grid_lon & grid_lat
    size_t gridLatDimX = grid_latvar.getDim(0).getSize();
    size_t gridLonDimY = grid_lonvar.getDim(0).getSize();
    int numPolygons = (gridLatDimX - 1) * (gridLonDimY - 1);

    if (timestep == -1) {
        //create surfaces ground and sea

        //****************** Create Sea surface ******************//
        //dim vector
        std::vector dimSurface{surfaceDimX, surfaceDimY, surfaceDimZ};

        // num of polygons
        size_t surfaceNumPoly = (surfaceDimX - 1) * (surfaceDimY - 1);

        // pointer for lat values and coords
        float *latVals = new float[surfaceDimX];
        float *lonVals = new float[surfaceDimY];
        std::vector coords{latVals, lonVals};

        // read in lat var ncdata into float-pointer
        latvar.getVar(latVals);
        lonvar.getVar(lonVals);

        // Now for the 2D variables, we create a surface
        sea = generateSurface(surfaceNumPoly, surfaceNumPoly * 4, surfaceDimX * surfaceDimY, dimSurface, coords);

        // Delete buffers from grid replication
        delete[] latVals;
        delete[] lonVals;

        //****************** Create Ground surface ******************//
        latVals = new float[gridLatDimX];
        lonVals = new float[gridLonDimY];

        // depth
        float *depthVals = new float[gridLatDimX * gridLonDimY];

        // set where to stream to data (float pointer)
        grid_latvar.getVar(latVals);
        grid_lonvar.getVar(lonVals);
        bathymetryvar.getVar(depthVals);

        Polygons::ptr grnd(new Polygons(numPolygons, numPolygons * 4, gridLatDimX * gridLonDimY));
        ground = grnd;

        // Fill the _coord arrays (memcpy faster?)
        int n{0};
        auto x_coord = grnd->x().data(), y_coord = grnd->y().data(), z_coord = grnd->z().data();
        for (size_t j = 0; j < gridLatDimX; j++)
            for (size_t k = 0; k < gridLonDimY; k++, n++) {
                x_coord[n] = latVals[j];
                y_coord[n] = lonVals[k];
                z_coord[n] = zCalcGround(depthVals, j, k, gridLonDimY);
            }

        // Fill the connectivitylist list = numPolygons * 4
        fillConnectList2DimPoly(grnd, gridLatDimX, gridLonDimY);

        // Fill the polygon list
        fillPolyList(grnd, numPolygons, 4);

        // Delete buffers from grid replication
        delete[] latVals;
        delete[] lonVals;
        delete[] depthVals;

        //****************** Set polygons to ports ******************//
        sea->updateInternals();
        /* sea->setBlock(block); */
        sea->setTimestep(-1);
        sea->addAttribute("_species", "sea surface");
        token.addObject(m_seaSurface_out, sea);

        ground->updateInternals();
        ground->addAttribute("_species", "ground surface");
        token.addObject(m_groundSurface_out, ground);

    } else {
        //****************** modify sea surface based on eta and height ******************//
        // read in max_height
        /* Vec<Scalar>::ptr scalarMaxHeight( */
        /*     new Vec<Scalar>(max_height.getDim(0).getSize() * max_height.getDim(1).getSize())); */
        /* float* scalarMaxHeight = new float[max_height.getDim(0).getSize() * max_height.getDim(1).getSize()]; */
        /* max_height.getVar(scalarMaxHeight); */

        // read in eta
        /* float *etaPtr = new float[eta.getDim(0).getSize() * eta.getDim(1).getSize() * eta.getDim(2).getSize()]; */
        /* Vec<Scalar>::ptr scalarETA( */
        /*     new Vec<Scalar>(eta.getDim(0).getSize() * eta.getDim(1).getSize() * eta.getDim(2).getSize())); */
        /* auto etaPtr = scalarETA->x().data(); */
        /* eta.getVar(etaPtr); */

        // get vertical Scale
        /* float zScale = m_verticalScale->getValue(); */

        // create watersurface with polygons for each timestep
        /* Polygons::ptr outSurface(new Polygons(numPolygons, numPolygons * 4, gridLatDimX * gridLonDimY)); */
        Polygons::ptr outSurface = sea->clone();

        /* auto x_coord = outSurface->x().data(), y_coord = outSurface->y().data(), z_coord = outSurface->z().data(); */
        auto z_coord = outSurface->z().data();

        for (size_t n = 0; n < surfaceDimX * surfaceDimY; n++) {
            z_coord[n] = zCalcSeaHeight(etaPtr, surfaceDimX, surfaceDimY, zScale, timestep, n);
        }

        sendInfo(std::to_string(z_coord[surfaceDimX*surfaceDimY - 1]));

        outSurface->updateInternals();
        outSurface->setTimestep(timestep);
        /* outSurface->setBlock(block); */
        outSurface->addAttribute("_species", "sea surface timestep: " + std::to_string(timestep));
        token.applyMeta(outSurface);
        token.addObject(m_seaSurface_out, outSurface);
    }
    return true;
}

/**
  * Called after the read function.
  *
  * @return true
  */
bool ReadTsunami::finishRead()
{
    return true;
}
