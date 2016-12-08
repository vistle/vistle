#include <limits>
#include <algorithm>
#include <boost/mpi/collectives/broadcast.hpp>
#include <core/vec.h>
#include "Tracer.h"
#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"

#ifdef TIMING
#include "TracerTimes.h"
#endif

using namespace vistle;

Particle::Particle(Index i, const Vector3 &pos, Scalar h, Scalar hmin,
      Scalar hmax, Scalar errtol, IntegrationMethod int_mode,const std::vector<std::unique_ptr<BlockData>> &bl,
      Index stepsmax, bool forward):
m_progress(false),
m_tracing(false),
m_forward(forward),
m_id(i),
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
m_integrator(h,hmin,hmax,errtol,int_mode,this, forward),
m_stopReason(StillActive),
m_useCelltree(true)
{
   m_integrator.enableCelltree(m_useCelltree);
   if (findCell(bl)) {
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

bool Particle::startTracing(std::vector<std::unique_ptr<BlockData> > &blocks, Index maxSteps, double traceLen, double minSpeed) {
    assert(!m_tracing);
    m_progress = false;
    if (inGrid()) {
        m_tracing = true;
        m_progressFuture =  std::async(std::launch::async, [this, &blocks, maxSteps, traceLen, minSpeed]() -> bool { return trace(blocks, maxSteps, traceLen, minSpeed); });
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

bool Particle::isMoving(Index maxSteps, Scalar traceLen, Scalar minSpeed) {

    if (!m_ingrid) {
       return false;
    }

    bool moving = m_v.norm() > minSpeed;
    if(std::abs(m_dist) > traceLen || m_stp > maxSteps || !moving){

       PointsToLines();
       if (std::abs(m_dist) > traceLen)
           this->Deactivate(DistanceLimitReached);
       else if (m_stp > maxSteps)
           this->Deactivate(StepLimitReached);
       else
           this->Deactivate(NotMoving);
       return false;
    }

    return true;
}

bool Particle::findCell(const std::vector<std::unique_ptr<BlockData>> &block) {

    if (!m_ingrid) {
        return false;
    }

    bool repeatDataFromLastBlock = false;

    if (m_block) {

        auto grid = m_block->getGrid();
        if(grid->inside(m_el, m_x)){
            return true;
        }
#ifdef TIMING
        times::celloc_start = times::start();
#endif
        m_el = grid->findCell(m_x, m_useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
#ifdef TIMING
        times::celloc_dur += times::stop(times::celloc_start);
#endif
        if (m_el==InvalidIndex) {
            PointsToLines();
            repeatDataFromLastBlock = true;
        } else {
            return true;
        }
    }

    for(Index i=0; i<block.size(); i++) {

        if (block[i].get() == m_block) {
            // don't try previous block again
            m_block = nullptr;
            continue;
        }
#ifdef TIMING
        times::celloc_start = times::start();
#endif
        auto grid = block[i]->getGrid();
        m_el = grid->findCell(m_x, m_useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
#ifdef TIMING
        times::celloc_dur += times::stop(times::celloc_start);
#endif
        if (m_el!=InvalidIndex) {

            UpdateBlock(block[i].get());
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

bool Particle::trace(std::vector<std::unique_ptr<BlockData>> &blocks, vistle::Index steps_max, double trace_len, double minspeed) {
    assert(m_tracing);

    bool traced = false;
    while(isMoving(steps_max, trace_len, minspeed) && findCell(blocks)) {
        if (!traced && m_stp>0) {
            // repeat data from previous block for particle that was just received from remote node
            EmitData();
        }
#ifdef TIMING
        double celloc_old = times::celloc_dur;
        double integr_old = times::integr_dur;
        times::integr_start = times::start();
#endif
        Step();
#ifdef TIMING
        times::integr_dur += times::stop(times::integr_start)-(times::celloc_dur-celloc_old)-(times::integr_dur-integr_old);
#endif
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
