#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"
#include "Tracer.h"
#include <core/vec.h>


using namespace vistle;



Integrator::Integrator(vistle::Scalar h, vistle::Scalar hmin,
           vistle::Scalar hmax, vistle::Scalar errtol,
           IntegrationMethod mode, Particle* ptcl, bool forward):
    m_h(h),
    m_hact(m_h),
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
    case ConstantVelocity:
        return StepConstantVelocity();
    }
    return false;
}

bool Integrator::StepEuler() {

    Scalar unit = 1.;
    if (m_ptcl->m_global.cell_relative) {
        Index el=m_ptcl->m_el;
        auto grid = m_ptcl->m_block->getGrid();
        Scalar cellSize = grid->cellDiameter(el);
        unit = cellSize;
    }

    Vector vel = m_forward ? m_ptcl->m_v : -m_ptcl->m_v;
    Scalar v = std::max(vel.norm(), Scalar(1e-7));
    unit /= v;
    m_ptcl->m_x = m_ptcl->m_x + vel*m_h*unit;
    m_hact = m_h*unit;
    return true;
}

void Integrator::hInit(){
    if (m_mode != RK32)
        return;

    Index el=m_ptcl->m_el;
    auto grid = m_ptcl->m_block->getGrid();
    Scalar unit = 1.;
    if (m_ptcl->m_global.cell_relative) {
        Scalar cellSize = grid->cellDiameter(el);
        unit = cellSize;
    }

    Vector3 vel = Interpolator(m_ptcl->m_block, m_ptcl->m_el, m_ptcl->m_x);
    Scalar v = std::max(vel.norm(), Scalar(1e-7));
    unit /= v;

    m_h = .5 * unit * std::sqrt(m_hmin*m_hmax);
    m_hact = m_h;
}

bool Integrator::hNew(Vector3 higher, Vector3 lower, Vector vel, Scalar unit){

   Scalar errest = (higher-lower).lpNorm<Eigen::Infinity>();

   if (!m_ptcl->m_global.cell_relative) {
       unit = 1.;
   }
   Scalar v = std::max(vel.norm(), Scalar(1e-7));
   unit /= v;

   if (!std::isfinite(errest)) {
       m_h = m_hmin*unit;
       std::cerr << "Integrator: invalid input for error estimation: higher=" << higher.transpose() << ", lower=" << lower.transpose() <<  std::endl;
       return true;
   }

   Scalar h = 0.9*m_h*std::pow(Scalar(m_errtol/errest),Scalar(1.0/3.0));
   h = std::min(h, 2*m_h);
   //Scalar h = 0.9*m_h*std::pow(Scalar(m_errtol/errest),Scalar(1.0/3.0))*unit;
   if(errest<=m_errtol) {
      if(h<m_hmin*unit) {
         m_h=m_hmin*unit;
         return true;
      } else if(h>m_hmax*unit) {
         m_h=m_hmax*unit;
         return true;
      } else {
         m_h = h;
         return true;
      }
   } else {
      if (h<m_hmin*unit) {
         m_h = m_hmin*unit;
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

Scalar Integrator::h() const {

    return m_hact;
}

// 3rd-order Runge-Kutta with embedded Heun
bool Integrator::StepRK32() {

   Scalar sign = m_forward ? 1. : -1.;
   Vector3 k[3];
   k[0] = sign*m_ptcl->m_v;
   auto grid = m_ptcl->m_block->getGrid();
   Scalar cellSize = grid->cellDiameter(m_ptcl->m_el);

   for (;;) {
      Index el=m_ptcl->m_el;

      const Vector x1 = m_ptcl->m_x + 0.5*m_h*k[0];
      if(!grid->inside(el,x1)){
         el = grid->findCell(x1, m_cellSearchFlags);
         if(el==InvalidIndex){
            m_ptcl->m_x = x1;
            m_hact = 0.5*m_h;
            return false;
         }
         cellSize = std::min(grid->cellDiameter(el), cellSize);
      }

      k[1] = sign*Interpolator(m_ptcl->m_block,el, x1);
      Vector3 x2nd = m_ptcl->m_x + m_h*(k[0]*0.5 + k[1]*0.5);

      const Vector x2 = m_ptcl->m_x +m_h*(-k[0]+2*k[1]);
      if(!grid->inside(el,x2)){
         el = grid->findCell(x2, m_cellSearchFlags);
         if(el==InvalidIndex){
            m_ptcl->m_x = x2nd;
            m_hact = m_h;
            return false;
         }
         cellSize = std::min(grid->cellDiameter(el), cellSize);
      }

      k[2] = sign*Interpolator(m_ptcl->m_block,el,x2);
      Vector3 x3rd = m_ptcl->m_x + m_h*(k[0]/6.0 + 2*k[1]/3.0 + k[2]/6.0);
      m_hact = m_h;

      bool accept = hNew(x3rd, x2nd, k[0], cellSize);
      if (accept) {
          m_ptcl->m_x = x3rd;
          return true;
      }
   }
}

bool Integrator::StepConstantVelocity() {

    Index el=m_ptcl->m_el;
    auto grid = m_ptcl->m_block->getGrid();

    Vector vel = m_forward ? m_ptcl->m_v : -m_ptcl->m_v;
    Scalar t = grid->exitDistance(el, m_ptcl->m_x, vel);
    if (t < 0)
        return false;

    m_ptcl->m_x = m_ptcl->m_x + vel.normalized()*t;
    Scalar v = vel.norm();
    m_hact = m_h = t/v;

    return true;
}

Vector3 Integrator::Interpolator(BlockData* bl, Index el,const Vector3 &point){
    auto grid = bl->getGrid();
    GridInterface::Interpolator interpolator = grid->getInterpolator(el, point, bl->getVecMapping());
    return interpolator(m_v[0], m_v[1], m_v[2]);
}
