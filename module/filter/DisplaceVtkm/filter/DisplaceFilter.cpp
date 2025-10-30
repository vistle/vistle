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

            this->Invoke(vistle::DisplaceVtkmWorklet{m_component, m_operation, m_scale}, scalars, coords, result);

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
