#ifndef VISTLE_MINI_SIM_H
#define VISTLE_MINI_SIM_H
#include "minisim/minisim.h"
#include <vistle/insitu/module/inSituModule.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/sensei/sensei.h>
#include <vistle/insitu/sensei/metaData.h>
#include <vistle/core/unstr.h>
#include <diy/master.hpp>
#include <diy/decomposition.hpp>
#include <memory>
#include <thread>
struct Particle;
class MiniSimModule: public vistle::insitu::InSituModule {
public:
    MiniSimModule(const std::string &name, int moduleID, mpi::communicator comm);
    ~MiniSimModule();
    void initialize(size_t nblocks, size_t n_local_blocks, float *origin, float *spacing, int domain_shape_x,
                    int domain_shape_y, int domain_shape_z, int *gid, int *from_x, int *from_y, int *from_z, int *to_x,
                    int *to_y, int *to_z, int *shape, int ghostLevels);
    void SetBlockData(int gid, float *data);
    void SetParticleData(int gid, const std::vector<Particle> &particles);
    void execute(long step, float time);
    void finalize();

private:
    void SetBlockExtent(int gid, int xmin, int xmax, int ymin, int ymax, int zmin, int zmax);

    void SetDomainExtent(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax);
    void createSimParams();
    void updateSimParams(const vistle::Parameter *param);
    std::unique_ptr<vistle::insitu::message::MessageHandler> connectToSim() override;
    bool changeParameter(const vistle::Parameter *param) override;
    void createGrid(int blockId, const diy::DiscreteBounds cellExts);

    vistle::insitu::sensei::ObjectRetriever::PortAssignedObjectList
    getData(const vistle::insitu::sensei::MetaData &meta);

    std::mutex m_waitForBeginExecuteMutex, m_waitForEndSimMutex;
    std::atomic_bool m_terminate{false};
    minisim::MiniSim m_sim;
    vistle::StringParameter *m_inputFilePath = nullptr;
    vistle::IntParameter *m_numTimesteps = nullptr;
    std::unique_ptr<std::thread> m_simThread;
    std::atomic_bool m_terminateSim{false};
    //params and according vistle parameter
    minisim::Parameter m_param;
    vistle::IntParameter *m_numThreads, *m_numGhost, *m_numParticles, *m_numBlocks;
    vistle::IntParameter *m_verbose, *m_sync;
    vistle::FloatParameter *m_velocityScale, *m_tEnd, *m_dt;

    diy::DiscreteBounds m_domainExtent = 3; // global index space
    std::map<int, diy::DiscreteBounds> m_blockExtents; // local block extents, indexed by global block id
    std::map<int, float *> m_blockData; // local data array, indexed by block id
    std::map<int, const std::vector<Particle> *> m_particleData;

    double m_origin[3] = {0, 0, 0}; // lower left corner of simulation domain
    double m_spacing[3] = {1, 1, 1}; // mesh spacing

    int m_shape[3] = {0, 0, 0};
    int m_numGhostCells = 0; // number of ghost cells

    std::map<int, vistle::UnstructuredGrid::ptr> m_grids;

    std::unique_ptr<vistle::insitu::sensei::Adapter> m_adapter;
};


#endif // !VISTLE_MINI_SIM_H
