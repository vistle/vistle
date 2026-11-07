#ifndef VISTLE_TRACER_PARTICLE_H
#define VISTLE_TRACER_PARTICLE_H

#include <algorithm>
#include <vector>
#include <future>
#include <iosfwd>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>
#include <boost/mpi/inplace.hpp>

#include <vistle/core/index.h>
#include <vistle/core/lines.h>
#include <vistle/core/scalars.h>
#include <vistle/core/vector.h>
#include <vistle/util/enum.h>

#include "Integrator.h"

class BlockData;
class GlobalData;

// clang-format off
DEFINE_ENUM_WITH_STRING_CONVERSIONS(
    StopReason,
    (StillActive)(InitiallyOutOfDomain)(OutOfDomain)(NotMoving)(StepLimitReached)(DistanceLimitReached)(TimeLimitReached)(ArithmeticError)
    (NumStopReasons) // keep last
)
// clang-format on
struct Segment {
    vistle::Index m_id = vistle::InvalidIndex; //! id of particle that was traced
    int m_num; // >= 0: forward, < 0: backward
    vistle::Index m_startStep;
    std::vector<vistle::Vector3> m_xhist; //!< trajectory
    std::vector<vistle::Vector3> m_vhist; //!< previous velocities
    std::vector<vistle::Scalar> m_stepWidth; //!< previous integration stepwidths
    std::vector<vistle::Index> m_steps; //!< previous steps
    std::vector<vistle::Scalar> m_times; //!< previous times
    std::vector<vistle::Scalar> m_dists; //!< signed distance travelled since the trace started
    std::vector<vistle::Index> m_cellIndex; //!< previous cell/element indices
    std::vector<vistle::Index> m_blockIndex; //!< index of current block
    std::vector<vistle::Scalar> m_wallclock; //!< time difference to when previous step was computed
    std::vector<std::vector<vistle::Scalar>> m_scalars; //!< previous scalars

    Segment(int num = 0): m_num(num), m_startStep(vistle::InvalidIndex) {}

    // the number of bytes to transmit for this segment, for estimating how many segments tobatch
    size_t numBytes() const;

    void simplify(double relerr);
    //! compute relative error when interpolating for index i from i0 and i1
    double interpolationError(vistle::Index i0, vistle::Index i1, vistle::Index i) const;
    vistle::Scalar cosAngle(vistle::Index i) const;

    // just for Boost.MPI
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &m_id;
        ar &m_num;
        ar &m_blockIndex;
        ar &m_xhist;
        ar &m_vhist;
        ar &m_stepWidth;
        ar &m_steps;
        ar &m_times;
        ar &m_dists;
        ar &m_cellIndex;
        ar &m_wallclock;
        for (auto &s: m_scalars) {
            ar &s;
        }
    }

    void clear()
    {
        m_id = vistle::InvalidIndex;
        m_num = 0;
        m_xhist.clear();
        m_vhist.clear();
        m_stepWidth.clear();
        m_steps.clear();
        m_times.clear();
        m_dists.clear();
        m_cellIndex.clear();
        m_wallclock.clear();
        for (auto &s: m_scalars) {
            s.clear();
        }
    }
};

typedef std::map<int, std::shared_ptr<Segment>> SegmentMap;

// collect a batch of segments for transmission to one rank
struct SegmentBatch {
    std::vector<vistle::Index> m_id; //!< particle the segment at the same index belongs to
    std::vector<Segment> m_segments;
    size_t m_bytes = 0; //!< the total size of the data segments to be sent

    void add(vistle::Index id, Segment &&segment);

    // just for Boost.MPI
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &m_id;
        ar &m_segments;
        ar &m_bytes;
    }
};

// collect segments and send them to the rank assembling the trajectory of a particle
class SegmentRouter {
public:
    SegmentRouter(const boost::mpi::communicator &comm, const void *client);

    //! called to take the batches which arrived, while no batch of ours can go out
    void setDrain(std::function<vistle::Index()> drain);
    void add(vistle::Index id, int dst, Segment &&segment);
    void flush(bool force = false); //!< send the batches which grew big enough, all of them when forced
    void sendAll(); //!< get every remaining batch out, once the tracing is over
    void finish(); //!< wait for the transfers to be taken
    const std::vector<int> &numSent() const; //!< batches put on the wire per destination

private:
    struct Pending {
        boost::mpi::request m_request;
        size_t m_bytes = 0;
    };

    void sendBatch(int dst);
    void reap();
    void waitPending();

    const boost::mpi::communicator &m_comm;
    const void *m_client; //!< stable key for the waiting between two batches
    std::map<int, SegmentBatch> m_batches; //!< segments per destination not sent yet
    std::deque<Pending> m_pending; //!< batches in flight, oldest first
    size_t m_bytes = 0; //!< bytes of the batches in flight
    std::vector<int> m_sent; //!< batches already sent per destination
    std::function<vistle::Index()> m_drain; //!< takes the batches which arrived
};

template<class S>
class Particle {
    friend class Integrator<S>;
    friend class boost::serialization::access;

public:
    typedef typename Integrator<S>::Scal Scal;
    typedef typename Integrator<S>::Vect3 Vect3;
    static vistle::Vector3 VV(const Vect3 &v) { return Integrator<S>::VV(v); }
    static Vect3 VI(const vistle::Vector3 &v) { return Integrator<S>::VI(v); }

    Particle(vistle::Index id, int rank, vistle::Index startId, const vistle::Vector3 &pos, bool forward,
             GlobalData &global, vistle::Index timestep);
    ~Particle();
    vistle::Index id() const;
    int rank();
    bool isActive() const;
    bool inGrid() const;
    bool isMoving();
    bool isForward() const;
    void Deactivate(StopReason reason);
    void EmitData();
    bool Step();
    void broadcast(boost::mpi::communicator mpi_comm, int root);
    void addSegment(Segment &&segment); //!< adds trajectory data received from another rank
    void UpdateBlock(BlockData *block);
    StopReason stopReason() const;
    void applyStopReason(int reason); //!< takes over a reason recorded elsewhere, keeping an own one
    void enableCelltree(bool value);
    void initiateRankSearch(bool skipSearch = false);
    bool rankSearchHit() const; //!< position lies in a cell of a block local to this rank
    int applyRankSearch(int rank); //!< takes over unresolved owning rank, returns rank where tracing continues
    void startTracing();
    bool isTracing(bool wait);
    bool madeProgress() const;
    bool trace();
    void finishSegment(SegmentRouter &router);
    void fetchSegments(Particle &other); //! move segments from other particle to this one
    void addToOutput();
    vistle::Scalar time() const;

    std::ostream &operator<<(std::ostream &os) const;

private:
    bool findCell(double time);

    GlobalData &m_global;
    vistle::Index m_id; //!< particle id
    vistle::Index m_startId; //!< id of start point;
    int m_rank; //! MPI rank where resulting geometry is assembled
    vistle::Index m_timestep; //! timestep of particle for streamlines
    std::future<bool> m_progressFuture; //!< future on whether particle has made progress during trace()
    bool m_progress;
    bool m_tracing; //!< particle is currently tracing on this node
    bool m_forward; //!< trace direction
    Vect3 m_x; //!< current position
    Vect3 m_xold; //!< previous position
    vistle::Vector3 m_v; //!< current velocity
    std::vector<vistle::Scalar> m_scalars; //!< current scalar values
    vistle::Index m_stp; //!< current integration step
    vistle::Scalar m_time; //! current time
    vistle::Scalar m_dist; //!< total distance traveled
    int m_segment; //!< number of segment being traced on this rank
    vistle::Index m_segmentStart; //! number of step where this segment started

    std::shared_ptr<Segment> m_currentSegment;
    SegmentMap m_segments; //!< finished segments, written only by the main thread

    BlockData *m_block; //!< current block for current particle position
    vistle::Index m_el; //!< index of cell for current particle position
    bool m_ingrid; //!< particle still within domain on some rank
    Integrator<S> m_integrator;
    StopReason m_stopReason; //! reason why particle was deactivated
    bool m_useCelltree; //! whether to use celltree for acceleration
    double m_wallclock = 0.; //! time when previous integration step was computed

    // just for Boost.MPI
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &m_x;
        ar &m_xold;
        ar &m_v;
        ar &m_stp;
        ar &m_time;
        ar &m_dist;
        ar &m_scalars;
        ar &m_ingrid;
        ar &m_integrator.m_h;
        ar &m_stopReason;
        ar &m_segment;
        ar &m_wallclock;
    }
};

// reduce one value per particle with a single collective per batch
template<class GetValue, class Op>
void batchReduceValues(const boost::mpi::communicator &mpi_comm, vistle::Index count, GetValue getValue, Op op,
                       std::vector<int> &values)
{
    const vistle::Index batch = 65536; // keeps a single collective from growing too large

    values.resize(count);
    for (vistle::Index begin = 0; begin < count; begin += batch) {
        const int n = int(std::min(batch, count - begin));
        int *value = values.data() + begin;
        for (int i = 0; i < n; ++i)
            value[i] = getValue(begin + i);

        boost::mpi::all_reduce(mpi_comm, boost::mpi::inplace(value), n, op);
    }
}

// agree in one batch on which rank tracing a particle shall continue
template<class GetParticle>
void batchFinishRankSearchImpl(const boost::mpi::communicator &mpi_comm, vistle::Index count, GetParticle getParticle,
                               std::vector<int> &ranks)
{
    batchReduceValues(
        mpi_comm, count, [&](vistle::Index i) { return getParticle(i)->rankSearchHit() ? mpi_comm.rank() : -1; },
        boost::mpi::maximum<int>(), ranks);

    for (vistle::Index i = 0; i < count; ++i)
        getParticle(i)->applyRankSearch(ranks[i]);
}

template<class S>
void batchFinishRankSearch(const boost::mpi::communicator &mpi_comm,
                           const std::vector<std::shared_ptr<Particle<S>>> &particles, std::vector<int> &ranks)
{
    batchFinishRankSearchImpl(
        mpi_comm, particles.size(), [&](vistle::Index i) { return particles[i].get(); }, ranks);
}

template<class S>
void batchFinishRankSearch(const boost::mpi::communicator &mpi_comm,
                           const std::vector<std::shared_ptr<Particle<S>>> &particles,
                           const std::vector<vistle::Index> &indices, std::vector<int> &ranks)
{
    batchFinishRankSearchImpl(
        mpi_comm, indices.size(), [&](vistle::Index i) { return particles[indices[i]].get(); }, ranks);
}

template<class S>
vistle::Index pollSegmentBatches(const boost::mpi::communicator &mpi_comm,
                                 const std::vector<std::shared_ptr<Particle<S>>> &particles);

template<class S>
void receiveSegmentBatches(const boost::mpi::communicator &mpi_comm,
                           const std::vector<std::shared_ptr<Particle<S>>> &particles, vistle::Index num);

extern template vistle::Index
pollSegmentBatches<double>(const boost::mpi::communicator &mpi_comm,
                           const std::vector<std::shared_ptr<Particle<double>>> &particles);

extern template void receiveSegmentBatches<double>(const boost::mpi::communicator &mpi_comm,
                                                   const std::vector<std::shared_ptr<Particle<double>>> &particles,
                                                   vistle::Index num);

extern template class Particle<double>;
extern template class Particle<float>;
#endif
