#include <vtkm/filter/entity_extraction/Threshold.h>

#include "ThresholdVtkm.h"

MODULE_MAIN(ThresholdVtkm)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Operation, (AnyLess)(AllLess)(Contains)(AnyGreater)(AllGreater))

ThresholdVtkm::ThresholdVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3)
{
    m_invert = addIntParameter("invert_selection", "invert selection", false, Parameter::Boolean);

    m_operation = addIntParameter("operation", "selection operation", AnyLess, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_operation, Operation);

    m_threshold = addFloatParameter("threshold", "selection threshold", 0);
}

ThresholdVtkm::~ThresholdVtkm()
{}

std::unique_ptr<vtkm::filter::Filter> ThresholdVtkm::setUpFilter() const
{
    auto filter = std::make_unique<vtkm::filter::entity_extraction::Threshold>();
    auto threshold = m_threshold->getValue();
    switch (m_operation->getValue()) {
    case AnyLess:
        filter->SetComponentToTestToAny();
        filter->SetThresholdBelow(threshold);
        break;
    case AllLess:
        filter->SetComponentToTestToAll();
        filter->SetThresholdBelow(threshold);
        break;
    case Contains:
        //TODO: not sure how this is in the CPU version
        filter->SetComponentToTestToAny();
        filter->SetThresholdBetween(threshold, threshold);
        break;
    case AnyGreater:
        filter->SetComponentToTestToAny();
        filter->SetThresholdAbove(threshold);
        break;
    case AllGreater:
        filter->SetComponentToTestToAll();
        filter->SetThresholdAbove(threshold);
        break;
    default:
        assert("operation not implemented" == nullptr);
        break;
    }

    filter->SetInvert(m_invert->getValue() != 0);
    return filter;
}
