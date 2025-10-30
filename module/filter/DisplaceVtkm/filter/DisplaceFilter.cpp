#include "DisplaceFilter.h"

VISKORES_CONT DisplaceFilter::DisplaceFilter()
: m_component(vistle::DisplaceComponent::X), m_operation(vistle::DisplaceOperation::Add), m_scale(1.0f)
{}

VISKORES_CONT viskores::cont::DataSet DisplaceFilter::DoExecute(const viskores::cont::DataSet &dataset)
{
    auto inputScalar = this->GetFieldFromDataSet(dataset);
    auto inputCoords = dataset.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

    auto coordsArray = inputCoords.GetData();

    viskores::cont::UnknownArrayHandle resultCoords;

    this->CastAndCallVecField<3>(coordsArray, [&](const auto &concreteCoords) {
        using CoordsArrayType = std::decay_t<decltype(concreteCoords)>;
        using CoordType = typename CoordsArrayType::ValueType;

        this->CastAndCallScalarField(inputScalar.GetData(), [&](const auto &concreteScalars) {
            viskores::cont::ArrayHandle<CoordType> resultArray;

            this->Invoke(vistle::DisplaceVtkmWorklet{m_component, m_operation, m_scale}, concreteScalars,
                         concreteCoords, resultArray);

            resultCoords = resultArray;
        });
    });

    auto outputName = this->GetOutputFieldName();
    if (outputName == "")
        outputName = inputScalar.GetName() + "_displaced";

    return this->CreateResultCoordinateSystem(
        dataset, dataset.GetCellSet(), inputCoords.GetName(), resultCoords,
        [&](viskores::cont::DataSet &out, const viskores::cont::Field &fieldToPass) {
            out.AddField(viskores::cont::Field(outputName, fieldToPass.GetAssociation(), fieldToPass.GetData()));
        });
}
