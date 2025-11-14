#include <viskores/filter/geometry_refinement/Tube.h>

#include "TubeVtkm.h"

MODULE_MAIN(TubeVtkm)

using namespace vistle;

TubeVtkm::TubeVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}

std::unique_ptr<viskores::filter::Filter> TubeVtkm::setUpFilter() const
{
    return std::make_unique<viskores::filter::geometry_refinement::Tube>();
}
