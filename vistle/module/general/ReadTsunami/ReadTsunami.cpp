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

//templates
#include "ReadTsunami_impl.h"

//vistle
#include "vistle/core/database.h"
#include "vistle/core/index.h"
#include "vistle/core/object.h"
#include "vistle/core/parameter.h"
#include "vistle/core/polygons.h"
#include "vistle/module/module.h"

//openmpi
#include <mpi.h>

//std
#include <algorithm>
#include <array>
#include <iterator>
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
    p_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter sea", 1.0);
    p_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 1);
    setParameterRange(p_ghostLayerWidth, Integer(0), Integer(999999));

    // define ports
    p_seaSurface_out = createOutputPort("Sea surface", "2D Grid Sea (Polygons)");
    p_groundSurface_out = createOutputPort("Ground surface", "2D Sea floor (Polygons)");
    /* p_maxHeight = createOutputPort("maxHeight", "Max water height (Float)"); */

    // block size
    m_blocks[0] = addIntParameter("blocks latitude", "number of blocks in lat-direction", 2);
    m_blocks[1] = addIntParameter("blocks longitude", "number of blocks in lon-direction", 2);
    setParameterRange(m_blocks[0], Integer(1), Integer(999999));
    setParameterRange(m_blocks[1], Integer(1), Integer(999999));

    // observer these parameters
    observeParameter(p_filedir);
    observeParameter(m_blocks[0]);
    observeParameter(m_blocks[1]);

    setParallelizationMode(ParallelizeBlocks);
    initNcVarVec();
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
  * Prints current rank and the number of all ranks to the console.
  */
inline void ReadTsunami::printMPIStats()
{
    sendInfo("Current Rank: " + std::to_string(rank()) + " Processes (MPISIZE): " + std::to_string(size()));
}

/**
  * Prints thread-id to /var/tmp/<user>/ReadTsunami-*.
  */
inline void ReadTsunami::printThreadStats()
{
    std::cout << std::this_thread::get_id() << '\n';
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
        if (rank() == 0)
            printMPIStats();
    }

    size_t nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue();
    /* setTimesteps(-1); */
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
                                     const std::vector<float *> &coords, const zCalcFunc &zCalc)
{
    //TODO: maybe define template or use algo for dump filling.
    int n = 0;
    auto sx_coord = poly->x().data(), sy_coord = poly->y().data(), sz_coord = poly->z().data();
    for (size_t i = 0; i < dimX; i++)
        for (size_t j = 0; j < dimY; j++, n++) {
            sx_coord[n] = coords.at(0)[i];
            sy_coord[n] = coords.at(1)[j];
            sz_coord[n] = zCalc(i, j);
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
    std::generate(poly->el().begin(), poly->el().end(), [n = 0, &numCorner]() mutable { return n++ * numCorner; });
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
                                           const size_t &dimX, const size_t &dimY, const std::vector<float *> &coords,
                                           const zCalcFunc &zCalc)
{
    Polygons::ptr surface(new Polygons(numElem, numCorner, numVertices));

    // fill coords 2D
    fillCoordsPoly2Dim(surface, dimX, dimY, coords, zCalc);

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
bool ReadTsunami::computeInitialPolygon(Token &token, const Index &blockNum)
{
    if (!initNcData())
        return false;

    initHelperVariables();

    std::array<Index, 2> blocks;
    for (int i = 0; i < 2; i++)
        blocks[i] = m_blocks[i]->getValue();

    auto numLatBlocks = m_blocks[0]->getValue();
    auto numLonBlocks = m_blocks[1]->getValue();
    size_t ghost = p_ghostLayerWidth->getValue();

    std::vector<Index> dist(2);
    auto blockLat = *blockPartitionStructured_tmpl(blocks.begin(), blocks.end(), dist.begin(), blockNum);
    auto blockLon = dist[1];

    // count and start vals for lat and lon for sea polygon
    size_t cntLatSea = dimLat / numLatBlocks;
    size_t cntLonSea = dimLon / numLonBlocks;
    size_t strtLatSea = blockLat * cntLatSea;
    size_t strtLonSea = blockLon * cntLonSea;

    // count and start vals for lat and lon for grnd polygon
    size_t cntLatGrnd = gridDimLat / numLatBlocks;
    size_t cntLonGrnd = gridDimLon / numLonBlocks;
    size_t strtLatGrnd = blockLat * cntLatGrnd;
    size_t strtLonGrnd = blockLon * cntLonGrnd;

    // add Ghostcells
    if (numLatBlocks == 1 || numLonBlocks == 1)
        ghost = 0;

    addGhostStructured_tmpl(strtLatSea, cntLatSea, dimLat, ghost);
    addGhostStructured_tmpl(strtLonSea, cntLonSea, dimLon, ghost);
    addGhostStructured_tmpl(strtLatGrnd, cntLatGrnd, gridDimLat, ghost);
    addGhostStructured_tmpl(strtLonGrnd, cntLonGrnd, gridDimLon, ghost);

    // num of polygons for sea & grnd
    size_t numPolySea = (cntLatSea - 1) * (cntLonSea - 1);
    size_t numPolyGrnd = (cntLatGrnd - 1) * (cntLonGrnd - 1);

    // vertices sea & grnd
    size_t vertSea = cntLatSea * cntLonSea;
    size_t vertGrnd = cntLatGrnd * cntLonGrnd;

    //************* create sea *************//
    // pointer for lat values and coords
    std::vector<float> vecLat(cntLatSea);
    std::vector<float> vecLon(cntLonSea);

    // start vector lat and lon
    std::vector<size_t> vecStrtLat{strtLatSea}, vecStrtLon{strtLonSea};

    // count vector for lat and lon
    std::vector<size_t> vecCntLat{cntLatSea}, vecCntLon{cntLonSea};
    std::vector coords{vecLat.data(), vecLon.data()};

    // read in lat var ncdata into float-pointer
    latvar.getVar(vecStrtLat, vecCntLat, vecLat.data());
    lonvar.getVar(vecStrtLon, vecCntLon, vecLon.data());

    // create a surface for sea
    ptr_sea = generateSurface(numPolySea, numPolySea * 4, vertSea, cntLatSea, cntLonSea, coords);

    //************* create grnd *************//
    std::vector<float> vecLatGrid(cntLatGrnd), vecLonGrid(cntLonGrnd);

    // depth
    std::vector<float> vecDepth(vertGrnd);
    std::vector<size_t> vecStrtDepth{strtLatGrnd, strtLonGrnd};
    std::vector<size_t> vecCntDepth{cntLatGrnd, cntLonGrnd};

    std::vector<size_t> vecStrtLatGrid{strtLatGrnd}, vecStrtLonGrid{strtLonGrnd};
    std::vector<size_t> vecCntLatGrid{cntLatGrnd}, vecCntLonGrid{cntLonGrnd};

    // set where to stream data to (float pointer)
    grid_latvar.getVar(vecStrtLatGrid, vecCntLatGrid, vecLatGrid.data());
    grid_lonvar.getVar(vecStrtLonGrid, vecCntLonGrid, vecLonGrid.data());
    bathymetryvar.getVar(vecStrtDepth, vecCntDepth, vecDepth.data());

    coords[0] = vecLatGrid.data();
    coords[1] = vecLonGrid.data();

    Polygons::ptr ptr_grnd =
        generateSurface(numPolyGrnd, numPolyGrnd * 4, vertGrnd, cntLatGrnd, cntLonGrnd, coords,
                        [&vecDepth, &cntLonGrnd](size_t j, size_t k) { return -vecDepth[j * cntLonGrnd + k]; });

    ptr_grnd->setBlock(blockNum);
    ptr_grnd->setTimestep(-1);
    ptr_grnd->updateInternals();
    token.addObject(p_groundSurface_out, ptr_grnd);

    //************* read in eta for other timesteps > eta = dim3 *************//
    std::vector<size_t> strtEta{0, strtLatSea, strtLonSea};
    std::vector<size_t> cntEta{eta.getDim(0).getSize(), cntLatSea, cntLonSea};

    vec_eta.resize(eta.getDim(0).getSize() * cntLatSea * cntLonSea);
    eta.getVar(strtEta, cntEta, vec_eta.data());

    m_ncDataFile.close();
    return true;
}

/**
  * Generates polygon for corresponding timestep and adds Object to scene.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  */
bool ReadTsunami::computeTimestepPolygon(Token &token, const Index &timestep, const Index &blockNum)
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
    if (rank() == 0)
        sendInfo("reading timestep: " + std::to_string(timestep));

    return computeBlock(token, block, timestep);
}

/**
  * Computing per block.
  *
  * @token Ref to internal vistle token.
  * @block current block number of parallelization.
  * @timestep current timestep.
  * @return true if all data is set and valid.
  */
bool ReadTsunami::computeBlock(Reader::Token &token, vistle::Index block, vistle::Index timestep)
{
    if (timestep == -1)
        return computeInitialPolygon(token, block);
    else
        return computeTimestepPolygon(token, timestep, block);
}
