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
#include "vistle/module/module.h"

//vistle-module-util
#include "vistle/module/utils/ghost.h"

//mpi
#include <mpi.h>

//std
#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <cstddef>
#include <thread>
#include <vector>

using namespace vistle;
using namespace PnetCDF;

MODULE_MAIN(ReadTsunami)
namespace {

constexpr auto ETA{"eta"};

} // namespace

ReadTsunami::ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader(name, moduleID, comm), seaTimeConn(false)
{
    // file-browser
    m_filedir = addStringParameter("file_dir", "NC File directory", "/data/ChEESE/tsunami/NewData/cadiz_5m.nc",
                                   Parameter::Filename);

    //ghost
    m_ghost = addIntParameter("ghost", "Show ghostcells.", 1, Parameter::Boolean);
    m_fill = addIntParameter("fill", "Replace filterValue.", 1, Parameter::Boolean);
    m_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter sea", 1.0);

    // define ports
    m_seaSurface_out = createOutputPort("Sea surface", "2D Grid Sea (Polygons)");
    m_groundSurface_out = createOutputPort("Ground surface", "2D Sea floor (Polygons)");

    // block size
    m_blocks[0] = addIntParameter("blocks latitude", "number of blocks in lat-direction", 2);
    m_blocks[1] = addIntParameter("blocks longitude", "number of blocks in lon-direction", 2);
    setParameterRange(m_blocks[0], Integer(1), Integer(999999));
    setParameterRange(m_blocks[1], Integer(1), Integer(999999));

    //fillvalue
    addFloatParameter("fillValue", "ncFile fillValue offset for eta", -9999.f);
    addFloatParameter("fillValueNew", "set new fillValue offset for eta", 0.0f);

    //bathymetryname
    m_bathy = addStringParameter("bathymetry ", "Select bathymetry stored in netCDF", "", Parameter::Choice);

    //scalar
    initScalarParamReader();

    // timestep built-in params
    setParameterRange(m_first, Integer(0), Integer(9999999));
    setParameterRange(m_last, Integer(-1), Integer(9999999));
    setParameterRange(m_increment, Integer(1), Integer(9999999));

    // observer these parameters
    observeParameter(m_filedir);
    observeParameter(m_blocks[0]);
    observeParameter(m_blocks[1]);
    observeParameter(m_verticalScale);

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
 * @brief Open Nc File and set pointer ncDataFile.
 *
 * @return true if its not empty or cannot be opened.
 */
bool ReadTsunami::openNcFile(std::shared_ptr<NcFile> file)
/* bool ReadTsunami::openNcFile(const NcFile &file) */
{
    std::string sFileName = m_filedir->getValue();

    if (sFileName.empty()) {
        printRank0("NetCDF filename is empty!");
        return false;
    } else {
        try {
            /* file->open(sFileName.c_str(), NcFile::read); */
            printRank0("Reading File: " + sFileName);
        } catch (...) {
            printRank0("Couldn't open NetCDF file!");
            return false;
        }
        if (file->getVarCount() == 0) {
            printRank0("empty NetCDF file!");
            return false;
        } else
            return true;
    }
}

/**
 * @brief Print string only on rank 0.
 *
 * @param str Format str to print.
 */
template<class... Args>
void ReadTsunami::printRank0(const std::string &str, Args... args) const
{
    if (rank() == 0)
        sendInfo(str, args...);
}

/**
  * @brief Prints current rank and the number of all ranks to the console.
  */
void ReadTsunami::printMPIStats() const
{
    printRank0("Current Rank: " + std::to_string(rank()) + " Processes (MPISIZE): " + std::to_string(size()));
}

/**
  * @brief Prints thread-id to /var/tmp/<user>/ReadTsunami-*.
  */
inline void ReadTsunami::printThreadStats() const
{
    std::cout << std::this_thread::get_id() << '\n';
}

/**
  * @brief Called when any of the reader parameter changing.
  *
  * @param: Parameter that got changed.
  * @return: true if all essential parameters could be initialized.
  */
bool ReadTsunami::examine(const vistle::Parameter *param)
{
    if (!param || param == m_filedir) {
        printMPIStats();

        if (!inspectNetCDFVars())
            return false;
    }

    const int &nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue();
    //TODO: threadsafe netcdf
    setPartitions(nBlocks);
    return true;
    /* if (nBlocks <= size()) { */
    /*     setPartitions(nBlocks); */
    /*     return true; */
    /* } else { */
    /*     printRank0("Total number of blocks should equal MPISIZE or less."); */
    /*     return false; */
    /* } */
}

/**
 * @brief Inspect netCDF variables stored in file.
 */
bool ReadTsunami::inspectNetCDFVars()
{
    /* std::shared_ptr<NcFile> ncFile(new NcFile()); */
    /* if (!openNcFile(ncFile)) */
    /*     return false; */
    NcFile ncFile(comm(), m_filedir->getValue(), NcFile::read);

    /* const int &maxTime = ncFile->getDim("time").getSize(); */
    const int &maxTime = ncFile.getDim("time").getSize();
    setTimesteps(maxTime);

    //scalar inspection
    std::vector<std::string> scalarChoiceVec;
    std::vector<std::string> bathyChoiceVec;
    auto strContains = [](const std::string &name, const std::string &contains) {
        return name.find(contains) != std::string::npos;
    };
    auto latLonContainsGrid = [=](auto &name, int i) {
        if (strContains(name, "grid"))
            m_latLon_Ground[i] = name;
        else
            m_latLon_Sea[i] = name;
    };

    //delete previous choicelists.
    m_bathy->setChoices(std::vector<std::string>());
    for (const auto &scalar: m_scalars)
        scalar->setChoices(std::vector<std::string>());

    //read names of scalars
    /* for (auto &name_val: ncFile->getVars()) { */
    for (auto &name_val: ncFile.getVars()) {
        auto &name = name_val.first;
        auto &val = name_val.second;
        if (strContains(name, "lat"))
            latLonContainsGrid(name, 0);
        else if (strContains(name, "lon"))
            latLonContainsGrid(name, 1);
        else if (strContains(name, "bathy"))
            bathyChoiceVec.push_back(name);
        else if (val.getDimCount() == 2) // for now: only scalars with 2 dim depend on lat lon.
            scalarChoiceVec.push_back(name);
    }

    //init choice param with scalardata
    setParameterChoices(m_bathy, bathyChoiceVec);

    for (auto &scalar: m_scalars)
        setParameterChoices(scalar, scalarChoiceVec);

    /* ncFile->close(); */

    sendInfo("examine");
    return true;
}

/**
  * @brief Set 2D coordinates for given polygon.
  *
  * @poly: Pointer on Polygon.
  * @dim: Dimension of coordinates.
  * @coords: Vector which contains coordinates.
  * @zCalc: Function for computing z-coordinate.
  */
template<class T, class U>
void ReadTsunami::contructLatLonSurface(Polygons::ptr poly, const Dim<T> &dim, const std::vector<U> &coords,
                                        const zCalcFunc &zCalc)
{
    int n = 0;
    auto sx_coord = poly->x().data(), sy_coord = poly->y().data(), sz_coord = poly->z().data();
    for (size_t i = 0; i < dim.dimLat; i++)
        for (size_t j = 0; j < dim.dimLon; j++, n++) {
            sx_coord[n] = coords.at(1)[j];
            sy_coord[n] = coords.at(0)[i];
            sz_coord[n] = zCalc(i, j);
        }
}

/**
  * @brief Set the connectivitylist for given polygon for 2 dimensions and 4 Corners.
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
  * @brief Set number of vertices which represent a polygon.
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
 * @brief Generate surface from polygons.
 *
 * @polyData: Data for creating polygon-surface (number elements, number corners, number vertices).
 * @dim: Dimension in lat and lon.
 * @coords: coordinates for polygons (lat, lon).
 * @zCalc: Function for computing z-coordinate.
 * @return vistle::Polygons::ptr
 */
template<class U, class T, class V>
void ReadTsunami::generateSurface(Polygons::ptr surface, const PolygonData<U> &polyData, const Dim<T> &dim,
                                  const std::vector<V> &coords, const zCalcFunc &zCalc)
{
    // fill coords 2D
    contructLatLonSurface(surface, dim, coords, zCalc);

    // fill vertices
    fillConnectListPoly2Dim(surface, dim);

    // fill the polygon list
    fillPolyList(surface, 4);
}

/**
  * @brief Generates NcVarParams struct which contains start, count and stride values computed based on given parameters.
  *
  * @dim: Current dimension of NcVar.
  * @ghost: Number of ghost cells to add.
  * @numDimBlocks: Total number of blocks for this direction.
  * @partition: Partition scalar.
  * @return: NcVarParams object.
  */
template<class T, class PartionIdx>
/* auto ReadTsunami::generateNcVarExt(const netCDF::NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlock, */
auto ReadTsunami::generateNcVarExt(const NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlock,
                                   const PartionIdx &partition) const
{
    T count = dim / numDimBlock;
    T start = partition * count;
    structured_ghost_addition(start, count, dim, ghost);
    return NcVarExtended(ncVar, start, count);
}

/**
 * @brief Called once before read. Checks if timestep polygon computation can be skipped.
 *
 * @return true if everything is prepared.
 */
bool ReadTsunami::prepareRead()
{
    seaTimeConn = m_seaSurface_out->isConnected();
    for (auto scalar_out: m_scalarsOut)
        seaTimeConn = seaTimeConn || scalar_out->isConnected();
    return true;
}

/**
  * @brief Called for each timestep and for each block (MPISIZE).
  *
  * @token: Ref to internal vistle token.
  * @timestep: current timestep.
  * @block: current block number of parallelization.
  * @return: true if all data is set and valid.
  */
bool ReadTsunami::read(Token &token, int timestep, int block)
{
    return computeBlock(token, block, timestep);
}

/**
  * @brief Computing per block.
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
    else if (seaTimeConn) {
        printRank0("reading timestep: " + std::to_string(timestep));
        return computeTimestep<int, size_t>(token, blockNum, timestep);
    }
    return true;
}

/**
 * @brief Compute actual last timestep.
 * @param incrementTimestep Stepwidth.
 * @param firstTimestep first timestep.
 * @param lastTimestep last timestep selected.
 * @param nTimesteps Storage value for number of timesteps.
 */
/* void ReadTsunami::computeActualLastTimestep(const ptrdiff_t &incrementTimestep, const size_t &firstTimestep, */
/*                                             size_t &lastTimestep, size_t &nTimesteps) */
void ReadTsunami::computeActualLastTimestep(const ptrdiff_t &incrementTimestep, const size_t &firstTimestep,
                                            size_t &lastTimestep, MPI_Offset &nTimesteps)
{
    if (lastTimestep < 0)
        lastTimestep--;

    m_actualLastTimestep = lastTimestep - (lastTimestep % incrementTimestep);
    auto rTime = ReaderTime(firstTimestep, m_actualLastTimestep, incrementTimestep);
    nTimesteps = rTime.calc_numtime();
}

/**
 * @brief Compute the unique block partition index for current block.
 *
 * @tparam Iter Iterator.
 * @param blockNum Current block.
 * @param ghost Initial number of ghostcells.
 * @param nLatBlocks Storage for number of blocks for latitude.
 * @param nLonBlocks Storage for number of blocks for longitude.
 * @param blockPartitionIterFirst Start iterator for storage partition indices.
 */
template<class Iter>
void ReadTsunami::computeBlockPartition(const int blockNum, vistle::Index &nLatBlocks, vistle::Index &nLonBlocks,
                                        Iter blockPartitionIterFirst)
{
    std::array<Index, NUM_BLOCKS> blocks;
    for (int i = 0; i < NUM_BLOCKS; i++)
        blocks[i] = m_blocks[i]->getValue();

    nLatBlocks = blocks[0];
    nLonBlocks = blocks[1];

    structured_block_partition(blocks.begin(), blocks.end(), blockPartitionIterFirst, blockNum);
}

/**
  * @brief Generates the initial polygon surfaces for sea and ground and adds only ground to scene.
  *
  * @token: Ref to internal vistle token.
  * @blockNum: current block number of parallel process.
  * @return: true if all date could be initialized.
  */
template<class T>
bool ReadTsunami::computeInitial(Token &token, const T &blockNum)
{
    /* std::shared_ptr<NcFile> ncFile(new NcFile()); */
    /* if (!openNcFile(ncFile)) */
    /*     return false; */
    try {
        std::string fileName = m_filedir->getValue();
        NcFile ncFile(MPI_COMM_WORLD, fileName, NcFile::read);

        // get nc var objects ref
        /* const NcVar &latvar = ncFile->getVar(m_latLon_Sea[0]); */
        /* const NcVar &lonvar = ncFile->getVar(m_latLon_Sea[1]); */
        /* const NcVar &grid_lat = ncFile->getVar(m_latLon_Ground[0]); */
        /* const NcVar &grid_lon = ncFile->getVar(m_latLon_Ground[1]); */
        /* const NcVar &bathymetryvar = ncFile->getVar(m_bathy->getValue()); */
        /* const NcVar &eta = ncFile->getVar(ETA); */
        const NcVar &latvar = ncFile.getVar(m_latLon_Sea[0]);
        const NcVar &lonvar = ncFile.getVar(m_latLon_Sea[1]);
        const NcVar &grid_lat = ncFile.getVar(m_latLon_Ground[0]);
        const NcVar &grid_lon = ncFile.getVar(m_latLon_Ground[1]);
        const NcVar &bathymetryvar = ncFile.getVar(m_bathy->getValue());
        const NcVar &eta = ncFile.getVar(ETA);

        // compute current time parameters
        const ptrdiff_t &incrementTimestep = m_increment->getValue();
        /* const size_t &firstTimestep = m_first->getValue(); */
        const MPI_Offset &firstTimestep = m_first->getValue();
        size_t lastTimestep = m_last->getValue();
        /* size_t nTimesteps{0}; */
        MPI_Offset nTimesteps{0};
        computeActualLastTimestep(incrementTimestep, firstTimestep, lastTimestep, nTimesteps);

        // compute partition borders => structured grid
        Index nLatBlocks{0};
        Index nLonBlocks{0};
        std::array<Index, NUM_BLOCKS> bPartitionIdx;
        computeBlockPartition(blockNum, nLatBlocks, nLonBlocks, bPartitionIdx.begin());

        // dimension from lat and lon variables
        const Dim<size_t> dimSea(latvar.getDim(0).getSize(), lonvar.getDim(0).getSize());

        // get dim from grid_lon & grid_lat
        const Dim<size_t> dimGround(grid_lat.getDim(0).getSize(), grid_lon.getDim(0).getSize());

        size_t ghost{0};
        if (m_ghost->getValue() == 1 && !(nLatBlocks == 1 && nLonBlocks == 1))
            ghost++;

        // count and start vals for lat and lon for sea polygon
        /* const auto latSea = generateNcVarExt<size_t, Index>(latvar, dimSea.dimLat, ghost, nLatBlocks, bPartitionIdx[0]); */
        /* const auto lonSea = generateNcVarExt<size_t, Index>(lonvar, dimSea.dimLon, ghost, nLonBlocks, bPartitionIdx[1]); */
        const auto latSea =
            generateNcVarExt<MPI_Offset, Index>(latvar, dimSea.dimLat, ghost, nLatBlocks, bPartitionIdx[0]);
        const auto lonSea =
            generateNcVarExt<MPI_Offset, Index>(lonvar, dimSea.dimLon, ghost, nLonBlocks, bPartitionIdx[1]);

        // count and start vals for lat and lon for ground polygon
        /* const auto latGround = */
        /*     generateNcVarExt<size_t, Index>(grid_lat, dimGround.dimLat, ghost, nLatBlocks, bPartitionIdx[0]); */
        /* const auto lonGround = */
        /*     generateNcVarExt<size_t, Index>(grid_lon, dimGround.dimLon, ghost, nLonBlocks, bPartitionIdx[1]); */
        const auto latGround =
            generateNcVarExt<MPI_Offset, Index>(grid_lat, dimGround.dimLat, ghost, nLatBlocks, bPartitionIdx[0]);
        const auto lonGround =
            generateNcVarExt<MPI_Offset, Index>(grid_lon, dimGround.dimLon, ghost, nLonBlocks, bPartitionIdx[1]);

        // num of polygons for sea & grnd
        const size_t &numPolySea = (latSea.count - 1) * (lonSea.count - 1);
        const size_t &numPolyGround = (latGround.count - 1) * (lonGround.count - 1);

        // vertices sea & grnd
        verticesSea = latSea.count * lonSea.count;
        const size_t &verticesGround = latGround.count * lonGround.count;

        // storage for read in values from ncdata
        std::vector<float> vecLat(latSea.count), vecLon(lonSea.count), vecLatGrid(latGround.count),
            vecLonGrid(lonGround.count), vecDepth(verticesGround);

        //************* read ncdata into float-pointer *************//
        {
            // read bathymetry
            {
                /* const std::vector<size_t> vecStartBathy{latGround.start, lonGround.start}; */
                const std::vector<MPI_Offset> vecStartBathy{latGround.start, lonGround.start};
                /* const std::vector<size_t> vecCountBathy{latGround.count, lonGround.count}; */
                const std::vector<MPI_Offset> vecCountBathy{latGround.count, lonGround.count};
                bathymetryvar.getVar_all(vecStartBathy, vecCountBathy, vecDepth.data());
            }

            // read eta
            {
                vecEta.resize(nTimesteps * verticesSea);
                /* const std::vector<size_t> vecStartEta{firstTimestep, latSea.start, lonSea.start}; */
                /* const std::vector<size_t> vecCountEta{nTimesteps, latSea.count, lonSea.count}; */
                /* const std::vector<ptrdiff_t> vecStrideEta{incrementTimestep, latSea.stride, lonSea.stride}; */
                const std::vector<MPI_Offset> vecStartEta{firstTimestep, latSea.start, lonSea.start};
                const std::vector<MPI_Offset> vecCountEta{nTimesteps, latSea.count, lonSea.count};
                const std::vector<MPI_Offset> vecStrideEta{incrementTimestep, latSea.stride, lonSea.stride};
                if (seaTimeConn) {
                    eta.getVar_all(vecStartEta, vecCountEta, vecStrideEta, vecEta.data());

                    //filter fillvalue
                    if (m_fill->getValue()) {
                        const float &fillValNew = getFloatParameter("fillValueNew");
                        const float &fillVal = getFloatParameter("fillValue");
                        //TODO: Bad! needs rework.
                        std::replace(vecEta.begin(), vecEta.end(), fillVal, fillValNew);
                    }
                }
            }

            // read lat, lon, grid_lat, grid_lon
            latSea.readNcVar(vecLat.data());
            lonSea.readNcVar(vecLon.data());
            latGround.readNcVar(vecLatGrid.data());
            lonGround.readNcVar(vecLonGrid.data());
        }

        //************* create Polygons ************//
        {
            std::vector<float *> coords{vecLat.data(), vecLon.data()};
            sendInfo("before sea");

            //************* create sea *************//
            {
                /* const auto &seaDim = Dim<size_t>(latSea.count, lonSea.count); */
                /* const auto &polyDataSea = PolygonData<size_t>(numPolySea, numPolySea * 4, verticesSea); */
                const auto &seaDim = Dim<MPI_Offset>(latSea.count, lonSea.count);
                const auto &polyData = PolygonData<MPI_Offset>(numPolySea, numPolySea * 4, verticesSea);
                sendInfo("polydata");
                /* ptr_sea.reset(new Polygons(polyData.numElements, polyData.numCorners, polyData.numVertices)); */
                Polygons::ptr ptr_tmp =
                    std::make_shared<Polygons>(polyData.numElements, polyData.numCorners, polyData.numVertices);
                generateSurface(ptr_tmp, polyData, seaDim, coords);
                ptr_sea = ptr_tmp->clone();
            }

            //************* create grnd *************//
            coords[0] = vecLatGrid.data();
            coords[1] = vecLonGrid.data();

            const auto &scale = m_verticalScale->getValue();
            /* const auto &grndDim = Dim<size_t>(latGround.count, lonGround.count); */
            /* const auto &polyDataGround = PolygonData<size_t>(numPolyGround, numPolyGround * 4, verticesGround); */
            const auto &grndDim = Dim<MPI_Offset>(latGround.count, lonGround.count);
            const auto &polyDataGround = PolygonData<MPI_Offset>(numPolyGround, numPolyGround * 4, verticesGround);
            /* auto grndZCalc = [&vecDepth, &lonGround, &scale](size_t j, size_t k) { */
            /*     return -vecDepth[j * lonGround.count + k] * scale; */
            /* }; */
            auto grndZCalc = [&vecDepth, &lonGround, &scale](MPI_Offset j, MPI_Offset k) {
                return -vecDepth[j * lonGround.count + k] * scale;
            };

            Polygons::ptr ptr_grnd = std::make_shared<Polygons>(polyDataGround.numElements, polyDataGround.numCorners,
                                                                polyDataGround.numVertices);
            generateSurface(ptr_grnd, polyDataGround, grndDim, coords, grndZCalc);
            sendInfo("grnd");

            //************* create selected scalars *************//
            /* const std::vector<size_t> vecScalarStart{latSea.start, lonSea.start}; */
            /* const std::vector<size_t> vecScalarCount{latSea.count, lonSea.count}; */
            const std::vector<MPI_Offset> vecScalarStart{latSea.start, lonSea.start};
            const std::vector<MPI_Offset> vecScalarCount{latSea.count, lonSea.count};
            std::vector<float> vecScalar(verticesGround);
            for (size_t i = 0; i < NUM_SCALARS; ++i) {
                if (!m_scalarsOut[i]->isConnected())
                    continue;
                const auto &scName = m_scalars[i]->getValue();
                /* const auto &val = ncFile->getVar(scName); */
                const auto &val = ncFile.getVar(scName);
                Vec<Scalar>::ptr ptr_scalar(new Vec<Scalar>(verticesSea));
                ptr_Scalar[i] = ptr_scalar;
                auto scX = ptr_scalar->x().data();
                val.getVar_all(vecScalarStart, vecScalarCount, scX);

                //set some meta data
                ptr_scalar->addAttribute("_species", scName);
                ptr_scalar->setTimestep(-1);
                ptr_scalar->setBlock(blockNum);
            }

            // add ground data to port
            if (m_groundSurface_out->isConnected()) {
                ptr_grnd->setBlock(blockNum);
                ptr_grnd->setTimestep(-1);
                ptr_grnd->updateInternals();
                token.addObject(m_groundSurface_out, ptr_grnd);
            }
        }
        /* ncFile->close(); */
        sendInfo("read");
        return true;
    } catch (PnetCDF::exceptions::NcmpiException &e) {
        sendInfo("%s error code=%d Error!", e.what(), e.errorCode());
        return false;
    }

    /* check if there is any PnetCDF internal malloc residue */
    MPI_Offset malloc_size, sum_size;
    int err = ncmpi_inq_malloc_size(&malloc_size);
    if (err == NC_NOERR) {
        MPI_Reduce(&malloc_size, &sum_size, 1, MPI_OFFSET, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank() == 0 && sum_size > 0)
            sendInfo("heap memory allocated by PnetCDF internally has %lld bytes yet to be freed\n", sum_size);
    }
}

/**
  * @brief Generates polygon for corresponding timestep and adds Object to scene.
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
    auto startCopy = vecEta.begin() + (indexEta++ * verticesSea);
    std::copy_n(startCopy, verticesSea, ptr_timestepPoly->z().begin());
    ptr_timestepPoly->updateInternals();
    ptr_timestepPoly->setTimestep(timestep);
    ptr_timestepPoly->setBlock(blockNum);

    if (m_seaSurface_out->isConnected())
        token.addObject(m_seaSurface_out, ptr_timestepPoly);

    //add scalar to ports
    for (size_t i = 0; i < NUM_SCALARS; ++i) {
        if (!m_scalarsOut[i]->isConnected())
            continue;

        auto scalar = ptr_Scalar[i]->clone();
        scalar->setGrid(ptr_timestepPoly);
        scalar->addAttribute("_species", scalar->getAttribute("_species"));
        scalar->setBlock(blockNum);
        scalar->setTimestep(timestep);
        scalar->updateInternals();

        token.addObject(m_scalarsOut[i], scalar);
    }

    if (timestep == m_actualLastTimestep) {
        sendInfo("Cleared Cache for rank: " + std::to_string(rank()));
        std::vector<float>().swap(vecEta);
        for (auto &val: ptr_Scalar)
            val.reset();
        indexEta = 0;
        ptr_sea.reset();
    }
    return true;
}
