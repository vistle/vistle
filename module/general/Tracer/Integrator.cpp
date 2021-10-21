#include "Integrator.h"
#include "Particle.h"
#include "BlockData.h"
#include "Tracer.h"
#include <vistle/core/vec.h>


using namespace vistle;


Integrator::Integrator(Particle *ptcl, bool forward)
: m_h(-1.), m_hact(m_h), m_ptcl(ptcl), m_forward(forward), m_cellSearchFlags(GridInterface::NoFlags)
{
    UpdateBlock();
}

void Integrator::hInit()
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
        Vector3 vel = Interpolator(m_ptcl->m_block, m_ptcl->m_el, m_ptcl->m_x);
        Scalar v = std::max(vel.norm(), Scalar(1e-7));
        unit /= v;
    }

    m_h = unit * h_min;
    m_hact = m_h;
}


void Integrator::UpdateBlock()
{
    if (BlockData *bl = m_ptcl->m_block) {
        Vec<Scalar, 3>::const_ptr vecfld = bl->getVecFld();
        for (int i = 0; i < 3; ++i)
            m_v[i] = &vecfld->x(i)[0];
        m_velTransform = bl->velocityTransform();
    } else {
        m_v[0] = m_v[1] = m_v[2] = nullptr;
        m_velTransform.setIdentity();
    }
}

bool Integrator::Step()
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

bool Integrator::StepEuler()
{
    Scalar unit = 1.;
    if (m_ptcl->m_global.cell_relative) {
        Index el = m_ptcl->m_el;
        auto grid = m_ptcl->m_block->getGrid();
        Scalar cellSize = grid->cellDiameter(el);
        unit = cellSize;
    }

    Vector vel = m_forward ? m_ptcl->m_v : -m_ptcl->m_v;
    Scalar v = std::max(vel.norm(), Scalar(1e-7));
    if (m_ptcl->m_global.velocity_relative)
        unit /= v;
    m_ptcl->m_x = m_ptcl->m_x + vel * m_h * unit;
    m_hact = m_h * unit;
    return true;
}

bool Integrator::hNew(Vector3 cur, Vector3 higher, Vector3 lower, Vector vel, Scalar unit)
{
    const auto &global = m_ptcl->m_global;
    const auto h_max = global.h_max;
    const auto h_min = global.h_min;
    const auto tol_rel = global.errtolrel;
    const auto tol_abs = global.errtolabs;

    Scalar errestabs = (higher - lower).lpNorm<Eigen::Infinity>();
    Scalar errestrel = (higher - lower).lpNorm<Eigen::Infinity>() / (higher - cur).lpNorm<Eigen::Infinity>();

    if (!global.cell_relative) {
        unit = 1.;
    }
    if (global.velocity_relative) {
        Scalar v = std::max(vel.norm(), Scalar(1e-7));
        unit /= v;
    }

    if (!std::isfinite(errestabs) || !std::isfinite(errestrel)) {
        m_h = h_min * unit;
        std::cerr << "Integrator: invalid input for error estimation: higher=" << higher.transpose()
                  << ", lower=" << lower.transpose() << std::endl;
        return true;
    }

    Scalar h_abs = 0.9 * m_h * std::pow(Scalar(tol_abs / errestabs), Scalar(1.0 / 3.0));
    Scalar h_rel = 0.9 * m_h * std::pow(Scalar(tol_rel / errestrel), Scalar(1.0 / 3.0));
    Scalar h = std::min(std::max(h_abs, h_rel), 2 * m_h);
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

void Integrator::enableCelltree(bool value)
{
    if (value) {
        m_cellSearchFlags = GridInterface::NoFlags;
    } else {
        m_cellSearchFlags = GridInterface::NoCelltree;
    }
}

Scalar Integrator::h() const
{
    return m_hact;
}

// 3rd-order Runge-Kutta with embedded Heun
bool Integrator::StepRK32()
{
    Scalar sign = m_forward ? 1. : -1.;
    Vector3 k[3];
    k[0] = sign * m_ptcl->m_v;
    auto grid = m_ptcl->m_block->getGrid();
    Scalar cellSize = grid->cellDiameter(m_ptcl->m_el);
    const Scalar third(1. / 3.);

    for (;;) {
        const Vector x1 = m_ptcl->m_x + 0.5 * m_h * k[0];
        Index el1 = grid->findCell(x1, m_ptcl->m_el, m_cellSearchFlags);
        if (el1 == InvalidIndex) {
            m_ptcl->m_x = x1;
            m_hact = 0.5 * m_h;
            return false;
        }
        if (el1 != m_ptcl->m_el) {
            cellSize = std::min(grid->cellDiameter(el1), cellSize);
        }

        k[1] = m_velTransform * sign * Interpolator(m_ptcl->m_block, el1, x1);
        Vector3 x2nd = m_ptcl->m_x + m_h * (k[0] * 0.5 + k[1] * 0.5);

        const Vector x2 = m_ptcl->m_x + m_h * (-k[0] + 2 * k[1]);
        Index el2 = grid->findCell(x2, el1, m_cellSearchFlags);
        if (el2 == InvalidIndex) {
            m_ptcl->m_x = x2nd;
            m_hact = m_h;
            return false;
        }
        if (el2 != el1) {
            cellSize = std::min(grid->cellDiameter(el2), cellSize);
        }

        k[2] = m_velTransform * sign * Interpolator(m_ptcl->m_block, el2, x2);
        Vector3 x3rd = m_ptcl->m_x + m_h * (0.5 * k[0] * third + 2 * k[1] * third + 0.5 * k[2] * third);
        m_hact = m_h;

        bool accept = hNew(m_ptcl->m_x, x3rd, x2nd, k[0], cellSize);
        if (accept) {
            m_ptcl->m_x = x3rd;
            return true;
        }
    }
}

bool Integrator::StepConstantVelocity()
{
    Index el = m_ptcl->m_el;
    auto grid = m_ptcl->m_block->getGrid();

    Vector vel = m_forward ? m_ptcl->m_v : -m_ptcl->m_v;
    vel = m_velTransform * vel;
    Scalar t = grid->exitDistance(el, m_ptcl->m_x, vel);
    if (t < 0)
        return false;

    const auto cellsize = grid->cellDiameter(el);

    Scalar v = vel.norm();
    const auto dir = vel.normalized();
    m_ptcl->m_x += dir * t;
    while (grid->inside(el, m_ptcl->m_x)) {
        m_ptcl->m_x += dir * cellsize * 0.001;
        t += cellsize * 0.001;
    }
    m_hact = m_h = t / v;

    return true;
}

Vector3 Integrator::Interpolator(BlockData *bl, Index el, const Vector3 &point)
{
    auto grid = bl->getGrid();
    GridInterface::Interpolator interpolator = grid->getInterpolator(el, point, bl->getVecMapping());
    return interpolator(m_v[0], m_v[1], m_v[2]);
}
