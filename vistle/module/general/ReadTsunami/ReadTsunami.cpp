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
#include "vistle/core/vertexownerlist.h"
#include "vistle/module/module.h"

//openmpi
#include <cstddef>
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
}

ReadTsunami::~ReadTsunami()
{}

/**
 * Open Nc File and set pointer ncDataFile.
 *
 * @return true if its not empty or cannot be opened.
 */
bool ReadTsunami::openNcFile(NcFile &file)
{
    std::string sFileName = p_filedir->getValue();

    if (sFileName.empty()) {
        sendInfo("NetCDF filename is empty!");
        return false;
    } else {
        try {
            file.open(sFileName.c_str(), NcFile::read);
            sendInfo("Reading File: " + sFileName);
        } catch (...) {
            sendInfo("Couldn't open NetCDF file!");
            return false;
        }
        if (file.getVarCount() == 0) {
            sendInfo("empty NetCDF file!");
            return false;
        } else {
            return true;
        }
    }
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

    int nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue();
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
template<class T, class U>
void ReadTsunami::fillCoordsPoly2Dim(Polygons::ptr poly, const Dim<T> &dim, const std::vector<U> &coords,
                                     const zCalcFunc &zCalc)
{
    int n = 0;
    auto sx_coord = poly->x().data(), sy_coord = poly->y().data(), sz_coord = poly->z().data();
    for (size_t i = 0; i < dim.dimLat; i++)
        for (size_t j = 0; j < dim.dimLon; j++, n++) {
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
template<class T>
void ReadTsunami::fillConnectListPoly2Dim(Polygons::ptr poly, const Dim<T> &dim)
{
    int n = 0;
    auto verticeConnectivityList = poly->cl().data();
    for (size_t j = 1; j < dim.dimLat; j++)
        for (size_t k = 1; k < dim.dimLon; k++) {
            verticeConnectivityList[n++] = (j - 1) * dim.dimLon + (k - 1);
            verticeConnectivityList[n++] = j * dim.dimLon + (k - 1);
            verticeConnectivityList[n++] = j * dim.dimLon + k;
            verticeConnectivityList[n++] = (j - 1) * dim.dimLon + k;
        }
}

/**
  * Set which vertices represent a polygon.
  *
  * @poly: Pointer on Polygon.
  * @numPoly: number of polygons.
  * @numCorner: number of corners.
  */
template<class T>
void ReadTsunami::fillPolyList(Polygons::ptr poly, const T &numCorner)
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
template<class U, class T, class V>
Polygons::ptr ReadTsunami::generateSurface(const PolygonData<U> &polyData, const Dim<T> &dim,
                                           const std::vector<V> &coords, const zCalcFunc &zCalc)
{
    Polygons::ptr surface(new Polygons(polyData.numElements, polyData.numCorners, polyData.numVertices));

    // fill coords 2D
    fillCoordsPoly2Dim(surface, dim, coords, zCalc);

    // fill vertices
    fillConnectListPoly2Dim(surface, dim);

    // fill the polygon list
    fillPolyList(surface, 4);

    return surface;
}

/**
  * Generates NcVarParams struct which contains start, count and stride values computed based on given parameters.
  *
  * @dim: Current dimension of NcVar.
  * @ghost: Number of ghost cells to add.
  * @numDimBlocks: Total number of blocks for this direction.
  * @partition: Partition scalar.
  * @return: NcVarParams object.
  */
template<class T, class PartionMultiplicator>
NcVarParams<T> ReadTsunami::generateNcVarParams(const T &dim, const T &ghost, const T &numDimBlock,
                                                const PartionMultiplicator &partition)
{
    T count = dim / numDimBlock;
    T start = partition * count;
    addGhostStructured_tmpl(start, count, dim, ghost);
    return NcVarParams<T>(start, count);
}

/**
  * Called for each timestep with number of blocks (MPISIZE).
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
template<class T, class U>
bool ReadTsunami::computeBlock(Reader::Token &token, const T &blockNum, const U &timestep)
{
    if (timestep == -1)
        return computeInitialPolygon(token, blockNum);
    else
        return computeTimestepPolygon(token, timestep, blockNum);
}

/**
  * Generates the inital polygon surfaces for sea and ground and adds them to scene.
  *
  * @token Ref to internal vistle token.
  */
template<class T>
bool ReadTsunami::computeInitialPolygon(Token &token, const T &blockNum)
{
    NcFile ncFile;
    if (!openNcFile(ncFile))
        return false;

    // get nc var objects
    NcVar latvar = ncFile.getVar("lat");
    NcVar lonvar = ncFile.getVar("lon");
    NcVar grid_lat = ncFile.getVar("grid_lat");
    NcVar grid_lon = ncFile.getVar("grid_lon");
    NcVar bathymetryvar = ncFile.getVar("bathymetry");
    NcVar max_height = ncFile.getVar("max_height");
    NcVar eta = ncFile.getVar("eta");

    // get vertical Scale
    zScale = p_verticalScale->getValue();

    // dimension from lat and lon variables
    Dim dimSea(latvar.getDim(0).getSize(), lonvar.getDim(0).getSize());

    // get dim from grid_lon & grid_lat
    Dim dimGround(grid_lat.getDim(0).getSize(), grid_lon.getDim(0).getSize());

    std::array<Index, 2> blocks;
    for (int i = 0; i < 2; i++)
        blocks[i] = m_blocks[i]->getValue();

    auto numLatBlocks = m_blocks[0]->getValue();
    auto numLonBlocks = m_blocks[1]->getValue();
    size_t ghost = p_ghostLayerWidth->getValue();

    std::vector<Index> dist(2);
    auto blockPartion = blockPartitionStructured_tmpl(blocks.begin(), blocks.end(), dist.begin(), blockNum);

    if (numLatBlocks == 1 || numLonBlocks == 1)
        ghost = 0;

    // count and start vals for lat and lon for sea polygon
    auto latSea = generateNcVarParams<decltype(ghost), decltype(blockPartion[0])>(dimSea.dimLat, ghost, numLatBlocks,
                                                                                  blockPartion[0]);
    auto lonSea = generateNcVarParams<decltype(ghost), decltype(blockPartion[1])>(dimSea.dimLon, ghost, numLonBlocks,
                                                                                  blockPartion[1]);

    // count and start vals for lat and lon for ground polygon
    auto latGround = generateNcVarParams<decltype(ghost), decltype(blockPartion[0])>(dimGround.dimLat, ghost,
                                                                                     numLatBlocks, blockPartion[0]);
    auto lonGround = generateNcVarParams<decltype(ghost), decltype(blockPartion[1])>(dimGround.dimLon, ghost,
                                                                                     numLonBlocks, blockPartion[1]);

    // num of polygons for sea & grnd
    size_t numPolySea = (latSea.count - 1) * (lonSea.count - 1);
    size_t numPolyGround = (latGround.count - 1) * (lonGround.count - 1);

    // vertices sea & grnd
    verticesSea = latSea.count * lonSea.count;
    size_t verticesGround = latGround.count * lonGround.count;

    // pointers for read in values from ncdata
    std::vector<float> vecLat(latSea.count), vecLon(lonSea.count), vecLatGrid(latGround.count),
        vecLonGrid(lonGround.count), vecDepth(verticesGround);

    // need Eta-data for timestep poly => member var of reader
    auto nTimesteps = eta.getDim(0).getSize();
    actualLastTimestep = m_last->getValue() - (m_last->getValue() % m_increment->getValue());
    /* nTimesteps = m_last->getValue() - m_first->getValue(); */
    vecEta.resize(nTimesteps * verticesSea);
    std::vector<size_t> vecStartLat{latSea.start}, vecStartLon{lonSea.start},
        vecStartDepth{latGround.start, lonGround.start}, vecStartLatGrid{latGround.start},
        vecStartLonGrid{lonGround.start}, vecStartEta{0, latSea.start, lonSea.start};
        /* vecStartLonGrid{lonGround.start}, */
        /* vecStartEta{m_first->getValue() * verticesSea, latSea.start, lonSea.start}; */

    // count vectors
    std::vector<size_t> vecCountLat{latSea.count}, vecCountLon{lonSea.count}, vecCountLatGrid{latGround.count},
        vecCountLonGrid{lonGround.count}, vecCountDepth{latGround.count, lonGround.count},
        vecCountEta{nTimesteps, latSea.count, lonSea.count};

    std::vector coords{vecLat.data(), vecLon.data()};

    // read in ncdata into float-pointer
    latvar.getVar(vecStartLat, vecCountLat, vecLat.data());
    lonvar.getVar(vecStartLon, vecCountLon, vecLon.data());
    grid_lat.getVar(vecStartLatGrid, vecCountLatGrid, vecLatGrid.data());
    grid_lon.getVar(vecStartLonGrid, vecCountLonGrid, vecLonGrid.data());
    bathymetryvar.getVar(vecStartDepth, vecCountDepth, vecDepth.data());
    eta.getVar(vecStartEta, vecCountEta, vecEta.data());

    //************* create sea *************//
    ptr_sea =
        generateSurface(PolygonData(numPolySea, numPolySea * 4, verticesSea), Dim(latSea.count, lonSea.count), coords);

    //************* create grnd *************//
    coords[0] = vecLatGrid.data();
    coords[1] = vecLonGrid.data();

    Polygons::ptr ptr_grnd = generateSurface(
        PolygonData(numPolyGround, numPolyGround * 4, verticesGround), Dim(latGround.count, lonGround.count), coords,
        [&vecDepth, &lonGround](size_t j, size_t k) { return -vecDepth[j * lonGround.count + k]; });

    // add data to port
    ptr_grnd->setBlock(blockNum);
    ptr_grnd->setTimestep(-1);
    ptr_grnd->updateInternals();
    token.addObject(p_groundSurface_out, ptr_grnd);

    ncFile.close();
    return true;
}

/**
  * Generates polygon for corresponding timestep and adds Object to scene.
  *
  * @token Ref to internal vistle token.
  * @timestep current timestep.
  */
template<class T, class U>
bool ReadTsunami::computeTimestepPolygon(Token &token, const T &timestep, const U &blockNum)
{
    //****************** modify sea surface based on eta ******************//
    Polygons::ptr ptr_timestepPoly = ptr_sea->clone();

    ptr_timestepPoly->resetArrays();

    // reuse data from sea polygon surface and calculate z new
    ptr_timestepPoly->d()->x[0] = ptr_sea->d()->x[0];
    ptr_timestepPoly->d()->x[1] = ptr_sea->d()->x[1];
    ptr_timestepPoly->d()->x[2].construct(ptr_timestepPoly->getSize());

    // copy only z for current timestep
    // verticesSea * timesteps = total count vecEta
    std::copy_n(vecEta.begin() + timestep * verticesSea, verticesSea, ptr_timestepPoly->z().begin());

    ptr_timestepPoly->updateInternals();
    ptr_timestepPoly->setTimestep(timestep);
    ptr_timestepPoly->setBlock(blockNum);
    token.applyMeta(ptr_timestepPoly);
    token.addObject(p_seaSurface_out, ptr_timestepPoly);

    if (timestep == actualLastTimestep) {
        vecEta.clear();
        vecEta.shrink_to_fit();
        /* ptr_sea.reset(); */
    }
    return true;
}
