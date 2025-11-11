#include "ToTrianglesVtkm.h"

MODULE_MAIN(ToTrianglesVtkm)

using namespace vistle;

ToTrianglesVtkm::ToTrianglesVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Require)
{}

std::unique_ptr<viskores::filter::Filter> ToTrianglesVtkm::setUpFilter() const
{
    return nullptr;
}
