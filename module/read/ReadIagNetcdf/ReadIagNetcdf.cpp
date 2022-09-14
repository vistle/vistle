#include "ReadIagNetcdf.h"
#include <cstddef>
#include <vector>
#include <boost/filesystem.hpp>
#include <vistle/core/points.h>
#include <vistle/netcdf/ncwrap.h>


using namespace vistle;
namespace bf = boost::filesystem;

#if defined(MODULE_THREAD)
static std::mutex netcdf_mutex; // avoid simultaneous access to NetCDF library
#define LOCK_NETCDF(comm) \
    std::unique_lock<std::mutex> netcdf_guard(netcdf_mutex, std::defer_lock); \
    if ((comm).rank() == 0) \
        netcdf_guard.lock(); \
    (comm).barrier();
#define UNLOCK_NETCDF(comm) \
    (comm).barrier(); \
    if (netcdf_guard) \
        netcdf_guard.unlock();
#else
#define LOCK_NETCDF(comm)
#define UNLOCK_NETCDF(comm)
#endif

static std::vector<std::string> elemTypes{"tetraeder", "prism", "pyramid", "hexaeder"};
static std::vector<UnstructuredGrid::Type> typeList{UnstructuredGrid::TETRAHEDRON, UnstructuredGrid::PRISM,
                                                    UnstructuredGrid::PYRAMID, UnstructuredGrid::HEXAHEDRON};
static std::vector<std::string> elemTypes2d{"surfacetriangle", "surfacequadrilateral"};
static std::vector<UnstructuredGrid::Type> typeList2d{UnstructuredGrid::TRIANGLE, UnstructuredGrid::QUAD};


static const char *DimNPoints = "no_of_points"; // no_of_points
static const char *DimNElements = "no_of_elements";
static const char *DimNSurface = "no_of_surfaceelements";

const std::array<std::string, 3> points_comp{"points_xc", "points_yc", "points_zc"};
const std::array<std::string, 3> vel_comp{"x_velocity", "y_velocity", "z_velocity"};
const std::array<std::string, 3> vel_old_comp{"x_velocity_old", "y_velocity_old", "z_velocity_old"};
const std::array<std::string, 3> mean_comp{"mean-u", "mean-v", "mean-w"};
const std::map<std::string, std::array<std::string, 3>> vector_variables{
    {"points", points_comp},
    {"velocity", vel_comp},
    {"velocity_old", vel_old_comp},
    {"mean", mean_comp},
};

const std::array<std::string, 3> *isCompound(const std::string &name)
{
    auto it = vector_variables.find(name);
    if (it == vector_variables.end())
        return nullptr;
    return &it->second;
}

const char *isComponent(const std::string &name)
{
    for (const auto &entry: vector_variables) {
        for (const auto &comp: entry.second) {
            if (comp == name)
                return entry.first.c_str();
        }
    }
    return nullptr;
}

ReadIagNetcdf::ReadIagNetcdf(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader(name, moduleID, comm)
{
    m_ncFile = addStringParameter("grid file", "File containing grid info", "", Parameter::ExistingFilename);
    m_dataFile = addStringParameter("data_file", "File containing data", "", Parameter::ExistingFilename);

    m_gridOut = createOutputPort("grid_out", "grid");
    std::vector<std::string> varChoices{"(NONE)"};
    for (int i = 0; i < NUMPORTS; ++i) {
        m_variables[i] = addStringParameter("Variable" + std::to_string(i), "Scalar Variables", "", Parameter::Choice);
        setParameterChoices(m_variables[i], varChoices);
        m_dataOut[i] = createOutputPort("data_out" + std::to_string(i), "volume data");
    }

    m_boundaryOut = createOutputPort("boundary_out", "boundary with patch markers");
    for (int i = 0; i < NUMPORTS; ++i) {
        m_bvariables[i] =
            addStringParameter("boundary_variable" + std::to_string(i), "Variable on boundary", "", Parameter::Choice);
        setParameterChoices(m_variables[i], varChoices);
        m_boundaryDataOut[i] = createOutputPort("boundary_out" + std::to_string(i), "boundary data");
    }

    observeParameter(m_ncFile);
    observeParameter(m_dataFile);

    setParallelizationMode(ParallelizeBlocks);
}

ReadIagNetcdf::~ReadIagNetcdf()
{}

bool ReadIagNetcdf::setVariableList(FileType type, int ncid)
{
    auto &variables = type == GridFile ? m_gridVariables : m_dataVariables;
    variables.clear();

    int nvars = -1;
    int err = nc_inq_nvars(ncid, &nvars);
    if (err != NC_NOERR) {
        return false;
    }

    int nvars2 = -1;
    std::vector<int> varids(nvars);
    err = nc_inq_varids(ncid, &nvars2, varids.data());
    if (err != NC_NOERR) {
        return false;
    }

    for (auto varid: varids) {
        std::vector<char> varname(NC_MAX_NAME);
        int err = nc_inq_varname(ncid, varid, varname.data());
        if (err != NC_NOERR) {
            continue;
        }
        std::string name(varname.data());
        variables.push_back(name);

        if (auto compound = isComponent(name)) {
            auto &components = vector_variables.find(compound)->second;
            bool haveCompound = true;
            for (auto &c: components) {
                if (std::find(variables.begin(), variables.end(), c) == variables.end())
                    haveCompound = false;
            }
            if (haveCompound) {
                variables.push_back(compound);
            }
        }
    }

    return true;
}

bool ReadIagNetcdf::validateFile(std::string fullFileName, std::string &redFileName, FileType type)
{
    redFileName = "";
    if (fullFileName.empty()) {
        sendWarning("validation of %s file failed because fileName is empty", type == GridFile ? "grid" : "data");
        return false;
    }

    try {
        LOCK_NETCDF(comm());
        bf::path dPath(fullFileName);
        if (!bf::is_regular_file(dPath)) {
            if (rank() == 0)
                sendWarning("validation error: File %s is not regular", fullFileName.c_str());
            return false;
        }

        redFileName = dPath.string();
        auto ncid = NcFile::open(redFileName);
        if (!ncid) {
            if (rank() == 0)
                sendWarning("could not open %s", redFileName.c_str());
            return false;
        }

        if (type == GridFile) {
            if (!hasDimension(ncid, DimNPoints)) {
                if (rank() == 0)
                    sendWarning("file %s does not have dimension %s, no suitable file", redFileName.c_str(),
                                DimNPoints);
                return false;
            }
            int dimid = -1;
            int err = nc_inq_dimid(ncid, DimNPoints, &dimid);
            if (err != NC_NOERR) {
                if (rank() == 0)
                    sendInfo("file %s does not provide dimid for %s", redFileName.c_str(), DimNPoints);
                return false;
            }
            size_t dimVerts = 0;
            err = nc_inq_dimlen(ncid, dimid, &dimVerts);
            if (err != NC_NOERR) {
                if (rank() == 0)
                    sendInfo("file %s does not provide dimlen for %s", redFileName.c_str(), DimNPoints);
                return false;
            }
            m_numVertices = dimVerts;
            //sendInfo("COUNTED:  %d points found", m_numVertices);
        }
        setVariableList(type, ncid);
        if (rank() == 0) {
            //sendInfo("file to load: %s", redFileName.c_str());
        }
        return true;
    } catch (std::exception &ex) {
        std::cerr << "filesystem exception: " << ex.what() << std::endl;
        if (rank() == 0)
            sendInfo("filesystem exception for %s: %s", fullFileName.c_str(), ex.what());
        return false;
    }
    sendInfo("validation of file %s completed with error", fullFileName.c_str());
    return false;
}

bool ReadIagNetcdf::examine(const vistle::Parameter *param)
{
    std::cerr << "examine: " << (param ? param->getName() : "(no param)") << std::endl;
    LOCK_NETCDF(comm());
    if (!param || param == m_ncFile) {
        if (!validateFile(m_ncFile->getValue(), m_gridFileName, GridFile))
            return false;
    }
    if (!param || param == m_dataFile) {
        if (!validateFile(m_dataFile->getValue(), m_dataFileName, DataFile))
            return false;
    }

    std::vector<std::string> choices{"(NONE)"};
    std::copy(m_dataVariables.begin(), m_dataVariables.end(), std::back_inserter(choices));
    //std::copy(m_gridVariables.begin(), m_gridVariables.end(), std::back_inserter(choices));
    for (int i = 0; i < NUMPORTS; i++) {
        setParameterChoices(m_variables[i], choices);
        setParameterChoices(m_bvariables[i], choices);
    }
    UNLOCK_NETCDF(comm());
    setPartitions(1);

    m_dataFiles.clear();
    if (m_dataFileName.empty()) {
        setTimesteps(-1);
        sendInfo("no valid data found");
        return true;
    }

    unsigned nIgnored = 0;
    try {
        bf::path dataFilePath(m_dataFileName);
        std::string common = dataFilePath.filename().string();
        auto pos = common.find("_i=");
        common = common.substr(0, pos);
        bf::path dataDir(dataFilePath.parent_path());
        for (bf::directory_iterator it2(dataDir); it2 != bf::directory_iterator(); ++it2) { //all files in dir
            if (it2->path().filename().string().find(common) == 0) {
                auto fn = it2->path().string();
                auto ncid = NcFile::open(fn, comm());
                if (!ncid) {
                    std::cerr << "ignoring " << fn << ", could not open" << std::endl;
                    continue;
                }
                if (!hasDimension(ncid, DimNPoints)) {
                    std::cerr << "ignoring " << fn << ", no " << DimNPoints << " dimension" << std::endl;
                    ++nIgnored;
                    continue;
                }
                size_t dimPoints = getDimension(ncid, DimNPoints);
                if (dimPoints != m_numVertices) {
                    std::cerr << "ignoring " << fn << ", expected nVertices=" << m_numVertices << ", but got "
                              << dimPoints << std::endl;
                    ++nIgnored;
                    continue;
                }

                m_dataFiles.push_back(fn);
            }
        }
    } catch (std::exception &ex) {
        std::cerr << "filesystem exception: " << ex.what() << std::endl;
    }

    std::sort(m_dataFiles.begin(),
              m_dataFiles.end()); // alpha-numeric sort hopefully brings timesteps into proper order

    setTimesteps(m_dataFiles.size());
    if (rank() == 0)
        sendInfo("reading %u timesteps, ignored %u candidates", (unsigned)m_dataFiles.size(), nIgnored);

    return true;
}

bool ReadIagNetcdf::emptyValue(vistle::StringParameter *ch) const
{
    std::string name = "";
    name = ch->getValue();
    return ((name == "") || (name == "NONE") || (name == "(NONE)"));
}

bool ReadIagNetcdf::prepareRead()
{
    return true;
}

void ReadIagNetcdf::readElemType(vistle::Index *cl, vistle::Index *el, const std::string &elem, vistle::Index &idx_cl,
                                 vistle::Index &idx_el, const int ncid, vistle::Byte *tl, vistle::Byte type)
{
    std::string no_str = "no_of_" + elem + "s";
    std::string pts_str = "points_per_" + elem;
    std::string pts_of_str = "points_of_" + elem + "s";
    size_t no_of_elem_of_type = getDimension(ncid, no_str.c_str());
    size_t pts_per_elem_of_type = getDimension(ncid, pts_str.c_str());
    if (!getVariable<Index>(ncid, pts_of_str, &cl[idx_cl], {0, 0}, {no_of_elem_of_type, pts_per_elem_of_type})) {
        sendWarning("could not read connectivity for %s", elem.c_str());
    }
    for (size_t i = 0; i < no_of_elem_of_type; ++i) {
        if (tl)
            tl[idx_el] = type;
        el[idx_el] = idx_cl;
        idx_cl += pts_per_elem_of_type;
        ++idx_el;
    }
}

void ReadIagNetcdf::countElements(const std::string &elem, const int ncid, size_t &numElems, size_t &numCorners)
{
    std::string no_str = "no_of_" + elem + "s";
    std::string pts_str = "points_per_" + elem;
    if (!hasDimension(ncid, no_str.c_str())) { //TODO: read total element number instead
        std::cerr << "ignoring" << elem.at(0) << std::endl;
    }
    size_t no_of_elem_of_type = getDimension(ncid, no_str.c_str());
    size_t pts_per_elem_of_type = getDimension(ncid, pts_str.c_str());
    numElems += no_of_elem_of_type;
    numCorners += (no_of_elem_of_type * pts_per_elem_of_type);
}

bool ReadIagNetcdf::read(Token &token, int timestep, int block)
{
    LOCK_NETCDF(comm());
    if (timestep == -1) {
        NcFile ncGridId = NcFile::open(m_gridFileName);
        if (!ncGridId) {
            return false;
        }
        if (!hasDimension(ncGridId, DimNPoints) || !hasDimension(ncGridId, DimNElements)) {
            sendError("dimension info missing: no %s or %s", DimNPoints, DimNElements);
            return false;
        }
        m_numVertices = getDimension(ncGridId, DimNPoints);
        size_t numElem = getDimension(ncGridId, DimNElements);

        //sendInfo("COUNTED: %d points and %zu elementes in total", m_numVertices, numElem);
        //COORDS

        size_t numCorners = 0;
        numElem = 0;
        //MESH
        //GET NUMBER OF ELEMENTS
        for (auto elem: elemTypes) {
            countElements(elem, ncGridId, numElem, numCorners);
        }
        m_grid.reset(new UnstructuredGrid(numElem, numCorners, m_numVertices));
        Index *cl = m_grid->cl().data();
        Index *el = m_grid->el().data();
        Byte *tl = m_grid->tl().data();

        // READ ELEMENT DATA
        Index idx_cl = 0, idx_el = 0, typeIdx = 0;
        for (auto elem: elemTypes) {
            auto type = typeList[typeIdx];
            readElemType(cl, el, elem, idx_cl, idx_el, ncGridId, tl, type);
            std::cerr << "read " << elem << ": #el=" << idx_el << ", #cl=" << idx_cl << std::endl;
            ++typeIdx;
        }
        el[idx_el] = idx_cl;
        std::cerr << "read #el=" << numElem << ", idx_el=" << idx_el << ", #cl=" << numCorners << ", idx_cl=" << idx_cl
                  << std::endl;
        assert(idx_el == numElem);
        assert(idx_cl == numCorners);
        //sendInfo("Read of all types completed: idxEL=%lu, idxCL=%lu", (unsigned long)idx_el, (unsigned long)idx_cl);

        auto *x = m_grid->x().data();
        auto *y = m_grid->y().data();
        auto *z = m_grid->z().data();
        if (!getVariable<Scalar>(ncGridId, "points_xc", x, {0}, {m_numVertices})) {
            sendError("could not read x coordinates: points_xc");
            return false;
        }
        if (!getVariable<Scalar>(ncGridId, "points_yc", y, {0}, {m_numVertices})) {
            sendError("could not read y coordinates: points_yc");
            return false;
        }
        if (!getVariable<Scalar>(ncGridId, "points_zc", z, {0}, {m_numVertices})) {
            sendError("could not read z coordinates: points_zc");
            return false;
        }

        token.applyMeta(m_grid);
        token.addObject(m_gridOut, m_grid);

        for (Index dataIdx = 0; dataIdx < NUMPORTS; ++dataIdx) {
            if (!emptyValue(m_variables[dataIdx])) {
                continue;
            }
            if (!m_dataOut[dataIdx]->isConnected()) {
                continue;
            }
            token.addObject(m_dataOut[dataIdx], m_grid);
        }

        bool needBoundary = m_boundaryOut->isConnected();
        for (unsigned i = 0; i < NUMPORTS; ++i) {
            if (m_boundaryDataOut[i]->isConnected())
                needBoundary = true;
        }
        if (needBoundary) {
            size_t numElem = getDimension(ncGridId, DimNSurface);

            size_t numCorners = 0;
            numElem = 0;
            //MESH
            //GET NUMBER OF ELEMENTS
            for (auto elem: elemTypes2d) {
                countElements(elem, ncGridId, numElem, numCorners);
            }
            m_boundary = std::make_shared<Polygons>(numElem, numCorners, 0);
            Index *cl = m_boundary->cl().data();
            Index *el = m_boundary->el().data();
            m_boundary->d()->x[0] = m_grid->d()->x[0];
            m_boundary->d()->x[1] = m_grid->d()->x[1];
            m_boundary->d()->x[2] = m_grid->d()->x[2];
            Index idx_cl = 0, idx_el = 0;
            for (auto elem: elemTypes2d) {
                readElemType(cl, el, elem, idx_cl, idx_el, ncGridId, nullptr, 0);
                std::cerr << "read " << elem << ": #el=" << idx_el << ", #cl=" << idx_cl << std::endl;
            }
            el[idx_el] = idx_cl;
            el[idx_el] = idx_cl;
            //std::cerr << "read boundary #el=" << numElem << ", idx_el=" << idx_el << ", #cl=" << numCorners << ", idx_cl=" << idx_cl << std::endl;
            assert(idx_el == numElem);
            assert(idx_cl == numCorners);
            token.applyMeta(m_boundary);

            for (Index dataIdx = 0; dataIdx < NUMPORTS; ++dataIdx) {
                if (!emptyValue(m_bvariables[dataIdx])) {
                    continue;
                }
                if (!m_boundaryDataOut[dataIdx]->isConnected()) {
                    continue;
                }
                token.addObject(m_boundaryDataOut[dataIdx], m_boundary);
            }

            if (m_boundaryOut->isConnected()) {
                auto markerData = std::make_shared<Vec<Index>>(numElem);
                if (!getVariable<Index>(ncGridId, "boundarymarker_of_surfaces", markerData->x().data(), {0},
                                        {numElem})) {
                    sendWarning("could not read surface markers");
                }
                markerData->addAttribute("_species", "marker");
                markerData->setGrid(m_boundary);
                token.applyMeta(markerData);
                token.addObject(m_boundaryOut, markerData);
            }
        }
        return true;
    }

    assert(m_grid);

    assert(m_dataFiles.size() > timestep);
    NcFile ncDataId = NcFile::open(m_dataFiles[timestep]);
    if (!ncDataId) {
        sendWarning("could not open data file %s", m_dataFiles[timestep].c_str());
        return false;
    }

    auto readVariable = [&ncDataId, timestep, this](const std::string &varName) -> DataBase::ptr {
        DataBase::ptr dataObj;
        if (auto *comp = isCompound(varName)) {
            Vec<Scalar, 3>::ptr vecObj(new Vec<Scalar, 3>(m_numVertices));
            for (int c = 0; c < 3; ++c) {
                Scalar *x = vecObj->x(c).data();
                if (!getVariable<Scalar>(ncDataId, comp->at(c), x, {0}, {m_numVertices})) {
                    sendError("failed to read %s for t=%d", varName.c_str(), timestep);
                    return dataObj;
                }
            }
            dataObj = vecObj;
        } else {
            Vec<Scalar>::ptr scalObj(new Vec<Scalar>(m_numVertices));
            Scalar *s = scalObj->x().data();
            if (!getVariable<Scalar>(ncDataId, varName, s, {0}, {m_numVertices})) {
                sendError("failed to read %s for t=%d", varName.c_str(), timestep);
                return dataObj;
            }
            dataObj = scalObj;
        }
        dataObj->addAttribute("_species", varName.c_str());
        dataObj->setMapping(DataBase::Vertex);
        return dataObj;
    };

    DataBase::ptr dataOut[NUMPORTS];
    for (Index dataIdx = 0; dataIdx < NUMPORTS; ++dataIdx) {
        if (emptyValue(m_variables[dataIdx])) {
            continue;
        }
        if (!m_dataOut[dataIdx]->isConnected()) {
            continue;
        }
        std::string varName = m_variables[dataIdx]->getValue();
        if (DataBase::ptr dataObj = readVariable(varName)) {
            dataOut[dataIdx] = dataObj;
            dataObj->setGrid(m_grid);

            //sendInfo("Done reading variable %s", varName.c_str());
            token.applyMeta(dataObj);
            token.addObject(m_dataOut[dataIdx], dataObj);
        }
    }

    for (Index bIdx = 0; bIdx < NUMPORTS; ++bIdx) {
        if (emptyValue(m_bvariables[bIdx])) {
            continue;
        }
        if (!m_boundaryDataOut[bIdx]->isConnected()) {
            continue;
        }
        std::string varName = m_bvariables[bIdx]->getValue();
        DataBase::ptr dataObj;
        for (unsigned dataIdx = 0; dataIdx < NUMPORTS; ++dataIdx) {
            if (m_variables[dataIdx]->getValue() == varName && dataOut[dataIdx]) {
                dataObj = dataOut[dataIdx]->clone();
                break;
            }
        }
        if (!dataObj) {
            dataObj = readVariable(varName);
            //sendInfo("Done reading boundary variable %s", varName.c_str());
        }
        if (dataObj) {
            dataObj->setGrid(m_boundary);

            token.applyMeta(dataObj);
            token.addObject(m_boundaryDataOut[bIdx], dataObj);
        }
    }

    return true;
}

bool ReadIagNetcdf::finishRead()
{
    m_grid.reset();
    return true;
}


MODULE_MAIN(ReadIagNetcdf)
