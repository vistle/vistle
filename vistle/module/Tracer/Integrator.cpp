#include "Integrator.h"
#include "Tracer.h"
#include <core/unstr.h>
#include <core/vec.h>
#include "TracerTimes.h"

using namespace vistle;



Integrator::Integrator(vistle::Scalar h, vistle::Scalar hmin,
           vistle::Scalar hmax, vistle::Scalar errtol,
           int int_mode, Particle* ptcl):
    m_h(h),
    m_hmin(hmin),
    m_hmax(hmax),
    m_errtol(errtol),
    m_mode(int_mode),
    m_ptcl(ptcl){
}

void Integrator::hInit(){
    if (m_mode != 0)
        return;

    Vector3 v = Interpolator(m_ptcl->m_block, m_ptcl->m_el, m_ptcl->m_x);
    Vector3::Index dimmax = 0, dummy;
    v.maxCoeff(&dimmax, &dummy);
    Scalar vmax = std::abs(v(dimmax));
    UnstructuredGrid::const_ptr grid = m_ptcl->m_block->getGrid();
    Index end = grid->el()[m_ptcl->m_el+1];
    Index begin = grid->el()[m_ptcl->m_el];
    Scalar dmin = std::numeric_limits<Scalar>::max(), dmax = -std::numeric_limits<Scalar>::max();
    for(Index i=begin; i<end; i++){
       const Scalar s = grid->x(dimmax)[grid->cl()[i]];
       dmin = std::min<Scalar>(dmin,s);
       dmax = std::max<Scalar>(dmax,s);
    }

    Scalar chlen = dmax-dmin;
    m_h =0.5*chlen/vmax;
    if(m_h>m_hmax) {m_h = m_hmax;}
    if(m_h<m_hmin) {m_h = m_hmin;}
}

bool Integrator::Step() {

    switch(m_mode){
    case 0:
        return RK32();
    case 1:
        return Euler();
    }
    return false;
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

bool Integrator::Euler() {

   m_ptcl->m_x = m_ptcl->m_x + m_ptcl->m_v*m_h;
   return true;
}

bool Integrator::RK32() {

   bool accept = false;
   Index el=m_ptcl->m_el;
   Vector3 x3rd;
   Vector3 k[3];
   k[0] = m_ptcl->m_v;
   Vector xtmp = m_ptcl->m_x + m_h*k[0];
   UnstructuredGrid::const_ptr grid = m_ptcl->m_block->getGrid();
   do {
      if(!grid->inside(el,xtmp)){
         times::celloc_start = times::start();
         el = grid->findCell(xtmp);
         times::celloc_dur += times::stop(times::celloc_start);
         if(el==InvalidIndex){
            m_ptcl->m_x = xtmp;
            return false;
         }
      }
      k[1] = Interpolator(m_ptcl->m_block,el, xtmp);
      xtmp = m_ptcl->m_x +m_h*0.25*(k[0]+k[1]);
      if(!grid->inside(el,xtmp)){
         times::celloc_start = times::start();
         el = grid->findCell(xtmp);
         times::celloc_dur += times::stop(times::celloc_start);
         if(el==InvalidIndex){
            m_ptcl->m_x = m_ptcl->m_x + m_h*0.5*(k[0]+k[1]);
            return false;
         }
      }
      k[2] = Interpolator(m_ptcl->m_block,el,xtmp);
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
times::interp_start = times::start();
    UnstructuredGrid::const_ptr grid = bl->getGrid();
    Vec<Scalar, 3>::const_ptr vecfld = bl->getVecFld();
    Vec<Scalar>::const_ptr scfield = bl->getScalFld();
    UnstructuredGrid::Interpolator interpolator = grid->getInterpolator(el, point);
    Scalar* u = vecfld->x().data();
    Scalar* v = vecfld->y().data();
    Scalar* w = vecfld->z().data();
times::interp_dur += times::stop(times::interp_start);
times::no_interp++;
    return interpolator(u,v,w);
}
