#include <limits>
#include <algorithm>
#include <boost/mpi/collectives/broadcast.hpp>
#include <core/vec.h>
#include "Tracer.h"
#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"

using namespace vistle;

Particle::Particle(Index id, const Vector3 &pos, bool forward, GlobalData &global, Index timestep):
m_global(global),
m_id(id),
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
m_block(nullptr),
m_el(InvalidIndex),
m_ingrid(true),
m_integrator(this, forward),
m_stopReason(StillActive),
m_useCelltree(m_global.use_celltree)
{
   m_integrator.enableCelltree(m_useCelltree);
   if (findCell()) {
      m_integrator.hInit();
   }
}

Particle::~Particle(){}

Index Particle::id() const {
    return m_id;
}

Particle::StopReason Particle::stopReason() const {
    return m_stopReason;
}

void Particle::enableCelltree(bool value) {

    m_useCelltree = value;
    m_integrator.enableCelltree(value);
}

bool Particle::startTracing() {
    assert(!m_tracing);
    m_progress = false;
    if (inGrid()) {
        m_tracing = true;
        m_progressFuture =  std::async(std::launch::async, [this]() -> bool { return trace(); });
        return true;
    }
    return false;
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
    if(std::abs(m_dist) > m_global.trace_len || m_stp > m_global.max_step || !moving){

       PointsToLines();
       if (std::abs(m_dist) > m_global.trace_len)
           this->Deactivate(DistanceLimitReached);
       else if (m_stp > m_global.max_step)
           this->Deactivate(StepLimitReached);
       else
           this->Deactivate(NotMoving);
       return false;
    }

    return true;
}

bool Particle::findCell() {

    if (!m_ingrid) {
        return false;
    }

    bool repeatDataFromLastBlock = false;

    if (m_block) {

        auto grid = m_block->getGrid();
        if (m_global.int_mode == ConstantVelocity) {
            const auto neigh = grid->getNeighborElements(m_el);
            for (auto el: neigh) {
                if(grid->inside(el, m_x)) {
                    m_el = el;
                    return true;
                }
            }
        } else {
            if(grid->inside(m_el, m_x)){
                return true;
            }
        }
        m_el = grid->findCell(m_x, m_useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (m_el==InvalidIndex) {
            PointsToLines();
            repeatDataFromLastBlock = true;
        } else {
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
        m_el = grid->findCell(m_x, m_useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (m_el!=InvalidIndex) {

            UpdateBlock(block.get());
            if (repeatDataFromLastBlock)
                EmitData();
            return true;
        }
    }

    UpdateBlock(nullptr);
    return false;
}

void Particle::PointsToLines(){

    if (m_block)
        m_block->addLines(m_id,m_xhist,m_vhist,m_pressures,m_steps, m_times, m_dists);

    m_xhist.clear();
    m_vhist.clear();
    m_pressures.clear();
    m_steps.clear();
    m_times.clear();
    m_dists.clear();
}

void Particle::EmitData() {

   m_xhist.push_back(m_xold);
   m_vhist.push_back(m_v);
   m_steps.push_back(m_stp);
   m_pressures.push_back(m_p); // will be ignored later on
   m_times.push_back(m_time);
   m_dists.push_back(m_dist);
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
    return m_progress;
}

bool Particle::trace() {
    assert(m_tracing);

    bool traced = false;
    while(isMoving() && findCell()) {
        if (!traced && m_stp>0) {
            // repeat data from previous block for particle that was just received from remote node
            EmitData();
        }
        Step();
        traced = true;
    }
    return traced;
}




void Particle::broadcast(boost::mpi::communicator mpi_comm, int root) {

    boost::mpi::broadcast(mpi_comm, *this, root);
    m_progress = false;
}

void Particle::UpdateBlock(BlockData *block) {

    m_block = block;
    m_integrator.UpdateBlock();
}
