#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_TPP_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_TPP_H

#include <viskores/cont/ErrorBadValue.h>

#include "DisplaceWorklet.h"

template<int CoordsDim>
viskores::Vec<viskores::FloatDefault, CoordsDim> DisplaceFilter::createMask()
{
    viskores::Vec<viskores::FloatDefault, CoordsDim> mask(0.0f);

    viskores::IdComponent c = 0; // must be declared outside switch-case
    switch (m_component) {
    case DisplaceComponent::X:
    case DisplaceComponent::Y:
    case DisplaceComponent::Z:
        c = static_cast<viskores::IdComponent>(m_component);
        if (c < CoordsDim) {
            mask[c] = 1.0f;
        } else {
            throw viskores::cont::ErrorBadValue("Error in DisplaceFilter: DisplaceComponent value (" +
                                                std::to_string(c) + ") out of bounds for coordinate dimension (" +
                                                std::to_string(CoordsDim) + ")!");
        }
        break;
    case DisplaceComponent::All:
        for (auto c = 0; c < CoordsDim; c++) {
            mask[c] = 1.0f;
        }
        break;
    default:
        throw viskores::cont::ErrorBadValue("Error in DisplaceFilter: Encountered unknown DisplaceComponent value!");
    }

    return mask;
}

template<int CoordsDim, typename FieldArrayType, typename CoordsArrayType, typename ResultArrayType>
void DisplaceFilter::applyDisplaceOperation(const FieldArrayType &field, const CoordsArrayType &inputCoords,
                                            ResultArrayType &resultCoords)
{
    auto mask = createMask<CoordsDim>();
    switch (m_operation) {
    case DisplaceOperation::Set:
        this->Invoke(SetDisplaceWorklet<CoordsDim>{m_scale, mask}, field, inputCoords, resultCoords);
        break;
    case DisplaceOperation::Add:
        this->Invoke(AddDisplaceWorklet<CoordsDim>{m_scale, mask}, field, inputCoords, resultCoords);
        break;
    case DisplaceOperation::Multiply:
        this->Invoke(MultiplyDisplaceWorklet<CoordsDim>{m_scale, mask}, field, inputCoords, resultCoords);
        break;
    default:
        throw viskores::cont::ErrorBadValue("Error in DisplaceFilter: Encountered unknown DisplaceOperation value!");
    }
}

#endif //VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_TPP_H
