#include "VectorFieldVtkm.h"
#include "filter/VectorFieldFilter.h"

#include <vistle/core/scalar.h>
#include <vistle/core/lines.h>
#include <vistle/core/vec.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>

#include <vistle/vtkm/convert.h> 

#include <algorithm>
#include <limits>
#include <vector>

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

    m_outputMapping = DataBase::Unspecified;
    m_numLines = 0;

    // Pull p0 and p1 back from Viskores as Vistle Vec<Scalar,3>
    auto p0db = vtkmGetField(dataset, "p0", DataBase::Vertex, false);
    auto p1db = vtkmGetField(dataset, "p1", DataBase::Vertex, false);
    if (p0db && p1db) {
        m_outputMapping = DataBase::Vertex;
    } else {
        p0db = vtkmGetField(dataset, "p0", DataBase::Element, false);
        p1db = vtkmGetField(dataset, "p1", DataBase::Element, false);
        if (p0db && p1db) {
            m_outputMapping = DataBase::Element;
        }
    }

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

    m_selectedVertices.clear();
    m_useSelectedVertices = false;
    Index numPoints = n;

    if (m_outputMapping == DataBase::Vertex && !m_allCoords->getValue() && inputGrid) {
        Index nconn = 0;
        const Index *cl = nullptr;
        if (auto tri = Triangles::as(inputGrid)) {
            cl = tri->cl().data();
            nconn = tri->cl().size();
        } else if (auto quads = Quads::as(inputGrid)) {
            cl = quads->cl().data();
            nconn = quads->cl().size();
        } else if (auto indexed = Indexed::as(inputGrid)) {
            cl = indexed->cl().data();
            nconn = indexed->cl().size();
        }

        if (cl && nconn > 0) {
            m_selectedVertices.reserve(nconn);
            std::copy(cl, cl + nconn, std::back_inserter(m_selectedVertices));
            std::sort(m_selectedVertices.begin(), m_selectedVertices.end());
            auto end = std::unique(m_selectedVertices.begin(), m_selectedVertices.end());
            m_selectedVertices.resize(end - m_selectedVertices.begin());
            numPoints = m_selectedVertices.size();
            m_useSelectedVertices = true;
        }
    }

    // One line per input point/cell -> 2 vertices per line
    Lines::ptr lines(new Lines(numPoints, 2 * numPoints, 2 * numPoints));

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

    for (Index i = 0; i < numPoints; ++i) {
        const Index sourceIdx = m_useSelectedVertices ? m_selectedVertices[i] : i;
        const Index i0 = 2 * i;
        const Index i1 = 2 * i + 1;

        lx[i0] = p0x[sourceIdx];
        ly[i0] = p0y[sourceIdx];
        lz[i0] = p0z[sourceIdx];

        lx[i1] = p1x[sourceIdx];
        ly[i1] = p1y[sourceIdx];
        lz[i1] = p1z[sourceIdx];

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

    updateMeta(lines);

    m_numLines = numPoints;
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
    if (m_outputMapping == vistle::DataBase::Vertex) {
        if (mapping != vistle::DataBase::Vertex) {
            return vistle::DataBase::ptr();
        }
        if (m_useSelectedVertices && !m_selectedVertices.empty() &&
            m_selectedVertices.back() >= inputField->getSize()) {
            return vistle::DataBase::ptr();
        }
        if (!m_useSelectedVertices && inputField->getSize() < m_numLines) {
            return vistle::DataBase::ptr();
        }
    } else if (m_outputMapping == vistle::DataBase::Element) {
        if (mapping != vistle::DataBase::Element) {
            return vistle::DataBase::ptr();
        }
        if (inputField->getSize() < m_numLines) {
            return vistle::DataBase::ptr();
        }
    } else {
        return vistle::DataBase::ptr();
    }

    const vistle::Index n       = m_numLines;  // number of original points / vectors
    const vistle::Index outSize = 2 * n;       // one value per line endpoint

    vistle::DataBase::ptr mapped = inputField->cloneType();
    mapped->setSize(outSize);

    for (vistle::Index i = 0; i < n; ++i) {
        const vistle::Index sourceIdx =
            (m_outputMapping == vistle::DataBase::Vertex && m_useSelectedVertices) ? m_selectedVertices[i] : i;

        mapped->copyEntry(2 * i,     inputField, sourceIdx);
        mapped->copyEntry(2 * i + 1, inputField, sourceIdx);
    }

    mapped->setMeta(inputField->meta());
    mapped->copyAttributes(inputField);
    mapped->setGrid(outputGrid);
    updateMeta(mapped);

    return mapped;
}
