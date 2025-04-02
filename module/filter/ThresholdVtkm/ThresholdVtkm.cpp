#include <vtkm/filter/entity_extraction/Threshold.h>

#include "ThresholdVtkm.h"

MODULE_MAIN(ThresholdVtkm)

using namespace vistle;

ThresholdVtkm::ThresholdVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3)
{
    addIntParameter("invert_selection", "invert selection", false, Parameter::Boolean);
}

ThresholdVtkm::~ThresholdVtkm()
{}

std::unique_ptr<vtkm::filter::Filter> ThresholdVtkm::setUpFilter() const
{
    auto filter = std::make_unique<vtkm::filter::entity_extraction::Threshold>();
    filter->SetInvert(m_invert->getValue() != 0);
    return filter;
}
