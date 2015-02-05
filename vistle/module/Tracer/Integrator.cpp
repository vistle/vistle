#include "Integrator.h"
#include "Tracer.h"
#include <core/unstr.h>
#include <core/vec.h>

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

bool Integrator::Step(){
    switch(m_mode){
    case 0:
        RK32();
    case 1:
        Euler();
    }
}

bool Integrator::Euler(){

    m_ptcl->m_v = Interpolator(m_ptcl->m_block,m_ptcl->m_el, m_ptcl->m_x);
    m_ptcl->m_vhist.push_back(m_ptcl->m_v);
    m_ptcl->m_x = m_ptcl->m_x + m_ptcl->m_v*m_h;
    m_ptcl->m_xhist.push_back(m_ptcl->m_x);
    return true;
}

bool Integrator::RK32(){

            Vector3 k[3];
            Vector3 xtmp;
            Index el=m_ptcl->m_el;
            k[0] = Interpolator(m_ptcl->m_block,m_ptcl->m_el, m_ptcl->m_x);
            m_ptcl->m_v = k[0];
            m_ptcl->m_vhist.push_back(m_ptcl->m_v);
            xtmp = m_ptcl->m_x + m_h*k[0];
            UnstructuredGrid::const_ptr grid = m_ptcl->m_block->getGrid();
            if(!grid->inside(el,xtmp)){
                el = grid->findCell(xtmp);
                if(el==InvalidIndex){
                    m_ptcl->m_x = xtmp;
                    m_ptcl->m_xhist.push_back(m_ptcl->m_x);
                    return false;
                }
            }
            k[1] = Interpolator(m_ptcl->m_block,el, xtmp);
            xtmp = m_ptcl->m_x +m_h*0.25*(k[0]+k[1]);
            if(!grid->inside(el,xtmp)){
                el = grid->findCell(xtmp);
                if(el==InvalidIndex){
                    m_ptcl->m_x = m_ptcl->m_x + m_h*0.5*(k[0]+k[1]);
                    m_ptcl->m_xhist.push_back(m_ptcl->m_x);
                    return false;
                }
            }
            /*Vector3 k[3];
            Vector3 xtmp;
            Index el=m_el;
            k[0] = Interpolator(m_el, m_x);
            m_v = k[0];
            m_vhist.push_back(m_v);
            xtmp = m_x + m_dt*k[0];
            if(!m_block->getGrid()->inside(m_el,xtmp)){
                el = m_block->getGrid()->findCell(xtmp);
                if(el==InvalidIndex){
                    m_x = xtmp;
                    m_xhist.push_back(m_x);
                    return;
                }
            }
            k[1] = Interpolator(el, xtmp);
            xtmp = m_x + m_dt*0.25*(k[0]+k[1]);
            if(!m_block->getGrid()->inside(m_el,xtmp)){
                el = m_block->getGrid()->findCell(xtmp);
                if(el==InvalidIndex){
                    m_x = m_x + m_dt*0.5*(k[0]+k[1]);
                    m_xhist.push_back(m_x);
                    return;
                }
            }
            k[2] = Interpolator(el,xtmp);
            m_x = m_x + m_dt*(k[0]/6.0 + k[1]/6.0 + 2*k[2]/3.0);
            m_xhist.push_back(m_x);*/
            k[2] = Interpolator(m_ptcl->m_block,el,xtmp);
            m_ptcl->m_x = m_ptcl->m_x + m_h*(k[0]/6.0 + k[1]/6.0 + 2*k[2]/3.0);
            m_ptcl->m_xhist.push_back(m_ptcl->m_x);
            return true;
}

Vector3 Integrator::Interpolator(BlockData* bl, Index el,const Vector3 &point){

    UnstructuredGrid::const_ptr grid = bl->getGrid();
    Vec<Scalar, 3>::const_ptr vecfld = bl->getVecFld();
    Vec<Scalar>::const_ptr scfield = bl->getScalFld();
    UnstructuredGrid::Interpolator interpolator = grid->getInterpolator(el, point);
    Scalar* u = vecfld->x().data();
    Scalar* v = vecfld->y().data();
    Scalar* w = vecfld->z().data();
    return interpolator(u,v,w);
}
