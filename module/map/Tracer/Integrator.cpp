#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"
#include "Tracer.h"
#include <vistle/core/vec.h>


using namespace vistle;


template<typename S>
Integrator<S>::Integrator(Particle<S> *ptcl, bool forward)
: m_h(-1.), m_hact(m_h), m_ptcl(ptcl), m_forward(forward), m_cellSearchFlags(GridInterface::NoFlags)
{
    UpdateBlock();
}

template<typename S>
void Integrator<S>::hInit()
{
    if (m_h > 0.)
        return;

    const auto &global = m_ptcl->m_global;
    const auto mode = global.int_mode;
    if (mode != RK32) {
        m_h = global.h_init;
        return;
    }

    const auto h_min = global.h_min;

    Index el = m_ptcl->m_el;
    auto grid = m_ptcl->m_block->getGrid();
    Scalar unit = 1.;
    if (m_ptcl->m_global.cell_relative) {
        Scalar cellSize = grid->cellDiameter(el);
        unit = cellSize;
    }

    if (m_ptcl->m_global.velocity_relative) {
        Vect3 vel = VI(Interpolator(m_ptcl->m_block, m_ptcl->m_el, VV(m_ptcl->m_x)));
        Scal v = std::max(vel.norm(), Scal(1e-7));
        unit /= v;
    }

    m_h = unit * h_min;
    m_hact = m_h;
}


template<typename S>
void Integrator<S>::UpdateBlock()
{
    if (BlockData *bl = m_ptcl->m_block) {
        Vec<Scalar, 3>::const_ptr vecfld = bl->getVecFld();
        for (int i = 0; i < 3; ++i)
            m_v[i] = &vecfld->x(i)[0];
    } else {
        m_v[0] = m_v[1] = m_v[2] = nullptr;
    }
}

template<typename S>
bool Integrator<S>::Step()
{
    switch (m_ptcl->m_global.int_mode) {
    case Euler:
        return StepEuler();
    case RK32:
        return StepRK32();
    case ConstantVelocity:
        return StepConstantVelocity();
    }
    return false;
}

template<typename S>
bool Integrator<S>::StepEuler()
{
    Scalar unit = 1.;
    if (m_ptcl->m_global.cell_relative) {
        Index el = m_ptcl->m_el;
        auto grid = m_ptcl->m_block->getGrid();
        Scalar cellSize = grid->cellDiameter(el);
        unit = cellSize;
    }

    Vect3 vel = VI(m_forward ? m_ptcl->m_v : -m_ptcl->m_v);
    Scal v = std::max(vel.norm(), Scal(1e-7));
    if (m_ptcl->m_global.velocity_relative)
        unit /= v;
    m_ptcl->m_x = m_ptcl->m_x + vel * m_h * unit;
    m_hact = m_h * unit;
    return true;
}

template<typename S>
bool Integrator<S>::hNew(Vect3 cur, Vect3 higher, Vect3 lower, Vect3 vel, Scalar unit)
{
    const auto &global = m_ptcl->m_global;
    const auto h_max = global.h_max;
    const auto h_min = global.h_min;
    const auto tol_rel = global.errtolrel;
    const auto tol_abs = global.errtolabs;

    Scal errestabs = (higher - lower).template lpNorm<Eigen::Infinity>();
    Scal errestrel = errestabs / (higher - cur).template lpNorm<Eigen::Infinity>();

    if (!global.cell_relative) {
        unit = 1.;
    }
    if (global.velocity_relative) {
        Scal v = std::max(vel.norm(), Scal(1e-7));
        unit /= v;
    }

    if (!std::isfinite(errestabs) || !std::isfinite(errestrel)) {
        m_h = h_min * unit;
        std::cerr << "Integrator: invalid input for error estimation: higher=" << higher.transpose()
                  << ", lower=" << lower.transpose();
        if (std::isfinite(errestabs))
            std::cerr << ", cur=" << cur.transpose();
        return true;
    }

    Scal h_abs = 0.9 * m_h * std::pow(Scal(tol_abs / errestabs), Scal(1.0 / 3.0));
    Scal h_rel = 0.9 * m_h * std::pow(Scal(tol_rel / errestrel), Scal(1.0 / 3.0));
    Scal h = std::min(std::max(h_abs, h_rel), 2 * m_h);
    //Scalar h = 0.9*m_h*std::pow(Scalar(m_errtol/errest),Scalar(1.0/3.0))*unit;
    if (errestabs <= tol_abs || errestrel <= tol_rel) {
        if (h < h_min * unit) {
            m_h = h_min * unit;
            return true;
        } else if (h > h_max * unit) {
            m_h = h_max * unit;
            return true;
        } else {
            m_h = h;
            return true;
        }
    } else {
        if (h < h_min * unit) {
            m_h = h_min * unit;
            return true;
        } else {
            m_h = h;
            return false;
        }
    }
}

template<typename S>
void Integrator<S>::enableCelltree(bool value)
{
    if (value) {
        m_cellSearchFlags = GridInterface::NoFlags;
    } else {
        m_cellSearchFlags = GridInterface::NoCelltree;
    }
}

template<typename S>
typename Integrator<S>::Scal Integrator<S>::h() const
{
    return m_hact;
}

// 3rd-order Runge-Kutta with embedded Heun
template<typename S>
bool Integrator<S>::StepRK32()
{
    Scalar sign = m_forward ? 1. : -1.;
    Vect3 k[3];
    k[0] = sign * VI(m_ptcl->m_v);
    auto grid = m_ptcl->m_block->getGrid();
    Scalar cellSize = grid->cellDiameter(m_ptcl->m_el);
    const Scalar third(1. / 3.);

    for (;;) {
        const Vect3 x1 = m_ptcl->m_x + 0.5 * m_h * k[0];
        Index el1 = grid->findCell(VV(x1), m_ptcl->m_el, m_cellSearchFlags);
        if (el1 == InvalidIndex) {
            m_ptcl->m_x = x1;
            m_hact = 0.5 * m_h;
            return false;
        }
        if (el1 != m_ptcl->m_el) {
            cellSize = std::min(grid->cellDiameter(el1), cellSize);
        }

        k[1] = sign * VI(Interpolator(m_ptcl->m_block, el1, VV(x1)));
        Vect3 x2nd = m_ptcl->m_x + m_h * (k[0] * 0.5 + k[1] * 0.5);

        const Vect3 x2 = m_ptcl->m_x + m_h * (-k[0] + 2 * k[1]);
        Index el2 = grid->findCell(VV(x2), el1, m_cellSearchFlags);
        if (el2 == InvalidIndex) {
            m_ptcl->m_x = x2nd;
            m_hact = m_h;
            return false;
        }
        if (el2 != el1) {
            cellSize = std::min(grid->cellDiameter(el2), cellSize);
        }

        k[2] = sign * VI(Interpolator(m_ptcl->m_block, el2, VV(x2)));
        Vect3 x3rd = m_ptcl->m_x + m_h * (0.5 * k[0] * third + 2 * k[1] * third + 0.5 * k[2] * third);
        m_hact = m_h;

        bool accept = hNew(m_ptcl->m_x, x3rd, x2nd, k[0], cellSize);
        if (accept) {
            m_ptcl->m_x = x3rd;
            return true;
        }
    }
}

template<typename S>
bool Integrator<S>::StepConstantVelocity()
{
    Index el = m_ptcl->m_el;
    auto grid = m_ptcl->m_block->getGrid();

    auto vel = m_forward ? m_ptcl->m_v : -m_ptcl->m_v;
    Scal t = grid->exitDistance(el, VV(m_ptcl->m_x), vel);
    if (t < 0)
        return false;

    const auto cellsize = grid->cellDiameter(el);

    Scal v = vel.norm();
    const auto dir = vel.normalized();
    m_ptcl->m_x += VI(dir) * t;
    while (grid->inside(el, VV(m_ptcl->m_x))) {
        m_ptcl->m_x += VI(dir) * cellsize * 0.001;
        t += cellsize * 0.001;
    }
    m_hact = m_h = t / v;

    return true;
}

template<typename S>
vistle::Vector3 Integrator<S>::Interpolator(BlockData *bl, Index el, const Vector3 &point)
{
    auto grid = bl->getGrid();
    GridInterface::Interpolator interpolator = grid->getInterpolator(el, point, bl->getVecMapping());
    return interpolator(m_v[0], m_v[1], m_v[2]);
}


template class Integrator<float>;
template class Integrator<double>;
