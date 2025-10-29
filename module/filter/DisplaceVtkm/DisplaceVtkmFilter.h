#ifndef VISTLE_DISPLACEVTKM_DISPLACEVTKMFILTER_H
#define VISTLE_DISPLACEVTKM_DISPLACEVTKMFILTER_H

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/filter/Filter.h>

#include <vistle/util/enum.h>

namespace vistle {
DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceComponent, (X)(Y)(Z)(All))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceOperation, (Set)(Add)(Multiply))

template<typename T, typename S>
VISKORES_EXEC T applyOperation(const T &value, const S &displacement, DisplaceOperation operation)
{
    switch (operation) {
    case Set:
        return displacement;
    case Add:
        return value + displacement;
    case Multiply:
        return value * displacement;
    default:
        std::cerr << "DisplaceVtkmWorklet: Unknown DisplaceOperation " << static_cast<int>(operation) << std::endl;
        return value;
    }
}

struct DisplaceVtkmWorklet: viskores::worklet::WorkletMapField {
    DisplaceComponent m_component;
    DisplaceOperation m_operation;
    viskores::FloatDefault m_scale;

    VISKORES_CONT DisplaceVtkmWorklet()
    : m_component(DisplaceComponent::X), m_operation(DisplaceOperation::Add), m_scale(1.0f)
    {}

    VISKORES_CONT DisplaceVtkmWorklet(DisplaceComponent component, DisplaceOperation operation,
                                      viskores::FloatDefault scale)
    : m_component(component), m_operation(operation), m_scale(scale)
    {}

    using ControlSignature = void(FieldIn scalarField, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &scalar, const viskores::Vec<S, 3> &coord,
                                  viskores::Vec<S, 3> &displacedCoord) const
    {
        displacedCoord = coord;
        switch (m_component) {
        case X:
            displacedCoord[0] = applyOperation(coord[0], scalar * m_scale, m_operation);
            break;
        case Y:
            displacedCoord[1] = applyOperation(coord[1], scalar * m_scale, m_operation);
            break;
        case Z:
            displacedCoord[2] = applyOperation(coord[2], scalar * m_scale, m_operation);
            break;
        case All:
            for (int c = 0; c < 3; ++c) {
                displacedCoord[c] = applyOperation(coord[c], scalar * m_scale, m_operation);
            }
            break;
        }
    }
};

class DisplaceVtkmFilter: public viskores::filter::Filter {
private:
    DisplaceComponent m_component;
    DisplaceOperation m_operation;
    viskores::FloatDefault m_scale;

public:
    VISKORES_CONT DisplaceVtkmFilter()
    : m_component(DisplaceComponent::X), m_operation(DisplaceOperation::Add), m_scale(1.0f)
    {}

    VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &dataset) override
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

    VISKORES_CONT void SetDisplacementComponent(const DisplaceComponent &component) { m_component = component; }
    VISKORES_CONT void SetDisplacementOperation(const DisplaceOperation &operation) { m_operation = operation; }
    VISKORES_CONT void SetScale(const viskores::FloatDefault &scale) { m_scale = scale; }

    VISKORES_CONT DisplaceComponent GetDisplacementComponent() const { return m_component; }
    VISKORES_CONT DisplaceOperation GetDisplacementOperation() const { return m_operation; }
    VISKORES_CONT viskores::FloatDefault GetScale() const { return m_scale; }
};

} // namespace vistle

#endif // VISTLE_DISPLACEVTKM_DISPLACEVTKMFILTER_H
