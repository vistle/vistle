#include <viskores/filter/field_transform/PointTransform.h>

#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>

#include "ApplyTransformVtkm.h"

MODULE_MAIN(ApplyTransformVtkm)

using namespace vistle;

ApplyTransformVtkm::ApplyTransformVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}

ApplyTransformVtkm::~ApplyTransformVtkm()
{}

ModuleStatusPtr ApplyTransformVtkm::prepareInputGrid(const vistle::Object::const_ptr &grid,
                                                    viskores::cont::DataSet &dataset) const
{
    
    return VtkmModule::prepareInputGrid(grid, dataset);
}

std::unique_ptr<viskores::filter::Filter> ApplyTransformVtkm::setUpFilter() const
{
    return std::make_unique<viskores::filter::field_transform::PointTransform>();
}
