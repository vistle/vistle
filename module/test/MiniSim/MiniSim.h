#ifndef VISTLE_MINI_SIM_H
#define VISTLE_MINI_SIM_H
#include "minisim/minisim.h"
#include <vistle/insitu/module/inSituReader.h>
#include <memory>
#include <thread>

struct Particle;
class MiniSimModule: public vistle::insitu::InSituReader {
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
    struct InternalsType;

private:
    virtual bool beginExecute() override;
    virtual bool endExecute() override;
    virtual bool changeParameter(const vistle::Parameter *param) override;
    void SetBlockExtent(int gid, int xmin, int xmax, int ymin, int ymax, int zmin, int zmax);

    void SetDomainExtent(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax);
    void createSimParams();
    void updateSimParams(const vistle::Parameter *param);

    InternalsType *Internals;
    std::mutex m_waitForBeginExecuteMutex, m_waitForEndSimMutex;
    std::condition_variable m_waitForBeginExecute, m_waitForEndSim;
    std::atomic_bool m_terminate{false}, m_simRunning{false};
    minisim::MiniSim m_sim;
    vistle::StringParameter *m_filePath = nullptr;
    vistle::IntParameter *m_numTimesteps = nullptr;
    std::unique_ptr<std::thread> m_simThread;
    std::atomic_bool m_terminateSim{false};
    vistle::Port *m_gridOut, *m_dataOut;
    //params and according vistle parameter
    minisim::Parameter m_param;
    vistle::IntParameter *m_numThreads, *m_numGhost, *m_numParticles, *m_numBlocks;
    vistle::IntParameter *m_verbose, *m_sync;
    vistle::FloatParameter *m_velocityScale, *m_tEnd, *m_dt;
};

#endif // !VISTLE_MINI_SIM_H
