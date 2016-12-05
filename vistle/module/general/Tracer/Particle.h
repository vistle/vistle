#ifndef TRACER_PARTICLE_H
#define TRACER_PARTICLE_H

// define for profiling
//#define TIMING

#include <vector>

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
                                        (NumStopReasons)
    )
    Particle(vistle::Index i, const vistle::Vector3 &pos, vistle::Scalar h, vistle::Scalar hmin,
             vistle::Scalar hmax, vistle::Scalar errtol, IntegrationMethod int_mode, const std::vector<std::unique_ptr<BlockData>> &bl,
             vistle::Index stepsmax, bool forward);
    ~Particle();
    void PointsToLines();
    bool isActive();
    bool inGrid();
    bool isMoving(vistle::Index maxSteps, vistle::Scalar minSpeed);
    bool findCell(const std::vector<std::unique_ptr<BlockData>> &block);
    void Deactivate(StopReason reason);
    void EmitData(bool havePressure);
    bool Step();
    void Communicator(boost::mpi::communicator mpi_comm, int root, bool havePressure);
    void UpdateBlock(BlockData *block);
    StopReason stopReason() const;
    void enableCelltree(bool value);

private:
    vistle::Index m_id; //!< partcle id
    vistle::Vector3 m_x; //!< current position
    vistle::Vector3 m_xold; //!< previous position
    std::vector<vistle::Vector3> m_xhist; //!< trajectory
    vistle::Vector3 m_v; //!< current velocity
    vistle::Scalar m_p; //!< current pressure
    std::vector<vistle::Vector3> m_vhist; //!< previous velocities
    std::vector<vistle::Scalar> m_pressures; //!< previous pressures
    std::vector<vistle::Index> m_steps; //!< previous steps
    vistle::Index m_stp; //!< current integration step
    BlockData *m_block; //!< current block for current particle position
    vistle::Index m_el; //!< index of cell for current particle position
    bool m_ingrid; //!< particle still within domain on some rank
    bool m_searchBlock; //!< particle is new - has to be initialized
    Integrator m_integrator;
    StopReason m_stopReason; //! reason why particle was deactivated
    bool m_useCelltree; //! whether to use celltree for acceleration

    // just for Boost.MPI
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & m_x;
        ar & m_xold;
        ar & m_v;
        ar & m_stp;
        ar & m_p;
        ar & m_ingrid;
        ar & m_integrator.m_h;
        ar & m_stopReason;
    }
};
#endif
