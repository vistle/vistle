#ifndef TRACER_H
#define TRACER_H

#include <future>
#include <vector>
#include <map>
#include <string>

#include <util/enum.h>
#include <core/vec.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/celltree.h>
#include <module/module.h>
#include "Integrator.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(TraceType,
      (Streamlines)
      (MovingPoints)
)

class BlockData;

class GlobalData {
    friend class Particle;
    friend class Tracer;
    friend class Integrator;

public:

private:
    std::vector<std::vector<std::shared_ptr<BlockData>>> blocks;

    TraceType task_type;
    IntegrationMethod int_mode;
    bool use_celltree;

    double errtolrel, errtolabs;
    double h_min, h_max, h_init;
    double min_vel;
    double dt_step;
    double trace_time;
    double trace_len;
    bool cell_relative, velocity_relative;

    vistle::Index max_step;

    bool computeVector = true;
    bool computeScalar = true;
    bool computeId = true;
    bool computeStep = true;
    bool computeStopReason = true;
    bool computeTime = true;
    bool computeDist = true;
    bool computeStepWidth = true;

    std::vector<vistle::Points::ptr> points; // points objects for each timestep (MovingPoints)
    std::vector<vistle::Lines::ptr> lines; // lines objects for each timestep (other modes)
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> vecField;
    std::vector<vistle::Vec<vistle::Scalar>::ptr> scalField;
    std::vector<vistle::Vec<vistle::Index>::ptr> idField, stepField, stopReasonField;
    std::vector<vistle::Vec<vistle::Scalar>::ptr> timeField, distField, stepWidthField;
    std::mutex mutex;
};

class Tracer: public vistle::Module {

public:
    Tracer(const std::string &name, int moduleID, mpi::communicator comm);
    ~Tracer();

    typedef std::map<std::string, std::string> AttributeMap;

 private:
    bool compute() override;
    bool prepare() override;
    bool reduce(int timestep) override;
    bool changeParameter(const vistle::Parameter *param) override;

    std::vector<std::vector<vistle::Object::const_ptr>> grid_in;
    std::vector<std::vector<std::future<vistle::Celltree3::const_ptr>>> celltree;
    std::vector<std::vector<vistle::Vec<vistle::Scalar,3>::const_ptr>> data_in0;
    std::vector<std::vector<vistle::Vec<vistle::Scalar>::const_ptr>> data_in1;

    std::vector<AttributeMap> m_gridAttr, m_data0Attr, m_data1Attr;

    vistle::IntParameter *m_taskType;
    vistle::IntParameter *m_maxStartpoints, *m_numStartpoints;
    vistle::IntParameter *m_useCelltree;
    bool m_havePressure;

    bool m_haveTimeSteps = false;

    bool m_numStartpointsPrinted = false;

};
#endif
