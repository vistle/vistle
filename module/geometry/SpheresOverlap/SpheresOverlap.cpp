#include <map>
#include <math.h>
#include <type_traits>
#include <utility>
#include <vector>

#include <vistle/alg/objalg.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/vtkm/convert.h>

#include "algo/CellListsAlgorithm.h"
#include "algo/ThicknessDeterminer.h"
#include "algo/VtkmSpheresOverlap.h"

#include "SpheresOverlap.h"

using namespace vistle;

MODULE_MAIN(SpheresOverlap)

// TODO: - instead of checking sId < sId2, adjust for range accordingly!

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

    if (!Points::as(geo)) {
        sendError("input port expects spheres");
        return true;
    }

    auto spheres = Points::as(geo);
    auto radii = spheres->radius()->x();


    Lines::ptr lines;
    Vec<Scalar, 1>::ptr lineThicknesses;

    if (m_useVtkm->getValue()) {
        // create vtk-m dataset from vistle data
        vtkm::cont::DataSet vtkmSpheres;
        auto status = vtkmSetGrid(vtkmSpheres, spheres);
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
        overlapFilter.SetRadiusFactor(m_radiusCoefficient->getValue());
        overlapFilter.SetThicknessDeterminer((ThicknessDeterminer)m_thicknessDeterminer->getValue());
        auto vtkmLines = overlapFilter.Execute(vtkmSpheres);

        lines = Lines::as(vtkmGetGeometry(vtkmLines));
        lineThicknesses = Vec<Scalar, 1>::as(vtkmGetField(vtkmLines, "lineThickness"));

    } else {
        auto maxRadius = std::numeric_limits<std::remove_reference<decltype(radii[0])>::type>::min();
        for (Index i = 0; i < radii.size(); i++) {
            if (radii[i] > maxRadius)
                maxRadius = radii[i];
        }

        // We want to ensure that no pair of overlapping spheres is missed. As two spheres overlap when
        // the Euclidean distance between them is less than the sum of their radii, the minimum search
        // radius is two times the maximum sphere radius.
        auto overlaps = CellListsAlgorithm(spheres, 2 * m_radiusCoefficient->getValue() * maxRadius,
                                           (ThicknessDeterminer)m_thicknessDeterminer->getValue());

        auto result = CreateConnectionLines(overlaps, spheres);

        lines = result.first;
        lineThicknesses = result.second;
    }
    auto lCoords = Coords::as(lines);

    std::cout << "Duplicate lines with different thicknesses: " << std::endl;

    for (auto i = 0; i < lines->cl().size(); i += 2) {
        auto i1 = lines->cl()[i];
        auto i2 = lines->cl()[i + 1];

        auto p1 = vtkm::Vec3f(lCoords->x()[i1], lCoords->y()[i1], lCoords->z()[i1]);
        auto p2 = vtkm::Vec3f(lCoords->x()[i2], lCoords->y()[i2], lCoords->z()[i2]);
        if (p1 < p2) {
            std::cout << "(" << p1[0] << " | " << p1[1] << " | " << p1[2] << ") and "
                      << "(" << p2[0] << " | " << p2[1] << " | " << p2[2] << ") --> " << lineThicknesses->x()[i / 2]
                      << ", indices: " << i1 << " " << i2 << std::endl;
        } else {
            std::cout << "(" << p2[0] << " | " << p2[1] << " | " << p2[2] << ") and "
                      << "(" << p1[0] << " | " << p1[1] << " | " << p1[2] << ") --> " << lineThicknesses->x()[i / 2]
                      << ", indices: " << i1 << " " << i2 << std::endl;
        }
    }
    /*for (auto i = 0; i < lines->cl().size(); i += 2) {
        for (auto j = i + 1; j < lines->cl().size(); j += 2) {
            auto i1 = lines->cl()[i];
            auto i2 = lines->cl()[i + 1];

            auto j1 = lines->cl()[j];
            auto j2 = lines->cl()[j + 1];

            // find identical pairs of points (minus the order)
            if ((i1 == j1 && i2 == j2) || (i1 == j2 && i2 == j1)) {
                // check if the line thickness for the identical lines is different
                if (lineThicknesses->x()[i / 2] != lineThicknesses->x()[j / 2])
                    std::cout << i << " + " << j << ":\n    " << i1 << " + " << i2 << " -> "
                              << lineThicknesses->x()[i / 2] << "\n    " << j1 << " + " << j2 << " -> "
                              << lineThicknesses->x()[j / 2] << std::endl;
            }
        }
    }*/

    auto myMin =
        *std::min_element(lineThicknesses->x().data(), lineThicknesses->x().data() + lineThicknesses->x().size() - 1);
    auto myMax =
        *std::max_element(lineThicknesses->x().data(), lineThicknesses->x().data() + lineThicknesses->x().size() - 1);

    std::cout << "Thickness min: " << myMin << ", max: " << myMax << std::endl;

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

    return true;
}
