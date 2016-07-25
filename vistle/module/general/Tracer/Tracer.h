#ifndef TRACER_H
#define TRACER_H

// define for profiling
//#define TIMING

#include <future>

#include <module/module.h>
#include <core/unstr.h>

class Tracer: public vistle::Module {

public:
    Tracer(const std::string &shmname, const std::string &name, int moduleID);
    ~Tracer();


 private:
    virtual bool compute();
    virtual bool prepare();
    virtual bool reduce(int timestep);

    std::vector<std::vector<vistle::Object::const_ptr>> grid_in;
    std::vector<std::vector<std::future<vistle::Celltree3::const_ptr>>> celltree;
    std::vector<std::vector<vistle::Vec<vistle::Scalar,3>::const_ptr>> data_in0;
    std::vector<std::vector<vistle::Vec<vistle::Scalar>::const_ptr>> data_in1;
    vistle::Vector3 Integration(const vistle::Vector3 &p0, const vistle::Vector3 &v0, const vistle::Scalar &dt);
    vistle::Index findCell(const vistle::Vector3 &point,
                           vistle::Object::const_ptr grid,
                           std::array<vistle::Vector3, 2> boundingbox,
                           vistle::Index lastcell);

    vistle::IntParameter *m_useCelltree;
    bool m_havePressure;

};
#endif
