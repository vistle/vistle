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
#include "vistle/core/polygons.h"
#include "vistle/core/scalar.h"
#include "vistle/core/vec.h"
#include "vistle/core/vector.h"
#include "vistle/module/module.h"

//std
#include <algorithm>
#include <cstddef>
#include <memory>
#include <mpi.h>
#include <string>

using namespace vistle;
using namespace netCDF;

MODULE_MAIN(ReadTsunami)

ReadTsunami::ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Tsunami files", name, moduleID, comm)
{
    // file-browser
    p_filedir = addStringParameter("file_dir", "NC File directory", "/data/ChEESE/tsunami/pelicula_eta.nc",
                                   Parameter::Filename);

    // visualise variables
    p_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter", 1.0);
    p_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 1);
    setParameterRange(p_ghostLayerWidth, Integer(0), Integer(999999));

    // define ports

    // 2D Surface
    p_seaSurface_out = createOutputPort("surfaceOut", "2D Grid output (Polygons)");
    p_groundSurface_out = createOutputPort("seaSurfaceOut", "2D See floor (Polygons)");
    p_maxHeight = createOutputPort("maxHeight", "Max water height (Float)");

    // block size
    m_blocks[0] = addIntParameter("blocks_lat", "number of blocks in lat-direction", 2);
    m_blocks[1] = addIntParameter("blocks_lon", "number of blocks in lon-direction", 2);

    // observer these parameters
    observeParameter(p_filedir);
    observeParameter(m_blocks[0]);
    observeParameter(m_blocks[1]);

    setParallelizationMode(ParallelizeBlocks);
    initNcVarVec();
}

/**
 * Open Nc File and set pointer ncDataFile.
 *
 * @return true if its not empty or cannot be opened.
 */
bool ReadTsunami::openNcFile()
{
    std::string sFileName = p_filedir->getValue();

    if (sFileName.empty()) {
        sendInfo("NetCDF filename is empty!");
        return false;
    } else {
        try {
            m_ncDataFile.open(sFileName.c_str(), NcFile::read);
            sendInfo("Reading File: " + sFileName);
        } catch (...) {
            sendInfo("Couldn't open NetCDF file!");
            return false;
        }

        /* if (p_ncDataFile->getVarCount() == 0) { */
        if (m_ncDataFile.getVarCount() == 0) {
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
void ReadTsunami::initNcVarVec()
{
    vec_NcVar.push_back(&latvar);
    vec_NcVar.push_back(&lonvar);
    vec_NcVar.push_back(&grid_latvar);
    vec_NcVar.push_back(&grid_lonvar);
    vec_NcVar.push_back(&bathymetryvar);
    vec_NcVar.push_back(&max_height);
    vec_NcVar.push_back(&eta);
}

/**
  * Called when any of the reader parameter changing.
  *
  * @param Parameter that got changed.
  * @return true if all essential parameters could be initialized.
  */
bool ReadTsunami::examine(const vistle::Parameter *param)
{
    if (!param || param == p_filedir) {
        if (!initNcData())
            return false;
        initHelperVariables();
        /* setTimesteps(eta.getDim(0).getSize()); */
        /* setTimesteps(-1); */
    }

    size_t nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue();
    setTimesteps(-1);
    setPartitions(nBlocks);
    return nBlocks > 0;
}

/**
  * Set 2D coordinates for given polygon.
  *
  * @poly: Pointer on Polygon.
  * @dimX: Dimension of coordinates in x.
  * @dimY: Dimension of coordinates in y.
  * @coords: Vector which contains coordinates.
  */
void ReadTsunami::fillCoordsPoly2Dim(Polygons::ptr poly, const size_t &dimX, const size_t &dimY,
                                     const std::vector<float *> &coords)
{
    //TODO: maybe define template or use algo for dump filling.
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
  * Set the connectivitylist for given polygon for 2 dimensions and 4 Corners.
  *
  * @poly: Pointer on Polygon.
  * @dimX: Dimension of vertice list in x direction.
  * @dimY: Dimension of vertice list in y direction.
  */
void ReadTsunami::fillConnectListPoly2Dim(Polygons::ptr poly, const size_t &dimX, const size_t &dimY)
{
    //TODO: maybe define template or use algo for dump filling of arrays.
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
void ReadTsunami::fillPolyList(Polygons::ptr poly, const size_t &numCorner)
{
    //TODO: maybe define template or use algo for dump filling.
    auto polyList = poly->el().data();
    for (size_t p = 0; p <= poly->getNumElements(); p++)
        polyList[p] = p * numCorner;
}

/**
 * Generate surface from polygons.
 *
 * @numElem Number of polygons that will be used to great the surface.
 * @numCorner Number of all corners.
 * @numVertices Number of all different corners.
 * @coords coordinates for polygons.
 * @return vistle::Polygons::ptr
 */
Polygons::ptr ReadTsunami::generateSurface(const size_t &numElem, const size_t &numCorner, const size_t &numVertices,
                                           const size_t &dimX, const size_t &dimY, const std::vector<float *> &coords)
{
    //TODO: make function more useable in general => at the moment only 2 dim based on same data like ChEESE-tsunami.
    Polygons::ptr surface(new Polygons(numElem, numCorner, numVertices));
    sendInfo("inside generate");

    // fill coords 2D
    fillCoordsPoly2Dim(surface, dimX, dimY, coords);

    // fill vertices
    fillConnectListPoly2Dim(surface, dimX, dimY);

    // fill the polygon list
    fillPolyList(surface, 4);

    return surface;
}

/**
  * Check if NcVars in t_NcVar are not null.
  *
  * @return: false if one of the variables are null.
  */
bool ReadTsunami::checkValidNcVar()
{
    return std::all_of(vec_NcVar.cbegin(), vec_NcVar.cend(), [](NcVar *var) { return !var->isNull(); });
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
        latvar = m_ncDataFile.getVar("lat");
        lonvar = m_ncDataFile.getVar("lon");
        grid_latvar = m_ncDataFile.getVar("grid_lat");
        grid_lonvar = m_ncDataFile.getVar("grid_lon");
        bathymetryvar = m_ncDataFile.getVar("bathymetry");
        max_height = m_ncDataFile.getVar("max_height");
        eta = m_ncDataFile.getVar("eta");

        return checkValidNcVar();
    }
    return false;
}

/**
  * Initialize some helper variables and pointers.
  * NcVars needs to be initialized before.
  */
void ReadTsunami::initHelperVariables()
{
    if (!checkValidNcVar()) {
        sendInfo("Helper variables cannot be initialized.");
        return;
    }

    // read max_height
    vec_maxH.resize(max_height.getDim(0).getSize() * max_height.getDim(1).getSize());
    max_height.getVar(vec_maxH.data());

    // read in eta
    vec_eta.resize(eta.getDim(0).getSize() * eta.getDim(1).getSize() * eta.getDim(2).getSize());
    eta.getVar(vec_eta.data());

    // get vertical Scale
    zScale = p_verticalScale->getValue();

    // dimension from lat and lon variables
    dimLat = latvar.getDim(0).getSize();
    dimLon = lonvar.getDim(0).getSize();
    surfaceDimZ = 0;

    // get dim from grid_lon & grid_lat
    gridDimLat = grid_latvar.getDim(0).getSize();
    gridDimLon = grid_lonvar.getDim(0).getSize();
    gridPolygons = (gridDimLat - 1) * (gridDimLon - 1);
}

/**
  * Generates the inital polygon surfaces for sea and ground and adds them to scene.
  *
  * @token Ref to internal vistle token.
  */
void ReadTsunami::computeInitialPolygon(Token &token, const Index &blockNum)
{
    Index blocks[2];
    for (int i = 0; i < 2; i++)
        blocks[i] = m_blocks[i]->getValue();

    auto numLatBlocks = m_blocks[0]->getValue();
    auto numLonBlocks = m_blocks[1]->getValue();
    auto ghost = p_ghostLayerWidth->getValue();

    Index b = blockNum;
    Index blockLat{b % blocks[0]};
    b /= blocks[0];
    Index blockLon{b};

    //****************** Create Sea surface ******************//
    size_t startLat = blockLat * dimLat / numLatBlocks;
    size_t startLon = blockLon * dimLon / numLonBlocks;
    size_t countLat = dimLat / numLatBlocks;
    size_t countLon = dimLon / numLonBlocks;

    // add Ghostcells
    if (startLat == 0) {
        countLat += ghost;
    } else if (startLat + countLat == dimLat) {
        startLat -= ghost;
        countLat += ghost;
    } else {
        countLat += 2*ghost;
        startLat -= ghost;
    }

    if (startLon == 0) {
        countLon += ghost;
    } else if (startLon + countLon == dimLon) {
        startLon -= ghost;
        countLon += ghost;
    } else {
        countLon += 2*ghost;
        startLon -= ghost;
    }

    sendInfo("blockNum: " + std::to_string(blockNum) + " blockLat: " + std::to_string(blockLat) +
             " blockLon: " + std::to_string(blockLon) + " startLat: " + std::to_string(startLat) +
             " startLon: " + std::to_string(startLon) + " countLat: " + std::to_string(countLat) +
             " countLon: " + std::to_string(countLon));

    std::vector<size_t> vecStartLat{startLat}, vecStartLon{startLon};
    std::vector<size_t> vecCountLat{countLat}, vecCountLon{countLon};

    // num of polygons for sea & grnd
    /* size_t sea_numPoly = (dimLat - 1) * (dimLon - 1) / (numLatBlocks * numLonBlocks); */
    size_t sea_numPoly = (countLat - 1) * (countLon - 1);
    /* size_t grnd_numPoly = gridPolygons / (blockX * blockY); */

    // vertices sea & grnd
    /* size_t sea_vertices = dimLat * dimLon / (numLatBlocks * numLonBlocks); */
    size_t sea_vertices = countLat * countLon;
    /* size_t grnd_vertices = gridLatDimX * gridLonDimY / (blockX * blockY); */

    // pointer for lat values and coords
    std::vector<float> vecLat(countLat);
    std::vector<float> vecLon(countLon);
    std::vector coords{vecLat.data(), vecLon.data()};

    // read in lat var ncdata into float-pointer
    latvar.getVar(vecStartLat, vecCountLat, vecLat.data());
    lonvar.getVar(vecStartLon, vecCountLon, vecLon.data());

    // create a surface for sea
    /* ptr_sea = generateSurface(sea_numPoly, sea_numPoly * 4, sea_vertices, dimLat / numLatBlocks, */
    /*                           dimLon / numLonBlocks, coords); */
    Polygons::ptr ptr_s = generateSurface(sea_numPoly, sea_numPoly * 4, sea_vertices, countLat, countLon, coords);

    //****************** Create Ground surface ******************//
    /* vecLat.resize(gridLatDimX / blockX); */
    /* vecLon.resize(gridLonDimY / blockY); */

    /* // depth */
    /* std::vector<float> vecDepth(grnd_numPoly / (blockX * blockY)); */
    /* std::vector<size_t> startBathy{blockNum * grnd_numPoly / (blockX * blockY)}; */
    /* std::vector<size_t> countBathy{grnd_numPoly / (blockX * blockY)}; */

    /* // set where to stream data to (float pointer) */
    /* grid_latvar.getVar(startLat, countLat, vecLat.data()); */
    /* grid_lonvar.getVar(startLon, countLon, vecLon.data()); */
    /* bathymetryvar.getVar(startBathy, countBathy, vecDepth.data()); */

    /* Polygons::ptr grnd(new Polygons(grnd_numPoly, grnd_numPoly * 4, grnd_vertices)); */
    /* ptr_ground = grnd; */

    /* //_____________________________________________________________________________________// */

    /* // Fill the coord arrays */
    /* int n{0}; */
    /* auto x_coord = grnd->x().data(), y_coord = grnd->y().data(), z_coord = grnd->z().data(); */
    /* for (size_t j = 0; j < gridLatDimX / blockX; j++) */
    /*     for (size_t k = 0; k < gridLonDimY / blockY; k++, n++) { */
    /*         x_coord[n] = vecLat[j]; */
    /*         y_coord[n] = vecLon[k]; */

    /*         //design data is equal to 2 dim array printed to vector */
    /*         //ptr_begin_arr2 = row * number of columns => begin of 2 dim array (e.g. float[][ptr*]) */
    /*         //element_inside_second_arr = ptr_begin_arr2 + number of searching element in arr2 */
    /*         z_coord[n] = -vecDepth[j * gridLonDimY + k]; */
    /*     } */

    /* // Fill the connectivitylist list = numPolygons * 4 */
    /* fillConnectListPoly2Dim(grnd, gridLatDimX / blockX, gridLonDimY / blockY); */

    /* // Fill the polygon list */
    /* fillPolyList(grnd, 4); */

    //****************** Set polygons to ports ******************//
    /* ptr_sea->setBlock(blockNum); */
    /* ptr_sea->updateInternals(); */
    /* ptr_sea->setTimestep(-1); */
    ptr_s->setBlock(blockNum);
    ptr_s->updateInternals();
    ptr_s->setTimestep(-1);
    token.addObject(p_seaSurface_out, ptr_s);

    /* ptr_ground->updateInternals(); */
    /* ptr_ground->setBlock(blockNum); */
    /* token.addObject(p_groundSurface_out, ptr_ground); */
}

/**
  * Generates polygon for corresponding timestep and adds Object to scene.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  */
void ReadTsunami::computeTimestepPolygon(Token &token, const Index &timestep, const Index &blockNum)
{
    //****************** modify sea surface based on eta and height ******************//
    //TODO: parallelization with blocks

    // create watersurface with polygons for each timestep
    Polygons::ptr ptr_timestepPoly = ptr_sea->clone();

    ptr_timestepPoly->resetArrays();

    // reuse data from sea polygon surface and calculate z new
    ptr_timestepPoly->d()->x[0] = ptr_sea->d()->x[0];
    ptr_timestepPoly->d()->x[1] = ptr_sea->d()->x[1];
    ptr_timestepPoly->d()->x[2].construct(ptr_timestepPoly->getSize());

    std::vector<float> source(dimLat * dimLon);

    for (size_t n = 0; n < dimLat * dimLon; n++)
        source[n] = vec_eta[timestep * dimLat * dimLon + n] * zScale;

    // copy only z
    std::copy(source.begin(), source.end(), ptr_timestepPoly->z().begin());

    ptr_timestepPoly->updateInternals();
    ptr_timestepPoly->setTimestep(timestep);
    ptr_timestepPoly->setBlock(blockNum);
    token.applyMeta(ptr_timestepPoly);
    token.addObject(p_seaSurface_out, ptr_timestepPoly);
}

/**
  * Called for each timestep or block based on parallelizationMode.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  * @block current block number of parallelization.
  * @return true if all data is set and valid.
  */
bool ReadTsunami::read(Token &token, int timestep, int block)
{
    sendInfo("reading timestep: " + std::to_string(timestep));
    computeBlock(token, block, timestep);
    return true;
}

/**
  * Computing per block. //TODO: implement later
  */
void ReadTsunami::computeBlock(Reader::Token &token, vistle::Index block, vistle::Index time)
{
    if (time == -1) {
        computeInitialPolygon(token, block);
    } else {
        /* computeTimestepPolygon(token, time, block); */
    }
}
