#include <string>
#include <type_traits>

#include <viskores/List.h>
#include <viskores/TypeList.h>

#include "DisplaceFilter.h"

VISKORES_CONT DisplaceFilter::DisplaceFilter()
: m_component(DisplaceComponent::X), m_operation(DisplaceOperation::Add), m_scale(1.0f)
{}

VISKORES_CONT viskores::cont::DataSet DisplaceFilter::DoExecute(const viskores::cont::DataSet &inputDataset)
{
    auto inputField = this->GetFieldFromDataSet(inputDataset);
    auto inputCoords = inputDataset.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

    viskores::cont::UnknownArrayHandle outputCoords;

    inputCoords.GetData()
        .CastAndCallForTypesWithFloatFallback<viskores::TypeListFloatVec, VISKORES_DEFAULT_STORAGE_LIST>(
            [&](const auto &coords) {
                using CoordsArrayType = std::decay_t<decltype(coords)>;
                using CoordType = typename CoordsArrayType::ValueType;
                constexpr int CoordsDim = CoordType::NUM_COMPONENTS;

                inputField.GetData()
                    .CastAndCallForTypesWithFloatFallback<viskores::TypeListField, VISKORES_DEFAULT_STORAGE_LIST>(
                        [&](const auto &field) {
                            using FieldType = typename std::decay_t<decltype(field)>::ValueType;
                            constexpr bool fieldIsScalar = std::is_arithmetic_v<FieldType>;

                            viskores::cont::ArrayHandle<CoordType> result;

                            // Note: We can't combine the if and else-if (even though the body is the same)
                            //       because NUM_COMPONENTS only exists for vector types and, thus, yields an
                            //       compilation error for scalar types.
                            if constexpr (fieldIsScalar) {
                                applyDisplaceOperation<CoordsDim>(field, coords, result);
                                outputCoords = result;
                            } else if constexpr (FieldType::NUM_COMPONENTS == CoordsDim) {
                                applyDisplaceOperation<CoordsDim>(field, coords, result);
                                outputCoords = result;
                            } else {
                                throw viskores::cont::ErrorBadValue(
                                    "Error in DisplaceFilter: Cannot apply filter on point coordinates of dimension " +
                                    std::to_string(CoordsDim) + " with data field of dimension " +
                                    std::to_string(FieldType::NUM_COMPONENTS) + "!");
                            }
                        });
            });

    auto outputFieldName = this->GetOutputFieldName();
    if (outputFieldName == "")
        outputFieldName = inputField.GetName() + "_displaced";

    return this->CreateResultCoordinateSystem(
        inputDataset, inputDataset.GetCellSet(), inputCoords.GetName(), outputCoords,
        [&](viskores::cont::DataSet &out, const viskores::cont::Field &fieldToPass) {
            out.AddField(viskores::cont::Field(
                fieldToPass.GetName() == this->GetActiveFieldName() ? outputFieldName : fieldToPass.GetName(),
                fieldToPass.GetAssociation(), fieldToPass.GetData()));
        });
}
