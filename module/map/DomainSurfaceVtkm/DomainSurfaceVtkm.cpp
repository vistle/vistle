#include <viskores/filter/entity_extraction/ExternalFaces.h>

#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>

#include "DomainSurfaceVtkm.h"

MODULE_MAIN(DomainSurfaceVtkm)

using namespace vistle;

DomainSurfaceVtkm::DomainSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}

ModuleStatusPtr DomainSurfaceVtkm::prepareInputGrid(const vistle::Object::const_ptr &grid,
                                                    viskores::cont::DataSet &dataset) const
{
    if (!StructuredGridBase::as(grid) && !UnstructuredGrid::as(grid))
        return Error("only structured and unstructured grids supported");
    return VtkmModule::prepareInputGrid(grid, dataset);
}

std::unique_ptr<viskores::filter::Filter> DomainSurfaceVtkm::setUpFilter() const
{
    return std::make_unique<viskores::filter::entity_extraction::ExternalFaces>();
}
