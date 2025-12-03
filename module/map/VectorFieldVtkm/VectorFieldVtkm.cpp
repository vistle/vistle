#include "VectorFieldVtkm.h"
#include "filter/VectorFieldFilter.h"

#include <vistle/core/scalar.h>
#include <vistle/core/lines.h>
#include <vistle/core/vec.h>

#include <vistle/vtkm/convert.h> 

#include <limits>

MODULE_MAIN(VectorFieldVtkm)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(AttachmentPoint, (Bottom)(Middle)(Top))

VectorFieldVtkm::VectorFieldVtkm(const std::string &name,
                                 int moduleID,
                                 mpi::communicator comm)
: VtkmModule(name, moduleID, comm,
             2, MappedDataHandling::Use)
{
    const auto maxLen = std::numeric_limits<Scalar>::max();

    m_scale = addFloatParameter("scale",
                                "scale factor for vector length",
                                1.0);
    setParameterMinimum(m_scale, Float(0.0));

    // 0 = Bottom, 1 = Middle, 2 = Top
    m_attachment = addIntParameter("attachment_point",
                               "where to attach line to carrying point",
                               static_cast<Integer>(Bottom),
                               Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_attachment, AttachmentPoint,);


    m_range = addVectorParameter("range",
                                 "allowed length range (before scaling)",
                                 ParamVector(0.0, maxLen));
    setParameterMinimum(m_range, ParamVector(0.0, 0.0));

    m_allCoords = addIntParameter("all_coordinates",
                                  "include all or only referenced coordinates",
                                  0,
                                  Parameter::Boolean);
}

std::unique_ptr<viskores::filter::Filter> VectorFieldVtkm::setUpFilter() const
{
    auto filter = std::make_unique<viskores::filter::VectorFieldFilter>();

    const auto range = m_range->getValue();

    filter->SetMinLength(static_cast<viskores::FloatDefault>(range[0]));
    filter->SetMaxLength(static_cast<viskores::FloatDefault>(range[1]));
    filter->SetScale(static_cast<viskores::FloatDefault>(m_scale->getValue()));

    // Pass attachment integer directly (0=Bottom, 1=Middle, 2=Top)
    filter->SetAttachmentPoint(
        static_cast<viskores::IdComponent>(m_attachment->getValue()));

    return filter;
}


// Build Lines geometry from p0/p1 fields
vistle::Object::const_ptr
VectorFieldVtkm::prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                   const vistle::Object::const_ptr &inputGrid) const
{
    using vistle::DataBase;

    // Pull p0 and p1 back from Viskores as Vistle Vec<Scalar,3>
    auto p0db = vtkmGetField(dataset, "p0", DataBase::Vertex, false);
    auto p1db = vtkmGetField(dataset, "p1", DataBase::Vertex, false);

    if (!p0db || !p1db) {
        // Fall back to default behavior (no custom grid)
        return vistle::Object::const_ptr();
    }

    auto p0 = Vec<Scalar, 3>::as(p0db);
    auto p1 = Vec<Scalar, 3>::as(p1db);
    if (!p0 || !p1) {
        return vistle::Object::const_ptr();
    }

    const Index n = p0->getSize();
    if (n == 0 || p1->getSize() != n) {
        return vistle::Object::const_ptr();
    }

    // One line per input point -> 2 vertices per line
    Lines::ptr lines(new Lines(n, 2 * n, 2 * n));

    auto lx = lines->x().data();
    auto ly = lines->y().data();
    auto lz = lines->z().data();
    auto el = lines->el().data();
    auto cl = lines->cl().data();

    const Scalar *p0x = p0->x().data();
    const Scalar *p0y = p0->y().data();
    const Scalar *p0z = p0->z().data();
    const Scalar *p1x = p1->x().data();
    const Scalar *p1y = p1->y().data();
    const Scalar *p1z = p1->z().data();

    el[0] = 0;

    for (Index i = 0; i < n; ++i) {
        const Index i0 = 2 * i;
        const Index i1 = 2 * i + 1;

        lx[i0] = p0x[i];
        ly[i0] = p0y[i];
        lz[i0] = p0z[i];

        lx[i1] = p1x[i];
        ly[i1] = p1y[i];
        lz[i1] = p1z[i];

        cl[i0] = i0;
        cl[i1] = i1;
        el[i + 1] = 2 * (i + 1);
    }

    // Copy basic meta from input grid if available
    if (inputGrid) {
        lines->setMeta(inputGrid->meta());
        lines->copyAttributes(inputGrid);
        lines->setTransform(inputGrid->getTransform());
    }

    m_numLines = n;
    return lines;
}


vistle::DataBase::ptr
VectorFieldVtkm::prepareOutputField(const viskores::cont::DataSet &,
                                    const vistle::Object::const_ptr &inputGrid,
                                    const vistle::DataBase::const_ptr &inputField,
                                    const std::string &,
                                    const vistle::Object::const_ptr &outputGrid) const
{
    // No geometry or no input field: nothing to do
    if (!outputGrid || !inputField || m_numLines == 0) {
        return vistle::DataBase::ptr();
    }

    auto mapping = inputField->guessMapping(inputGrid);
    if (mapping != vistle::DataBase::Vertex) {
        
        return vistle::DataBase::ptr();
    }

    const vistle::Index n       = m_numLines;  // number of original points / vectors
    const vistle::Index outSize = 2 * n;       // one value per line endpoint

    vistle::DataBase::ptr mapped = inputField->cloneType();
    mapped->setSize(outSize);

    for (vistle::Index i = 0; i < n; ++i) {
        mapped->copyEntry(2 * i,     inputField, i);
        mapped->copyEntry(2 * i + 1, inputField, i);
    }

    mapped->setMeta(inputField->meta());
    mapped->copyAttributes(inputField);
    mapped->setGrid(outputGrid);
    updateMeta(mapped);

    return mapped;
}
