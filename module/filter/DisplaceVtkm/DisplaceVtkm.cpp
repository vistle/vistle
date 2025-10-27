#include <viskores/filter/field_transform/Warp.h>

#include "DisplaceVtkm.h"

MODULE_MAIN(DisplaceVtkm)

using namespace vistle;

DisplaceVtkm::DisplaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm)
{}

DisplaceVtkm::~DisplaceVtkm()
{}

std::unique_ptr<viskores::filter::Filter> DisplaceVtkm::setUpFilter() const
{
    return std::make_unique<viskores::filter::field_transform::Warp>();
}
