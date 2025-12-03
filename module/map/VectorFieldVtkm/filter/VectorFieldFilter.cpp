
#include "VectorFieldFilter.h"
#include "VectorFieldWorklet.h"

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/ArrayHandle.h>

#include <limits>
#include <type_traits>

namespace viskores
{
namespace filter
{

VISKORES_CONT VectorFieldFilter::VectorFieldFilter()
    : MinLength(0.0f)
    , MaxLength(static_cast<viskores::FloatDefault>(
          std::numeric_limits<float>::max()))
    , Scale(1.0f)
    , Attachment(0)
{
    this->SetOutputFieldName("");
}

VISKORES_CONT void VectorFieldFilter::SetMinLength(viskores::FloatDefault minLen)
{
    MinLength = minLen;
}

VISKORES_CONT void VectorFieldFilter::SetMaxLength(viskores::FloatDefault maxLen)
{
    MaxLength = maxLen;
}

VISKORES_CONT void VectorFieldFilter::SetScale(viskores::FloatDefault scale)
{
    Scale = scale;
}

VISKORES_CONT void VectorFieldFilter::SetAttachmentPoint(viskores::IdComponent attachment)
{
    Attachment = attachment;
}

VISKORES_CONT viskores::FloatDefault VectorFieldFilter::GetMinLength() const
{
    return MinLength;
}

VISKORES_CONT viskores::FloatDefault VectorFieldFilter::GetMaxLength() const
{
    return MaxLength;
}

VISKORES_CONT viskores::FloatDefault VectorFieldFilter::GetScale() const
{
    return Scale;
}

VISKORES_CONT viskores::IdComponent VectorFieldFilter::GetAttachmentPoint() const
{
    return Attachment;
}

VISKORES_CONT viskores::cont::DataSet
VectorFieldFilter::DoExecute(const viskores::cont::DataSet &inDataSet)
{
    using namespace viskores::cont;

    Field inField = inDataSet.GetField(0);


    // Only point-associated 3D vector fields; otherwise just pass through.
    if (inField.GetAssociation() != Field::Association::Points) {
        return this->CreateResult(inDataSet);
    }

    CoordinateSystem coords = inDataSet.GetCoordinateSystem(0);

    DataSet result;
    bool success = false;

    auto resolveType = [&](const auto &inputArray)
    {
        using ArrayHandleType = std::decay_t<decltype(inputArray)>;
        using VecType         = typename ArrayHandleType::ValueType;

        // Copy coordinates into a simple VecType array, independent of storage.
        ArrayHandle<VecType> baseArray;
        ArrayCopy(coords.GetData(), baseArray);

        ArrayHandle<VecType> p0Array;
        ArrayHandle<VecType> p1Array;

        viskores::worklet::VectorFieldWorklet worklet(
            MinLength, MaxLength, Scale, Attachment);

        this->Invoke(worklet, inputArray, baseArray, p0Array, p1Array);

        auto outDataSet = this->CreateResult(inDataSet);

        // Fixed names; Vistle side will look for these.
        outDataSet.AddField(Field("p0", Field::Association::Points, p0Array));
        outDataSet.AddField(Field("p1", Field::Association::Points, p1Array));

        result  = outDataSet;
        success = true;
    };

    // Handle 3-component vector fields
    this->CastAndCallVecField<3>(inField, resolveType);

    if (!success) {
        return this->CreateResult(inDataSet);
    }

    return result;
}


} // namespace filter
} // namespace viskores
