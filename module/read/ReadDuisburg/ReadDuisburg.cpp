/* ------------------------------------------------------------------------------------
           READ Duisburg Simulation Data from Netcdf file
------------------------------------------------------------------------------------ */
#include "ReadDuisburg.h"
#include <boost/algorithm/string.hpp>
#include <vistle/util/filesystem.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/layergrid.h>
#include <fstream>

using namespace vistle;
namespace bf = vistle::filesystem;

#include <vistle/netcdf/ncwrap.h>
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

namespace {
struct Field {
    std::string name;
    std::string description;
    std::string species;
};
const std::array<Field, 3> fields = {Field{"h", "amount of water", "water"},
                                     Field{"u", "velocity - u component", "velocity_u"},
                                     Field{"v", "velocity - v component", "velocity_v"}};
} // namespace

ReadDuisburg::ReadDuisburg(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    //initialize ports
    m_gridFile = addStringParameter("grid_file", "File containing grid and data",
                                    "/home/hpcleker/Desktop/duisburg_5t.nc", Parameter::ExistingFilename);
    setParameterFilters(m_gridFile, "NetCDF Grid Files (*.grid.nc)/NetCDF Files (*.nc)");

    m_gridOut = createOutputPort("grid_out", "grid");
    for (auto &field: fields) {
        createOutputPort(field.name, field.description);
    }

    //set other initial options
    setParallelizationMode(Serial);
    setAllowTimestepDistribution(true);
    // setHandlePartitions(Reader::PartitionTimesteps);

    setCollectiveIo(Reader::Collective);

    observeParameter(m_gridFile);
}


bool ReadDuisburg::examine(const vistle::Parameter *param)
{
    if (!param || param == m_gridFile) {
        LOCK_NETCDF(comm());
        auto file = m_gridFile->getValue();
        auto ncid = NcFile::open(file, comm());
        if (!ncid || !*ncid) {
            if (rank() == 0)
                sendError("Could not open %s", file.c_str());
            return false;
        }
        for (auto dim: {"x", "y", "t"}) {
            if (!hasDimension(*ncid, dim)) {
                if (rank() == 0)
                    sendError("File %s does not have dimension %s, not expected format", file.c_str(), dim);
                return false;
            }
        }
        auto nTimes = getDimension(*ncid, "t");

        for (auto var: {"x", "y", "z", "t"}) {
            if (!hasVariable(*ncid, var)) {
                if (rank() == 0)
                    sendError("File %s does not have variable %s, not expected format", file.c_str(), var);
                return false;
            }
        }
        for (auto field: fields) {
            if (!hasVariable(*ncid, field.name)) {
                if (rank() == 0)
                    sendError("File %s does not have variable %s, not expected format", file.c_str(),
                              field.name.c_str());
                return false;
            }
        }

        UNLOCK_NETCDF(comm());

        setTimesteps(nTimes);
        setPartitions(1);
    }
    return true;
}

bool ReadDuisburg::prepareRead()
{
    LOCK_NETCDF(comm());
    auto file = m_gridFile->getValue();
    m_ncFile = NcFile::open(file, comm());
    if (!m_ncFile || !*m_ncFile) {
        if (rank() == 0)
            sendError("Could not open %s", file.c_str());
        return false;
    }
    for (auto dim: {"x", "y", "t"}) {
        if (!hasDimension(*m_ncFile, dim)) {
            if (rank() == 0)
                sendError("File %s does not have dimension %s, not expected format", file.c_str(), dim);
            return false;
        }
    }

    auto x = getVariable<vistle::Scalar>(*m_ncFile, "x");
    auto nx = x.size();
    m_dim[0] = nx;
    auto y = getVariable<vistle::Scalar>(*m_ncFile, "y");
    auto ny = y.size();
    m_dim[1] = ny;

    auto isUniform = []<typename T>(const std::vector<T> &v) {
        auto N = v.size();
        if (N < 2)
            return true;
        auto step = (v.back() - v.front()) / (N - 1);
        for (size_t i = 1; i < N; ++i) {
            if (v[i] - v[i - 1] != step)
                return false;
        }
        return true;
    };

    if (isUniform(x) && isUniform(y)) {
        auto lg = std::make_shared<LayerGrid>(nx, ny, 1);
        lg->min()[0] = x.front();
        lg->min()[1] = y.front();
        lg->max()[0] = x.back();
        lg->max()[1] = y.back();
        getVariable(*m_ncFile, "z", lg->z().data(), {0, 0}, {nx, ny});
        m_grid = lg;
    } else {
        auto sg = std::make_shared<StructuredGrid>(nx, ny, 1);
        getVariable(*m_ncFile, "z", sg->z().data(), {0, 0}, {nx, ny});
        m_grid = sg;
    }

    UNLOCK_NETCDF(comm());

    return true;
}


//cellIsWater: checks if vertex and neighbor vertices are also water e.g. if z-coord is varying
bool ReadDuisburg::cellIsWater(const std::vector<double> &h, int i, int j, int dimX, int dimY) const
{
    if ((h[j * dimX + i] > 0.) && (h[(j + 1) * dimX + (i + 1)] > 0.) && (h[(j + 1) * dimX + i] > 0.) &&
        (h[j * dimX + (i + 1)] > 0.)) {
        return true;
    }
    return false;
}

bool ReadDuisburg::read(Reader::Token &token, int timestep, int block)
{
    assert(m_grid);
    if (timestep < 0) {
        token.applyMeta(Object::as(m_grid));
        token.addObject("grid_out", m_grid);
        return true;
    }

    auto size = m_dim[0] * m_dim[1];

    LOCK_NETCDF(*token.comm());
    for (auto &field: fields) {
        if (!isConnected(field.name))
            continue;
        auto data = std::make_shared<Vec<Scalar>>(size);
        getVariable(*m_ncFile, field.name, data->x().data(), {(Index)timestep, 0, 0}, {1, m_dim[1], m_dim[0]});
        data->describe(field.species, id());
        data->setGrid(m_grid);
        token.applyMeta(data);
        token.addObject(field.name, data);
    }
    UNLOCK_NETCDF(*token.comm());

    return true;
}

bool ReadDuisburg::finishRead()
{
    return true;
}

MODULE_MAIN(ReadDuisburg)
