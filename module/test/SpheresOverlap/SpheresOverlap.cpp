#include <map>
#include <math.h>
#include <type_traits>
#include <utility>
#include <vector>

#include <vistle/alg/objalg.h>
#include <vistle/core/lines.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/spheres.h>
#include <vistle/vtkm/convert.h>

#include "algo/CellListsAlgorithm.h"
#include "algo/VtkmSpheresOverlap.h"

#include "SpheresOverlap.h"

using namespace vistle;

MODULE_MAIN(SpheresOverlap)

// TODO: - instead of checking sId < sId2, adjust for range accordingly!
//       - debug map with Gendat data is missing connections when rendered as tubes (as opposed to lines)


SpheresOverlap::SpheresOverlap(const std::string &name, int moduleID, mpi::communicator comm)
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

    m_useVtkm = addIntParameter("use_vtkm", "", false, Parameter::Boolean);
    setReducePolicy(message::ReducePolicy::OverAll);
}


SpheresOverlap::~SpheresOverlap()
{}

bool SpheresOverlap::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto dataIn = task->expect<Object>("spheres_in");
    auto container = splitContainerObject(dataIn);

    auto geo = container.geometry;
    auto mappedData = container.mapped;

    if (!Spheres::as(geo)) {
        sendError("input port expects spheres");
        return true;
    }

    auto spheres = Spheres::as(geo);
    auto radii = spheres->r();

    if (m_useVtkm->getValue()) {
        // create vtk-m dataset from vistle data
        vtkm::cont::DataSet vtkmSpheres;
        auto status = vtkmSetGrid(vtkmSpheres, geo);
        if (status == VtkmTransformStatus::UNSUPPORTED_GRID_TYPE) {
            sendError("Currently only supporting unstructured grids");
            return true;
        } else if (status == VtkmTransformStatus::UNSUPPORTED_CELL_TYPE) {
            sendError("Can only transform these cells from vistle to vtkm: point, bar, triangle, polygon, quad, tetra, "
                      "hexahedron, pyramid");
            return true;
        }

        vtkmSpheres.AddPointField("radius", radii.handle());
        
        VtkmSpheresOverlap overlapFilter;
        overlapFilter.Execute(vtkmSpheres);

    } else {
        auto maxRadius = std::numeric_limits<std::remove_reference<decltype(radii[0])>::type>::min();
        for (Index i = 0; i < radii.size(); i++) {
            if (radii[i] > maxRadius)
                maxRadius = radii[i];
        }

        // We want to ensure that no pair of overlapping spheres is missed. As two spheres overlap when
        // the Euclidean distance between them is less than the sum of their radii, the minimum search
        // radius is two times the maximum sphere radius.
        auto overlaps = CellListsAlgorithm(spheres, 2.1 * m_radiusCoefficient->getValue() * maxRadius,
                                           (ThicknessDeterminer)m_thicknessDeterminer->getValue());

        auto result = CreateConnectionLines(overlaps, spheres);

        auto lines = result.first;
        auto lineThicknesses = result.second;

        if (lines->getNumCoords()) {
            if (mappedData) {
                lines->copyAttributes(mappedData);
            } else {
                lines->copyAttributes(geo);
            }

            updateMeta(lines);
            task->addObject(m_linesOut, lines);

            lineThicknesses->copyAttributes(lines);
            lineThicknesses->setMapping(DataBase::Element);
            lineThicknesses->setGrid(lines);
            lineThicknesses->addAttribute("_species", "line thickness");
            updateMeta(lineThicknesses);
            task->addObject(m_dataOut, lineThicknesses);
        }
    }

    return true;
}
