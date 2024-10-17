//header
#include "ReadTsunami.h"

//vistle
#include "vistle/core/database.h"
#include "vistle/core/index.h"
#include "vistle/core/layergrid.h"
#include "vistle/core/object.h"
#include "vistle/core/parameter.h"
#include "vistle/core/polygons.h"
#include "vistle/core/points.h"
#include "vistle/core/scalar.h"
#include "vistle/core/vec.h"
#include "vistle/module/module.h"
#include <vistle/alg/ghost.h>

//boost
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/intercommunicator.hpp>

//mpi
#include <mpi.h>

//std
#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <cstddef>
#include <vector>

using namespace vistle;
using namespace PnetCDF;

MODULE_MAIN_THREAD(ReadTsunami, boost::mpi::threading::multiple)
namespace {
constexpr auto lat{"lat"};
constexpr auto grid_lat{"grid_lat"};
constexpr auto lon{"lon"};
constexpr auto grid_lon{"grid_lon"};
constexpr auto bathy{"bathy"};
constexpr auto ETA{"eta"};
/* constexpr auto fault_lat{"lat_barycenter"}; */
/* constexpr auto fault_lon{"lon_barycenter"}; */
/* constexpr auto fault_slip{"slip"}; */
constexpr auto _species{"_species"};
constexpr auto fillValue{"fillValue"};
constexpr auto fillValueNew{"fillValueNew"};
constexpr auto NONE{"None"};

struct COMP_VAR_DIM {
    bool operator()(const ReadTsunami::PNcVarExt &var, const ReadTsunami::NcDim &dim) const
    {
        return (var.getName() == dim.getName()) && !(var.getId() ^ dim.getId());
    }
};
} // namespace

ReadTsunami::ReadTsunami(const string &name, int moduleID, mpi::communicator comm)
: vistle::Reader(name, moduleID, comm)
, m_needSea(false)
, m_pnetcdf_comm(comm, boost::mpi::comm_create_kind::comm_duplicate)
{
    // file-browser
    m_filedir = addStringParameter("file_dir", "NC File directory", "", Parameter::Filename);
    m_ghost = addIntParameter("ghost", "Show ghostcells.", 1, Parameter::Boolean);
    /* m_fault = addIntParameter("fault", "Show faults.", 1, Parameter::Boolean); */
    m_fill = addIntParameter("fill", "Replace filterValue.", 1, Parameter::Boolean);
    m_verticalScale = addFloatParameter("VerticalScale", "Vertical Scale parameter sea", 1.0);

    // define ports
    m_seaSurface_out = createOutputPort("Sea surface", "Grid Sea (Heightmap/LayerGrid)");
    m_groundSurface_out = createOutputPort("Ground surface", "Sea floor (Heightmap/LayerGrid)");
    /* m_fault_out = createOutputPort("Fault wall surface", "Faults (Polygons)"); */

    // block size
    m_blocks[0] = addIntParameter("blocks_latitude", "number of blocks in lat-direction", 2);
    m_blocks[1] = addIntParameter("blocks_longitude", "number of blocks in lon-direction", 2);

    //fillvalue
    addFloatParameter(fillValue, "ncFile fillValue offset for eta", -9999.f);
    addFloatParameter(fillValueNew, "set new fillValue offset for eta", 0.0f);

    //bathymetryname
    m_bathy = addStringParameter("bathymetry", "Select bathymetry stored in netCDF", "", Parameter::Choice);

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
    constexpr auto scalar_name{"scalar_"};
    constexpr auto scalarPort_name{"scalar_port_"};
    constexpr auto scalarPort_descr{"Port for scalar number "};

    for (int i = 0; i < NUM_SCALARS; ++i) {
        const string i_str{to_string(i)};
        const string &scName = scalar_name + i_str;
        const string &portName = scalarPort_name + i_str;
        const string &portDescr = scalarPort_descr + i_str;
        m_scalars[i] = addStringParameter(scName, "Select scalar.", "", Parameter::Choice);
        m_scalarsOut[i] = createOutputPort(portName, portDescr);
    }
}

/**
 * @brief Print string only on rank 0.
 *
 * @param str Format str to print.
 */
void ReadTsunami::printRank0(const string &str) const
{
    if (rank() == 0)
        sendInfo(str);
}

/**
 * @brief Print string only on rank 0.
 *
 * @param str Format str to print.
 * @param block current block.
 */
void ReadTsunami::printRank0AndBlock0(const string &str, int block) const
{
    if (block == 0)
        printRank0(str);
}

/**
  * @brief Prints current rank and the number of all ranks to the console.
  */
void ReadTsunami::printMPIStats() const
{
    printRank0("Current Rank: " + to_string(rank()) + " Processes (MPISIZE): " + to_string(size()));
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

        if (!inspectNetCDF())
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
ReadTsunami::NcFilePtr ReadTsunami::openNcmpiFile()
{
    const auto &fileName = m_filedir->getValue();
    try {
        return make_unique<NcFile>(m_pnetcdf_comm, fileName, NcFile::read);
    } catch (PnetCDF::exceptions::NcmpiException &e) {
        sendInfo("%s error code=%d Error!", e.what(), e.errorCode());
        return nullptr;
    }
}

/**
 * @brief Inspect global attributes of netCDF file.
 *
 * @param ncFile Open nc file pointer.
 *
 * @return True if there is no error.
 */
bool ReadTsunami::inspectAtts(const NcFilePtr &ncFile)
{
    for (auto &[name, val]: ncFile->getAtts())
        if (!m_faultInfo)
            if (boost::algorithm::starts_with(name, "fault_"))
                m_faultInfo = true;
    return true;
}

/**
 * @brief Inspect netCDF dimension (all independent variables.)
 *
 * @param ncFile Open nc file pointer.
 *
 * @return true if everything is initialized.
 */
bool ReadTsunami::inspectDims(const NcFilePtr &ncFile)
{
    const int &maxTime = ncFile->getDim("time").getSize();
    setTimesteps(maxTime);

    const Integer &maxlatDim = ncFile->getDim(lat).getSize();
    const Integer &maxlonDim = ncFile->getDim(lon).getSize();
    setParameterRange(m_blocks[0], Integer(1), maxlatDim);
    setParameterRange(m_blocks[1], Integer(1), maxlonDim);
    return true;
}

/**
 * @brief Helper for checking if name includes contains.
 *
 * @param name name of attribute.
 * @param contains string which will be checked for.
 *
 * @return true if contains is in name.
 */
inline auto strContains(const string &name, const string &contains)
{
    return name.find(contains) != string::npos;
}

/**
 * @brief Helper which adds NONE to vector if its empty.
 *
 * @param vec ref to string vector.
 */
inline void ifEmptyAddNone(vector<string> &vec)
{
    if (vec.empty())
        vec.push_back(NONE);
}

/**
 * @brief Inspect netCDF variables which depends on dim variables.
 *
 * @param ncFile Open nc file pointer.
 *
 * @return true if everything is initialized.
 */
bool ReadTsunami::inspectScalars(const NcFilePtr &ncFile)
{
    vector<string> scalarChoiceVec;
    vector<string> bathyChoiceVec;
    auto latLonContainsGrid = [&](auto &name, int i) {
        if (strContains(name, "grid"))
            m_latLon_Ground[i] = name;
        else
            m_latLon_Sea[i] = name;
    };

    //delete previous choicelists.
    m_bathy->setChoices(vector<string>());
    for (const auto &scalar: m_scalars)
        scalar->setChoices(vector<string>());

    //read names of scalars
    for (auto &[name, val]: ncFile->getVars()) {
        if (strContains(name, lat))
            latLonContainsGrid(name, 0);
        else if (strContains(name, lon))
            latLonContainsGrid(name, 1);
        else if (strContains(name, bathy)) // || strContains(name, "deformation"))
            bathyChoiceVec.push_back(name);
        else
            scalarChoiceVec.push_back(name);
    }
    ifEmptyAddNone(scalarChoiceVec);
    ifEmptyAddNone(bathyChoiceVec);

    //init choice param with scalardata
    setParameterChoices(m_bathy, bathyChoiceVec);

    for (auto &scalar: m_scalars)
        setParameterChoices(scalar, scalarChoiceVec);

    if (m_latLon_Ground.empty() || m_latLon_Sea.empty()) {
        sendInfo("No parameter lat, lon, grid_lat or grid_lon found. Reader not able to read tsunami.");
        return false;
    }
    return true;
}


/**
 * @brief Inspect netCDF variables stored in file.
 */
bool ReadTsunami::inspectNetCDF()
{
    auto ncFile = openNcmpiFile();

    if (!ncFile)
        return false;

    return inspectDims(ncFile) && inspectScalars(ncFile); // && inspectAtts(ncFile);
}

/**
 * @brief Fill z coordinate from layergrid.
 *
 * @surface: LayerGrid pointer which will be modified.
 * @dim: Dimension in lat (y) and lon (x).
 * @zCalc: Function for computing z-coordinate.
 */
template<class T>
void ReadTsunami::fillHeight(LayerGrid::ptr surface, const Dim<T> &dim, const ZCalcFunc &zCalc)
{
    int n = 0;
    auto z = surface->z().begin();
    for (T i = 0; i < dim.X(); ++i)
        for (T j = 0; j < dim.Y(); ++j, ++n)
            z[n] = zCalc(i, j);
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
template<class T, class PartitionIDs>
auto ReadTsunami::generateNcVarExt(const NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlock,
                                   const PartitionIDs &partitionIDs) const
{
    T count = dim / numDimBlock;
    T start = partitionIDs * count;
    structured_ghost_addition(start, count, dim, ghost);
    return PNcVarExt(ncVar, start, count);
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
 * @brief Called before read and used to initialize steering parameters.
 *
 * @return True if everything is initialized.
 */
bool ReadTsunami::prepareRead()
{
    const auto &part = numPartitions();

    m_block_etaIdx = vector<int>(part);
    m_block_dimSea = vector<moffDim>(part);
    m_block_etaVec = vector<vector<float>>(part);
    m_block_VecScalarPtr = VecArrVecScalarPtrs(part);
    m_block_min = VecLatLon(part);
    m_block_max = VecLatLon(part);

    m_needSea = m_seaSurface_out->isConnected();
    for (auto &val: m_scalarsOut)
        m_needSea = m_needSea || val->isConnected();
    return true;
}

/**
  * @brief Computing per block.
  *
  * @token: Ref to internal vistle token.
  * @block: current block number of parallelization.
  * @timestep: current timestep.
  * @return: true if all data is set and valid.
  */
bool ReadTsunami::computeBlock(Reader::Token &token, const int block, const int timestep)
{
    if (timestep == -1)
        return computeConst(token, block);
    else if (m_needSea)
        return computeTimestep(token, block, timestep);
    return true;
}

/**
 * @brief Compute the unique block partition index for current block.
 *
 * @tparam Iter Iterator.
 * @param block Current block.
 * @param nLatBlocks Storage for number of blocks for latitude.
 * @param nLonBlocks Storage for number of blocks for longitude.
 * @param blockPartitionIterFirst Start iterator for storage partition indices.
 */
template<class Iter>
void ReadTsunami::computeBlockPartition(const int block, vistle::Index &nLatBlocks, vistle::Index &nLonBlocks,
                                        Iter blockPartitionIterFirst)
{
    array<Index, NUM_BLOCKS> blocks;
    for (int i = 0; i < NUM_BLOCKS; ++i)
        blocks[i] = m_blocks[i]->getValue();

    nLatBlocks = blocks[0];
    nLonBlocks = blocks[1];

    structured_block_partition(blocks.begin(), blocks.end(), blockPartitionIterFirst, block);
}

/**
 * @brief Initialize eta values (wave amplitude per timestep).
 *
 * @param ncFile open ncfile pointer.
 * @param mapNcVars array which contains latitude and longitude NcVarExtended objects.
 * @param time ReadTimer object for current run.
 * @param verticesSea number of vertices for sea surface of the current block.
 * @param block current block.
 */
void ReadTsunami::initETA(const NcFilePtr &ncFile, const map<string, PNcVarExt> &mapNcVars, const ReaderTime &time,
                          const size_t &verticesSea, int block)
{
    const NcVar &etaVar = ncFile->getVar(ETA);
    const auto &latSea = mapNcVars.at(lat);
    const auto &lonSea = mapNcVars.at(lon);
    const auto &nTimesteps = time.calc_numtime();
    m_block_etaVec[block] = vector<float>(nTimesteps * verticesSea);
    const vector<MPI_Offset> vecStartEta{time.first(), latSea.Start(), lonSea.Start()};
    const vector<MPI_Offset> vecCountEta{nTimesteps, latSea.Count(), lonSea.Count()};
    const vector<MPI_Offset> vecStrideEta{time.inc(), latSea.Stride(), lonSea.Stride()};
    auto &vecEta = m_block_etaVec[block];
    etaVar.getVar_all(vecStartEta, vecCountEta, vecStrideEta, vecEta.data());

    //filter fillvalue
    if (m_fill->getValue()) {
        const float &fillNew = getFloatParameter(fillValueNew);
        const float &fill = getFloatParameter(fillValue);
        //TODO: Bad! needs rework.
        replace(vecEta.begin(), vecEta.end(), fill, fillNew);
    }
}

/**
 * @brief Initialize scalars which depend on latitude and longitude.
 *
 * @param visVecScalarPtr storage ptr.
 * @param var NcVar.
 * @param latSea NcVarExtended of latitude.
 * @param lonSea NcVarExtended of longitude.
 * @param block current block.
 */
void ReadTsunami::initScalarDepLatLon(VisVecScalarPtr &scalarPtr, const NcVar &var, const PNcVarExt &latSea,
                                      const PNcVarExt &lonSea, int block)
{
    const auto &dim = latSea.Count() * lonSea.Count();
    scalarPtr = make_shared<Vec<Scalar>>(dim);
    vector<float> vecScalar(dim);
    const vector<MPI_Offset> vecScalarStart{latSea.Start(), lonSea.Start()};
    const vector<MPI_Offset> vecScalarCount{latSea.Count(), lonSea.Count()};
    const vector<MPI_Offset> vecScalarStride{1, 1};
    const vector<MPI_Offset> vecScalarImap{1, latSea.Count()}; // transponse
    var.getVar_all(vecScalarStart, vecScalarCount, vecScalarStride, vecScalarImap, scalarPtr->x().begin());

    //set some meta data
    scalarPtr->addAttribute(_species, var.getName());
    scalarPtr->setBlock(block);
}

/**
 * @brief Initialize scalars for each attribute and each block.
 *
 * @param var NcVar.
 * @param visVecScalarPtr storage ptr.
 * @param mapNcVars array which contains latitude and longitude NcVarExtended objects.
 * @param block current block.
 */
void ReadTsunami::initScalars(const NcVar &var, VisVecScalarPtr &visVecScalarPtr,
                              const map<string, PNcVarExt> &mapNcVars, int block)
{
    const auto &dimCount = var.getDimCount();
    if (dimCount > 2 || dimCount < 2) {
        // no scalars with dimCount > 2 at the moment except ETA which is used as height of the tsunami => await ChEESE 2 addition.
        if (var.getName() != ETA) // ignore ETA
            printRank0AndBlock0("Please add implementation for " + var.getName() + " to function initScalars.", block);
        visVecScalarPtr = nullptr;
        return;
    }
    int hitLatLon = 0;
    int hitLatLonGrid = 0;
    for (auto &dim: var.getDims())
        for (const auto &[name, ncVarExt]: mapNcVars) {
            if (hitLatLon == 2 || hitLatLonGrid == 2)
                break;
            if (COMP_VAR_DIM()(ncVarExt, dim)) {
                if (strContains(dim.getName(), "grid_"))
                    ++hitLatLonGrid;
                else
                    ++hitLatLon;
            }
        }
    if (hitLatLon == 2 || hitLatLonGrid == 2) {
        const auto &lat_ref = hitLatLon ? mapNcVars.at(lat) : mapNcVars.at(grid_lat);
        const auto &lon_ref = hitLatLon ? mapNcVars.at(lon) : mapNcVars.at(grid_lon);
        initScalarDepLatLon(visVecScalarPtr, var, lat_ref, lon_ref, block);
    } else
        printRank0AndBlock0(
            "Scalar " + var.getName() +
                " not depending on latitude or longitude. Please add own implementation to function initScalars.",
            block);
}

/**
 * @brief Initialize sea surface variables for current block.
 *
 * @param ncFile open ncfile pointer.
 * @param mapNcVars array which contains latitude and longitude NcVarExtended objects.
 * @param nBlocks number of blocks in latitude and longitude direction.
 * @param nBlockPartIdx partition index for latitude and longitude.
 * @param time ReaderTime object which holds time parameters.
 * @param ghost ghostcells to add.
 * @param block current block.
 */
void ReadTsunami::initSea(const NcFilePtr &ncFile, const map<string, PNcVarExt> &mapNcVars,
                          const array<Index, 2> &nBlocks, const array<Index, NUM_BLOCKS> &nBlockPartIdx,
                          const ReaderTime &time, int ghost, int block)
{
    // lat = Y and lon = X
    const auto &latSea = mapNcVars.at(lat);
    const auto &lonSea = mapNcVars.at(lon);
    const sztDim dimSeaTotal(latSea.Count(), lonSea.Count(), 1);
    const size_t &vertSea = latSea.Count() * lonSea.Count();

    initETA(ncFile, mapNcVars, time, vertSea, block);

    vector<float> vecLat(latSea.Count()), vecLon(lonSea.Count());

    latSea.readNcVar(vecLat.data());
    lonSea.readNcVar(vecLon.data());
    m_block_dimSea[block] = moffDim(lonSea.Count(), latSea.Count(), 1);
    m_block_min[block][0] = *vecLon.begin();
    m_block_min[block][1] = *vecLat.begin();
    m_block_max[block][0] = *(vecLon.end() - 1);
    m_block_max[block][1] = *(vecLat.end() - 1);
}

/**
 * @brief Initialize and create ground surface.
 *
 * @param token Token object ref.
 * @param ncFile open ncfile pointer.
 * @param mapNcVars array which contains latitude and longitude NcVarExtended objects.
 * @param nBlocks number of blocks in latitude and longitude.
 * @param blockPartIdx partition index for latitude and longitude.
 * @param ghost ghostcells to add.
 * @param block current block.
 */
void ReadTsunami::createGround(Token &token, const NcFilePtr &ncFile, const map<string, PNcVarExt> &mapNcVars,
                               const array<vistle::Index, 2> &nBlocks,
                               const array<vistle::Index, NUM_BLOCKS> &blockPartIdx, int ghost, int block)
{
    const auto &latGround = mapNcVars.at(grid_lat);
    const auto &lonGround = mapNcVars.at(grid_lon);
    const size_t &verticesGround = latGround.Count() * lonGround.Count();
    vector<float> vecDepth(verticesGround);
    const NcVar &bathymetryVar = ncFile->getVar(m_bathy->getValue());
    const vector<MPI_Offset> vecStartBathy{latGround.Start(), lonGround.Start()};
    const vector<MPI_Offset> vecCountBathy{latGround.Count(), lonGround.Count()};
    bathymetryVar.getVar_all(vecStartBathy, vecCountBathy, vecDepth.data());
    if (!vecDepth.empty()) {
        vector<float> vecLatGrid(latGround.Count()), vecLonGrid(lonGround.Count());
        latGround.readNcVar(vecLatGrid.data());
        lonGround.readNcVar(vecLonGrid.data());

        const auto &scale = m_verticalScale->getValue();
        LayerGrid::ptr grndPtr(new LayerGrid(lonGround.Count(), latGround.Count(), 1));
        const auto grndDim = moffDim(lonGround.Count(), latGround.Count(), 0);
        grndPtr->min()[0] = *vecLonGrid.begin();
        grndPtr->min()[1] = *vecLatGrid.begin();
        grndPtr->max()[0] = *(vecLonGrid.end() - 1);
        grndPtr->max()[1] = *(vecLatGrid.end() - 1);
        fillHeight(grndPtr, grndDim, [&vecDepth, &lonGround, &scale](const auto &j, const auto &k) {
            return -vecDepth[k * lonGround.Count() + j] * scale;
        });

        // add ground data to port
        token.applyMeta(grndPtr);
        token.addObject(m_groundSurface_out, grndPtr);
    }
}

/* void ReadTsunami::fillPolyElementList_fault(PolyPtr poly, int corners) */
/* { */
/*     std::generate(poly->el().begin(), poly->el().end(), [n = -1]() mutable { return ++n * 4; }); */
/* } */

/* void ReadTsunami::fillPolyConnectList_fault(PolyPtr poly, int verts) */
/* { */
/*     int n = -1; */
/*     auto connectList = poly->cl().begin(); */
/*     for (auto i = 2; i < verts; i = i + 2) { */
/*         connectList[++n] = (i - 2); */
/*         connectList[++n] = (i - 1); */
/*         connectList[++n] = i + 1; */
/*         connectList[++n] = i; */
/*     } */
/* } */

/* void ReadTsunami::fillCoords_fault(PointsPtr poly, const vector<NcGrpAtt> &faults) */
/* { */
/*     auto x = poly->x().begin(), y = poly->y().begin(), z = poly->z().begin(); */
/* auto radius = poly->r().begin(); */
/* string input(""); */
/* int n = 0; */
/* for (auto &flt: faults) { */
/*     stringstream ss; */
/*     flt.getValues(input); */
/*     ss << input; */
/*     float lon; */
/*     float lat; */
/*     float slip; */
/*     float tmpF; */
/*     string tmp; */
/*     string ptmp; */
/*     while (!ss.eof()) { */
/*         ss >> tmp; */
/*         tmp.erase(remove(tmp.begin(), tmp.end(), ':'), tmp.end()); */
/* if (stringstream(tmp) >> tmpF) { */
/*     if (ptmp == fault_lon) */
/*         lon = tmpF; */
/*     else if (ptmp == fault_lat) */
/*         lat = tmpF; */
/*     else if (ptmp == fault_slip) */
/*         slip = tmpF; */
/* } */
/* ptmp = tmp; */
/* tmp = ""; */
/* } */
/* x[n] = lon; */
/* y[n] = lat; */
/* z[n] = -0.001230245; */
/* radius[n] = 0.01; */
/* for (int i = 0; i < 2; ++i, ++n) { */
/*     x[n] = lon; */
/*     y[n] = lat; */
/*     /1* z[n] = n % 2 == 0 ? slip : 0; *1/ */
/*     z[n] = 0; */
/*     radius[n] = slip; */
/* } */
/* } */
/* } */

void ReadTsunami::createFault(Token &token, const NcFilePtr &ncFile, int block)
{
    string nFaultsStr("");
    vector<NcGrpAtt> faults;
    for (auto &[name, val]: ncFile->getAtts())
        if (boost::algorithm::starts_with(name, "fault_"))
            faults.push_back(val);

    const auto &nFaults = faults.size();
    /* const auto &verts = faults.size(); */
    /* const auto &elements = verts - 1; */
    /* const auto &corners = elements * 4; */
    PointsPtr fault(new Points(nFaults));
    /* Polygons::ptr fault(new Polygons(elements, corners, verts * 2)); */
    /* fillPolyElementList_fault(fault, 4); */
    /* fillPolyConnectList_fault(fault, verts); */
    /* fillCoords_fault(fault, faults); */
    token.applyMeta(fault);
    token.addObject(m_fault_out, fault);
}

/**
  * @brief Generates the initial surfaces for sea and ground and adds only ground to scene.
  *
  * @token: Ref to internal vistle token.
  * @block: current block number of parallel process.
  * @return: true if all data could be initialized.
  */
bool ReadTsunami::computeConst(Token &token, const int block)
{
    m_mtx.lock();
    auto ncFile = openNcmpiFile();
    m_mtx.unlock();
    if (!ncFile)
        return false;

    array<Index, 2> nBlocks;
    array<Index, NUM_BLOCKS> blockPartitionIdx;
    computeBlockPartition(block, nBlocks[0], nBlocks[1], blockPartitionIdx.begin());

    int ghost{0};
    if (m_ghost->getValue() == 1 && !(nBlocks[0] == 1 && nBlocks[1] == 1))
        ghost++;

    const auto &lat_str = m_latLon_Sea[0];
    const auto &lon_str = m_latLon_Sea[1];
    const auto &grid_lat_str = m_latLon_Ground[0];
    const auto &grid_lon_str = m_latLon_Ground[1];
    const NcVar &latVar = ncFile->getVar(lat_str);
    const NcVar &lonVar = ncFile->getVar(lon_str);
    const NcVar &gridLatVar = ncFile->getVar(grid_lat_str);
    const NcVar &gridLonVar = ncFile->getVar(grid_lon_str);
    map<string, PNcVarExt> mapNcVarExt;
    const sztDim dimSeaTotal(latVar.getDim(0).getSize(), lonVar.getDim(0).getSize(), 1);
    const sztDim dimGroundTotal(gridLatVar.getDim(0).getSize(), gridLonVar.getDim(0).getSize(), 1);
    mapNcVarExt[lat_str] =
        generateNcVarExt<MPI_Offset, Index>(latVar, dimSeaTotal.X(), ghost, nBlocks[0], blockPartitionIdx[0]);
    mapNcVarExt[lon_str] =
        generateNcVarExt<MPI_Offset, Index>(lonVar, dimSeaTotal.Y(), ghost, nBlocks[1], blockPartitionIdx[1]);
    mapNcVarExt[grid_lat_str] =
        generateNcVarExt<MPI_Offset, Index>(gridLatVar, dimGroundTotal.X(), ghost, nBlocks[0], blockPartitionIdx[0]);
    mapNcVarExt[grid_lon_str] =
        generateNcVarExt<MPI_Offset, Index>(gridLonVar, dimGroundTotal.Y(), ghost, nBlocks[1], blockPartitionIdx[1]);

    if (m_needSea) {
        auto last = m_last->getValue();
        if (last < 0)
            return false;
        auto inc = m_increment->getValue();
        auto first = m_first->getValue();
        last = last - (last % inc);
        const auto &time = ReaderTime(first, last, inc);
        initSea(ncFile, mapNcVarExt, nBlocks, blockPartitionIdx, time, ghost, block);
    }

    /*     if (m_fault->getValue() && m_faultInfo) */
    /*         createFault(token, ncFile, block); */

    if (m_groundSurface_out->isConnected()) {
        if (m_bathy->getValue() == NONE)
            printRank0AndBlock0("File doesn't provide bathymetry data", block);
        else
            createGround(token, ncFile, mapNcVarExt, nBlocks, blockPartitionIdx, ghost, block);
    }

    for (int i = 0; i < NUM_SCALARS; ++i) {
        if (!m_scalarsOut[i]->isConnected())
            continue;
        const auto &scName = m_scalars[i]->getValue();
        if (scName == NONE)
            continue;
        initScalars(ncFile->getVar(scName), m_block_VecScalarPtr[block][i], mapNcVarExt, block);
    }

    /* check if there is any PnetCDF internal malloc residue */
    MPI_Offset malloc_size, sum_size;
    int err = ncmpi_inq_malloc_size(&malloc_size);
    if (err == NC_NOERR) {
        MPI_Reduce(&malloc_size, &sum_size, 1, MPI_OFFSET, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank() == 0 && sum_size > 0)
            sendInfo("heap memory allocated by PnetCDF internally has %ld bytes yet to be freed\n", (long)sum_size);
    }
    return true;
}

/**
  * @brief Generates heightmap (layergrid) for corresponding timestep and adds object to scene.
  *
  * @token: Ref to internal vistle token.
  * @block: current block number of parallel process.
  * @timestep: current timestep.
  * @return: true. TODO: add proper error-handling here.
  */
bool ReadTsunami::computeTimestep(Token &token, const int block, const int timestep)
{
    auto &blockSeaDim = m_block_dimSea[block];
    LayerGrid::ptr gridPtr(new LayerGrid(blockSeaDim.X(), blockSeaDim.Y(), blockSeaDim.Z()));
    copy_n(m_block_min[block].begin(), 2, gridPtr->min());
    copy_n(m_block_max[block].begin(), 2, gridPtr->max());

    // getting z from vecEta and copy to z()
    // verticesSea * timesteps = total Count() vecEta
    const auto &verticesSea = gridPtr->getNumVertices();
    vector<float> &etaVec = m_block_etaVec[block];
    auto count = m_block_etaIdx[block]++ * verticesSea;
    fillHeight(gridPtr, blockSeaDim, [&etaVec, &blockSeaDim, &count](const auto &j, const auto &k) {
        return etaVec[count + k * blockSeaDim.X() + j];
    });
    token.applyMeta(gridPtr);
    if (m_seaSurface_out->isConnected()) {
        token.addObject(m_seaSurface_out, gridPtr);
    }

    //add scalar to ports
    ArrVecScalarPtrs &arrVecScaPtrs = m_block_VecScalarPtr[block];
    for (int i = 0; i < NUM_SCALARS; ++i) {
        if (!m_scalarsOut[i]->isConnected())
            continue;

        const auto &vecScalarPtr = arrVecScaPtrs[i];
        if (!vecScalarPtr)
            continue;

        auto scalarPtr = vecScalarPtr->clone();

        scalarPtr->setGrid(gridPtr);
        scalarPtr->addAttribute(_species, vecScalarPtr->getAttribute(_species));

        token.applyMeta(scalarPtr);
        token.addObject(m_scalarsOut[i], scalarPtr);
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
    m_block_max.clear();
    m_block_min.clear();
    m_block_etaIdx.clear();
    sendInfo("Cleared Cache for rank: " + to_string(rank()));
    return true;
}
