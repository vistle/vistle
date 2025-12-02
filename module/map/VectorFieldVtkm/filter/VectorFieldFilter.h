#ifndef VISTLE_VECTORFIELDVTKM_VECTORFIELDFILTER_H
#define VISTLE_VECTORFIELDVTKM_VECTORFIELDFILTER_H

#include <viskores/Types.h>
#include <viskores/filter/Filter.h>
#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace filter
{

class VectorFieldFilter : public viskores::filter::Filter
{
public:
    VISKORES_CONT VectorFieldFilter();

    VISKORES_CONT void SetMinLength(viskores::FloatDefault minLen);
    VISKORES_CONT void SetMaxLength(viskores::FloatDefault maxLen);
    VISKORES_CONT void SetScale(viskores::FloatDefault scale);
    VISKORES_CONT void SetAttachmentPoint(viskores::IdComponent attachment);

    VISKORES_CONT viskores::FloatDefault GetMinLength() const;
    VISKORES_CONT viskores::FloatDefault GetMaxLength() const;
    VISKORES_CONT viskores::FloatDefault GetScale() const;
    VISKORES_CONT viskores::IdComponent  GetAttachmentPoint() const;

private:
    VISKORES_CONT viskores::cont::DataSet DoExecute(
        const viskores::cont::DataSet &inDataSet) override;

    viskores::FloatDefault MinLength;
    viskores::FloatDefault MaxLength;
    viskores::FloatDefault Scale;
    viskores::IdComponent  Attachment; // 0=Bottom, 1=Middle, 2=Top
};

} // namespace filter
} // namespace viskores

#endif // VISTLE_VECTORFIELDVTKM_VECTORFIELDFILTER_H