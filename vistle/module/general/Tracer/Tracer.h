#ifndef TRACER_H
#define TRACER_H

// define for profiling
//#define TIMING

#include <future>
#include <vector>

#include <util/enum.h>
#include <core/vec.h>
#include <core/celltree.h>
#include <module/module.h>
#include "Integrator.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(TraceType,
      (Streamlines)
)

class BlockData;

class GlobalData {
    friend class Particle;
    friend class Tracer;

public:

private:
    std::vector<std::vector<std::unique_ptr<BlockData>>> blocks;

    TraceType task_type;
    IntegrationMethod int_mode;

    double errtol;
    double h_min, h_max, h_init;
    double min_vel;
    double t_max;
    double trace_len;
    vistle::Index max_step;
};

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
