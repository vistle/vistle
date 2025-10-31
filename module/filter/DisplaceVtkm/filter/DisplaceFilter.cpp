#include <string>

#include "DisplaceFilter.h"
#include "DisplaceWorklet.h"

VISKORES_CONT DisplaceFilter::DisplaceFilter()
: m_component(DisplaceComponent::X), m_operation(DisplaceOperation::Add), m_scale(1.0f)
{}

VISKORES_CONT viskores::cont::DataSet DisplaceFilter::DoExecute(const viskores::cont::DataSet &inputDataset)
{
    auto inputScalar = this->GetFieldFromDataSet(inputDataset);
    auto inputCoords = inputDataset.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

    viskores::cont::UnknownArrayHandle outputCoords;

    this->CastAndCallVecField<3>(inputCoords.GetData(), [&](const auto &coords) {
        using CoordsArrayType = std::decay_t<decltype(coords)>;
        using CoordType = typename CoordsArrayType::ValueType;
        constexpr int N = CoordType::NUM_COMPONENTS;

        this->CastAndCallScalarField(inputScalar.GetData(), [&](const auto &scalars) {
            viskores::cont::ArrayHandle<CoordType> result;

            viskores::Vec<viskores::FloatDefault, N> mask(0.0f);

            viskores::IdComponent c = 0; // must be declared outside switch-case
            switch (m_component) {
            case DisplaceComponent::X:
            case DisplaceComponent::Y:
            case DisplaceComponent::Z:
                c = static_cast<viskores::IdComponent>(m_component);
                if (c < N) {
                    mask[c] = 1.0f;
                } else {
                    throw viskores::cont::ErrorBadValue(
                        "Error in DisplaceFilter: DisplaceComponent value (" + std::to_string(c) +
                        ") out of bounds for coordinate dimension (" + std::to_string(N) + ")!");
                }
                break;
            case DisplaceComponent::All:
                for (auto c = 0; c < N; c++) {
                    mask[c] = 1.0f;
                }
                break;
            default:
                throw viskores::cont::ErrorBadValue(
                    "Error in DisplaceFilter: Encountered unknown DisplaceComponent value!");
            }

            switch (m_operation) {
            case DisplaceOperation::Set:
                this->Invoke(SetDisplaceWorklet<N>{m_scale, mask}, scalars, coords, result);
                break;
            case DisplaceOperation::Add:
                this->Invoke(AddDisplaceWorklet<N>{m_scale, mask}, scalars, coords, result);
                break;
            case DisplaceOperation::Multiply:
                this->Invoke(MultiplyDisplaceWorklet<N>{m_scale, mask}, scalars, coords, result);
                break;
            default:
                throw viskores::cont::ErrorBadValue(
                    "Error in DisplaceFilter: Encountered unknown DisplaceOperation value!");
            }

            outputCoords = result;
        });
    });

    auto outputFieldName = this->GetOutputFieldName();
    if (outputFieldName == "")
        outputFieldName = inputScalar.GetName() + "_displaced";

    return this->CreateResultCoordinateSystem(
        inputDataset, inputDataset.GetCellSet(), inputCoords.GetName(), outputCoords,
        [&](viskores::cont::DataSet &out, const viskores::cont::Field &fieldToPass) {
            out.AddField(viskores::cont::Field(
                fieldToPass.GetName() == this->GetActiveFieldName() ? outputFieldName : fieldToPass.GetName(),
                fieldToPass.GetAssociation(), fieldToPass.GetData()));
        });
}
