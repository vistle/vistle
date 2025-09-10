#include <limits>
#include <algorithm>
#include <boost/mpi/collectives/broadcast.hpp>
#include <boost/serialization/vector.hpp>
#include <vistle/core/vec.h>
#include <vistle/util/math.h>
#include <vistle/util/threadname.h>
#include "Tracer.h"
#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"

using namespace vistle;

template<class S>
Particle<S>::Particle(Index id, int rank, Index startId, const Vector3 &pos, bool forward, GlobalData &global,
                      Index timestep)
: m_global(global)
, m_id(id)
, m_startId(startId)
, m_rank(rank)
, m_timestep(timestep)
, m_progress(false)
, m_tracing(false)
, m_forward(forward)
, m_x(VI(pos))
, m_xold(VI(pos))
, m_v(Vector3(std::numeric_limits<Scalar>::max(), 0, 0))
// keep large enough so that particle moves initially
, m_stp(0)
, m_time(0)
, m_dist(0)
, m_segment(forward ? 0 : -1)
, m_segmentStart(0)
, m_block(nullptr)
, m_el(InvalidIndex)
, m_ingrid(true)
, m_integrator(this, forward)
, m_stopReason(StillActive)
, m_useCelltree(m_global.use_celltree)
{
    m_scalars.resize(global.numScalars);
    m_integrator.enableCelltree(m_useCelltree);

    if (!global.blocks[timestep].empty()) {
        m_time = global.blocks[timestep][0]->m_grid->getRealTime();
    }
}

template<class S>
Particle<S>::~Particle()
{}

template<class S>
Index Particle<S>::id() const
{
    return m_id;
}

template<class S>
int Particle<S>::rank()
{
    return m_rank;
}

template<class S>
StopReason Particle<S>::stopReason() const
{
    return m_stopReason;
}

template<class S>
void Particle<S>::enableCelltree(bool value)
{
    m_useCelltree = value;
    m_integrator.enableCelltree(value);
}

template<class S>
int Particle<S>::searchRank(boost::mpi::communicator mpi_comm)
{
    assert(!m_tracing);
    assert(!m_currentSegment);
    m_progress = false;

    int rank = -1;

    if (findCell(m_time)) {
        m_integrator.hInit();
        rank = mpi_comm.rank();
    }

    rank = boost::mpi::all_reduce(mpi_comm, rank, boost::mpi::maximum<int>());
    if (m_rank == -1) {
        m_rank = rank;
    }

    return rank;
}

template<class S>
void Particle<S>::startTracing()
{
    assert(inGrid());
    m_tracing = true;
    m_progressFuture = std::async(std::launch::async, [this]() -> bool {
        std::string tname =
            std::to_string(m_global.module->id()) + "p" + std::to_string(id()) + ":" + m_global.module->name();
        setThreadName(tname);
        return trace();
    });
}

template<class S>
bool Particle<S>::isActive() const
{
    return m_ingrid && (m_tracing || m_progress || m_block);
}

template<class S>
bool Particle<S>::inGrid() const
{
    return m_ingrid;
}

template<class S>
bool Particle<S>::isMoving()
{
    if (!m_ingrid) {
        return false;
    }

    bool moving = m_v.norm() > m_global.min_vel;
    if ((m_global.trace_len >= 0 && std::abs(m_dist) > m_global.trace_len) ||
        (m_global.trace_time >= 0 && std::abs(m_time) > m_global.trace_time) || (m_stp > m_global.max_step) ||
        !moving) {
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

template<class S>
bool Particle<S>::isForward() const
{
    return m_forward;
}

template<class S>
bool Particle<S>::findCell(double time)
{
    if (!m_ingrid) {
        return false;
    }

    if (m_block) {
        auto grid = m_block->getGrid();
        if (m_global.int_mode == ConstantVelocity) {
            const auto neigh = grid->getNeighborElements(m_el);
            for (auto el: neigh) {
                if (grid->inside(el, VV(m_x))) {
                    m_el = el;
                    assert(m_currentSegment);
                    return true;
                }
            }
        } else {
            m_el = grid->findCell(VV(m_x), m_el, m_useCelltree ? GridInterface::NoFlags : GridInterface::NoCelltree);
        }
        if (m_el != InvalidIndex) {
            assert(m_currentSegment);
            return true;
        }
    }

    for (auto &block: m_global.blocks[m_timestep]) {
        if (block.get() == m_block) {
            // don't try previous block again
            UpdateBlock(nullptr);
            continue;
        }
        auto grid = block->getGrid();
        m_el = grid->findCell(transformPoint(block->invTransform(), VV(m_x)), InvalidIndex,
                              m_useCelltree ? GridInterface::NoFlags : GridInterface::NoCelltree);
        if (m_el != InvalidIndex) {
            if (!m_currentSegment) {
                m_currentSegment = std::make_shared<Segment>();
                m_currentSegment->m_id = id();
                m_currentSegment->m_startStep = m_stp;
                m_currentSegment->m_num = m_segment;
                m_currentSegment->m_blockIndex = block->m_grid->getBlock();
                m_currentSegment->m_scalars.resize(m_global.numScalars);
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

template<class S>
void Particle<S>::EmitData()
{
    m_currentSegment->m_xhist.push_back(transformPoint(m_block->transform(), VV(m_xold)));
    m_currentSegment->m_vhist.push_back(m_block->velocityTransform() * m_v);
    m_currentSegment->m_times.push_back(m_time);
    if (m_global.computeStep)
        m_currentSegment->m_steps.push_back(m_stp);
    assert(m_global.numScalars == m_scalars.size());
    assert(m_currentSegment->m_scalars.size() == m_scalars.size());
    for (unsigned i = 0; i < m_scalars.size(); ++i) {
        m_currentSegment->m_scalars[i].push_back(m_scalars[i]);
    }
    if (m_global.computeStepWidth)
        m_currentSegment->m_stepWidth.push_back(m_integrator.h());
    if (m_global.computeDist)
        m_currentSegment->m_dists.push_back(m_dist);
    if (m_global.computeCellIndex) {
        auto m = m_global.cell_index_modulus;
        if (m > 0) {
            m_currentSegment->m_cellIndex.push_back(m_el % m);
        } else {
            m_currentSegment->m_cellIndex.push_back(m_el);
        }
    }
}

template<class S>
void Particle<S>::Deactivate(StopReason reason)
{
    if (m_stopReason == StillActive)
        m_stopReason = reason;
    UpdateBlock(nullptr);
    m_ingrid = false;
}

template<class S>
bool Particle<S>::Step()
{
    const auto &grid = m_block->getGrid();
    auto inter = grid->getInterpolator(m_el, VV(m_x), m_block->m_vecmap);
    m_v = inter(m_block->m_vx, m_block->m_vy, m_block->m_vz);
    GridInterface::Interpolator otherInter;
    bool haveOtherInter = false;
    for (unsigned i = 0; i < m_block->m_scal.size(); ++i) {
        if (m_block->m_scalmap[i] == m_block->m_vecmap) {
            m_scalars[i] = inter(m_block->m_scal[i]);
        } else {
            if (!haveOtherInter) {
                haveOtherInter = true;
                otherInter = grid->getInterpolator(m_el, VV(m_x), m_block->m_scalmap[i]);
            }
            m_scalars[i] = otherInter(m_block->m_scal[i]);
        }
    }
    Scal ddist = (m_x - m_xold).norm();
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

template<class S>
bool Particle<S>::isTracing(bool wait)
{
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

template<class S>
bool Particle<S>::madeProgress() const
{
    assert(!m_tracing);
    if (m_progress && !m_currentSegment) {
        std::cerr << "inconsistency for particle " << id() << ": made progress, but no line segment" << std::endl;
    }
    return m_progress;
}

template<class S>
bool Particle<S>::trace()
{
    assert(m_tracing);

    bool traced = false;
    while (isMoving() && findCell(m_time)) {
        Step();
        traced = true;
    }
    UpdateBlock(nullptr);
    return traced;
}

template<class S>
void Particle<S>::finishSegment()
{
    if (m_currentSegment) {
        m_currentSegment->simplify(m_global.simplification_error);
        m_segments[m_currentSegment->m_num] = m_currentSegment;
        m_currentSegment.reset();
    }
    UpdateBlock(nullptr);
}

template<class S>
void Particle<S>::fetchSegments(Particle &other)
{
    assert(m_rank == other.m_rank);
    assert(!m_currentSegment);
    assert(!other.m_currentSegment);
    for (auto seg: other.m_segments) {
        m_segments.emplace(seg);
    }
    other.m_segments.clear();
}

inline Scalar interp(Scalar f, const Scalar f0, const Scalar f1)
{
    const Scalar EPSILON = 1e-12;
    const Scalar diff = (f1 - f0);
    const Scalar d0 = f - f0;
    if (fabs(diff) < EPSILON) {
        const Scalar d1 = f1 - f;
        return fabs(d0) < fabs(d1) ? 0 : 1;
    }

    return clamp(d0 / diff, Scalar(0), Scalar(1));
}

template<class S>
void Particle<S>::addToOutput()
{
    const bool second_order = true;

    if (m_segments.empty())
        return;

    if (m_global.task_type == MovingPoints || m_global.task_type == Pathlines) {
        Scalar prevTime(0);
        Scalar time(0);
        Index timestep = 0;
        Index prevIdx = 0;
        std::shared_ptr<const Segment> prevSeg;
        for (auto &ent: m_segments) {
            const auto &seg = *ent.second;
            if (seg.m_num < 0) {
                continue;
            }
            if (!prevSeg)
                prevSeg = ent.second;

            auto N = seg.m_xhist.size();
            //std::cerr << "particle " << seg.m_id << ", N=" << N << " steps" << std::endl;
            for (Index i = 0; i < N; ++i) {
                auto nextTime = seg.m_times[i];

                //std::cerr << "particle " << seg.m_id << " time=" << time << ", prev=" << prevTime << ", next=" << nextTime << std::endl;

                while (time < nextTime) {
                    Scalar t = interp(time, prevTime, nextTime);
                    //std::cerr << "particle " << seg.m_id << " interpolation weight=" << t << std::endl;

                    const auto vel1 = seg.m_vhist[i];
                    const auto vel0 = prevSeg->m_vhist[prevIdx];
                    const auto pos1 = seg.m_xhist[i];
                    const auto pos0 = prevSeg->m_xhist[prevIdx];
                    Vector3 pos;
                    if (second_order) {
                        Scal dt0 = time - prevTime, dt1 = nextTime - time;
                        const Vector3 pos0p = pos0 + dt0 * vel0;
                        const Vector3 pos1p = pos1 - dt1 * vel1;
                        pos = lerp(pos0p, pos1p, t);
                    } else {
                        pos = lerp(pos0, pos1, t);
                    }

                    auto addToArray = [this, timestep](auto &field, auto value) {
                        if (m_global.task_type == MovingPoints) {
                            field->push_back(value);
                        } else if (m_global.task_type == Pathlines) {
                            if (field->size() < (timestep + 1) * m_global.num_particles) {
                                field->resize((timestep + 1) * m_global.num_particles);
                            }
                            auto idx = static_cast<size_t>(timestep) * m_global.num_particles + id();
                            field->at(idx) = value;
                        }
                    };

                    auto addToScalar = [this, timestep](auto &field, auto value) {
                        if (m_global.task_type == MovingPoints) {
                            field->x().push_back(value);
                        } else if (m_global.task_type == Pathlines) {
                            if (field->getSize() < (timestep + 1) * m_global.num_particles) {
                                field->setSize((timestep + 1) * m_global.num_particles);
                            }
                            auto idx = static_cast<size_t>(timestep) * m_global.num_particles + id();
                            field->x()[idx] = value;
                        }
                    };

                    auto addToVector = [this, timestep](auto &field, auto value) {
                        if (m_global.task_type == MovingPoints) {
                            field->x().push_back(value[0]);
                            field->y().push_back(value[1]);
                            field->z().push_back(value[2]);
                        } else if (m_global.task_type == Pathlines) {
                            if (field->getSize() < (timestep + 1) * m_global.num_particles) {
                                field->setSize((timestep + 1) * m_global.num_particles);
                            }
                            auto idx = static_cast<size_t>(timestep) * m_global.num_particles + id();
                            field->x()[idx] = value[0];
                            field->y()[idx] = value[1];
                            field->z()[idx] = value[2];
                        }
                    };

                    std::lock_guard<std::mutex> locker(m_global.mutex);
                    Coords::ptr coords;
                    Points::ptr points;
                    Lines::ptr lines;
                    if (m_global.task_type == MovingPoints) {
                        points = m_global.points[timestep];
                        coords = points;
                    } else {
                        lines = m_global.lines[timestep];
                        coords = lines;
                        for (Index t = 0; t < timestep; ++t) {
                            auto i = static_cast<size_t>(t) * m_global.num_particles + id();
                            lines->cl().push_back(i);
                        }
                        lines->el().push_back(lines->cl().size());
                    }
                    addToVector(coords, pos);

                    if (m_global.computeVector) {
                        Vector3 vel = lerp(vel0, vel1, t);
                        auto vout = m_global.vecField[timestep];
                        addToVector(vout, vel);
                    }

                    if (m_global.numScalars > 0) {
                        std::vector<shm<Scalar>::array *> scalars;
                        scalars.reserve(m_global.numScalars);
                        for (int p = 1; p < Tracer::NumPorts; ++p) {
                            if (!m_global.computeField[p])
                                continue;
                            auto &f = m_global.fields[p][timestep];
                            if (auto vec = Vec<Scalar, 3>::as(f)) {
                                scalars.push_back(&vec->x());
                                scalars.push_back(&vec->y());
                                scalars.push_back(&vec->z());
                            } else if (auto scal = Vec<Scalar>::as(f)) {
                                scalars.push_back(&scal->x());
                            }
                        }
                        assert(scalars.size() == m_global.numScalars);
                        for (int s = 0; s < m_global.numScalars; ++s) {
                            const auto &scalars1 = seg.m_scalars[s];
                            const auto &scalars0 = prevSeg->m_scalars[s];
                            Scalar scal = lerp(scalars0[i], scalars1[i], t);
                            addToArray(scalars[s], scal);
                        }
                    }

                    if (m_global.computeDist) {
                        const auto dist1 = seg.m_dists[i];
                        const auto dist0 = prevSeg->m_dists[prevIdx];
                        Scalar dist = lerp(dist0, dist1, t);
                        addToScalar(m_global.distField[timestep], dist);
                    }

                    if (m_global.computeStep)
                        addToScalar(m_global.stepField[timestep], seg.m_steps[i]);
                    if (m_global.computeTime)
                        addToScalar(m_global.timeField[timestep], time);
                    if (m_global.computeStepWidth)
                        addToScalar(m_global.stepWidthField[timestep], seg.m_stepWidth[i]);
                    if (m_global.computeId)
                        addToScalar(m_global.idField[timestep], m_startId);
                    if (m_global.computeStopReason)
                        addToScalar(m_global.stopReasonField[timestep], m_stopReason);
                    if (m_global.computeCellIndex)
                        addToScalar(m_global.cellField[timestep], seg.m_cellIndex[i]);
                    if (m_global.computeBlockIndex)
                        addToScalar(m_global.blockField[timestep], seg.m_blockIndex);

                    time += m_global.dt_step;
                    ++timestep;
                }
                prevTime = nextTime;
                prevSeg = ent.second;
                prevIdx = i;
            }
        }
        // for pathlines, repeat last full pathline instead of removing stopped particles from visualization
        if (m_global.task_type == Pathlines) {
            auto lastTimestep = timestep;
            for (; timestep < m_global.lines.size(); ++timestep) {
                auto lines = m_global.lines[timestep];
                for (Index t = 0; t < lastTimestep; ++t) {
                    auto i = static_cast<size_t>(t) * m_global.num_particles + id();
                    lines->cl().push_back(i);
                }
                lines->el().push_back(lines->cl().size());
            }
        }
    } else if (m_global.task_type == Streaklines) {
        // TODO
    } else {
        // streamlines
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
        Index nsz = sz + numPoints;
        auto &x = lines->x(), &y = lines->y(), &z = lines->z();
        auto &cl = lines->cl();
        assert(sz == cl.size());
        x.reserve(nsz);
        y.reserve(nsz);
        z.reserve(nsz);
        cl.reserve(nsz);

        shm<Scalar>::array *vec_x = nullptr, *vec_y = nullptr, *vec_z = nullptr;
        if (m_global.computeVector) {
            vec_x = &m_global.vecField[t]->x();
            vec_y = &m_global.vecField[t]->y();
            vec_z = &m_global.vecField[t]->z();
            vec_x->reserve(nsz);
            vec_y->reserve(nsz);
            vec_z->reserve(nsz);
        }

        std::vector<shm<Scalar>::array *> scalars;
        if (m_global.numScalars > 0) {
            scalars.reserve(m_global.numScalars);
            for (int p = 1; p < Tracer::NumPorts; ++p) {
                if (!m_global.computeField[p])
                    continue;
                auto &f = m_global.fields[p][t];
                if (auto vec = Vec<Scalar, 3>::as(f)) {
                    scalars.push_back(&vec->x());
                    scalars.push_back(&vec->y());
                    scalars.push_back(&vec->z());
                } else if (auto scal = Vec<Scalar>::as(f)) {
                    scalars.push_back(&scal->x());
                }
            }
            assert(scalars.size() == m_global.numScalars);
            for (int s = 0; s < m_global.numScalars; ++s) {
                scalars[s]->reserve(nsz);
            }
        }
        shm<Scalar>::array *stepwidth = nullptr;
        if (m_global.computeStepWidth) {
            stepwidth = &m_global.stepWidthField[t]->x();
            stepwidth->reserve(nsz);
        }
        shm<Index>::array *id = nullptr;
        if (m_global.computeId) {
            id = &m_global.idField[t]->x();
            id->reserve(nsz);
        }
        shm<Index>::array *step = nullptr;
        if (m_global.computeStep) {
            step = &m_global.stepField[t]->x();
            step->reserve(nsz);
        }
        shm<Scalar>::array *time = nullptr;
        if (m_global.computeTime) {
            time = &m_global.timeField[t]->x();
            time->reserve(nsz);
        }
        shm<Scalar>::array *dist = nullptr;
        if (m_global.computeDist) {
            auto &dist = m_global.distField[t]->x();
            dist.reserve(nsz);
        }
        shm<Index>::array *stopReason = nullptr;
        if (m_global.computeStopReason) {
            stopReason = &m_global.stopReasonField[t]->x();
            stopReason->reserve(nsz);
        }
        shm<Index>::array *cellIndex = nullptr;
        if (m_global.computeCellIndex) {
            cellIndex = &m_global.cellField[t]->x();
            cellIndex->reserve(nsz);
        }
        shm<Index>::array *blockIndex = nullptr;
        if (m_global.computeBlockIndex) {
            blockIndex = &m_global.blockField[t]->x();
            blockIndex->reserve(nsz);
        }

        auto addStep = [this, &x, &y, &z, &cl, vec_x, vec_y, vec_z, scalars, id, step, stepwidth, time, dist,
                        stopReason, cellIndex, blockIndex](const Segment &seg, Index i) {
            const auto &vec = seg.m_xhist[i];
            x.push_back(vec[0]);
            y.push_back(vec[1]);
            z.push_back(vec[2]);
            cl.push_back(cl.size());

            const auto &vel = seg.m_vhist[i];
            if (vec_x)
                vec_x->push_back(vel[0]);
            if (vec_y)
                vec_y->push_back(vel[1]);
            if (vec_z)
                vec_z->push_back(vel[2]);
            for (int s = 0; s < m_global.numScalars; ++s) {
                scalars[s]->push_back(seg.m_scalars[s][i]);
            }
            if (stepwidth)
                stepwidth->push_back(seg.m_stepWidth[i]);
            if (step)
                step->push_back(seg.m_steps[i]);
            if (time)
                time->push_back(seg.m_times[i]);
            if (dist)
                dist->push_back(seg.m_dists[i]);
            if (id)
                id->push_back(m_startId);
            if (stopReason)
                stopReason->push_back(m_stopReason);
            if (cellIndex)
                cellIndex->push_back(seg.m_cellIndex[i]);
            if (blockIndex)
                blockIndex->push_back(seg.m_blockIndex);
        };

        for (auto &ent: m_segments) {
            const auto &seg = *ent.second;
            auto N = seg.m_xhist.size();
            if (seg.m_num < 0) {
                for (Index i = N - 1; i < N; --i) {
                    if (i != 0 || seg.m_num != -1 || !m_forward) {
                        // avoid duplicate entry for startpoint when tracing in both directions
                        addStep(seg, i);
                    }
                }
            } else {
                for (Index i = 0; i < N; ++i) {
                    addStep(seg, i);
                }
            }
        }
        lines->el().push_back(cl.size());
    }

    //std::cerr << "Tracer: line for particle " << m_id << " of length " << numPoints << std::endl;

    m_segments.clear();
}

template<class S>
Scalar Particle<S>::time() const
{
    return m_time;
}


template<class S>
void Particle<S>::broadcast(boost::mpi::communicator mpi_comm, int root)
{
    boost::mpi::broadcast(mpi_comm, *this, root);
    m_integrator.m_hact = m_integrator.m_h;
    m_progress = false;
}

namespace boost {
namespace serialization {

template<class Archive>
void save(Archive &ar, const SegmentMap &segments, const unsigned int version)
{
    int numsegs = segments.size();
    ar &numsegs;
    for (auto &seg: segments)
        ar &*seg.second;
}

template<class Archive>
void load(Archive &ar, SegmentMap &segments, const unsigned int version)
{
    int numsegs = 0;
    ar &numsegs;
    for (int i = 0; i < numsegs; ++i) {
        Segment seg;
        ar &seg;
        segments.emplace(seg.m_num, std::make_shared<Segment>(std::move(seg)));
    }
}

} // namespace serialization
} // namespace boost

BOOST_SERIALIZATION_SPLIT_FREE(SegmentMap)

template<class S>
void Particle<S>::startSendData(boost::mpi::communicator mpi_comm)
{
    assert(rank() != mpi_comm.rank());
    m_requests.emplace_back(mpi_comm.isend(rank(), id(), m_segments));
}

template<class S>
void Particle<S>::finishSendData()
{
    mpi::wait_all(m_requests.begin(), m_requests.end());
    m_requests.clear();
    m_segments.clear();
}

template<class S>
void Particle<S>::receiveData(boost::mpi::communicator mpi_comm, int rank)
{
    SegmentMap segments;
    mpi_comm.recv(rank, id(), segments);
    for (auto &segpair: segments) {
        auto &seg = segpair.second;
        m_segments[seg->m_num] = seg;
        if (seg->m_id != id()) {
            std::cerr << "Particle " << id() << ": added segment " << seg->m_num << " with " << seg->m_xhist.size()
                      << " points for particle " << seg->m_id << std::endl;
        }
        assert(seg->m_id == id());
    }
}

template<class S>
void Particle<S>::UpdateBlock(BlockData *block)
{
    if (m_block) {
        m_x = VI(transformPoint(m_block->transform(), VV(m_x)));
        m_xold = VI(transformPoint(m_block->transform(), VV(m_xold)));
    }

    m_block = block;

    if (m_block) {
        m_x = VI(transformPoint(m_block->invTransform(), VV(m_x)));
        m_xold = VI(transformPoint(m_block->invTransform(), VV(m_xold)));
    }

    m_integrator.UpdateBlock();
}

template<typename T>
static void skipVector(std::vector<T> &v, const std::vector<Index> &use)
{
    if (use.empty()) {
        v.clear();
        return;
    }

    if (use.back() >= v.size()) {
        assert(v.empty());
        return;
    }

    std::vector<T> vv;
    for (auto i: use) {
        vv.emplace_back(v[i]);
    }
    std::swap(v, vv);
}

Scalar Segment::cosAngle(Index i) const
{
    assert(i > 0);
    assert(i + 1 < m_xhist.size());

    const auto &x0 = m_xhist[i - 1], x1 = m_xhist[i + 1], x = m_xhist[i];
    return (x - x0).normalized().dot((x1 - x).normalized());
}

double Segment::interpolationError(Index i0, Index i1, Index i) const
{
    assert(i0 < i);
    assert(i < i1);

    Scalar eps(1e-10);

    auto x0 = m_xhist[i0], x1 = m_xhist[i1], x = m_xhist[i];

    auto dir = (x1 - x0).normalized();
    auto xi = x0 + dir.dot(x - x0) * dir;

    int maxc = -1;
    dir.maxCoeff(&maxc);
    double t = interpolation_weight<double>(x0[maxc], x1[maxc], xi[maxc]);

    double err = 0.;

    if (m_vhist.size() == m_xhist.size()) {
        auto v0 = m_vhist[i0], v1 = m_vhist[i1], v = m_vhist[i];
        auto vi = lerp(v0, v1, t);
        auto d = v - vi;
        int c = -1;
        d.maxCoeff(&c);
        double e = std::abs(d[c]) > eps ? 1. : 0.;
        if (std::abs(v[c]) > eps)
            e = std::abs(d[c] / v[c]);
        if (e > err)
            err = e;
    }

    for (unsigned k = 0; k < m_scalars.size(); ++k) {
        if (m_scalars[k].size() != m_xhist.size()) {
            continue;
        }
        auto s0 = m_scalars[k][i0], s1 = m_scalars[k][i1], s = m_scalars[k][i];
        auto si = lerp(s0, s1, t);
        auto d = s - si;
        double e = std::abs(si) > eps ? 1. : 0.;
        if (std::abs(s) > eps)
            e = std::abs(d / s);
        if (e > err)
            err = e;
    }

    return err;
}

void Segment::simplify(double relerr)
{
    // no error allowed
    if (relerr <= 0.)
        return;

    Index size = m_xhist.size();
    // nothing to simplify
    if (size < 2)
        return;

    std::vector<Index> use{0}; // all vertices to retain
    Index noKinks = 0;
    for (Index i0 = 0, i1 = 0; i0 < size; i0 = i1) {
        use.push_back(i1);
        // don't skip vertices where there is a kink
        Index step = 1;
        if (noKinks > i0) {
            step = noKinks - i0;
        }
        while (i0 + step < size - 1 && Scalar(1) - cosAngle(i0 + step) < Scalar(relerr * 0.1)) {
            ++step;
            noKinks = i0 + step;
        }

        // skip all vertices where interpolation works well
        i1 = i0 + step;
        assert(i1 <= size);
        while (step > 1) {
            bool canSkip = true;
            for (Index i = i0 + 1; i < i1; ++i) {
                if (interpolationError(i0, i1, i) > relerr) {
                    canSkip = false;
                    break;
                }
            }
            if (canSkip)
                break;
            step = (step + 1) / 2;
            i1 = i0 + step;
            assert(i1 <= size);
        }
        assert(i1 > use.back());
    }
    assert(use.back() <= size - 1);
    if (use.back() != size - 1)
        use.emplace_back(size - 1);

    if (use.size() != size) {
        //std::cerr << "reducing from " << size << " to " << use.size() << std::endl;
        skipVector(m_xhist, use);
        skipVector(m_vhist, use);
        skipVector(m_stepWidth, use);
        for (unsigned i = 0; i < m_scalars.size(); ++i) {
            skipVector(m_scalars[i], use);
        }
        skipVector(m_steps, use);
        skipVector(m_times, use);
        skipVector(m_dists, use);
        skipVector(m_cellIndex, use);
    }
}

template class Particle<float>;
template class Particle<double>;
