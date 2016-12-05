#ifndef TRACER_H
#define TRACER_H

// define for profiling
//#define TIMING

#include <future>
#include <vector>

#include <core/vec.h>
#include <module/module.h>

class Tracer: public vistle::Module {

public:
    Tracer(const std::string &shmname, const std::string &name, int moduleID);
    ~Tracer();


 private:
    bool compute() override;
    bool prepare() override;
    bool reduce(int timestep) override;

    std::vector<std::vector<vistle::Object::const_ptr>> grid_in;
    std::vector<std::vector<std::future<vistle::Celltree3::const_ptr>>> celltree;
    std::vector<std::vector<vistle::Vec<vistle::Scalar,3>::const_ptr>> data_in0;
    std::vector<std::vector<vistle::Vec<vistle::Scalar>::const_ptr>> data_in1;

    vistle::IntParameter *m_useCelltree;
    bool m_havePressure;

};
#endif
