#include "VectorFieldVtkm.h"
#include "filter/VectorFieldFilter.h"

#include <vistle/core/scalar.h>
#include <vistle/core/lines.h>

#include <limits>

MODULE_MAIN(VectorFieldVtkm)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(AttachmentPoint, (Bottom)(Middle)(Top))

VectorFieldVtkm::VectorFieldVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 2, MappedDataHandling::Use)
{
    const auto maxLen = std::numeric_limits<Scalar>::max();

    m_scale = addFloatParameter("scale", "scale factor for vector length", 1.0);
    setParameterMinimum(m_scale, Float(0.0));

    // 0 = Bottom, 1 = Middle, 2 = Top
    m_attachment = addIntParameter("attachment_point", "where to attach line to carrying point",
                                   static_cast<Integer>(Bottom), Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_attachment, AttachmentPoint, );


    m_range = addVectorParameter("range", "allowed length range (before scaling)", ParamVector(0.0, maxLen));
    setParameterMinimum(m_range, ParamVector(0.0, 0.0));
}

std::unique_ptr<viskores::filter::Filter> VectorFieldVtkm::setUpFilter() const
{
    auto filter = std::make_unique<viskores::filter::VectorFieldFilter>();

    const auto range = m_range->getValue();

    filter->SetMinLength(static_cast<viskores::FloatDefault>(range[0]));
    filter->SetMaxLength(static_cast<viskores::FloatDefault>(range[1]));
    filter->SetScale(static_cast<viskores::FloatDefault>(m_scale->getValue()));

    // Pass attachment integer directly (0=Bottom, 1=Middle, 2=Top)
    filter->SetAttachmentPoint(static_cast<viskores::IdComponent>(m_attachment->getValue()));

    return filter;
}

vistle::DataBase::ptr VectorFieldVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                                          const vistle::Object::const_ptr &inputGrid,
                                                          const vistle::DataBase::const_ptr &inputField,
                                                          const std::string &fieldName,
                                                          const vistle::Object::const_ptr &outputGrid) const
{
    if (!outputGrid || !inputField) {
        return vistle::DataBase::ptr();
    }

    // Prefer a field produced by the Viskores filter if present.
    if (dataset.HasField(fieldName)) {
        return VtkmModule::prepareOutputField(dataset, inputGrid, inputField, fieldName, outputGrid);
    }

    // Fall back to duplicating the incoming field onto each line endpoint.
    auto lines = vistle::Lines::as(outputGrid);
    if (!lines) {
        return vistle::DataBase::ptr();
    }

    const auto numLines = lines->getNumElements();
    if (numLines <= 0 || inputField->getSize() < numLines) {
        return vistle::DataBase::ptr();
    }

    const vistle::Index outSize = 2 * numLines;
    vistle::DataBase::ptr mapped = inputField->cloneType();
    mapped->setSize(outSize);

    for (vistle::Index i = 0; i < numLines; ++i) {
        mapped->copyEntry(2 * i, inputField, i);
        mapped->copyEntry(2 * i + 1, inputField, i);
    }

    mapped->setMeta(inputField->meta());
    mapped->copyAttributes(inputField);
    mapped->setGrid(outputGrid);
    updateMeta(mapped);

    return mapped;
}
