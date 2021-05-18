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
#include "vistle/core/scalar.h"
#include "vistle/core/vec.h"
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

    //ghost
    addIntParameter("ghost", "Show ghostcells.", 1, Parameter::Boolean);

    // visualise variables
    p_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter sea", 1.0);

    // define ports
    p_seaSurface_out = createOutputPort("Sea surface", "2D Grid Sea (Polygons)");
    p_groundSurface_out = createOutputPort("Ground surface", "2D Sea floor (Polygons)");

    // block size
    m_blocks[0] = addIntParameter("blocks latitude", "number of blocks in lat-direction", 2);
    m_blocks[1] = addIntParameter("blocks longitude", "number of blocks in lon-direction", 2);
    setParameterRange(m_blocks[0], Integer(1), Integer(999999));
    setParameterRange(m_blocks[1], Integer(1), Integer(999999));

    //bathymetryname
    p_bathy = addStringParameter("bathymetry ", "Select bathymetry stored in netCDF", "", Parameter::Choice);

    //scalar
    initScalarParamReader();

    // timestep built-in params
    setParameterRange(m_first, Integer(0), Integer(9999999));
    setParameterRange(m_last, Integer(-1), Integer(9999999));
    setParameterRange(m_increment, Integer(1), Integer(9999999));

    // observer these parameters
    observeParameter(p_filedir);
    observeParameter(m_blocks[0]);
    observeParameter(m_blocks[1]);
    observeParameter(p_verticalScale);

    setParallelizationMode(ParallelizeBlocks);
}

/**
 * @brief Destructor.
 */
ReadTsunami::~ReadTsunami()
{}

/**
 * @brief Initialize scalar choice parameter and ports.
 */
void ReadTsunami::initScalarParamReader()
{
    constexpr auto scalar_name{"Scalar "};
    constexpr auto scalarPort_name{"Scalar port "};
    constexpr auto scalarPort_descr{"Port for scalar number "};

    for (Index i = 0; i < NUM_SCALARS; ++i) {
        const std::string i_str{std::to_string(i)};
        const std::string &scName = scalar_name + i_str;
        const std::string &portName = scalarPort_name + i_str;
        const std::string &portDescr = scalarPort_descr + i_str;
        m_scalars[i] = addStringParameter(scName, "Select scalar.", "", Parameter::Choice);
        m_scalarsOut[i] = createOutputPort(portName, portDescr);
        observeParameter(m_scalars[i]);
    }
}

/**
 * Open Nc File and set pointer ncDataFile.
 *
 * @return true if its not empty or cannot be opened.
 */
bool ReadTsunami::openNcFile(NcFile &file) const
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
inline void ReadTsunami::printMPIStats() const
{
    sendInfo("Current Rank: " + std::to_string(rank()) + " Processes (MPISIZE): " + std::to_string(size()));
}

/**
  * Prints thread-id to /var/tmp/<user>/ReadTsunami-*.
  */
inline void ReadTsunami::printThreadStats() const
{
    std::cout << std::this_thread::get_id() << '\n';
}

/**
  * Called when any of the reader parameter changing.
  *
  * @param: Parameter that got changed.
  * @return: true if all essential parameters could be initialized.
  */
bool ReadTsunami::examine(const vistle::Parameter *param)
{
    if (!param || param == p_filedir) {
        if (rank() == 0)
            printMPIStats();

        if (!inspectNetCDFVars())
            return false;
    }

    const int &nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue();
    setTimesteps(-1);
    if (nBlocks == size()) {
        setPartitions(nBlocks);
        return true;
    } else {
        sendInfo("Number of blocks should equal MPISIZE.");
        return false;
    }
}

/**
 * @brief Inspect netCDF variables stored in file.
 */
bool ReadTsunami::inspectNetCDFVars()
{
    NcFile ncFile;
    if (!openNcFile(ncFile))
        return false;

    std::vector<std::string> scalarChoiceVec;
    std::vector<std::string> bathyChoiceVec;
    auto latLonContainsGrid = [=](auto &name, int i) {
        if (name.find("grid") != std::string::npos)
            m_latLon_Ground[i] = name;
        else
            m_latLon_Surface[i] = name;
    };

    //delete previous choicelists.
    p_bathy->setChoices(std::vector<std::string>());
    for (const auto &scalar: m_scalars)
        scalar->setChoices(std::vector<std::string>());

    for (auto &[name, val]: ncFile.getVars()) {
        if (name.find("lat") != std::string::npos)
            latLonContainsGrid(name, 0);
        else if (name.find("lon") != std::string::npos)
            latLonContainsGrid(name, 1);
        else if (name.find("bathy") != std::string::npos)
            bathyChoiceVec.push_back(name);
        else if (val.getDimCount() == 2) // for now: only scalars with 2 dim depend on lat lon.
            scalarChoiceVec.push_back(name);
    }

    setParameterChoices(p_bathy, bathyChoiceVec);

    for (auto &scalar: m_scalars)
        setParameterChoices(scalar, scalarChoiceVec);

    return true;
}

/**
  * Set 2D coordinates for given polygon.
  *
  * @poly: Pointer on Polygon.
  * @dim: Dimension of coordinates.
  * @coords: Vector which contains coordinates.
  * @zCalc: Function for computing z-coordinate.
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
  * @dim: Dimension of vertice list.
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
 * @polyData: Data for creating polygon-surface (number elements, number corners, number vertices).
 * @dim: Dimension in lat and lon.
 * @coords: coordinates for polygons.
 * @zCalc: Function for computing z-coordinate.
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
template<class T, class PartionIdx>
NcVarParams<T> ReadTsunami::generateNcVarParams(const T &dim, const T &ghost, const T &numDimBlock,
                                                const PartionIdx &partition) const
{
    T count = dim / numDimBlock;
    T start = partition * count;
    addGhostStructured_tmpl(start, count, dim, ghost);
    return NcVarParams<T>(start, count);
}

/**
  * Called for each timestep and for each block (MPISIZE).
  *
  * @token: Ref to internal vistle token.
  * @timestep: current timestep.
  * @block: current block number of parallelization.
  * @return: true if all data is set and valid.
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
  * @token: Ref to internal vistle token.
  * @blockNum: current block number of parallelization.
  * @timestep: current timestep.
  * @return: true if all data is set and valid.
  */
template<class T, class U>
bool ReadTsunami::computeBlock(Reader::Token &token, const T &blockNum, const U &timestep)
{
    if (timestep == -1)
        return computeInitial(token, blockNum);
    else
        return computeTimestep<int, size_t>(token, blockNum, timestep);
}

/**
 * @brief Compute actual last timestep.
 *
 * @param incrementTimestep Stepwidth.
 * @param firstTimestep first timestep.
 * @param lastTimestep last timestep selected.
 * @param nTimesteps Storage value for number of timesteps.
 */
void ReadTsunami::computeActualLastTimestep(const ptrdiff_t &incrementTimestep, const size_t &firstTimestep,
                                            size_t &lastTimestep, size_t &nTimesteps)
{
    if (lastTimestep < 0)
        lastTimestep--;

    m_actualLastTimestep = lastTimestep - (lastTimestep % incrementTimestep);
    if (incrementTimestep > 0)
        if (m_actualLastTimestep >= firstTimestep)
            nTimesteps = (m_actualLastTimestep - firstTimestep) / incrementTimestep + 1;
}

/**
 * @brief Compute the unique block partition index borders for current block.
 *
 * @tparam Iter Iterator.
 * @param blockNum Current block.
 * @param ghost Initial number of ghostcells.
 * @param nLatBlocks Storage for number of blocks for latitude.
 * @param nLonBlocks Storage for number of blocks for longitude.
 * @param blockPartitionIterFirst Start iterator for storage partion indices.
 */
template<class Iter>
void ReadTsunami::computeBlockPartion(const int blockNum, size_t &ghost, vistle::Index &nLatBlocks,
                                      vistle::Index &nLonBlocks, Iter blockPartitionIterFirst)
{
    std::array<Index, NUM_BLOCKS> blocks;
    for (int i = 0; i < NUM_BLOCKS; i++)
        blocks[i] = m_blocks[i]->getValue();

    nLatBlocks = blocks[0];
    nLonBlocks = blocks[1];

    if (getIntParameter("ghost") && !(nLatBlocks == 1 && nLonBlocks == 1))
        ghost++;

    blockPartitionStructured_tmpl(blocks.begin(), blocks.end(), blockPartitionIterFirst, blockNum);
}

/**
 * @brief Reads given NcVar into storage container in chunks specified with ncParams.
 *
 * @tparam T Start and count type for read operation in data. (default = float)
 * @tparam U Stride type for read operation through data. (default = ptrdiff_t)
 * @tparam S Storage type.
 * @param storage Storage ptr.
 * @param ncParams Vector container which holds NcVarParams object references.
 * @param nc NcVar object to be read.
 */
template<class T, class U, class S>
void ReadTsunami::readNcVar(S *storage, const NcVarContainer<T, U> &ncParams, const netCDF::NcVar &nc)
{
    std::vector<T> start;
    std::vector<T> count;
    std::vector<U> stride;
    for (auto &val: ncParams) {
        start.push_back(val.start);
        count.push_back(val.count);
        stride.push_back(val.stride);
    }

    nc.getVar(start, count, storage);
}

/**
  * Generates the inital polygon surfaces for sea and ground and adds them to scene.
  *
  * @token: Ref to internal vistle token.
  * @blockNum: current block number of parallel process.
  * @return: true if all date could be initialized.
  */
template<class T>
bool ReadTsunami::computeInitial(Token &token, const T &blockNum)
{
    NcFile ncFile;
    if (!openNcFile(ncFile))
        return false;

    // get nc var objects ref
    const NcVar &latvar = ncFile.getVar(m_latLon_Surface[0]);
    const NcVar &lonvar = ncFile.getVar(m_latLon_Surface[1]);
    const NcVar &grid_lat = ncFile.getVar(m_latLon_Ground[0]);
    const NcVar &grid_lon = ncFile.getVar(m_latLon_Ground[1]);
    const NcVar &bathymetryvar = ncFile.getVar(p_bathy->getValue());
    const NcVar &eta = ncFile.getVar("eta");

    // compute current time parameters
    const ptrdiff_t &incrementTimestep = m_increment->getValue();
    const size_t &firstTimestep = m_first->getValue();
    size_t lastTimestep = m_last->getValue();
    size_t nTimesteps{0};
    computeActualLastTimestep(incrementTimestep, firstTimestep, lastTimestep, nTimesteps);

    // compute partition borders
    size_t ghost{0};
    Index nLatBlocks;
    Index nLonBlocks;
    std::array<Index, NUM_BLOCKS> bPartitionIdx;
    computeBlockPartion(blockNum, ghost, nLatBlocks, nLonBlocks, bPartitionIdx.begin());

    // dimension from lat and lon variables
    const Dim dimSea(latvar.getDim(0).getSize(), lonvar.getDim(0).getSize());

    // get dim from grid_lon & grid_lat
    const Dim dimGround(grid_lat.getDim(0).getSize(), grid_lon.getDim(0).getSize());

    // count and start vals for lat and lon for sea polygon
    const auto latSea = generateNcVarParams<size_t, Index>(dimSea.dimLat, ghost, nLatBlocks, bPartitionIdx[0]);
    const auto lonSea = generateNcVarParams<size_t, Index>(dimSea.dimLon, ghost, nLonBlocks, bPartitionIdx[1]);

    // count and start vals for lat and lon for ground polygon
    const auto latGround = generateNcVarParams<size_t, Index>(dimGround.dimLat, ghost, nLatBlocks, bPartitionIdx[0]);
    const auto lonGround = generateNcVarParams<size_t, Index>(dimGround.dimLon, ghost, nLonBlocks, bPartitionIdx[1]);

    // num of polygons for sea & grnd
    const size_t &numPolySea = (latSea.count - 1) * (lonSea.count - 1);
    const size_t &numPolyGround = (latGround.count - 1) * (lonGround.count - 1);

    // vertices sea & grnd
    verticesSea = latSea.count * lonSea.count;
    const size_t &verticesGround = latGround.count * lonGround.count;

    // storage for read in values from ncdata
    std::vector<float> vecLat(latSea.count), vecLon(lonSea.count), vecLatGrid(latGround.count),
        vecLonGrid(lonGround.count), vecDepth(verticesGround);

    // need Eta-data for timestep poly => member var of reader
    vecEta.resize(nTimesteps * verticesSea);

    // read parameter for eta
    const std::vector<size_t> vecStartEta{firstTimestep, latSea.start, lonSea.start};
    const std::vector<size_t> vecCountEta{nTimesteps, latSea.count, lonSea.count};
    const std::vector<ptrdiff_t> vecStrideEta{incrementTimestep, latSea.stride, lonSea.stride};

    // read in ncdata into float-pointer
    readNcVar(vecLat.data(), std::vector{latSea}, latvar);
    readNcVar(vecLon.data(), std::vector{lonSea}, lonvar);
    readNcVar(vecLatGrid.data(), std::vector{latGround}, grid_lat);
    readNcVar(vecLonGrid.data(), std::vector{lonGround}, grid_lon);
    readNcVar(vecDepth.data(), std::vector{latGround, lonGround}, bathymetryvar);
    eta.getVar(vecStartEta, vecCountEta, vecStrideEta, vecEta.data());

    //************* create sea *************//
    std::vector coords{vecLat.data(), vecLon.data()};
    const auto &seaDim = Dim(latSea.count, lonSea.count);
    const auto &polyDataSea = PolygonData(numPolySea, numPolySea * 4, verticesSea);
    ptr_sea = generateSurface(polyDataSea, seaDim, coords);

    //************* create grnd *************//
    coords[0] = vecLatGrid.data();
    coords[1] = vecLonGrid.data();

    const auto &scale = p_verticalScale->getValue();
    const auto &grndDim = Dim(latGround.count, lonGround.count);
    const auto &polyDataGround = PolygonData(numPolyGround, numPolyGround * 4, verticesGround);
    auto grndZCalc = [&vecDepth, &lonGround, &scale](size_t j, size_t k) {
        return -vecDepth[j * lonGround.count + k] * scale;
    };
    Polygons::ptr ptr_grnd = generateSurface(polyDataGround, grndDim, coords, grndZCalc);

    //************* create selected scalars *************//
    const std::vector<size_t> vecScalarStart{latSea.start, lonSea.start};
    const std::vector<size_t> vecScalarCount{latSea.count, lonSea.count};
    std::vector<float> vecScalar(verticesGround);
    for (size_t i = 0; i < NUM_SCALARS; ++i) {
        sendInfo("In");
        if (!m_scalarsOut[i]->isConnected())
            continue;
        sendInfo("out");

        const auto &scName = m_scalars[i]->getValue();
        const auto &val = ncFile.getVar(scName);
        auto &ptr_scalar = ptr_Scalar[i];
        ptr_scalar.reset();
        ptr_scalar->setSize(verticesSea);
        sendInfo("%d", ptr_scalar->getSize());
        auto scX = ptr_scalar->x().data();
        val.getVar(vecScalarStart, vecScalarCount, scX);
        sendInfo("read");
        ptr_scalar->setMapping(DataBase::Element);
        ptr_scalar->addAttribute("_species", scName);
        ptr_scalar->setBlock(blockNum);
        sendInfo("out2");
    }

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
  * @token: Ref to internal vistle token.
  * @blockNum: current block number of parallel process.
  * @timestep: current timestep.
  * @return: true. TODO: add proper error-handling here.
  */
template<class T, class U>
bool ReadTsunami::computeTimestep(Token &token, const T &blockNum, const U &timestep)
{
    Polygons::ptr ptr_timestepPoly = ptr_sea->clone();
    static int indexEta{0};

    ptr_timestepPoly->resetArrays();

    // reuse data from sea polygon surface and calculate z new
    ptr_timestepPoly->d()->x[0] = ptr_sea->d()->x[0];
    ptr_timestepPoly->d()->x[1] = ptr_sea->d()->x[1];
    ptr_timestepPoly->d()->x[2].construct(ptr_timestepPoly->getSize());

    // getting z from vecEta and copy to z()
    // verticesSea * timesteps = total count vecEta

    // debugging
    /* sendInfo("timestep COVER: " + std::to_string(indexEta)); */
    auto startCopy = vecEta.begin() + indexEta++ * verticesSea;
    std::copy_n(startCopy, verticesSea, ptr_timestepPoly->z().begin());
    /* auto zPoly = ptr_timestepPoly->z().data(); */
    /* auto start = indexEta++ * verticesSea; */
    /* for (size_t i = start; i < verticesSea; i++) { */
    /*     auto &tmp = vecEta[i]; */
    /*     if (tmp <= -9999) */
    /*         zPoly[i] = 0; */
    /*     else */
    /*         zPoly[i] = tmp; */
    /* } */
    for (size_t i = 0; i < NUM_SCALARS; ++i) {
        if (!m_scalarsOut[i]->isConnected())
            continue;

        auto scalar = ptr_Scalar[i]->clone();
        scalar->setGrid(ptr_timestepPoly);
        scalar->setTimestep(timestep);
        scalar->updateInternals();

        token.addObject(m_scalarsOut[i], scalar);
    }

    if (p_seaSurface_out->isConnected()) {
        ptr_timestepPoly->updateInternals();
        ptr_timestepPoly->setTimestep(timestep);
        ptr_timestepPoly->setBlock(blockNum);
        token.applyMeta(ptr_timestepPoly);
        token.addObject(p_seaSurface_out, ptr_timestepPoly);
    }

    if (timestep == m_actualLastTimestep) {
        sendInfo("Cleared Cache for rank: " + std::to_string(rank()));
        vecEta.clear();
        vecEta.shrink_to_fit();
        indexEta = 0;
    }
    return true;
}
