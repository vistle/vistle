#ifndef TRACER_INTEGRATOR_H
#define TRACER_INTEGRATOR_H

#include <core/vec.h>


class Particle;
class BlockData;

class Integrator{
friend class Particle;
private:
    vistle::Vector3 m_k[3];
    vistle::Scalar m_h;
    vistle::Scalar m_hmin;
    vistle::Scalar m_hmax;
    vistle::Scalar m_errtol;
    int m_mode;
    Particle* m_ptcl;


public:
    Integrator(vistle::Scalar h, vistle::Scalar hmin,
               vistle::Scalar hmax, vistle::Scalar errtol,
               int int_mode, Particle* ptcl);
    bool Step();
    bool Euler();
    bool RK32();
    vistle::Vector3 Interpolator(BlockData* bl, vistle::Index el,const vistle::Vector3 &point);
    void hInit();
    bool hNew(vistle::Vector3 higher, vistle::Vector3 lower);
};

#endif
