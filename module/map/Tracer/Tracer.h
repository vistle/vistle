#ifndef VISTLE_TRACER_TRACER_H
#define VISTLE_TRACER_TRACER_H

#include <future>
#include <vector>
#include <map>
#include <string>

#include <vistle/util/enum.h>
#include <vistle/core/vec.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/celltree.h>
#include <vistle/module/module.h>
#include "Integrator.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(TraceType,
                                    // this order is expected by COVER's TracerInteraction
                                    (Streamlines)(MovingPoints)(Pathlines)(Streaklines))

class BlockData;
class Tracer;
template<typename S>
class Particle;
template<typename S>
class Integrator;

class Tracer: public vistle::Module {
    static const int NumAddPorts = 2;

public:
    static const int NumPorts = 3;
    Tracer(const std::string &name, int moduleID, mpi::communicator comm);

    typedef std::map<std::string, std::string> AttributeMap;

private:
    bool compute() override;
    bool prepare() override;
    bool reduce(int timestep) override;
    bool changeParameter(const vistle::Parameter *param) override;

    std::vector<std::vector<vistle::Object::const_ptr>> grid_in;
    std::vector<std::vector<std::future<vistle::Celltree3::const_ptr>>> celltree;
    std::vector<std::vector<vistle::Vec<vistle::Scalar, 3>::const_ptr>> data_in0;
    std::vector<std::vector<vistle::DataBase::const_ptr>> data_in[NumPorts];
    int data_dim[NumPorts];
    std::vector<double> m_realtimes;

    std::vector<AttributeMap> m_gridAttr, m_dataAttr[NumPorts];
    std::vector<vistle::Meta> m_gridTime, m_dataTime[NumPorts];

    vistle::Port *m_inPort[NumPorts];
    vistle::Port *m_outPort[NumPorts];
    vistle::Port *m_addPort[NumAddPorts];
    vistle::IntParameter *m_addField[NumAddPorts];
    vistle::IntParameter *m_taskType;
    vistle::IntParameter *m_maxStartpoints, *m_numStartpoints;
    vistle::IntParameter *m_useCelltree;
    vistle::IntParameter *m_particlePlacement = nullptr;
    vistle::FloatParameter *m_simplificationError = nullptr;
    vistle::FloatParameter *m_dtStep = nullptr;
    vistle::IntParameter *m_verbose = nullptr;
    vistle::IntParameter *m_traceDirection = nullptr;
    vistle::IntParameter *m_startStyle = nullptr;
    vistle::VectorParameter *m_direction = nullptr;

    bool m_haveTimeSteps = false;
    void addDescription(int kind, const std::string &name, const std::string &description);
    const std::string &getFieldName(int kind) const;
    const std::string &getFieldDescription(int kind) const;
    std::map<int, std::string> m_addFieldName;
    std::map<int, std::string> m_addFieldDescription;

    std::vector<vistle::Index> m_stopReasonCount;
    vistle::Index m_numTotalParticles = 0;
    bool m_stopStatsPrinted = false;
    bool m_numStartpointsPrinted = false;
};

class GlobalData {
    friend class Particle<float>;
    friend class Particle<double>;
    friend class Integrator<float>;
    friend class Integrator<double>;
    friend class Tracer;

public:
private:
    std::vector<std::vector<std::shared_ptr<BlockData>>> blocks;

    TraceType task_type;
    IntegrationMethod int_mode;
    bool use_celltree;
    int num_particles = 0;

    double simplification_error = 0.;
    double errtolrel, errtolabs;
    double h_min, h_max, h_init;
    double min_vel;
    double dt_step;
    double trace_time;
    double trace_len;
    bool cell_relative, velocity_relative;
    int cell_index_modulus = -1;

    vistle::Index max_step;

    bool computeVector = false;
    bool computeField[Tracer::NumPorts]{false};
    bool computeId = false;
    bool computeStep = false;
    bool computeStopReason = false;
    bool computeTime = false;
    bool computeDist = false;
    bool computeStepWidth = false;
    bool computeCellIndex = false;
    bool computeBlockIndex = false;

    std::vector<vistle::Points::ptr> points; // points objects for each timestep (MovingPoints)
    std::vector<vistle::Lines::ptr> lines; // lines objects for each timestep (other modes)
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> vecField;
    std::vector<std::vector<vistle::DataBase::ptr>> fields;
    std::vector<std::vector<vistle::shm<vistle::Scalar>::array>> scalars;
    std::vector<vistle::Vec<vistle::Index>::ptr> idField, stepField, stopReasonField;
    std::vector<vistle::Vec<vistle::Index>::ptr> blockField, cellField;
    std::vector<vistle::Vec<vistle::Scalar>::ptr> timeField, distField, stepWidthField;
    int numScalars = 0;
    std::mutex mutex;

    Tracer *module = nullptr;
};

#endif
