#include "Integrator.h"
#include "Tracer.h"
#include "Particle.h"
#include <core/unstr.h>
#include <core/vec.h>

#include "BlockData.h"

#ifdef TIMING
#include "TracerTimes.h"
#endif

using namespace vistle;



Integrator::Integrator(vistle::Scalar h, vistle::Scalar hmin,
           vistle::Scalar hmax, vistle::Scalar errtol,
           IntegrationMethod mode, Particle* ptcl, bool forward):
    m_h(h),
    m_hmin(hmin),
    m_hmax(hmax),
    m_errtol(errtol),
    m_mode(mode),
    m_ptcl(ptcl),
    m_forward(forward),
    m_cellSearchFlags(GridInterface::NoFlags)
{
    UpdateBlock();
}

void Integrator::UpdateBlock() {

    if (BlockData *bl = m_ptcl->m_block) {
        Vec<Scalar, 3>::const_ptr vecfld = bl->getVecFld();
        for (int i=0; i<3; ++i)
            m_v[i] = &vecfld->x(i)[0];
    }
    else {
        m_v[0] = m_v[1] = m_v[2] = nullptr;
    }
}

bool Integrator::Step() {

    switch(m_mode){
    case Euler:
        return StepEuler();
    case RK32:
        return StepRK32();
    }
    return false;
}

bool Integrator::StepEuler() {

    if (m_forward)
        m_ptcl->m_x = m_ptcl->m_x + m_ptcl->m_v*m_h;
    else
        m_ptcl->m_x = m_ptcl->m_x - m_ptcl->m_v*m_h;
    return true;
}

void Integrator::hInit(){
    if (m_mode == Euler)
        return;

    Vector3 v = Interpolator(m_ptcl->m_block, m_ptcl->m_el, m_ptcl->m_x);
    Vector3::Index dimmax = 0, dummy;
    v.maxCoeff(&dimmax, &dummy);
    Scalar vmax = std::abs(v(dimmax));
    auto grid = m_ptcl->m_block->getGrid();
    const auto bounds = grid->cellBounds(m_ptcl->m_el);
    const auto &min = bounds.first, &max = bounds.second;

    Scalar chlen = max[dimmax]-min[dimmax];
    m_h =0.5*chlen/vmax;
    if(m_h>m_hmax) {m_h = m_hmax;}
    if(m_h<m_hmin) {m_h = m_hmin;}
}

bool Integrator::hNew(Vector3 higher, Vector3 lower){

   Scalar xerr = std::abs(higher(0)-lower(0));
   Scalar yerr = std::abs(higher(1)-lower(1));
   Scalar zerr = std::abs(higher(2)-lower(2));
   Scalar errest = std::max(std::max(xerr,yerr),zerr);

   Scalar h = 0.9*m_h*std::pow(Scalar(m_errtol/errest),Scalar(1.0/3.0));
   if(errest<=m_errtol) {
      if(h<m_hmin) {
         m_h=m_hmin;
         return true;
      } else if(h>m_hmax) {
         m_h=m_hmax;
         return true;
      } else {
         m_h = h;
         return true;
      }
   } else {
      if (h<m_hmin) {
         m_h = m_hmin;
         return true;
      } else {
         m_h = h;
         return false;
      }
   }
}

void Integrator::enableCelltree(bool value) {

    if (value) {
        m_cellSearchFlags = GridInterface::NoFlags;
    } else {

        m_cellSearchFlags = GridInterface::NoCelltree;
    }
}

bool Integrator::StepRK32() {

   bool accept = false;
   Index el=m_ptcl->m_el;
   Vector3 x3rd;
   Vector3 k[3];
   Scalar sign = m_forward ? 1. : -1.;
   k[0] = sign*m_ptcl->m_v;
   Vector xtmp = m_ptcl->m_x + m_h*k[0];
   auto grid = m_ptcl->m_block->getGrid();
   do {
      if(!grid->inside(el,xtmp)){
#ifdef TIMING
         times::celloc_start = times::start();
#endif
         el = grid->findCell(xtmp);
#ifdef TIMING
         times::celloc_dur += times::stop(times::celloc_start);
#endif
         if(el==InvalidIndex){
            m_ptcl->m_x = xtmp;
            return false;
         }
      }
      k[1] = sign*Interpolator(m_ptcl->m_block,el, xtmp);
      xtmp = m_ptcl->m_x +m_h*0.25*(k[0]+k[1]);
      if(!grid->inside(el,xtmp)){
#ifdef TIMING
         times::celloc_start = times::start();
#endif
         el = grid->findCell(xtmp);
#ifdef TIMING
         times::celloc_dur += times::stop(times::celloc_start);
#endif
         if(el==InvalidIndex){
            m_ptcl->m_x = m_ptcl->m_x + m_h*0.5*(k[0]+k[1]);
            return false;
         }
      }
      k[2] = sign*Interpolator(m_ptcl->m_block,el,xtmp);
      Vector3 x2nd = m_ptcl->m_x + m_h*(k[0]*0.5 + k[1]*0.5);
      x3rd = m_ptcl->m_x + m_h*(k[0]/6.0 + k[1]/6.0 + 2*k[2]/3.0);

      accept = hNew(x3rd,x2nd);
      if(!accept){
         el = m_ptcl->m_el;
         xtmp = m_ptcl->m_x + m_h*k[0];
      }
   } while(!accept);
   m_ptcl->m_x = x3rd;
   return true;
}

Vector3 Integrator::Interpolator(BlockData* bl, Index el,const Vector3 &point){
#ifdef TIMING
times::interp_start = times::start();
#endif
    auto grid = bl->getGrid();
    GridInterface::Interpolator interpolator = grid->getInterpolator(el, point, bl->getVecMapping());
#ifdef TIMING
times::interp_dur += times::stop(times::interp_start);
times::no_interp++;
#endif
    return interpolator(m_v[0], m_v[1], m_v[2]);
}
