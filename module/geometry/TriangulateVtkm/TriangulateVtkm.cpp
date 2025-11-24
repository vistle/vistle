#include <viskores/filter/geometry_refinement/Triangulate.h>

#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/unstr.h>

#include <vistle/vtkm/convert_worklets.h>

#include "TriangulateVtkm.h"

MODULE_MAIN(TriangulateVtkm)

using namespace vistle;

TriangulateVtkm::TriangulateVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}


std::unique_ptr<viskores::filter::Filter> TriangulateVtkm::setUpFilter() const
{
    return std::make_unique<viskores::filter::geometry_refinement::Triangulate>();
}
