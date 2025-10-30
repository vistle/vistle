#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H

#include <viskores/filter/Filter.h>

#include "DisplaceWorklet.h"

class DisplaceFilter: public viskores::filter::Filter {
private:
    vistle::DisplaceComponent m_component;
    vistle::DisplaceOperation m_operation;
    viskores::FloatDefault m_scale;

public:
    VISKORES_CONT DisplaceFilter();

    VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &dataset) override;

    VISKORES_CONT void SetDisplacementComponent(const vistle::DisplaceComponent &component) { m_component = component; }
    VISKORES_CONT void SetDisplacementOperation(const vistle::DisplaceOperation &operation) { m_operation = operation; }
    VISKORES_CONT void SetScale(const viskores::FloatDefault &scale) { m_scale = scale; }

    VISKORES_CONT vistle::DisplaceComponent GetDisplacementComponent() const { return m_component; }
    VISKORES_CONT vistle::DisplaceOperation GetDisplacementOperation() const { return m_operation; }
    VISKORES_CONT viskores::FloatDefault GetScale() const { return m_scale; }
};

#endif
