
#include "VectorFieldFilter.h"
#include "VectorFieldWorklet.h"

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Field.h>
#include <viskores/CellShape.h>
#include <viskores/filter/field_conversion/worklet/CellAverage.h>

#include <limits>
#include <type_traits>

namespace viskores {
namespace filter {

VISKORES_CONT VectorFieldFilter::VectorFieldFilter()
: MinLength(0.0f)
, MaxLength(static_cast<viskores::FloatDefault>(std::numeric_limits<float>::max()))
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

VISKORES_CONT viskores::cont::DataSet VectorFieldFilter::DoExecute(const viskores::cont::DataSet &inDataSet)
{
    using namespace viskores::cont;

    Field inField = inDataSet.GetField(0);
    const auto association = inField.GetAssociation();

    // Only point- or cell-associated 3D vector fields; otherwise just pass through.
    if (association != Field::Association::Points && association != Field::Association::Cells) {
        return this->CreateResult(inDataSet);
    }

    CoordinateSystem coords = inDataSet.GetCoordinateSystem(0);
    auto cellSet = inDataSet.GetCellSet();

    DataSet result;
    bool success = false;

    auto resolveType = [&](const auto &inputArray) {
        using ArrayHandleType = std::decay_t<decltype(inputArray)>;
        using VecType = typename ArrayHandleType::ValueType;
        using CoordType = viskores::Vec<vistle::Scalar, 3>;

        const viskores::Id numLines = inputArray.GetNumberOfValues();
        if (numLines == 0) {
            return;
        }

        const viskores::Id numOutCoords = numLines * 2;
        // Guard against overflow or unexpected allocation size.
        if (numOutCoords < 0 || numOutCoords / 2 != numLines) {
            return;
        }

        ArrayHandle<CoordType> outCoords;
        outCoords.Allocate(numOutCoords);
        if (outCoords.GetNumberOfValues() != numOutCoords) {
            return;
        }

        ArrayHandle<VecType> baseArray;
        if (association == Field::Association::Points) {
            // Copy coordinates into a simple VecType array, independent of storage.
            ArrayCopy(coords.GetData(), baseArray);
        } else if (association == Field::Association::Cells) {
            // Compute cell centers as base positions.
            viskores::worklet::CellAverage centers;
            this->Invoke(centers, cellSet, coords.GetData(), baseArray);
        } else {
            return;
        }

        // Write both endpoints directly into the output coordinate array to avoid intermediate p0/p1 storage.
        viskores::worklet::VectorFieldWorklet worklet(MinLength, MaxLength, Scale, Attachment);
        this->Invoke(worklet, inputArray, baseArray, outCoords);

        const viskores::Id numPoints = 2 * numLines;
        viskores::cont::ArrayHandle<viskores::Id> connectivity;
        viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, 1, numPoints),
                                  connectivity);
        viskores::cont::CellSetSingleType<> cellSet;
        cellSet.Fill(numPoints, viskores::CELL_SHAPE_LINE, 2, connectivity);

        viskores::cont::DataSet outDataSet;
        outDataSet.SetCellSet(cellSet);
        outDataSet.AddCoordinateSystem(CoordinateSystem("coords", outCoords));

        // Duplicate the input field values to each line endpoint on-device so the Vistle module
        // can consume it without a host-side copy.
        ArrayHandle<VecType> expandedField;
        expandedField.Allocate(numOutCoords);
        if (expandedField.GetNumberOfValues() == numOutCoords) {
            viskores::worklet::DuplicateFieldToEndpoints expandField;
            this->Invoke(expandField, inputArray, expandedField);
            outDataSet.AddField(Field(inField.GetName(), Field::Association::Points, expandedField));
        }

        result = outDataSet;
        success = true;
    };

    // Handle 3-component vector fields
    this->CastAndCallVecField<3>(inField, resolveType);

    if (!success) {
        return this->CreateResult(inDataSet);
    }

    // Map remaining selected fields to endpoints on-device.
    const auto &fieldsToPass = this->GetFieldsToPass();
    const auto activeName = inField.GetName();
    for (viskores::IdComponent fi = 0; fi < inDataSet.GetNumberOfFields(); ++fi) {
        auto field = inDataSet.GetField(fi);
        if (field.GetName() == activeName) {
            continue; // already handled
        }
        if (!fieldsToPass.IsFieldSelected(field)) {
            continue;
        }
        const auto assoc = field.GetAssociation();
        if (assoc != viskores::cont::Field::Association::Points && assoc != viskores::cont::Field::Association::Cells) {
            continue;
        }
        if (assoc != association) {
            // FIXME try to convert?
            continue; // different association than active field
        }

        auto mapField = [&](const auto &array) {
            using ArrayHandleType = std::decay_t<decltype(array)>;
            using ValueType = typename ArrayHandleType::ValueType;

            const viskores::Id numValues = array.GetNumberOfValues();
            const viskores::Id outSize = numValues * 2;
            if (outSize < 0 || outSize / 2 != numValues) {
                return;
            }

            viskores::cont::ArrayHandle<ValueType> expanded;
            expanded.Allocate(outSize);
            if (expanded.GetNumberOfValues() != outSize) {
                return;
            }

            viskores::worklet::DuplicateFieldToEndpoints duplicate;
            this->Invoke(duplicate, array, expanded);

            result.AddField(
                viskores::cont::Field(field.GetName(), viskores::cont::Field::Association::Points, expanded));
        };

        // Normalize storage so SOA etc. are handled.
        viskores::cont::UnknownArrayHandle data = field.GetData();
        viskores::cont::UnknownArrayHandle basic = data.NewInstanceBasic();
        viskores::cont::ArrayCopy(data, basic);
        basic.CastAndCallForTypesWithFloatFallback<viskores::TypeListField, VISKORES_DEFAULT_STORAGE_LIST>(mapField);
    }

    return result;
}

} // namespace filter
} // namespace viskores
