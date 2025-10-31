#include <viskores/filter/entity_extraction/Threshold.h>

#include <vistle/vtkm/convert.h>

#include "ThresholdVtkm.h"

MODULE_MAIN(ThresholdVtkm)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Operation, (AnyLess)(AllLess)(AnyGreater)(AllGreater))

ThresholdVtkm::ThresholdVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3)
{
    m_invert = addIntParameter("invert_selection", "invert selection", false, Parameter::Boolean);

    m_operation = addIntParameter("operation", "selection operation", AnyLess, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_operation, Operation, );

    m_threshold = addFloatParameter("threshold", "selection threshold", 0);
}

std::unique_ptr<viskores::filter::Filter> ThresholdVtkm::setUpFilter() const
{
    auto filter = std::make_unique<viskores::filter::entity_extraction::Threshold>();
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

Object::const_ptr ThresholdVtkm::prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                   const Object::const_ptr &inputGrid) const
{
    // overriding this method because it should not be treated as error if the output grid is empty
    auto outputGrid = vtkmGetGeometry(dataset);
    if (!outputGrid)
        return nullptr;
    updateMeta(outputGrid);
    outputGrid->copyAttributes(inputGrid);
    return outputGrid;
}

DataBase::ptr ThresholdVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                                const Object::const_ptr &inputGrid,
                                                const DataBase::const_ptr &inputField, const std::string &fieldName,
                                                const Object::const_ptr &outputGrid) const
{
    // only prepare the output field if the output grid exists
    if (outputGrid) {
        return VtkmModule::prepareOutputField(dataset, inputGrid, inputField, fieldName, outputGrid);
    } else {
        return nullptr;
    }
}
