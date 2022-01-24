//header
#include "ReadTsunami.h"

//vistle
#include "vistle/core/database.h"
#include "vistle/core/index.h"
#include "vistle/core/layergrid.h"
#include "vistle/core/object.h"
#include "vistle/core/parameter.h"
#include "vistle/core/scalar.h"
#include "vistle/core/vec.h"
#include "vistle/module/module.h"

//vistle-module-util
#include "vistle/module/utils/ghost.h"

//mpi
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/intercommunicator.hpp>
#include <mpi.h>

//std
#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <omp.h>
#include <string>
#include <cstddef>
#include <vector>

using namespace vistle;
using namespace PnetCDF;

MODULE_MAIN_THREAD(ReadTsunami, boost::mpi::threading::multiple)
namespace {
constexpr auto ETA{"eta"};
constexpr auto _species{"_species"};
constexpr auto fillValue{"fillValue"};
constexpr auto fillValueNew{"fillValueNew"};
constexpr auto choiceParamNone{"None"};
} // namespace

ReadTsunami::ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader(name, moduleID, comm)
, m_needSea(false)
, m_pnetcdf_comm(comm, boost::mpi::comm_create_kind::comm_duplicate)
{
    // file-browser
    m_filedir = addStringParameter("file_dir", "NC File directory", "", Parameter::Filename);

    //ghost
    m_ghost = addIntParameter("ghost", "Show ghostcells.", 1, Parameter::Boolean);
    m_fill = addIntParameter("fill", "Replace filterValue.", 1, Parameter::Boolean);
    m_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter sea", 1.0);

    // define ports
    m_seaSurface_out = createOutputPort("Sea surface", "2D Grid Sea (Heightmap)");
    m_groundSurface_out = createOutputPort("Ground surface", "2D Sea floor (Heightmap)");

    // block size
    m_blocks[0] = addIntParameter("blocks latitude", "number of blocks in lat-direction", 2);
    m_blocks[1] = addIntParameter("blocks longitude", "number of blocks in lon-direction", 2);

    //fillvalue
    addFloatParameter(fillValue, "ncFile fillValue offset for eta", -9999.f);
    addFloatParameter(fillValueNew, "set new fillValue offset for eta", 0.0f);

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

    for (int i = 0; i < NUM_SCALARS; ++i) {
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
    if (nBlocks % size() != 0) {
        printRank0("Number of blocks = x * MPISIZE!");
        return false;
    }
    setPartitions(nBlocks);
    return true;
}


/**
 * @brief Open NcmpiFile and catch failure.
 *
 * @return Return open ncFile as unique_ptr if there is no error, else a nullptr.
 */
std::unique_ptr<NcmpiFile> ReadTsunami::openNcmpiFile()
{
    const auto &fileName = m_filedir->getValue();
    try {
        return make_unique<NcmpiFile>(m_pnetcdf_comm, fileName, NcFile::read);
    } catch (PnetCDF::exceptions::NcmpiException &e) {
        sendInfo("%s error code=%d Error!", e.what(), e.errorCode());
        return nullptr;
    }
}

/**
 * @brief Inspect netCDF variables stored in file.
 */
bool ReadTsunami::inspectNetCDFVars()
{
    auto ncFile = openNcmpiFile();

    if (!ncFile)
        return false;

    const int &maxTime = ncFile->getDim("time").getSize();
    setTimesteps(maxTime);

    const Integer &maxlatDim = ncFile->getDim("lat").getSize();
    const Integer &maxlonDim = ncFile->getDim("lon").getSize();
    setParameterRange(m_blocks[0], Integer(1), maxlatDim);
    setParameterRange(m_blocks[1], Integer(1), maxlonDim);

    //scalar inspection
    std::vector<std::string> scalarChoiceVec;
    std::vector<std::string> bathyChoiceVec;
    auto strContains = [](const std::string &name, const std::string &contains) {
        return name.find(contains) != std::string::npos;
    };
    auto latLonContainsGrid = [&](auto &name, int i) {
        if (strContains(name, "grid"))
            m_latLon_Ground[i] = name;
        else
            m_latLon_Sea[i] = name;
    };
    auto isEmpty = [](std::vector<std::string> &vec) {
        if (vec.empty())
            vec.push_back(choiceParamNone);
    };

    //delete previous choicelists.
    m_bathy->setChoices(std::vector<std::string>());
    for (const auto &scalar: m_scalars)
        scalar->setChoices(std::vector<std::string>());

    //read names of scalars
    for (auto &name_val: ncFile->getVars()) {
        auto &name = name_val.first;
        auto &val = name_val.second;
        if (strContains(name, "lat"))
            latLonContainsGrid(name, 0);
        else if (strContains(name, "lon"))
            latLonContainsGrid(name, 1);
        else if (strContains(name, "bathy")) // || strContains(name, "deformation"))
            bathyChoiceVec.push_back(name);
        else if (val.getDimCount() == 2) // for now: only scalars with 2 dim depend on lat lon.
            scalarChoiceVec.push_back(name);
    }
    isEmpty(scalarChoiceVec);
    isEmpty(bathyChoiceVec);

    //init choice param with scalardata
    setParameterChoices(m_bathy, bathyChoiceVec);

    for (auto &scalar: m_scalars)
        setParameterChoices(scalar, scalarChoiceVec);

    return true;
}

/**
 * @brief Generate surface from layergrid.
 *
 * @surface: LayerGrid pointer which will be modified.
 * @dim: Dimension in lat and lon.
 * @zCalc: Function for computing z-coordinate.
 */
template<class T>
void ReadTsunami::generateSurface(LayerGrid::ptr surface, const Dim<T> &dim, const ZCalcFunc &zCalc)
{
    int n = 0;
    auto sz_coord = surface->z().begin();
    for (T i = 0; i < dim.X(); ++i)
        for (T j = 0; j < dim.Y(); ++j, ++n)
            sz_coord[n] = zCalc(i, j);
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
auto ReadTsunami::generateNcVarExt(const NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlock,
                                   const PartionIdx &partition) const
{
    T count = dim / numDimBlock;
    T start = partition * count;
    structured_ghost_addition(start, count, dim, ghost);
    return NcVarExtended(ncVar, start, count);
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

bool ReadTsunami::prepareRead()
{
    const auto &part = numPartitions();

    m_block_VecScalarPtr = std::vector<std::array<VecScalarPtr, NUM_SCALARS>>(part);
    m_block_etaIdx = std::vector<int>(part);
    m_block_etaVec = std::vector<std::vector<float>>(part);
    m_block_dimSea = std::vector<moffDim>(part);

    m_needSea = m_seaSurface_out->isConnected();
    for (auto &val: m_scalarsOut)
        m_needSea = m_needSea || val->isConnected();
    return true;
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
    else if (m_needSea)
        return computeTimestep<T, U>(token, blockNum, timestep);
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

    auto m_actualLastTimestep = lastTimestep - (lastTimestep % incrementTimestep);
    auto rTime = ReaderTime(firstTimestep, m_actualLastTimestep, incrementTimestep);
    nTimesteps = rTime.calc_numtime();
}

/**
 * @brief Compute the unique block partition index for current block.
 *
 * @tparam Iter Iterator.
 * @param blockNum Current block.
 * @param nLatBlocks Storage for number of blocks for latitude.
 * @param nLonBlocks Storage for number of blocks for longitude.
 * @param blockPartitionIterFirst Start iterator for storage partition indices.
 */
template<class Iter>
void ReadTsunami::computeBlockPartition(const int blockNum, vistle::Index &nLatBlocks, vistle::Index &nLonBlocks,
                                        Iter blockPartitionIterFirst)
{
    std::array<Index, NUM_BLOCKS> blocks;
    for (int i = 0; i < NUM_BLOCKS; ++i)
        blocks[i] = m_blocks[i]->getValue();

    nLatBlocks = blocks[0];
    nLonBlocks = blocks[1];

    structured_block_partition(blocks.begin(), blocks.end(), blockPartitionIterFirst, blockNum);
}

/**
  * @brief Generates the initial surfaces for sea and ground and adds only ground to scene.
  *
  * @token: Ref to internal vistle token.
  * @blockNum: current block number of parallel process.
  * @return: true if all data could be initialized.
  */
template<class T>
bool ReadTsunami::computeInitial(Token &token, const T &blockNum)
{
    m_mtx.lock();
    auto ncFile = openNcmpiFile();
    m_mtx.unlock();

    // get nc var objects ref
    const NcVar &latVar = ncFile->getVar(m_latLon_Sea[0]);
    const NcVar &lonVar = ncFile->getVar(m_latLon_Sea[1]);
    const NcVar &gridLatVar = ncFile->getVar(m_latLon_Ground[0]);
    const NcVar &gridLonVar = ncFile->getVar(m_latLon_Ground[1]);
    const NcVar &etaVar = ncFile->getVar(ETA);

    // compute current time parameters
    const ptrdiff_t &incTimestep = m_increment->getValue();
    const MPI_Offset &firstTimestep = m_first->getValue();
    size_t lastTimestep = m_last->getValue();
    MPI_Offset nTimesteps{0};
    computeActualLastTimestep(incTimestep, firstTimestep, lastTimestep, nTimesteps);

    // compute partition borders => structured grid
    Index nLatBlocks{0};
    Index nLonBlocks{0};
    std::array<Index, NUM_BLOCKS> blockPartitionIdx;
    computeBlockPartition(blockNum, nLatBlocks, nLonBlocks, blockPartitionIdx.begin());

    // dimension from lat and lon variables
    const sztDim dimSeaTotal(latVar.getDim(0).getSize(), lonVar.getDim(0).getSize(), etaVar.getDim(0).getSize());

    // get dim from grid_lon & grid_lat
    const sztDim dimGroundTotal(gridLatVar.getDim(0).getSize(), gridLonVar.getDim(0).getSize(), 0);

    size_t ghost{0};
    if (m_ghost->getValue() == 1 && !(nLatBlocks == 1 && nLonBlocks == 1))
        ghost++;

    // count and start vals for lat and lon for sea
    const auto latSea =
        generateNcVarExt<MPI_Offset, Index>(latVar, dimSeaTotal.X(), ghost, nLatBlocks, blockPartitionIdx[0]);
    const auto lonSea =
        generateNcVarExt<MPI_Offset, Index>(lonVar, dimSeaTotal.Y(), ghost, nLonBlocks, blockPartitionIdx[1]);

    // count and start vals for lat and lon for ground
    const auto latGround =
        generateNcVarExt<MPI_Offset, Index>(gridLatVar, dimGroundTotal.X(), ghost, nLatBlocks, blockPartitionIdx[0]);
    const auto lonGround =
        generateNcVarExt<MPI_Offset, Index>(gridLonVar, dimGroundTotal.Y(), ghost, nLonBlocks, blockPartitionIdx[1]);

    // vertices sea & grnd
    const size_t &verticesSea = latSea.count * lonSea.count;
    const size_t &verticesGround = latGround.count * lonGround.count;

    // storage for read in values from ncdata
    std::vector<float> vecLat(latSea.count), vecLon(lonSea.count), vecLatGrid(latGround.count),
        vecLonGrid(lonGround.count), vecDepth(verticesGround);

    std::vector coords{vecLat.begin(), vecLon.begin()};
    if (m_needSea) {
        latSea.readNcVar(vecLat.data());
        lonSea.readNcVar(vecLon.data());

        m_block_etaVec[blockNum] = std::vector<float>(nTimesteps * verticesSea);
        auto &vecEta = m_block_etaVec[blockNum];
        const std::vector<MPI_Offset> vecStartEta{firstTimestep, latSea.start, lonSea.start};
        const std::vector<MPI_Offset> vecCountEta{nTimesteps, latSea.count, lonSea.count};
        const std::vector<MPI_Offset> vecStrideEta{incTimestep, latSea.stride, lonSea.stride};
        etaVar.getVar_all(vecStartEta, vecCountEta, vecStrideEta, vecEta.data());

        //filter fillvalue
        if (m_fill->getValue()) {
            const float &fillValNew = getFloatParameter(fillValueNew);
            const float &fillVal = getFloatParameter(fillValue);
            //TODO: Bad! needs rework.
            std::replace(vecEta.begin(), vecEta.end(), fillVal, fillValNew);
        }

        //************* create sea *************//
        const auto dimSea = moffDim(latSea.count, lonSea.count, 1);
        m_block_dimSea[blockNum] = dimSea;
        LayerGrid::ptr seaPtr(new LayerGrid(dimSea.X(), dimSea.Y(), dimSea.Z()));
        seaPtr->min()[0] = *vecLat.begin();
        seaPtr->min()[1] = *vecLon.begin();
        seaPtr->max()[0] = *(vecLat.end() - 1);
        seaPtr->max()[1] = *(vecLon.end() - 1);
        generateSurface(seaPtr, dimSea);

        //************* create selected scalars *************//
        const std::vector<MPI_Offset> vecScalarStart{latSea.start, lonSea.start};
        const std::vector<MPI_Offset> vecScalarCount{latSea.count, lonSea.count};
        std::vector<float> vecScalar(verticesGround);
        auto &vecScalarPtrArr = m_block_VecScalarPtr[blockNum];
        for (int i = 0; i < NUM_SCALARS; ++i) {
            if (!m_scalarsOut[i]->isConnected())
                continue;
            const auto &scName = m_scalars[i]->getValue();
            if (scName == choiceParamNone)
                continue;
            const auto &val = ncFile->getVar(scName);
            VecScalarPtr scalarPtr(new Vec<Scalar>(verticesSea));
            vecScalarPtrArr[i] = scalarPtr;
            auto scX = scalarPtr->x().data();
            val.getVar_all(vecScalarStart, vecScalarCount, scX);

            //set some meta data
            scalarPtr->addAttribute(_species, scName);
            scalarPtr->setTimestep(-1);
            scalarPtr->setBlock(blockNum);
        }
    }

    //************* create grnd *************//
    if (m_groundSurface_out->isConnected()) {
        if (m_bathy->getValue() == choiceParamNone)
            printRank0("File doesn't provide bathymetry data");
        else {
            const NcVar &bathymetryVar = ncFile->getVar(m_bathy->getValue());
            const std::vector<MPI_Offset> vecStartBathy{latGround.start, lonGround.start};
            const std::vector<MPI_Offset> vecCountBathy{latGround.count, lonGround.count};
            bathymetryVar.getVar_all(vecStartBathy, vecCountBathy, vecDepth.data());
            if (!vecDepth.empty()) {
                latGround.readNcVar(vecLatGrid.data());
                lonGround.readNcVar(vecLonGrid.data());

                coords[0] = vecLatGrid.begin();
                coords[1] = vecLonGrid.begin();

                const auto &scale = m_verticalScale->getValue();
                ZCalcFunc grndZCalc = [&vecDepth, &lonGround, &scale](MPI_Offset j, MPI_Offset k) {
                    return -vecDepth[j * lonGround.count + k] * scale;
                };

                LayerGrid::ptr grndPtr(new LayerGrid(latGround.count, lonGround.count, 1));
                const auto grndDim = moffDim(latGround.count, lonGround.count, 0);
                grndPtr->min()[0] = *vecLatGrid.begin();
                grndPtr->min()[1] = *vecLonGrid.begin();
                grndPtr->max()[0] = *(vecLatGrid.end() - 1);
                grndPtr->max()[1] = *(vecLonGrid.end() - 1);
                generateSurface(grndPtr, grndDim, grndZCalc);

                // add ground data to port
                grndPtr->setBlock(blockNum);
                grndPtr->setTimestep(-1);
                grndPtr->updateInternals();
                token.addObject(m_groundSurface_out, grndPtr);
            }
        }
    }

    /* check if there is any PnetCDF internal malloc residue */
    MPI_Offset malloc_size, sum_size;
    int err = ncmpi_inq_malloc_size(&malloc_size);
    if (err == NC_NOERR) {
        MPI_Reduce(&malloc_size, &sum_size, 1, MPI_OFFSET, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank() == 0 && sum_size > 0)
            sendInfo("heap memory allocated by PnetCDF internally has %lld bytes yet to be freed\n", sum_size);
        return false;
    }
    return true;
}

/**
  * @brief Generates heightmap (layergrid) for corresponding timestep and adds Object to scene.
  *
  * @token: Ref to internal vistle token.
  * @blockNum: current block number of parallel process.
  * @timestep: current timestep.
  * @return: true. TODO: add proper error-handling here.
  */
template<class T, class U>
bool ReadTsunami::computeTimestep(Token &token, const T &blockNum, const U &timestep)
{
    auto &blockSeaDim = m_block_dimSea[blockNum];
    LayerGrid::ptr timestepPtr(new LayerGrid(blockSeaDim.X(), blockSeaDim.Y(), blockSeaDim.Z()));

    // getting z from vecEta and copy to z()
    // verticesSea * timesteps = total count vecEta
    const auto &verticesSea = timestepPtr->getNumVertices();
    std::vector<float> &etaVec = m_block_etaVec[blockNum];
    auto startCopy = etaVec.begin() + (m_block_etaIdx[blockNum]++ * verticesSea);
    std::copy_n(startCopy, verticesSea, timestepPtr->z().begin());
    timestepPtr->updateInternals();
    timestepPtr->setTimestep(timestep);
    timestepPtr->setBlock(blockNum);
    if (m_seaSurface_out->isConnected())
        token.addObject(m_seaSurface_out, timestepPtr);

    //add scalar to ports
    std::array<VecScalarPtr, NUM_SCALARS> &vecScalarPtrArr = m_block_VecScalarPtr[blockNum];
    for (int i = 0; i < NUM_SCALARS; ++i) {
        if (!m_scalarsOut[i]->isConnected())
            continue;

        auto vecScalarPtr = vecScalarPtrArr[i]->clone();
        vecScalarPtr->setGrid(timestepPtr);
        vecScalarPtr->addAttribute(_species, vecScalarPtr->getAttribute(_species));
        vecScalarPtr->setBlock(blockNum);
        vecScalarPtr->setTimestep(timestep);
        vecScalarPtr->updateInternals();

        token.addObject(m_scalarsOut[i], vecScalarPtr);
    }

    return true;
}

/**
 * @brief Called after last read operation for clearing reader parameter.
 *
 * @return true
 */
bool ReadTsunami::finishRead()
{
    //reset block partition variables
    m_block_VecScalarPtr.clear();
    m_block_dimSea.clear();
    m_block_etaVec.clear();
    m_block_etaIdx.clear();
    sendInfo("Cleared Cache for rank: " + std::to_string(rank()));
    return true;
}
