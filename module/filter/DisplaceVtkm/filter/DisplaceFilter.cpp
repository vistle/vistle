#include <string>
#include <type_traits>

#include <viskores/List.h>
#include <viskores/TypeList.h>

#include "DisplaceFilter.h"
#include "DisplaceWorklet.h"

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
                    .CastAndCallForTypesWithFloatFallback<viskores::TypeListField,
                                                          VISKORES_DEFAULT_STORAGE_LIST>([&](const auto &field) {
                        using FieldType = typename std::decay_t<decltype(field)>::ValueType;
                        constexpr bool fieldIsScalar = std::is_arithmetic_v<FieldType>;

                        viskores::cont::ArrayHandle<CoordType> result;
                        viskores::Vec<viskores::FloatDefault, CoordsDim> mask(0.0f);

                        if constexpr (fieldIsScalar) {
                            viskores::IdComponent c = 0; // must be declared outside switch-case
                            switch (m_component) {
                            case DisplaceComponent::X:
                            case DisplaceComponent::Y:
                            case DisplaceComponent::Z:
                                c = static_cast<viskores::IdComponent>(m_component);
                                if (c < CoordsDim) {
                                    mask[c] = 1.0f;
                                } else {
                                    throw viskores::cont::ErrorBadValue(
                                        "Error in DisplaceFilter: DisplaceComponent value (" + std::to_string(c) +
                                        ") out of bounds for coordinate dimension (" + std::to_string(CoordsDim) +
                                        ")!");
                                }
                                break;
                            case DisplaceComponent::All:
                                for (auto c = 0; c < CoordsDim; c++) {
                                    mask[c] = 1.0f;
                                }
                                break;
                            default:
                                throw viskores::cont::ErrorBadValue(
                                    "Error in DisplaceFilter: Encountered unknown DisplaceComponent value!");
                            }

                            switch (m_operation) {
                            case DisplaceOperation::Set:
                                this->Invoke(SetDisplaceWorklet<CoordsDim>{m_scale, mask}, field, coords, result);
                                break;
                            case DisplaceOperation::Add:
                                this->Invoke(AddDisplaceWorklet<CoordsDim>{m_scale, mask}, field, coords, result);
                                break;
                            case DisplaceOperation::Multiply:
                                this->Invoke(MultiplyDisplaceWorklet<CoordsDim>{m_scale, mask}, field, coords, result);
                                break;
                            default:
                                throw viskores::cont::ErrorBadValue(
                                    "Error in DisplaceFilter: Encountered unknown DisplaceOperation value!");
                            }

                            outputCoords = result;
                        } else if constexpr (!fieldIsScalar && FieldType::NUM_COMPONENTS == CoordsDim) {
                            switch (m_operation) {
                            case DisplaceOperation::Set:
                                this->Invoke(SetDisplaceWorklet<CoordsDim>{m_scale, mask}, field, coords, result);
                                break;
                            case DisplaceOperation::Add:
                                this->Invoke(AddDisplaceWorklet<CoordsDim>{m_scale, mask}, field, coords, result);
                                break;
                            case DisplaceOperation::Multiply:
                                this->Invoke(MultiplyDisplaceWorklet<CoordsDim>{m_scale, mask}, field, coords, result);
                                break;
                            default:
                                throw viskores::cont::ErrorBadValue(
                                    "Error in DisplaceFilter: Encountered unknown DisplaceOperation value!");
                            }

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
