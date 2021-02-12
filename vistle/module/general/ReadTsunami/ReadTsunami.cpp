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
    // define parameters

    // file-browser
    p_filedir = addStringParameter("file_dir", "NC File directory", "/data/ChEESE/tsunami/pelicula_eta.nc",
                                   Parameter::Filename);

    // visualise variables
    p_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter", 1.0);
    p_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 0);

    // define ports

    // 2D Surface
    p_seaSurface_out = createOutputPort("surfaceOut", "2D Grid output (Polygons)");
    p_groundSurface_out = createOutputPort("seaSurfaceOut", "2D See floor (Polygons)");
    p_maxHeight = createOutputPort("maxHeight", "Max water height (Float)");

    m_blocks[0] = addIntParameter("blocks_x", "number of blocks in x-direction", 2);
    m_blocks[1] = addIntParameter("blocks_y", "number of blocks in y-direction", 2);
    m_blocks[2] = addIntParameter("blocks_z", "number of blocks in z-direction", 1);

    //observer parameters
    observeParameter(p_filedir);
    observeParameter(m_blocks[0]);
    observeParameter(m_blocks[1]);
    observeParameter(m_blocks[2]);

    /* setParallelizationMode(ParallelizeBlocks); */
    setParallelizationMode(Serial);
    /* setAllowTimestepDistribution(true); */
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
        setTimesteps(eta.getDim(0).getSize());
    }

    /* size_t nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue() * m_blocks[2]->getValue(); */
    size_t nBlocks = 4;
    /* setTimesteps(eta.getDim(0).getSize()); */
    /* setPartitions(1); */
    setPartitions(nBlocks);
    /* return true; */
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
    /* for (size_t i = 0; i < dimX; i++) */
    /*     for (size_t j = 0; j < dimY; j++, n++) { */
    /*         sx_coord[n] = coords.at(0)[i]; */
    /*         sy_coord[n] = coords.at(1)[j]; */
    /*         sz_coord[n] = 0; */
    /*     } */

    /**
      Some general thoughts for parallelization with 4 blocks. 2 in X , 2 in Y.

      block 0 => n=[0, dimY/2], [dimY,3*dimY/2], [2*dimY, 5*dimY/2], [3*dimY, 7dimY/2] ... [dimX * dimY/2 - dimY, dimX*dimY/2 - dimY/2];
        x_coord = [0, dimX/2],
        y_coord = [0, dimY/2],
      block 1 => n=[dimY/2, dimY], [3*dimY/2, 2*dimY], [5*dimY/2, 3*dimY], ... [dimX*dimY/2 - dimY/2, dimX*dimY/2]
        x_coord = [0, dimX/2],
        y_coord = [dimY/2, dimY],
      block 2 => n=[dimX*dimY/2, dimX*dimY/2 + dimY/2] ... [dimX*dimY - dimY, dimX*dimY - dimY/2]
        x_coord = [dimX/2, dimX],
        y_coord = [0, dimY/2],
      block 3 => n=[dimX*dimY/2 +dimY/2, dimX*dimY/2 + dimY)] ... [dimX*dimY -dimY/2, dimX*dimY]
        x_coord = [dimX/2, dimX],
        y_coord = [dimY/2, dimY],
      */
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
                                           const std::vector<float *> &coords)
{
    //TODO: make function more useable in general => at the moment only 2 dim based on same data like ChEESE-tsunami.
    Polygons::ptr surface(new Polygons(numElem, numCorner, numVertices));

    // fill coords 2D
    fillCoordsPoly2Dim(surface, surfaceDimX, surfaceDimY, coords);

    // fill vertices
    fillConnectListPoly2Dim(surface, surfaceDimX, surfaceDimY);

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
    surfaceDimX = latvar.getDim(0).getSize();
    surfaceDimY = lonvar.getDim(0).getSize();
    surfaceDimZ = 0;

    // get dim from grid_lon & grid_lat
    gridLatDimX = grid_latvar.getDim(0).getSize();
    gridLonDimY = grid_lonvar.getDim(0).getSize();
    gridPolygons = (gridLatDimX - 1) * (gridLonDimY - 1);
}

/**
  * Generates the inital polygon surfaces for sea and ground and adds them to scene.
  *
  * @token Ref to internal vistle token.
  */
void ReadTsunami::computeInitialPolygon(Token &token)
{
    //create surfaces ground and sea

    //****************** Create Sea surface ******************//

    // num of polygons
    size_t surfaceNumPoly = (surfaceDimX - 1) * (surfaceDimY - 1);

    // pointer for lat values and coords
    std::vector<float> vec_lat(surfaceDimX);
    std::vector<float> vec_lon(surfaceDimY);
    std::vector coords{vec_lat.data(), vec_lon.data()};

    // read in lat var ncdata into float-pointer
    latvar.getVar(vec_lat.data());
    lonvar.getVar(vec_lon.data());

    // create a surface for sea
    ptr_sea = generateSurface(surfaceNumPoly, surfaceNumPoly * 4, surfaceDimX * surfaceDimY, coords);

    //****************** Create Ground surface ******************//
    vec_lat.resize(surfaceDimX);
    vec_lon.resize(surfaceDimY);

    // depth
    std::vector<float> vec_depth(surfaceDimX * surfaceDimY);

    // set where to stream data to (float pointer)
    grid_latvar.getVar(vec_lat.data());
    grid_lonvar.getVar(vec_lon.data());
    bathymetryvar.getVar(vec_depth.data());

    Polygons::ptr grnd(new Polygons(gridPolygons, gridPolygons * 4, gridLatDimX * gridLonDimY));
    ptr_ground = grnd;

    // Fill the coord arrays
    int n{0};
    auto x_coord = grnd->x().data(), y_coord = grnd->y().data(), z_coord = grnd->z().data();
    for (size_t j = 0; j < gridLatDimX; j++)
        for (size_t k = 0; k < gridLonDimY; k++, n++) {
            x_coord[n] = vec_lat[j];
            y_coord[n] = vec_lon[k];

            //design data is equal to 2 dim array printed to vector
            //ptr_begin = row * number of columns => begin of 2 dim array (e.g. float[][ptr*])
            //element_inside_second_arr = ptr_begin + number of searching element in arr
            z_coord[n] = -vec_depth[j * gridLonDimY + k];
        }

    // Fill the connectivitylist list = numPolygons * 4
    fillConnectListPoly2Dim(grnd, gridLatDimX, gridLonDimY);

    // Fill the polygon list
    fillPolyList(grnd, 4);

    //****************** Set polygons to ports ******************//
    ptr_sea->updateInternals();
    /* ptr_sea->setBlock(blockNum); */
    ptr_sea->setTimestep(-1);
    token.addObject(p_seaSurface_out, ptr_sea);

    ptr_ground->updateInternals();
    /* ptr_ground->setBlock(blockNum); */
    token.addObject(p_groundSurface_out, ptr_ground);
}

/**
  * Generates the inital polygon surfaces for sea and ground and adds them to scene.
  *
  * @token Ref to internal vistle token.
  */
void ReadTsunami::computeInitialPolygon(Token &token, const int &blockNum)
{
    //create surfaces ground and sea

    //****************** Create Sea surface ******************//

    // num of polygons
    size_t surfaceNumPoly = (surfaceDimX - 1) * (surfaceDimY - 1);

    // pointer for lat values and coords
    std::vector<float> vec_lat(surfaceDimX);
    std::vector<float> vec_lon(surfaceDimY);
    std::vector coords{vec_lat.data(), vec_lon.data()};

    // read in lat var ncdata into float-pointer
    latvar.getVar(vec_lat.data());
    lonvar.getVar(vec_lon.data());

    // create a surface for sea
    ptr_sea = generateSurface(surfaceNumPoly, surfaceNumPoly * 4, surfaceDimX * surfaceDimY, coords);

    //****************** Create Ground surface ******************//
    vec_lat.resize(surfaceDimX);
    vec_lon.resize(surfaceDimY);

    // depth
    std::vector<float> vec_depth(surfaceDimX * surfaceDimY);

    // set where to stream data to (float pointer)
    grid_latvar.getVar(vec_lat.data());
    grid_lonvar.getVar(vec_lon.data());
    bathymetryvar.getVar(vec_depth.data());

    Polygons::ptr grnd(new Polygons(gridPolygons, gridPolygons * 4, gridLatDimX * gridLonDimY));
    ptr_ground = grnd;

    // Fill the coord arrays
    int n{0};
    auto x_coord = grnd->x().data(), y_coord = grnd->y().data(), z_coord = grnd->z().data();
    for (size_t j = 0; j < gridLatDimX; j++)
        for (size_t k = 0; k < gridLonDimY; k++, n++) {
            x_coord[n] = vec_lat[j];
            y_coord[n] = vec_lon[k];

            //design data is equal to 2 dim array printed to vector
            //ptr_begin = row * number of columns => begin of 2 dim array (e.g. float[][ptr*])
            //element_inside_second_arr = ptr_begin + number of searching element in arr
            z_coord[n] = -vec_depth[j * gridLonDimY + k];
        }

    // Fill the connectivitylist list = numPolygons * 4
    fillConnectListPoly2Dim(grnd, gridLatDimX, gridLonDimY);

    // Fill the polygon list
    fillPolyList(grnd, 4);

    //****************** Set polygons to ports ******************//
    ptr_sea->updateInternals();
    ptr_sea->setBlock(blockNum);
    ptr_sea->setTimestep(-1);
    token.addObject(p_seaSurface_out, ptr_sea);

    ptr_ground->updateInternals();
    ptr_ground->setBlock(blockNum);
    token.addObject(p_groundSurface_out, ptr_ground);
}

/**
  * Generates polygon for corresponding timestep and adds Object to scene.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  */
void ReadTsunami::computeTimestepPolygon(Token &token, const int &timestep)
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

    std::vector<float> source(surfaceDimX * surfaceDimY);

    for (size_t n = 0; n < surfaceDimX * surfaceDimY; n++)
        source[n] = vec_eta[timestep * surfaceDimX * surfaceDimY + n] * zScale;

    // copy only z
    std::copy(source.begin(), source.end(), ptr_timestepPoly->z().begin());

    ptr_timestepPoly->updateInternals();
    ptr_timestepPoly->setTimestep(timestep);
    /* outSurface->setBlock(block); */
    token.applyMeta(ptr_timestepPoly);
    token.addObject(p_seaSurface_out, ptr_timestepPoly);
}

/**
  * Generates polygon for corresponding timestep and adds Object to scene.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  */
void ReadTsunami::computeTimestepPolygon(Token &token, const int &timestep, const int &blockNum)
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

    std::vector<float> source(surfaceDimX * surfaceDimY);

    for (size_t n = 0; n < surfaceDimX * surfaceDimY; n++)
        source[n] = vec_eta[timestep * surfaceDimX * surfaceDimY + n] * zScale;

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
  * @block parallelization block.
  * @return true if all data is set and valid.
  */
bool ReadTsunami::read(Token &token, int timestep, int block)
{
    /* Index blocks[3]; */
    /* for (int i = 0; i < 3; ++i) { */
    /*     blocks[i] = m_blocks[i]->getValue(); */
    /* } */

    /* Index b = block; */
    /* Index bx = b % blocks[0]; */
    /* b /= blocks[0]; */
    /* Index by = b % blocks[1]; */
    /* b /= blocks[1]; */
    /* Index bz = b; */
    /* block(token, bx, by, bz, block, timestep); */

    sendInfo("reading timestep: " + std::to_string(timestep));

    if (timestep == -1) {
        computeInitialPolygon(token, block);
    } else {
        computeTimestepPolygon(token, timestep, block);
    }
    return true;
}

/**
  * Computing per block. //TODO: implement later
  */
void ReadTsunami::block(Reader::Token &token, Index bx, Index by, Index bz, vistle::Index block,
                        vistle::Index time) const
{
    /* computeTimestepPolygon(token, time); */
}
