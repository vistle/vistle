#include "CuttingSurfaceVtkm.h"

MODULE_MAIN(CuttingSurfaceVtkm)

using namespace vistle;

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(SurfaceOption, (Plane)(Sphere)(CylinderX)(CylinderY)(CylinderZ))

CuttingSurfaceVtkm::CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    // ports
    m_mapDataIn = createInputPort("data_in", "input grid or geometry with mapped data");
    m_dataOut = createOutputPort("data_out", "surface with mapped data");

    // parameters
    m_point = addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
    m_vertex = addVectorParameter("vertex", "normal on plane", ParamVector(1.0, 0.0, 0.0));
    m_scalar = addFloatParameter("scalar", "distance to origin of ordinates", 0.0);
    m_option = addIntParameter("option", "option", Plane, Parameter::Choice);
    setParameterChoices(m_option, valueList((SurfaceOption)0));

    m_direction = addVectorParameter("direction", "direction for variable Cylinder", ParamVector(0.0, 0.0, 0.0));
    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", 0, Parameter::Boolean);
}

CuttingSurfaceVtkm::~CuttingSurfaceVtkm()
{}

bool CuttingSurfaceVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    return true;
}
