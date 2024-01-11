#include "MiniSim.h"
#include <diy/master.hpp>
#include <diy/decomposition.hpp>
#include <vistle/core/unstr.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/util/shmconfig.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/addObjectMsq.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/util/stopwatch.h>
using namespace vistle;
MODULE_MAIN(MiniSimModule)


struct MiniSimModule::InternalsType {
    InternalsType(): NumBlocks(0), DomainExtent(3), Origin{}, Spacing{1, 1, 1}, Shape{}, NumGhostCells(0) {}

    long NumBlocks; // total number of blocks on all ranks
    diy::DiscreteBounds DomainExtent; // global index space
    std::map<int, diy::DiscreteBounds> BlockExtents; // local block extents, indexed by global block id
    std::map<int, float *> BlockData; // local data array, indexed by block id
    std::map<int, const std::vector<Particle> *> ParticleData;

    double Origin[3]; // lower left corner of simulation domain
    double Spacing[3]; // mesh spacing

    int Shape[3];
    int NumGhostCells; // number of ghost cells

    std::unique_ptr<vistle::insitu::message::AddObjectMsq> sendMessageQueue =
        nullptr; // Queue to send addObject messages to module

    insitu::message::SyncShmIDs shmIDs;
    std::map<int, UnstructuredGrid::ptr> grids;
};

MiniSimModule::MiniSimModule(const std::string &name, int moduleID, mpi::communicator comm)
: insitu::InSituReader(name, moduleID, comm)
{
    m_filePath = addStringParameter("input_params", "path to file with input parameters", "",
                                    vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "input files (*.osc)");
    m_numTimesteps = addIntParameter("num_timesteps", "maximum number of timesteps to execute", 10);
    setParameterMinimum(m_numTimesteps, Integer{1});
    m_gridOut = createOutputPort("grid_out", "structured grid");
    m_dataOut = createOutputPort("data_out", "oscillators");
    reconnect();
    createSimParams();

    m_simThread.reset(new std::thread{[this]() {
        Internals = new InternalsType();
        insitu::message::ModuleInfo modInfo;
        modInfo.update(gatherModuleInfo());
        Internals->shmIDs.initialize(id(), rank(), std::to_string(instanceNum()),
                                     insitu::message::SyncShmIDs::Mode::Attach);
        Internals->sendMessageQueue = std::make_unique<insitu::message::AddObjectMsq>(modInfo, rank());

        while (!m_terminate) {
            std::unique_lock<std::mutex> lk{m_waitForBeginExecuteMutex};
            m_waitForBeginExecute.wait(lk);

            m_simRunning = true;
            try {
                StopWatch w{"simulation took "};
                m_sim.run(*this, (size_t)m_numTimesteps->getValue(), m_filePath->getValue(), this->comm(), m_param);
            } catch (const std::runtime_error &e) {
                std::cerr << "Failed to create simulation: " << e.what() << std::endl;
                break;
            }
            m_waitForEndSim.notify_all();
            m_simRunning = false;
        }
    }});
}

MiniSimModule::~MiniSimModule()
{
    m_terminate = true;
    if (m_simThread->joinable()) {
        m_waitForBeginExecute.notify_all();
        m_simThread->join();
    }
}

void MiniSimModule::initialize(size_t nblocks, size_t n_local_blocks, float *origin, float *spacing, int domain_shape_x,
                               int domain_shape_y, int domain_shape_z, int *gid, int *from_x, int *from_y, int *from_z,
                               int *to_x, int *to_y, int *to_z, int *shape, int ghostLevels)
{
    Internals->BlockExtents.clear();
    Internals->BlockData.clear();
    Internals->ParticleData.clear();
    Internals->grids.clear();

    Internals->NumBlocks = nblocks;
    for (int i = 0; i < 3; ++i)
        Internals->Origin[i] = origin[i];

    for (int i = 0; i < 3; ++i)
        Internals->Spacing[i] = spacing[i];

    for (int i = 0; i < 3; ++i)
        Internals->Shape[i] = shape[i];

    Internals->NumGhostCells = ghostLevels;

    SetDomainExtent(0, domain_shape_x - 1, 0, domain_shape_y - 1, 0, domain_shape_z - 1);

    for (size_t cc = 0; cc < n_local_blocks; ++cc) {
        SetBlockExtent(gid[cc], from_x[cc], to_x[cc], from_y[cc], to_y[cc], from_z[cc], to_z[cc]);
    }
}


bool MiniSimModule::beginExecute()
{
    m_waitForBeginExecute.notify_all();
    return true;
}

bool MiniSimModule::endExecute()
{
    if (m_simRunning) {
        m_sim.terminate(); //terminate the sim
        std::unique_lock<std::mutex> lk{m_waitForEndSimMutex};
        m_waitForEndSim.wait(lk);
    }

    return true;
}

bool MiniSimModule::changeParameter(const Parameter *param)
{
    updateSimParams(param);
    return Module::changeParameter(param);
}

void MiniSimModule::SetBlockExtent(int gid, int xmin, int xmax, int ymin, int ymax, int zmin, int zmax)
{
    Internals->BlockExtents.insert_or_assign(gid, diy::DiscreteBounds{diy::DiscreteBounds::Point{xmin, ymin, zmin},
                                                                      diy::DiscreteBounds::Point{xmax, ymax, zmax}});
}

void MiniSimModule::SetDomainExtent(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax)
{
    Internals->DomainExtent.min[0] = xmin;
    Internals->DomainExtent.min[1] = ymin;
    Internals->DomainExtent.min[2] = zmin;

    Internals->DomainExtent.max[0] = xmax;
    Internals->DomainExtent.max[1] = ymax;
    Internals->DomainExtent.max[2] = zmax;
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
    Internals->BlockData[gid] = data;
}

void MiniSimModule::SetParticleData(int gid, const std::vector<Particle> &particles)
{
    Internals->ParticleData[gid] = &particles;
}

void createGrid(MiniSimModule::InternalsType *Internals, int blockId, long step, float time,
                const diy::DiscreteBounds cellExts)
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
    auto &grid = Internals->grids[blockId];
    grid = Internals->shmIDs.createVistleObject<UnstructuredGrid>(numElements, numCorners, numVertices);
    auto nxny = nx * ny;

    Index ghostWidth[3][2];

    for (unsigned i = 0; i < 3; i++) {
        ghostWidth[i][0] =
            cellExts.min[i] + Internals->NumGhostCells == Internals->DomainExtent.min[i] ? 0 : Internals->NumGhostCells;
        ghostWidth[i][1] =
            cellExts.max[i] - Internals->NumGhostCells == Internals->DomainExtent.max[i] ? 0 : Internals->NumGhostCells;
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
                    grid->setIsGhost(elem, true);
                    ++numGostElements;
                } else {
                    grid->setIsGhost(elem, false);
                }
                ++elem;
                el[elem] = idx;
            }
        }
    }
    std::cerr << " numGostElements = " << numGostElements << std::endl;
    idx = 0;
    for (int k = cellExts.min[2]; k <= cellExts.max[2] + 1; ++k) {
        double z = Internals->Origin[2] + Internals->Spacing[2] * k;
        for (int j = cellExts.min[1]; j <= cellExts.max[1] + 1; ++j) {
            double y = Internals->Origin[1] + Internals->Spacing[1] * j;
            for (int i = cellExts.min[0]; i <= cellExts.max[0] + 1; ++i) {
                double x = Internals->Origin[0] + Internals->Spacing[0] * i;
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
    //            x[idx] = cellExts.min[0] + i * Internals->Spacing[0];
    //            y[idx] = cellExts.min[1] + j * Internals->Spacing[1];
    //            z[idx] = cellExts.min[2] + k * Internals->Spacing[2];
    //        }
    //    }
    //}
    grid->setTimestep(-1);
    assert(el[numElements] == numCorners);
}

void MiniSimModule::execute(long step, float time)
{
    for (const auto &block: Internals->BlockExtents) {
        auto &grid = Internals->grids[block.first];
        if (!grid) {
            createGrid(Internals, block.first, step, time, block.second);
            Internals->sendMessageQueue->addObject(m_gridOut->getName(), grid);
        }
        std::cerr << "oscillation size = " << grid->getNumElements() << std::endl;
        auto oscillation = Internals->shmIDs.createVistleObject<Vec<Scalar, 1>>(grid->getNumElements());
        auto data = Internals->BlockData[block.first];
        for (size_t i = 0; i < oscillation->getSize(); i++) {
            oscillation->x()[i] = data[i];
        }

        //std::copy(data, data + grid->getNumVertices(), oscillation->x().begin());
        oscillation->setMapping(DataBase::Mapping::Element);
        oscillation->setGrid(grid);
        oscillation->setTimestep(step);
        oscillation->setRealTime(time);
        oscillation->setBlock(block.first);
        oscillation->addAttribute("_species", "oscillation");
        Internals->sendMessageQueue->addObject(m_dataOut->getName(), oscillation);
    }
    Internals->sendMessageQueue->sendObjects();
}


void MiniSimModule::finalize()
{}
