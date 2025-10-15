#include "PassThroughVtkm.h"

using namespace vistle;

MODULE_MAIN(PassThroughVtkm)

PassThroughVtkm::PassThroughVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}

std::unique_ptr<viskores::filter::Filter> PassThroughVtkm::setUpFilter() const
{
    return nullptr;
}
