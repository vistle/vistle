#include <limits>
#include <algorithm>
#include <boost/mpi/collectives/broadcast.hpp>
#include <core/vec.h>
#include <util/math.h>
#include "Tracer.h"
#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"

using namespace vistle;

Particle::Particle(Index id, int rank, Index startId, const Vector3 &pos, bool forward, GlobalData &global, Index timestep):
m_global(global),
m_id(id),
m_startId(startId),
m_rank(rank),
m_timestep(timestep),
m_progress(false),
m_tracing(false),
m_forward(forward),
m_x(pos),
m_xold(pos),
m_v(Vector3(std::numeric_limits<Scalar>::max(),0,0)), // keep large enough so that particle moves initially
m_p(0),
m_stp(0),
m_time(0),
m_dist(0),
m_segment(forward ? 0 : -1),
m_segmentStart(0),
m_block(nullptr),
m_el(InvalidIndex),
m_ingrid(true),
m_integrator(this, forward),
m_stopReason(StillActive),
m_useCelltree(m_global.use_celltree)
{
   m_integrator.enableCelltree(m_useCelltree);

   if (!global.blocks[timestep].empty())
       m_time = global.blocks[timestep][0]->m_grid->getRealTime();
}

Particle::~Particle(){}

Index Particle::id() const {
    return m_id;
}

int Particle::rank() {

    return m_rank;
}

Particle::StopReason Particle::stopReason() const {
    return m_stopReason;
}

void Particle::enableCelltree(bool value) {

    m_useCelltree = value;
    m_integrator.enableCelltree(value);
}

int Particle::startTracing(boost::mpi::communicator mpi_comm) {

    assert(!m_tracing);
    assert(!m_currentSegment);
    m_progress = false;

    int rank = -1;

    if (findCell(m_time)) {
        m_integrator.hInit();
        rank = mpi_comm.rank();
    }

    rank = boost::mpi::all_reduce(mpi_comm, rank, boost::mpi::maximum<int>());
    if (rank == mpi_comm.rank()) {
        assert(inGrid());
        m_tracing = true;
        m_progressFuture =  std::async(std::launch::async, [this]() -> bool { return trace(); });
    }
    return rank;
}

bool Particle::isActive() const {

    return m_ingrid && (m_tracing || m_progress || m_block);
}

bool Particle::inGrid() const {

    return m_ingrid;
}

bool Particle::isMoving() {

    if (!m_ingrid) {
       return false;
    }

    bool moving = m_v.norm() > m_global.min_vel;
    if(std::abs(m_dist) > m_global.trace_len || std::abs(m_time) > m_global.trace_time || m_stp > m_global.max_step || !moving){

       if (std::abs(m_dist) > m_global.trace_len)
           this->Deactivate(DistanceLimitReached);
       else if (std::abs(m_time) > m_global.trace_time)
           this->Deactivate(TimeLimitReached);
       else if (m_stp > m_global.max_step)
           this->Deactivate(StepLimitReached);
       else
           this->Deactivate(NotMoving);
       return false;
    }

    return true;
}

bool Particle::isForward() const {
    return m_forward;
}

bool Particle::findCell(double time) {

    if (!m_ingrid) {
        return false;
    }

    if (m_block) {

        auto grid = m_block->getGrid();
        if (m_global.int_mode == ConstantVelocity) {
            const auto neigh = grid->getNeighborElements(m_el);
            for (auto el: neigh) {
                if(grid->inside(el, m_x)) {
                    m_el = el;
                    assert(m_currentSegment);
                    return true;
                }
            }
        } else {
           m_el = grid->findCell(m_x, m_el, m_useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        }
        if (m_el != InvalidIndex) {
            assert(m_currentSegment);
            return true;
        }
    }

    for(auto &block: m_global.blocks[m_timestep]) {

        if (block.get() == m_block) {
            // don't try previous block again
            m_block = nullptr;
            continue;
        }
        auto grid = block->getGrid();
        m_el = grid->findCell(m_x, InvalidIndex, m_useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (m_el!=InvalidIndex) {

            if (!m_currentSegment) {
                m_currentSegment = std::make_shared<Segment>();
                m_currentSegment->m_startStep = m_stp;
                m_currentSegment->m_num = m_segment;
                if (m_forward)
                    ++m_segment;
                else
                    --m_segment;
            }

            UpdateBlock(block.get());
            assert(m_currentSegment);
            return true;
        }
    }

    UpdateBlock(nullptr);
    return false;
}

void Particle::EmitData() {

   m_currentSegment->m_xhist.push_back(m_xold);
   m_currentSegment->m_vhist.push_back(m_v);
   m_currentSegment->m_steps.push_back(m_stp);
   m_currentSegment->m_pressures.push_back(m_p); // will be ignored later on
   m_currentSegment->m_times.push_back(m_time);
   m_currentSegment->m_dists.push_back(m_dist);
}

void Particle::Deactivate(StopReason reason) {

    if (m_stopReason == StillActive)
        m_stopReason = reason;
    UpdateBlock(nullptr);
    m_ingrid = false;
}

bool Particle::Step() {

   const auto &grid = m_block->getGrid();
   auto inter = grid->getInterpolator(m_el, m_x, m_block->m_vecmap);
   m_v = inter(m_block->m_vx, m_block->m_vy, m_block->m_vz);
   if (m_block->m_p) {
      if (m_block->m_scamap != m_block->m_vecmap)
          inter = grid->getInterpolator(m_el, m_x, m_block->m_scamap);
      m_p = inter(m_block->m_p);
   }
   Scalar ddist = (m_x-m_xold).norm();
   m_xold = m_x;

   EmitData();

   if (m_forward) {
       m_time += m_integrator.h();
       m_dist += ddist;
   } else {
       m_time -= m_integrator.h();
       m_dist -= ddist;
   }

   bool ret = m_integrator.Step();
   ++m_stp;
   return ret;
}

bool Particle::isTracing(bool wait) {

    if (!m_tracing) {
        return false;
    }

    if (!m_progressFuture.valid()) {
        std::cerr << "Particle::isTracing(): future not valid" << std::endl;
        m_tracing = false;
        return false;
    }

    auto status = m_progressFuture.wait_for(std::chrono::milliseconds(wait ? 10 : 0));
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ <= 6)
    if (!status)
        return true;
#else
    if (status != std::future_status::ready)
        return true;
#endif

    m_tracing = false;
    m_progress = m_progressFuture.get();
    return false;
}

bool Particle::madeProgress() const {

    assert(!m_tracing);
    if (m_progress && !m_currentSegment) {
        std::cerr << "inconsistency for particle " << id() << ": made progress, but no line segment" << std::endl;
    }
    return m_progress;
}

bool Particle::trace() {
    assert(m_tracing);

    bool traced = false;
    while(isMoving() && findCell(m_time)) {
        Step();
        traced = true;
    }
    return traced;
}

void Particle::finishSegment() {

    if (m_currentSegment) {
        m_segments[m_currentSegment->m_num] = m_currentSegment;
        m_currentSegment.reset();
    }
}

void Particle::fetchSegments(Particle &other) {

    assert(m_rank == other.m_rank);
    assert(!m_currentSegment);
    assert(!other.m_currentSegment);
    for (auto seg: other.m_segments) {
        m_segments.emplace(seg);
    }
    other.m_segments.clear();
}

inline Scalar interp(Scalar f, const Scalar f0, const Scalar f1) {

    const Scalar EPSILON = 1e-6;
    const Scalar diff = (f1 - f0);
    const Scalar d0 = f - f0;
    if (fabs(diff) < EPSILON) {
        const Scalar d1 = f1 - f;
        return fabs(d0) < fabs(d1) ? 1 : 0;
    }

    return std::min(vistle::Scalar(1), std::max(vistle::Scalar(0), d0 / diff));
}

void Particle::addToOutput() {

   if (m_segments.empty())
       return;

   if (m_global.task_type == MovingPoints) {

       std::lock_guard<std::mutex> locker(m_global.mutex);
       Scalar prevTime(0), nextTime(m_global.dt_step);
       std::shared_ptr<Segment> lastSeg;
       Index lastIdx = 0;
       Index timestep = 0;
       for (auto &ent: m_segments) {
           const auto &seg = *ent.second;
           if (seg.m_num < 0) {
               continue;
           }
           auto N = seg.m_xhist.size();
           for (Index i=0; i<N; ++i) {
               auto time = seg.m_times[i];
               prevTime = time;

               if (time < nextTime) {
                   lastSeg = ent.second;
                   continue;
               }

               while (nextTime < time) {
                   Scalar t = interp(nextTime, prevTime, time);

                   const auto pos1 = seg.m_xhist[i];
                   const auto pos0 = lastSeg ? lastSeg->m_xhist[lastIdx] : pos1;
                   auto points = m_global.points[timestep];
                   Vector pos = lerp(pos0, pos1, t);
                   points->x().push_back(pos[0]);
                   points->y().push_back(pos[1]);
                   points->z().push_back(pos[2]);

                   const auto vel1 = seg.m_vhist[i];
                   const auto vel0 = lastSeg ? lastSeg->m_vhist[lastIdx] : vel1;
                   Vector vel = lerp(vel0, vel1, t);
                   auto vout = m_global.vecField[timestep];
                   vout->x().push_back(vel[0]);
                   vout->y().push_back(vel[1]);
                   vout->z().push_back(vel[2]);

                   const auto pres1 = seg.m_pressures[i];
                   const auto pres0 = lastSeg ? lastSeg->m_pressures[lastIdx] : pres1;
                   Scalar pres = lerp(pres0, pres1, t);
                   m_global.scalField[timestep]->x().push_back(pres);

                   const auto dist1 = seg.m_dists[i];
                   const auto dist0 = lastSeg ? lastSeg->m_dists[lastIdx] : dist1;
                   Scalar dist = lerp(dist0, dist1, t);
                   m_global.distField[timestep]->x().push_back(dist);

                   m_global.stepField[timestep]->x().push_back(seg.m_steps[i]);
                   m_global.timeField[timestep]->x().push_back(nextTime);
                   m_global.idField[timestep]->x().push_back(m_startId);

                   nextTime += m_global.dt_step;
                   ++timestep;
               }
               lastSeg = ent.second;
               lastIdx = i;
           }
       }
   } else {
       Index numPoints = 0;
       for (auto &ent: m_segments) {
           const auto &seg = *ent.second;
           numPoints += seg.m_xhist.size();
       }

       int t = m_timestep;
       t = 0;

       std::lock_guard<std::mutex> locker(m_global.mutex);
       auto lines = m_global.lines[t];
       Index sz = lines->getNumVertices();
       Index nsz = sz+numPoints;
       auto &x = lines->x(), &y = lines->y(), &z = lines->z();
       auto &cl = lines->cl();
       assert(sz == cl.size());
       x.reserve(nsz);
       y.reserve(nsz);
       z.reserve(nsz);
       cl.reserve(nsz);

       auto &vec_x = m_global.vecField[t]->x(), &vec_y = m_global.vecField[t]->y(), &vec_z = m_global.vecField[t]->z();
       auto &scal = m_global.scalField[t]->x();
       auto &step = m_global.stepField[t]->x(), &id = m_global.idField[t]->x();
       auto &time = m_global.timeField[t]->x(), &dist = m_global.distField[t]->x();

       vec_x.reserve(nsz);
       vec_y.reserve(nsz);
       vec_z.reserve(nsz);
       scal.reserve(nsz);
       id.reserve(nsz);
       step.reserve(nsz);
       time.reserve(nsz);
       dist.reserve(nsz);

       auto addStep = [this, &x, &y, &z, &cl, &vec_x, &vec_y, &vec_z, &scal, &id, &step, &time, &dist](const Segment &seg, Index i){
           const auto &vec = seg.m_xhist[i];
           x.push_back(vec[0]);
           y.push_back(vec[1]);
           z.push_back(vec[2]);
           cl.push_back(cl.size());

           const auto &vel = seg.m_vhist[i];
           vec_x.push_back(vel[0]);
           vec_y.push_back(vel[1]);
           vec_z.push_back(vel[2]);
           scal.push_back(seg.m_pressures[i]);
           step.push_back(seg.m_steps[i]);
           time.push_back(seg.m_times[i]);
           dist.push_back(seg.m_dists[i]);
           id.push_back(m_startId);
       };

       for (auto &ent: m_segments) {
           const auto &seg = *ent.second;
           auto N = seg.m_xhist.size();
           if (seg.m_num < 0) {
               for (Index i=N; i>1; --i) {
                   if (i>1 || seg.m_num != -1 || !m_forward) {
                       // avoid duplicate entry for startpoint when tracing in both directions
                       addStep(seg, i-1);
                   }
               }
           } else {
               for (Index i=0; i<N; ++i) {
                   addStep(seg, i);
               }
           }
       }
       lines->el().push_back(cl.size());
   }

   //std::cerr << "Tracer: line for particle " << m_id << " of length " << numPoints << std::endl;

   m_segments.clear();
}

Scalar Particle::time() const {

    return m_time;
}




void Particle::broadcast(boost::mpi::communicator mpi_comm, int root) {

    boost::mpi::broadcast(mpi_comm, *this, root);
    m_progress = false;
}

void Particle::sendData(boost::mpi::communicator mpi_comm) {

    m_currentSegment->m_rank = mpi_comm.rank();
    assert(rank() != mpi_comm.rank());
    mpi_comm.send(rank(), 0, id());
    mpi_comm.send(rank(), 0, *m_currentSegment);
    m_currentSegment.reset();
}

void Particle::receiveData(boost::mpi::communicator mpi_comm, int rank) {

    Index recvId;
    mpi_comm.recv(rank, 0, recvId);
    if (id() != recvId) {
        std::cerr << "MPI rank " << this->rank() << " received invalid id " << recvId << " for particle " << id() << " from rank " << rank << std::endl;
    }
    assert(id() == recvId);
    auto seg = std::make_shared<Segment>();
    mpi_comm.recv(rank, 0, *seg);
    m_segments[seg->m_num] = seg;
    //std::cerr << "Particle " << id() << ": added segment " << seg->m_num << " with " << seg->m_xhist.size() << " points" << std::endl;
}

void Particle::UpdateBlock(BlockData *block) {

    m_block = block;
    m_integrator.UpdateBlock();
}
