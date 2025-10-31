#include "DisplaceFilter.h"

VISKORES_CONT DisplaceFilter::DisplaceFilter()
: m_component(vistle::DisplaceComponent::X), m_operation(vistle::DisplaceOperation::Add), m_scale(1.0f)
{}

VISKORES_CONT viskores::cont::DataSet DisplaceFilter::DoExecute(const viskores::cont::DataSet &inputDataset)
{
    auto inputScalar = this->GetFieldFromDataSet(inputDataset);
    auto inputCoords = inputDataset.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

    viskores::cont::UnknownArrayHandle outputCoords;

    this->CastAndCallVecField<3>(inputCoords.GetData(), [&](const auto &coords) {
        using CoordsArrayType = std::decay_t<decltype(coords)>;
        using CoordType = typename CoordsArrayType::ValueType;

        this->CastAndCallScalarField(inputScalar.GetData(), [&](const auto &scalars) {
            viskores::cont::ArrayHandle<CoordType> result;

            viskores::Vec<viskores::FloatDefault, 3> mask{0.0f, 0.0f, 0.0f};
            switch (m_component) {
            case vistle::DisplaceComponent::X:
                mask[0] = 1.0f;
                break;
            case vistle::DisplaceComponent::Y:
                mask[1] = 1.0f;
                break;
            case vistle::DisplaceComponent::Z:
                mask[2] = 1.0f;
                break;
            case vistle::DisplaceComponent::All:
                mask[0] = 1.0f;
                mask[1] = 1.0f;
                mask[2] = 1.0f;
                break;
            default:
                throw viskores::cont::ErrorBadValue(
                    "Error in DisplaceFilter: Encountered unknown DisplaceComponent value!");
            }

            switch (m_operation) {
            case vistle::DisplaceOperation::Set:
                this->Invoke(SetDisplaceWorklet{m_scale, mask}, scalars, coords, result);
                break;
            case vistle::DisplaceOperation::Add:
                this->Invoke(AddDisplaceWorklet{m_scale, mask}, scalars, coords, result);
                break;
            case vistle::DisplaceOperation::Multiply:
                this->Invoke(MultiplyDisplaceWorklet{m_scale, mask}, scalars, coords, result);
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
            out.AddField(viskores::cont::Field(outputFieldName, fieldToPass.GetAssociation(), fieldToPass.GetData()));
        });
}
