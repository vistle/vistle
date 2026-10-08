#include <viskores/filter/entity_extraction/ExternalFaces.h>

#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>

#include "DomainSurfaceVtkm.h"

MODULE_MAIN(DomainSurfaceVtkm)

using namespace vistle;

DomainSurfaceVtkm::DomainSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}

ModuleStatusPtr DomainSurfaceVtkm::prepareInputGrid(InputData &input) const
{
    if (!StructuredGridBase::as(input.vistleGrid) && !UnstructuredGrid::as(input.vistleGrid))
        return Error("only structured and unstructured grids supported");
    return VtkmModule::prepareInputGrid(input);
}

std::unique_ptr<viskores::filter::Filter> DomainSurfaceVtkm::setUpFilter(const VtkmModule::InputData &input) const
{
    return std::make_unique<viskores::filter::entity_extraction::ExternalFaces>();
}
