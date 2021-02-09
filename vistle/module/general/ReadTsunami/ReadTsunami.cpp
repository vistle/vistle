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
#include <array>
#include <cstddef>
#include <memory>
#include <string>

using namespace vistle;
using namespace netCDF;

MODULE_MAIN(ReadTsunami)

ReadTsunami::ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Tsunami files", name, moduleID, comm)
{
    // define parameters

    // file-browser
    p_filedir = addStringParameter("file_dir", "NC File directory", "/data/ChEESE/Tsunami/pelicula_eta.nc",
                                   Parameter::Filename);

    // visualise variables
    p_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter", 1.0);

    // define ports

    // 2D Surface
    p_seaSurface_out = createOutputPort("surfaceOut", "2D Grid output (Polygons)");
    p_groundSurface_out = createOutputPort("seaSurfaceOut", "2D See floor (Polygons)");
    p_maxHeight = createOutputPort("maxHeight", "Max water height (Float)");

    p_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 0);

    //observer parameters
    observeParameter(p_filedir);

    /* setParallelizationMode(ParallelizeBlocks); */
    setParallelizationMode(Serial);
    /* setAllowTimestepDistribution(true); */
    initTupList();
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
            p_ncDataFile = std::unique_ptr<NcFile>(new NcFile(sFileName.c_str(), NcFile::read));
            sendInfo("Reading File: " + sFileName);
        } catch (...) {
            sendInfo("Couldn't open NetCDF file!");
            return false;
        }

        if (p_ncDataFile->getVarCount() == 0) {
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
    /* vec_NcVar.push_back(std::make_unique<NcVar>(latvar)); */
    /* vec_NcVar.push_back(std::make_unique<NcVar>(lonvar)); */
    /* vec_NcVar.push_back(std::make_unique<NcVar>(grid_latvar)); */
    /* vec_NcVar.push_back(std::make_unique<NcVar>(grid_lonvar)); */
    /* vec_NcVar.push_back(std::make_unique<NcVar>(bathymetryvar)); */
    /* vec_NcVar.push_back(std::make_unique<NcVar>(max_height)); */
    /* vec_NcVar.push_back(std::make_unique<NcVar>(eta)); */
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
    for (const auto &ptr_NcVar: vec_NcVar) {
        if (ptr_NcVar->isNull()) {
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
        latvar = p_ncDataFile->getVar("lat");
        lonvar = p_ncDataFile->getVar("lon");
        grid_latvar = p_ncDataFile->getVar("grid_lat");
        grid_lonvar = p_ncDataFile->getVar("grid_lon");
        bathymetryvar = p_ncDataFile->getVar("bathymetry");
        max_height = p_ncDataFile->getVar("max_height");
        eta = p_ncDataFile->getVar("eta");

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
    numPolygons = (gridLatDimX - 1) * (gridLonDimY - 1);
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
    std::string timestep_repr = std::to_string(timestep);

    sendInfo("reading timestep: " + timestep_repr);

    if (timestep == -1) {
        //create surfaces ground and sea

        //****************** Create Sea surface ******************//
        //dim vector
        std::vector dimSurface{surfaceDimX, surfaceDimY, surfaceDimZ};

        // num of polygons
        size_t surfaceNumPoly = (surfaceDimX - 1) * (surfaceDimY - 1);

        // pointer for lat values and coords
        std::unique_ptr<float[]> ptr_lat(new float[surfaceDimX]);
        std::unique_ptr<float[]> ptr_lon(new float[surfaceDimY]);
        std::vector coords{ptr_lat.get(), ptr_lon.get()};

        // read in lat var ncdata into float-pointer
        latvar.getVar(ptr_lat.get());
        lonvar.getVar(ptr_lon.get());

        // Now for the 2D variables, we create a surface
        ptr_sea = generateSurface(surfaceNumPoly, surfaceNumPoly * 4, surfaceDimX * surfaceDimY, dimSurface, coords);

        //****************** Create Ground surface ******************//
        ptr_lat = std::unique_ptr<float[]>(new float[surfaceDimX]);
        ptr_lon = std::unique_ptr<float[]>(new float[surfaceDimY]);

        // depth
        std::unique_ptr<float[]> ptr_depth(new float[gridLatDimX * gridLonDimY]);

        // set where to stream to data (float pointer)
        grid_latvar.getVar(ptr_lat.get());
        grid_lonvar.getVar(ptr_lon.get());
        bathymetryvar.getVar(ptr_depth.get());

        Polygons::ptr grnd(new Polygons(numPolygons, numPolygons * 4, gridLatDimX * gridLonDimY));
        ptr_ground = grnd;

        // Fill the _coord arrays (memcpy faster?)
        int n{0};
        auto x_coord = grnd->x().data(), y_coord = grnd->y().data(), z_coord = grnd->z().data();
        for (size_t j = 0; j < gridLatDimX; j++)
            for (size_t k = 0; k < gridLonDimY; k++, n++) {
                x_coord[n] = ptr_lat.get()[j];
                y_coord[n] = ptr_lon.get()[k];
                z_coord[n] = zCalcGround(ptr_depth.get(), j, k, gridLonDimY);
            }

        // Fill the connectivitylist list = numPolygons * 4
        fillConnectList2DimPoly(grnd, gridLatDimX, gridLonDimY);

        // Fill the polygon list
        fillPolyList(grnd, numPolygons, 4);

        //****************** Set polygons to ports ******************//
        ptr_sea->updateInternals();
        /* sea->setBlock(block); */
        ptr_sea->setTimestep(-1);
        ptr_sea->addAttribute("_species", "sea surface");
        token.addObject(p_seaSurface_out, ptr_sea);

        ptr_ground->updateInternals();
        ptr_ground->addAttribute("_species", "ground surface");
        token.addObject(p_groundSurface_out, ptr_ground);

    } else {
        //****************** modify sea surface based on eta and height ******************//
        //TODO: parallelization with blocks

        // create watersurface with polygons for each timestep
        Polygons::ptr ptr_timestepPoly = ptr_sea->clone();

        ptr_timestepPoly->resetArrays();

        // reuse data from sea polygon surface
        ptr_timestepPoly->d()->x[0] = ptr_sea->d()->x[0];
        ptr_timestepPoly->d()->x[1] = ptr_sea->d()->x[1];
        ptr_timestepPoly->d()->x[2].construct(ptr_timestepPoly->getSize());

        std::vector<float> source(surfaceDimX * surfaceDimY);

        for (size_t n = 0; n < surfaceDimX * surfaceDimY; n++)
            source[n] = zCalcSeaHeight(vec_eta.data(), surfaceDimX, surfaceDimY, zScale, timestep, n);

        std::copy(source.begin(), source.end(), ptr_timestepPoly->z().begin());

        ptr_timestepPoly->updateInternals();
        ptr_timestepPoly->setTimestep(timestep);
        /* outSurface->setBlock(block); */
        ptr_timestepPoly->addAttribute("_species", "sea surface timestep: " + timestep_repr);
        token.applyMeta(ptr_timestepPoly);
        token.addObject(p_seaSurface_out, ptr_timestepPoly);
    }
    return true;
}
