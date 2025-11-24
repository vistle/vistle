#include <string>
#include <type_traits>

#include <viskores/List.h>

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

    constexpr viskores::IdComponent COORDS_DIM = 3;
    this->CastAndCallVecField<COORDS_DIM>(inputCoords.GetData(), [&](const auto &coords) {
        using CoordsArrayType = std::decay_t<decltype(coords)>;
        using CoordType = typename CoordsArrayType::ValueType;

        // we assume the data field is either 1D or 3D
        using TypeListMappedData =
            viskores::List<viskores::Float32, viskores::Float64, viskores::Vec3f_32, viskores::Vec3f_64>;

        inputField.GetData().CastAndCallForTypesWithFloatFallback<TypeListMappedData, VISKORES_DEFAULT_STORAGE_LIST>(
            [&](const auto &field) {
                using FieldType = typename std::decay_t<decltype(field)>::ValueType;
                constexpr bool fieldIsScalar = std::is_arithmetic_v<FieldType>;

                viskores::cont::ArrayHandle<CoordType> result;
                viskores::Vec<viskores::FloatDefault, COORDS_DIM> mask(0.0f);

                if constexpr (fieldIsScalar) {
                    viskores::IdComponent c = 0; // must be declared outside switch-case
                    switch (m_component) {
                    case DisplaceComponent::X:
                    case DisplaceComponent::Y:
                    case DisplaceComponent::Z:
                        c = static_cast<viskores::IdComponent>(m_component);
                        if (c < COORDS_DIM) {
                            mask[c] = 1.0f;
                        } else {
                            throw viskores::cont::ErrorBadValue(
                                "Error in DisplaceFilter: DisplaceComponent value (" + std::to_string(c) +
                                ") out of bounds for coordinate dimension (" + std::to_string(COORDS_DIM) + ")!");
                        }
                        break;
                    case DisplaceComponent::All:
                        for (auto c = 0; c < COORDS_DIM; c++) {
                            mask[c] = 1.0f;
                        }
                        break;
                    default:
                        throw viskores::cont::ErrorBadValue(
                            "Error in DisplaceFilter: Encountered unknown DisplaceComponent value!");
                    }

                    switch (m_operation) {
                    case DisplaceOperation::Set:
                        this->Invoke(SetDisplaceWorklet<COORDS_DIM>{m_scale, mask}, field, coords, result);
                        break;
                    case DisplaceOperation::Add:
                        this->Invoke(AddDisplaceWorklet<COORDS_DIM>{m_scale, mask}, field, coords, result);
                        break;
                    case DisplaceOperation::Multiply:
                        this->Invoke(MultiplyDisplaceWorklet<COORDS_DIM>{m_scale, mask}, field, coords, result);
                        break;
                    default:
                        throw viskores::cont::ErrorBadValue(
                            "Error in DisplaceFilter: Encountered unknown DisplaceOperation value!");
                    }

                    outputCoords = result;
                } else if constexpr (!fieldIsScalar && FieldType::NUM_COMPONENTS == COORDS_DIM) {
                    switch (m_operation) {
                    case DisplaceOperation::Set:
                        this->Invoke(SetDisplaceWorklet<COORDS_DIM>{m_scale, mask}, field, coords, result);
                        break;
                    case DisplaceOperation::Add:
                        this->Invoke(AddDisplaceWorklet<COORDS_DIM>{m_scale, mask}, field, coords, result);
                        break;
                    case DisplaceOperation::Multiply:
                        this->Invoke(MultiplyDisplaceWorklet<COORDS_DIM>{m_scale, mask}, field, coords, result);
                        break;
                    default:
                        throw viskores::cont::ErrorBadValue(
                            "Error in DisplaceFilter: Encountered unknown DisplaceOperation value!");
                    }

                    outputCoords = result;
                } else {
                    throw viskores::cont::ErrorBadValue(
                        "Error in DisplaceFilter: Cannot apply filter on point coordinates of dimension " +
                        std::to_string(COORDS_DIM) + " with data field of dimension " +
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
