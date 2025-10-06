#include <viskores/filter/vector_analysis/VectorMagnitude.h>

#include <vistle/vtkm/convert.h>

#include "VecToScalarVtkm.h"

MODULE_MAIN(VecToScalarVtkm)

using namespace vistle;



VecToScalarVtkm::VecToScalarVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1)
{
    
}

VecToScalarVtkm::~VecToScalarVtkm()
{}

std::unique_ptr<viskores::filter::Filter> VecToScalarVtkm::setUpFilter() const
{

    return std::make_unique<viskores::filter::vector_analysis::VectorMagnitude>();

}