#ifndef TRACER_PARTICLE_H
#define TRACER_PARTICLE_H

// define for profiling
//#define TIMING

#include <vector>
#include <future>

#include <boost/mpi/communicator.hpp>

#include <util/enum.h>
#include <core/vector.h>
#include <core/scalars.h>
#include <core/index.h>

#include "Integrator.h"

class BlockData;

class Particle {

    friend class Integrator;
    friend class boost::serialization::access;

public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(StopReason,
                                        (StillActive)
                                        (InitiallyOutOfDomain)
                                        (OutOfDomain)
                                        (NotMoving)
                                        (StepLimitReached)
                                        (DistanceLimitReached)
                                        (NumStopReasons)
    )
    Particle(vistle::Index i, const vistle::Vector3 &pos, vistle::Scalar h, vistle::Scalar hmin,
             vistle::Scalar hmax, vistle::Scalar errtol, IntegrationMethod int_mode, const std::vector<std::unique_ptr<BlockData>> &bl,
             vistle::Index stepsmax, bool forward);
    ~Particle();
    vistle::Index id() const;
    void PointsToLines();
    bool isActive() const;
    bool inGrid() const;
    bool isMoving(vistle::Index maxSteps, vistle::Scalar traceLen, vistle::Scalar minSpeed);
    bool findCell(const std::vector<std::unique_ptr<BlockData>> &block);
    void Deactivate(StopReason reason);
    void EmitData();
    bool Step();
    void broadcast(boost::mpi::communicator mpi_comm, int root);
    void UpdateBlock(BlockData *block);
    StopReason stopReason() const;
    void enableCelltree(bool value);
    bool startTracing(std::vector<std::unique_ptr<BlockData>> &blocks, vistle::Index maxSteps, double traceLen, double minSpeed);
    bool isTracing(bool wait);
    bool madeProgress() const;
    void setTracing(bool trace);
    bool trace(std::vector<std::unique_ptr<BlockData>> &blocks, vistle::Index maxSteps, double traceLen, double minSpeed);

private:
    std::future<bool> m_progressFuture; //!< future on whether particle has made progress during trace()
    bool m_progress;
    bool m_tracing; //!< particle is currently tracing on this node
    bool m_forward; //!< trace direction
    vistle::Index m_id; //!< partcle id
    vistle::Vector3 m_x; //!< current position
    vistle::Vector3 m_xold; //!< previous position
    std::vector<vistle::Vector3> m_xhist; //!< trajectory
    vistle::Vector3 m_v; //!< current velocity
    vistle::Scalar m_p; //!< current pressure
    vistle::Index m_stp; //!< current integration step
    vistle::Scalar m_time; //! current time
    vistle::Scalar m_dist; //!< total distance travelled

    BlockData *m_block; //!< current block for current particle position
    vistle::Index m_el; //!< index of cell for current particle position
    bool m_ingrid; //!< particle still within domain on some rank
    Integrator m_integrator;
    StopReason m_stopReason; //! reason why particle was deactivated
    bool m_useCelltree; //! whether to use celltree for acceleration

    // updated by EmitData
    std::vector<vistle::Vector3> m_vhist; //!< previous velocities
    std::vector<vistle::Scalar> m_pressures; //!< previous pressures
    std::vector<vistle::Index> m_steps; //!< previous steps
    std::vector<vistle::Scalar> m_times; //!< previous times
    std::vector<vistle::Scalar> m_dists; //!< previous times


    // just for Boost.MPI
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & m_x;
        ar & m_xold;
        ar & m_v;
        ar & m_stp;
        ar & m_time;
        ar & m_dist;
        ar & m_p;
        ar & m_ingrid;
        ar & m_integrator.m_h;
        ar & m_stopReason;
    }
};
#endif
