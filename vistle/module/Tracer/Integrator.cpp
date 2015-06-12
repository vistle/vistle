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
    Vector3 v = Interpolator(m_ptcl->m_block, m_ptcl->m_el, m_ptcl->m_x);
    unsigned char dimmax=0;
    if(std::abs(v(1))>std::abs(v(dimmax))){dimmax=1;}
    if(std::abs(v(2))>std::abs(v(dimmax))){dimmax=2;}
    Scalar vmax = std::abs(v(dimmax));
    UnstructuredGrid::const_ptr grid = m_ptcl->m_block->getGrid();
    Index numvert = grid->el()[m_ptcl->m_el+1] - grid->el()[m_ptcl->m_el];
    Scalar dmin,dmax;
    switch(dimmax){
    case 0:
        dmin = grid->x()[grid->cl()[grid->el()[m_ptcl->m_el]]];
        dmax = dmin;
        for(Index i=1; i<numvert; i++){
            dmin = std::min<Scalar>(dmin,grid->x()[grid->cl()[grid->el()[m_ptcl->m_el]+i]]);
            dmax = std::max<Scalar>(dmax,grid->x()[grid->cl()[grid->el()[m_ptcl->m_el]+i]]);
        }
        break;
    case 1:
        dmin = grid->y()[grid->cl()[grid->el()[m_ptcl->m_el]]];
        dmax = dmin;
        for(Index i=1; i<numvert; i++){
            dmin = std::min<Scalar>(dmin,grid->y()[grid->cl()[grid->el()[m_ptcl->m_el]+i]]);
            dmax = std::max<Scalar>(dmax,grid->y()[grid->cl()[grid->el()[m_ptcl->m_el]+i]]);
        }
        break;
    case 2:
        dmin = grid->z()[grid->cl()[grid->el()[m_ptcl->m_el]]];
        dmax = dmin;
        for(Index i=1; i<numvert; i++){
            dmin = std::min<Scalar>(dmin,grid->z()[grid->cl()[grid->el()[m_ptcl->m_el]+i]]);
            dmax = std::max<Scalar>(dmax,grid->z()[grid->cl()[grid->el()[m_ptcl->m_el]+i]]);
        }
    }
    Scalar chlen = dmax-dmin;
    m_h =0.5*chlen/vmax;
    if(m_h>m_hmax){m_h = m_hmax;}
    if(m_h<m_hmin){m_h = m_hmin;}
}

bool Integrator::Step(Index step){

   const auto &block = m_ptcl->m_block;
   const auto &grid = block->getGrid();
   const auto &el = m_ptcl->m_el;
   const auto &point = m_ptcl->m_x;
   const auto inter = grid->getInterpolator(el, point);
   m_ptcl->m_xhist.push_back(point);
   m_ptcl->m_v = inter(block->m_vx, block->m_vy, block->m_vz);
   m_ptcl->m_vhist.push_back(m_ptcl->m_v);
   m_ptcl->m_steps.push_back(step);
   if (block->m_p) {
      m_ptcl->m_pressures.push_back(inter(block->m_p));
   }

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
    if(errest<=m_errtol){
        if(h<m_hmin){
            m_h=m_hmin;
            return true;}
        else if(h>m_hmax){
            m_h=m_hmax;
            return true;}
        m_h = h;
        return true;
    }
    else{
        if(h<m_hmin){
            m_h = m_hmin;
            return true;}
        m_h = h;
        return false;
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
   k[0] = Interpolator(m_ptcl->m_block,m_ptcl->m_el, m_ptcl->m_x);
   m_ptcl->m_v = k[0];
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
