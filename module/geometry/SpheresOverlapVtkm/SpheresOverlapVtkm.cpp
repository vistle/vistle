#include <math.h>
#include <type_traits>
#include <utility>

#include <vistle/alg/objalg.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/uniformgrid.h>

#include <vistle/vtkm/convert.h>
#include <vistle/vtkm/vtkm_module_utils.h>

#include "algo/VtkmSpheresOverlap.h"
#include "algo/CalculateThicknessVtkm.h"

#include "SpheresOverlapVtkm.h"

using namespace vistle;

MODULE_MAIN(SpheresOverlapVtkm)

// TODO: - instead of checking sId < sId2, adjust for range accordingly!

SpheresOverlapVtkm::SpheresOverlapVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_spheresIn = createInputPort("spheres_in", "spheres or data mapped to spheres");
    m_linesOut = createOutputPort("lines_out", "lines between all overlapping spheres");
    m_dataOut = createOutputPort("data_out", "data mapped onto the output lines");

    m_radiusCoefficient = addFloatParameter("multiply_search_radius_by",
                                            "increase search radius for the Cell Lists algorithm by this factor", 1);

    m_thicknessDeterminer = addIntParameter("thickness_determiner", "the line thickness will be mapped to this value",
                                            (Integer)OverlapRatio, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_thicknessDeterminer, ThicknessDeterminer);

    setReducePolicy(message::ReducePolicy::OverAll);
}


bool SpheresOverlapVtkm::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto dataIn = task->expect<Object>("spheres_in");
    auto container = splitContainerObject(dataIn);

    auto geo = container.geometry;
    auto mappedData = container.mapped;

    if (!Points::as(geo)) {
        sendError("input port expects spheres");
        return true;
    }

    auto spheres = Points::as(geo);
    auto radii = spheres->radius()->x();


    Lines::ptr lines;
    Vec<Scalar, 1>::ptr lineThicknesses;

    // create Viskores dataset from vistle data
    viskores::cont::DataSet vtkmSpheres;
    auto status = vtkmSetGrid(vtkmSpheres, spheres);

    if (!checkAndNotify(status))
        return true;


    vtkmSpheres.AddPointField("radius", radii.handle());

    VtkmSpheresOverlap overlapFilter;
    overlapFilter.SetRadiusFactor(m_radiusCoefficient->getValue());
    overlapFilter.SetThicknessDeterminer((ThicknessDeterminer)m_thicknessDeterminer->getValue());
    viskores::cont::DataSet vtkmLines;
    if (!vistle::vtkm::tryToExecuteFilter(*this, overlapFilter, vtkmSpheres, vtkmLines))
        return true;

    lines = Lines::as(vtkmGetGeometry(vtkmLines));
    lineThicknesses = Vec<Scalar, 1>::as(vtkmGetField(vtkmLines, "lineThickness"));

    if (lines->getNumCoords()) {
        lines->copyAttributes(geo);
        if (mappedData) {
            lines->copyAttributes(mappedData);
            lines->setTransform(geo->getTransform());
        }

        updateMeta(lines);
        task->addObject(m_linesOut, lines);

        lineThicknesses->copyAttributes(lines);
        lineThicknesses->setMapping(DataBase::Element);
        lineThicknesses->setGrid(lines);
        lineThicknesses->describe("line thickness", id());
        updateMeta(lineThicknesses);
        task->addObject(m_dataOut, lineThicknesses);
    }

    return true;
}

bool SpheresOverlapVtkm::checkAndNotify(const ModuleStatusPtr &status) const
{
    return vistle::vtkm::checkAndNotify(*this, status);
}
