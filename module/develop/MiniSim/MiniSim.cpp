#include "MiniSim.h"

#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/unstr.h>
#include <vistle/insitu/core/exception.h>
#include <vistle/insitu/message/addObjectMsq.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/util/directory.h>
#include <vistle/util/shmconfig.h>
#include <vistle/util/stopwatch.h>
#include <vistle/util/threadname.h>

using namespace vistle;
namespace sensei = vistle::insitu;

MODULE_MAIN(MiniSimModule)
#define CERR std::cerr << "MiniSimModule[" << rank() << "/" << size() << "] "
const char *meshName = "structured mesh";
const char *varName = "oscillation";
MiniSimModule::MiniSimModule(const std::string &name, int moduleID, mpi::communicator comm)
: insitu::InSituModuleBase(name, moduleID, comm)
{
    m_inputFilePath = addStringParameter("input_params", "path to file with input parameters", "",
                                         vistle::Parameter::ExistingFilename);
    setParameterFilters(m_inputFilePath, "input files (*.osc)");
    m_numTimesteps = addIntParameter("num_timesteps", "maximum number of timesteps to execute", 10);
    setParameterMinimum(m_numTimesteps, Integer{1});
    createSimParams();

    m_intOptions.push_back(addIntParameter("frequency", "the pipeline is processed for every nth simulation cycle", 1));
    m_intOptions.push_back(
        addIntParameter("mostImportantIteration", "force the pipeline to process this iteration", -1));
    m_intOptions.push_back(addIntParameter("keep_timesteps",
                                           "if true timesteps are cached and processed as time series", true,
                                           vistle::Parameter::Boolean));

    m_simThread.reset(new std::thread{[this]() {
        setThreadName("MiniSim");
        constexpr bool pause = true;
        insitu::MetaData metaData;
        insitu::MetaMesh metaMesh(meshName);
        metaMesh.addVar(varName);
        metaData.addMesh(metaMesh);

        auto getDataFunc = std::bind(&MiniSimModule::getData, this, std::placeholders::_1);
        m_adapter.reset(new insitu::Adapter(pause, this->comm(), std::move(metaData),
                                            insitu::ObjectRetriever{getDataFunc}, VISTLE_ROOT, VISTLE_BUILD_TYPE, ""));

        m_connectionFileDumped = true;
        while (!m_terminate) {
            while (!m_terminate && m_adapter->paused()) {
                m_adapter->execute(0);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            try {
                StopWatch w{"simulation took "};
                m_sim.run(*this, (size_t)m_numTimesteps->getValue(), m_inputFilePath->getValue(), this->comm(),
                          m_param);
            } catch (const std::runtime_error &e) {
                std::cerr << "Failed to create simulation: " << e.what() << std::endl;
                break;
            }
            // if (!m_adapter->paused())
            //     m_simulationMessageQueue->send(insitu::message::ExecuteCommand{std::make_pair("run_simulation", "")});
        }
        m_adapter->finalize();
    }});

    while (!m_connectionFileDumped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    m_filePath = addStringParameter("path", "path to the connection file written by the simulation",
                                    directory::configHome() + "/insitu.vistle", vistle::Parameter::ExistingFilename);
}

insitu::ObjectRetriever::PortAssignedObjectList MiniSimModule::getData(const insitu::MetaData &meta)
{
    std::vector<insitu::ObjectRetriever::PortAssignedObject> outputData;
    for (const auto &meshIter: meta) {
        if (meshName == meshIter.name()) {
            for (const auto &block: m_blockExtents) {
                if (!m_grids[block.first]) {
                    createGrid(block.first, block.second);
                    outputData.push_back(insitu::ObjectRetriever::PortAssignedObject{meshName, m_grids[block.first]});
                }
                std::cerr << "oscillation size = " << m_grids[block.first]->getNumElements() << std::endl;
                auto oscillation =
                    m_adapter->createVistleObject<Vec<Scalar, 1>>(m_grids[block.first]->getNumElements());
                auto data = m_blockData[block.first];
                for (size_t i = 0; i < oscillation->getSize(); i++) {
                    oscillation->x()[i] = data[i];
                }

                //std::copy(data, data + grid->getNumVertices(), oscillation->x().begin());
                oscillation->setMapping(DataBase::Mapping::Element);
                oscillation->setGrid(m_grids[block.first]);
                oscillation->setBlock(block.first);
                oscillation->describe("oscillation", id());
                m_adapter->updateMeta(oscillation);
                outputData.push_back(insitu::ObjectRetriever::PortAssignedObject{meshName, varName, oscillation});
            }
        }
    }
    return outputData;
}

std::unique_ptr<insitu::message::MessageHandler> MiniSimModule::connectToSim()
{
    CERR << "trying to connect to sim with file " << m_filePath->getValue() << std::endl;
    std::ifstream infile(m_filePath->getValue());
    if (infile.fail()) {
        CERR << "failed to open file " << m_filePath->getValue() << std::endl;
        return nullptr;
    }
    std::string key, rankStr;
    while (rankStr != std::to_string(rank())) {
        infile >> rankStr;
        infile >> key;
        if (infile.eof()) {
            CERR << "missing connection key for rank " << rank() << std::endl;
            return nullptr;
        }
    }
    try {
        return std::make_unique<vistle::insitu::message::InSituShmMessage>(key, m_simulationCommandsComm);
    } catch (const insitu::InsituException &e) {
        std::cerr << e.what() << '\n';
        return nullptr;
    }
}

MiniSimModule::~MiniSimModule()
{
    m_sim.terminate();
    m_terminate = true;
    if (getMessageHandler())
        getMessageHandler()->send(
            insitu::message::ConnectionClosed(insitu::message::DisconnectState::ShutdownNoRestart));
    if (m_simThread->joinable()) {
        m_simThread->join();
    }
}

void MiniSimModule::initialize(size_t nblocks, size_t n_local_blocks, float *origin, float *spacing, int domain_shape_x,
                               int domain_shape_y, int domain_shape_z, int *gid, int *from_x, int *from_y, int *from_z,
                               int *to_x, int *to_y, int *to_z, int *shape, int ghostLevels)
{
    m_blockExtents.clear();
    m_blockData.clear();
    m_particleData.clear();
    m_grids.clear();

    setParameter(m_numBlocks, (vistle::Integer)nblocks);

    for (int i = 0; i < 3; ++i)
        m_origin[i] = origin[i];

    for (int i = 0; i < 3; ++i)
        m_spacing[i] = spacing[i];

    for (int i = 0; i < 3; ++i)
        m_shape[i] = shape[i];

    m_numGhostCells = ghostLevels;

    SetDomainExtent(0, domain_shape_x - 1, 0, domain_shape_y - 1, 0, domain_shape_z - 1);

    for (size_t cc = 0; cc < n_local_blocks; ++cc) {
        SetBlockExtent(gid[cc], from_x[cc], to_x[cc], from_y[cc], to_y[cc], from_z[cc], to_z[cc]);
    }
}

bool MiniSimModule::changeParameter(const Parameter *param)
{
    updateSimParams(param);
    return InSituModuleBase::changeParameter(param);
}

void MiniSimModule::SetBlockExtent(int gid, int xmin, int xmax, int ymin, int ymax, int zmin, int zmax)
{
    m_blockExtents.insert_or_assign(gid, diy::DiscreteBounds{diy::DiscreteBounds::Point{xmin, ymin, zmin},
                                                             diy::DiscreteBounds::Point{xmax, ymax, zmax}});
}

void MiniSimModule::SetDomainExtent(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax)
{
    m_domainExtent.min[0] = xmin;
    m_domainExtent.min[1] = ymin;
    m_domainExtent.min[2] = zmin;

    m_domainExtent.max[0] = xmax;
    m_domainExtent.max[1] = ymax;
    m_domainExtent.max[2] = zmax;
}

void MiniSimModule::createSimParams()
{
    m_numThreads = addIntParameter("numThreads", "number of threads used by the sim", 1);
    m_numGhost = addIntParameter("numGhost", "number of ghost cells in each direction", 1);
    m_numParticles = addIntParameter("numParticles", "number of particles", 0);
    m_numBlocks = addIntParameter("numBlocks", "number of overall blocks, if zero mpi size is used", 0);

    m_verbose = addIntParameter("verbose", "print debug messages", false, Parameter::Boolean);
    m_sync = addIntParameter("sync", "synchronize after each timestep", false, Parameter::Boolean);

    m_velocityScale =
        addFloatParameter("velocityScale", "scale factor to convert function gradient to velocity", 50.0f);
    m_tEnd = addFloatParameter("tEnd", "end time", 10);
    m_dt = addFloatParameter("dt", "time step length", 0.01);
}

void MiniSimModule::updateSimParams(const Parameter *param)
{
    if (param == m_numThreads) {
        m_param.threads = m_numThreads->getValue();
    } else if (param == m_numGhost) {
        m_param.ghostCells = m_numGhost->getValue();
    } else if (param == m_numParticles) {
        m_param.numberOfParticles = m_numParticles->getValue();
    } else if (param == m_numBlocks) {
        m_param.nblocks = m_numBlocks->getValue();
    } else if (param == m_verbose) {
        m_param.verbose = m_verbose->getValue();
    } else if (param == m_sync) {
        m_param.sync = m_sync->getValue();
    } else if (param == m_velocityScale) {
        m_param.velocity_scale = m_velocityScale->getValue();
    } else if (param == m_tEnd) {
        m_param.t_end = m_tEnd->getValue();
    } else if (param == m_dt) {
        m_param.dt = m_dt->getValue();
    }
}

void MiniSimModule::SetBlockData(int gid, float *data)
{
    m_blockData[gid] = data;
}

void MiniSimModule::SetParticleData(int gid, const std::vector<Particle> &particles)
{
    m_particleData[gid] = &particles;
}

void MiniSimModule::createGrid(int blockId, const diy::DiscreteBounds cellExts)
{
    Index nx = cellExts.max[0] - cellExts.min[0] + 1 + 1;
    Index ny = cellExts.max[1] - cellExts.min[1] + 1 + 1;
    Index nz = cellExts.max[2] - cellExts.min[2] + 1 + 1;
    Index ncx = nx - 1;
    Index ncy = ny - 1;
    Index ncz = nz - 1;
    //std::array<Index, 3> dim = {nx, ny, nz};

    const Index numElements = ncx * ncy * ncz;
    const Index numCorners = numElements * 8;
    const Index numVertices = nx * ny * nz;
    auto &grid = m_grids[blockId];
    grid = m_adapter->createVistleObject<UnstructuredGrid>(numElements, numCorners, numVertices);
    auto nxny = nx * ny;

    Index ghostWidth[3][2];

    for (unsigned i = 0; i < 3; i++) {
        ghostWidth[i][0] = cellExts.min[i] + m_numGhostCells == m_domainExtent.min[i] ? 0 : m_numGhostCells;
        ghostWidth[i][1] = cellExts.max[i] - m_numGhostCells == m_domainExtent.max[i] ? 0 : m_numGhostCells;
    }

    Index numGostElements = 0;
    Index elem = 0;
    Index idx = 0;
    Index *cl = grid->cl().data();
    Index *el = grid->el().data();
    Byte *tl = grid->tl().data();
    for (Index iz = 0; iz < ncz; ++iz) {
        for (Index iy = 0; iy < ncy; ++iy) {
            for (Index ix = 0; ix < ncx; ++ix) {
                cl[idx++] = (iz)*nxny + iy * nx + ix;
                cl[idx++] = (iz + 1) * nxny + iy * nx + ix;
                cl[idx++] = (iz + 1) * nxny + iy * nx + ix + 1;
                cl[idx++] = (iz)*nxny + iy * nx + ix + 1;
                cl[idx++] = (iz)*nxny + (iy + 1) * nx + ix;
                cl[idx++] = (iz + 1) * nxny + (iy + 1) * nx + ix;
                cl[idx++] = (iz + 1) * nxny + (iy + 1) * nx + ix + 1;
                cl[idx++] = (iz)*nxny + (iy + 1) * nx + ix + 1;

                tl[elem] = UnstructuredGrid::HEXAHEDRON;
                if ((ix < ghostWidth[0][0] || ix + ghostWidth[0][1] >= nx) ||
                    (iy < ghostWidth[1][0] || iy + ghostWidth[1][1] >= ny) ||
                    (iz < ghostWidth[2][0] || iz + ghostWidth[2][1] >= nz)) {
                    grid->setGhost(elem, true);
                    ++numGostElements;
                } else {
                    grid->setGhost(elem, false);
                }
                ++elem;
                el[elem] = idx;
            }
        }
    }
    std::cerr << " numGostElements = " << numGostElements << std::endl;
    idx = 0;
    for (int k = cellExts.min[2]; k <= cellExts.max[2] + 1; ++k) {
        double z = m_origin[2] + m_spacing[2] * k;
        for (int j = cellExts.min[1]; j <= cellExts.max[1] + 1; ++j) {
            double y = m_origin[1] + m_spacing[1] * j;
            for (int i = cellExts.min[0]; i <= cellExts.max[0] + 1; ++i) {
                double x = m_origin[0] + m_spacing[0] * i;
                grid->x().data()[idx] = x;
                grid->y().data()[idx] = y;
                grid->z().data()[idx] = z;
                ++idx;
            }
        }
    }

    //Scalar *x = grid->x().data();
    //Scalar *y = grid->y().data();
    //Scalar *z = grid->z().data();
    //for (Index i = 0; i < dim[0]; ++i) {
    //    for (Index j = 0; j < dim[1]; ++j) {
    //        for (Index k = 0; k < dim[2]; ++k) {
    //            Index idx = StructuredGrid::vertexIndex(i, j, k, dim.data());
    //            x[idx] = cellExts.min[0] + i * m_Spacing[0];
    //            y[idx] = cellExts.min[1] + j * m_Spacing[1];
    //            z[idx] = cellExts.min[2] + k * m_Spacing[2];
    //        }
    //    }
    //}
    m_adapter->updateMeta(grid);
    grid->setTimestep(-1);
    grid->setIteration(-1);

    assert(el[numElements] == numCorners);
}

void MiniSimModule::execute(long step, float time)
{
    m_adapter->execute(step);
}


void MiniSimModule::finalize()
{}
