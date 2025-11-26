#include <viskores/filter/geometry_refinement/Triangulate.h>

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
