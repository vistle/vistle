#ifndef VISTLE_TRACER_INTEGRATOR_H
#define VISTLE_TRACER_INTEGRATOR_H

#include <vistle/core/vec.h>
#include <vistle/core/vector.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(IntegrationMethod, (Euler)(RK32)(ConstantVelocity))

template<typename S>
class Particle;
class BlockData;

template<typename S>
class Integrator {
    friend class Particle<S>;

public:
    typedef S Scal;
    typedef vistle::VistleVector<S, 3> Vect3;
    static vistle::Vector3 VV(const Vect3 &v) { return vistle::Vector3(v[0], v[1], v[2]); } // Vector Vistle
    static Vect3 VI(const vistle::Vector3 &v) { return Vect3(v[0], v[1], v[2]); } // Vector Integrator

    Integrator(Particle<S> *ptcl, bool forward);
    void UpdateBlock();
    bool Step();
    bool StepEuler();
    bool StepRK32();
    bool StepConstantVelocity();
    vistle::Vector3 Interpolator(BlockData *bl, vistle::Index el, const vistle::Vector3 &point);
    void hInit();
    bool hNew(Vect3 cur, Vect3 higher, Vect3 lower, Vect3 vel, vistle::Scalar unit);
    void enableCelltree(bool value);
    Scal h() const;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
    Scal m_h, m_hact;
    Particle<S> *m_ptcl;
    bool m_forward;
    const vistle::Scalar *m_v[3];
    int m_cellSearchFlags;
};

extern template class Integrator<float>;
extern template class Integrator<double>;
#endif
