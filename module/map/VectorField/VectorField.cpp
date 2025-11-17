
#include <vistle/core/object.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>
#include <vistle/core/coords.h>
#include <vistle/util/math.h>
#include <vistle/alg/objalg.h>

#include "VectorField.h"

#ifdef VECTORFIELDVTKM
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Invoker.h>
#include "filter/VectorFieldWorklet.h"
#endif

DEFINE_ENUM_WITH_STRING_CONVERSIONS(AttachmentPoint, (Bottom)(Middle)(Top))

MODULE_MAIN(VectorField)

using namespace vistle;


VectorField::VectorField(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_gridIn = createInputPort("grid_in", "vector data mapped to grid or geometry");
    m_dataIn = createInputPort("data_in", "mapped data field");
    m_gridOut = createOutputPort("grid_out", "line strokes");
    m_dataOut = createOutputPort("data_out", "line strokes with mapped data");
    linkPorts(m_gridIn, m_gridOut);
    linkPorts(m_gridIn, m_dataOut);
    linkPorts(m_dataIn, m_dataOut);
    setPortOptional(m_dataIn, true);

    auto MaxLength = std::numeric_limits<Scalar>::max();

    m_scale = addFloatParameter("scale", "scale factor for vector length", 1.);
    setParameterMinimum(m_scale, (Float)0.);
    m_attachmentPoint = addIntParameter("attachment_point", "where to attach line to carrying point", (Integer)Bottom,
                                        Parameter::Choice);
    V_ENUM_SET_CHOICES(m_attachmentPoint, AttachmentPoint);
    m_range = addVectorParameter("range", "allowed length range (before scaling)", ParamVector(0., MaxLength));
    setParameterMinimum(m_range, ParamVector(0., 0.));
    m_allCoordinates =
        addIntParameter("all_coordinates", "include all or only referenced coordinates", false, Parameter::Boolean);
}

bool VectorField::compute()
{
    auto vecs = expect<Vec<Scalar, 3>>(m_gridIn);
    if (!vecs) {
        sendError("no vector input");
        return true;
    }
    auto split = splitContainerObject(vecs);
    auto grid = split.geometry;
    if (!grid) {
        sendError("vectors without grid");
        return true;
    }
    auto coords = Coords::as(grid);
    if (!coords) {
        sendError("grid does not contain coordinates");
        return true;
    }
    bool perElement = false;
    if (vecs->guessMapping() == DataBase::Element) {
        perElement = true;
    } else if (vecs->guessMapping() != DataBase::Vertex) {
        sendError("unknown mapping for vectors");
        return true;
    }
    Index numCoords = coords->getNumCoords();
    auto elements = grid->getInterface<ElementInterface>();
    if (perElement) {
        if (!elements) {
            sendError("vectors are mapped per element, but grid does not contain elements");
            return true;
        }
        numCoords = elements->getNumElements();
    }
    if (vecs->getSize() != numCoords) {
        sendError("geometry size does not match array size: #points=%lu, but #vecs=%lu", (unsigned long)numCoords,
                  (unsigned long)vecs->getSize());
        return true;
    }

    DataBase::ptr mapped;
    DataBase::const_ptr data;
    if (isConnected(*m_dataIn) && isConnected(*m_dataOut)) {
        data = expect<DataBase>(m_dataIn);
        if (data) {
            if (data->guessMapping() == DataBase::Element) {
                if (!perElement) {
                    sendError("vectors are per-element, but mapped data is per-vertex");
                    return true;
                }
            } else if (data->guessMapping() == DataBase::Vertex) {
                if (perElement) {
                    sendError("vectors are per-vertex, but mapped data is per-element");
                    return true;
                }
            } else {
                sendError("unknown mapping for data");
                return true;
            }

            if (data->getSize() != numCoords) {
                sendError("geometry size does not match data array size: #vecs=%lu, but #data=%lu",
                          (unsigned long)numCoords, (unsigned long)data->getSize());
                return true;
            }

            mapped = data->cloneType();
        }
    }

    bool indexed = false;
    Index numPoints = numCoords;
    std::vector<Index> verts;
    if (!perElement && !m_allCoordinates->getValue()) {
        Index nconn = 0;
        const Index *cl = nullptr;
        if (auto tri = Triangles::as(split.geometry)) {
            cl = tri->cl().data();
            nconn = tri->cl().size();
        } else if (auto quads = Quads::as(split.geometry)) {
            cl = quads->cl().data();
            nconn = quads->cl().size();
        } else if (auto indexed = Indexed::as(split.geometry)) {
            cl = indexed->cl().data();
            nconn = indexed->cl().size();
        }

        if (cl && nconn > 0) {
            indexed = true;
            verts.reserve(nconn);
            std::copy(cl, cl + nconn, std::back_inserter(verts));
            std::sort(verts.begin(), verts.end());
            auto end = std::unique(verts.begin(), verts.end());
            verts.resize(end - verts.begin());
            numPoints = verts.size();
        }
    }

    Scalar minLen = m_range->getValue()[0];
    Scalar maxLen = m_range->getValue()[1];
    Scalar scale = m_scale->getValue();

    const AttachmentPoint att = (AttachmentPoint)m_attachmentPoint->getValue();

    const auto px = coords->x().data();
    const auto py = coords->y().data();
    const auto pz = coords->z().data();
    const auto vx = vecs->x().data();
    const auto vy = vecs->y().data();
    const auto vz = vecs->z().data();

    Lines::ptr lines;

#ifdef VECTORFIELDVTKM
    // ---- GPU path using Viskores worklet ----
    using ViskVec3 = viskores::Vec<viskores::FloatDefault, 3>;

    std::vector<ViskVec3> gpuVecs;
    std::vector<ViskVec3> gpuCoords;
    gpuVecs.resize(numPoints);
    gpuCoords.resize(numPoints);

    // Precompute the vectors and seed points (vertex or cell center) on the host.
    for (Index i = 0; i < numPoints; ++i) {
        Index ii = indexed ? verts[i] : i;

        // Input vector
        Vector3 v(vx[ii], vy[ii], vz[ii]);
        gpuVecs[i] = ViskVec3(static_cast<viskores::FloatDefault>(v[0]),
                              static_cast<viskores::FloatDefault>(v[1]),
                              static_cast<viskores::FloatDefault>(v[2]));

        // Seed position (vertex position or element center)
        Vector3 p;
        if (perElement) {
            p = elements->cellCenter(ii);
        } else {
            p = Vector3(px[ii], py[ii], pz[ii]);
        }
        gpuCoords[i] = ViskVec3(static_cast<viskores::FloatDefault>(p[0]),
                                static_cast<viskores::FloatDefault>(p[1]),
                                static_cast<viskores::FloatDefault>(p[2]));
    }

    // Create Viskores array handles
    auto vecAH = viskores::cont::make_ArrayHandle(gpuVecs, viskores::CopyFlag::On);
    auto coordAH = viskores::cont::make_ArrayHandle(gpuCoords, viskores::CopyFlag::On);


    viskores::cont::ArrayHandle<ViskVec3> endpointsAH;
    endpointsAH.Allocate(static_cast<viskores::Id>(numPoints * 2));

    // Configure and invoke the worklet
    VectorFieldWorklet worklet;
    worklet.MinLength = static_cast<viskores::FloatDefault>(minLen);
    worklet.MaxLength = static_cast<viskores::FloatDefault>(maxLen);
    worklet.Scale     = static_cast<viskores::FloatDefault>(scale);
    worklet.Attachment =
        static_cast<viskores::IdComponent>(att); // Bottom/Middle/Top -> 0/1/2

    viskores::cont::Invoker invoker;
    invoker(worklet, vecAH, coordAH, endpointsAH);

    auto endpointsPortal = endpointsAH.ReadPortal();

    // Build Vistle Lines from the GPU-computed endpoints
    lines.reset(new Lines(numPoints, 2 * numPoints, 2 * numPoints));
    auto lx = lines->x().data(), ly = lines->y().data(), lz = lines->z().data();
    auto el = lines->el().data(), cl = lines->cl().data();

    el[0] = 0;
    for (Index i = 0; i < numPoints; ++i) {
        const auto p0 = endpointsPortal.Get(static_cast<viskores::Id>(2) * i);
        const auto p1 = endpointsPortal.Get(static_cast<viskores::Id>(2) * i + 1);

        lx[2 * i]     = p0[0];
        ly[2 * i]     = p0[1];
        lz[2 * i]     = p0[2];
        lx[2 * i + 1] = p1[0];
        ly[2 * i + 1] = p1[1];
        lz[2 * i + 1] = p1[2];

        cl[2 * i]     = 2 * i;
        cl[2 * i + 1] = 2 * i + 1;
        el[i + 1]     = 2 * (i + 1);
    }

#else
    // ---- Original CPU path ----
    lines.reset(new Lines(numPoints, 2 * numPoints, 2 * numPoints));
    auto lx = lines->x().data(), ly = lines->y().data(), lz = lines->z().data();
    auto el = lines->el().data(), cl = lines->cl().data();


    el[0] = 0;
    for (Index i = 0; i < numPoints; ++i) {
        Index ii = indexed ? verts[i] : i;
        Vector3 v(vx[ii], vy[ii], vz[ii]);
        Scalar l = v.norm();
        if (l < minLen && l > 0) {
            v *= minLen / l;
        } else if (l > maxLen) {
            v *= maxLen / l;
        }
        v *= scale;

        Vector3 p;
        if (perElement) {
            p = elements->cellCenter(ii);
        } else {
            p = Vector3(px[ii], py[ii], pz[ii]);
        }
        Vector3 p0, p1;
        switch (att) {
        case Bottom:
            p0 = p;
            p1 = p + v;
            break;
        case Middle:
            p0 = p - 0.5 * v;
            p1 = p + 0.5 * v;
            break;
        case Top:
            p0 = p - v;
            p1 = p;
            break;
        default:
            assert(!"invalid AttachmentPoint");
            return false;
        }

        lx[2 * i] = p0[0];
        ly[2 * i] = p0[1];
        lz[2 * i] = p0[2];
        lx[2 * i + 1] = p1[0];
        ly[2 * i + 1] = p1[1];
        lz[2 * i + 1] = p1[2];
        cl[2 * i] = 2 * i;
        cl[2 * i + 1] = 2 * i + 1;
        el[i + 1] = 2 * (i + 1);
    }
#endif

    // Common post-processing (unchanged)
    lines->setMeta(vecs->meta());
    lines->setTimestep(split.timestep);
    lines->copyAttributes(coords);
    lines->copyAttributes(vecs);
    lines->setTransform(coords->getTransform());
    updateMeta(lines);

    if (isConnected(*m_gridOut)) {
        addObject(m_gridOut, lines);
    }
    if (isConnected(*m_dataOut) && mapped) {
        mapped->setSize(numPoints * 2);
        for (Index i = 0; i < numPoints; ++i) {
            Index ii = indexed ? verts[i] : i;
            mapped->copyEntry(i * 2, data, ii);
            mapped->copyEntry(i * 2 + 1, data, ii);
        }
        mapped->setMeta(data->meta());
        mapped->copyAttributes(data);

        mapped->setGrid(lines);
        updateMeta(mapped);
        addObject(m_dataOut, mapped);
    }

    return true;
}
