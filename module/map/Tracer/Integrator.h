#ifndef TRACER_INTEGRATOR_H
#define TRACER_INTEGRATOR_H

#include <vistle/core/vec.h>
#include <vistle/core/vector.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(IntegrationMethod, (Euler)(RK32)(ConstantVelocity))

class Particle;
class BlockData;

class Integrator {
    friend class Particle;

private:
    vistle::Scalar m_h, m_hact;
    Particle *m_ptcl;
    bool m_forward;
    const vistle::Scalar *m_v[3];
    int m_cellSearchFlags;

public:
    Integrator(Particle *ptcl, bool forward);
    void UpdateBlock();
    bool Step();
    bool StepEuler();
    bool StepRK32();
    bool StepConstantVelocity();
    vistle::Vector3 Interpolator(BlockData *bl, vistle::Index el, const vistle::Vector3 &point);
    void hInit();
    bool hNew(vistle::Vector3 cur, vistle::Vector3 higher, vistle::Vector3 lower, vistle::Vector3 vel,
              vistle::Scalar unit);
    void enableCelltree(bool value);
    vistle::Scalar h() const;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif
