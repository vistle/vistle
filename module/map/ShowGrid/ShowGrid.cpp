#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/lines.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/triangles.h>
#include <vistle/module/resultcache.h>
#include <vistle/alg/objalg.h>
#include <vistle/util/coRestraint.h>

#include "ShowGrid.h"

MODULE_MAIN(ShowGrid)

using namespace vistle;

ShowGrid::ShowGrid(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    auto pin = createInputPort("grid_in", "grid or data mapped to grid");
    auto pout = createOutputPort("grid_out", "edges of grid cells");
    linkPorts(pin, pout);

    m_cells = addStringParameter("cells", "show cells with these indices", "all", Parameter::Restraint);
    m_cellScale = addFloatParameter("cell_scale", "factor for scaling cells around their center", 1.);
    m_makeBars =
        addIntParameter("make_bars", "create sequences of bars instead of line strips", true, Parameter::Boolean);

    addIntParameter("normalcells", "Show normal (non ghost) cells", 1, Parameter::Boolean);
    addIntParameter("ghostcells", "Show ghost cells", 0, Parameter::Boolean);

    addIntParameter("tetrahedron", "Show tetrahedron", 1, Parameter::Boolean);
    addIntParameter("pyramid", "Show pyramid", 1, Parameter::Boolean);
    addIntParameter("prism", "Show prism", 1, Parameter::Boolean);
    addIntParameter("hexahedron", "Show hexahedron", 1, Parameter::Boolean);
    addIntParameter("polyhedron", "Show polyhedron", 1, Parameter::Boolean);
    addIntParameter("polygon", "Show polygon", 1, Parameter::Boolean);
    addIntParameter("polyline", "Show polylines/line strips", 1, Parameter::Boolean);
    addIntParameter("quad", "Show quad", 1, Parameter::Boolean);
    addIntParameter("triangle", "Show triangle", 1, Parameter::Boolean);
    addIntParameter("bar", "Show bar", 1, Parameter::Boolean);

    setCurrentParameterGroup("cell range", false);
    m_CellNrMin = addIntParameter("min_cell_index", "show cells starting from this index", -1);
    m_CellNrMax = addIntParameter("max_cell_index", "show cells up to this index", -1);

    addResultCache(m_cache);
}

bool ShowGrid::compute()
{
    std::array<bool, UnstructuredGrid::NUM_TYPES> showTypes;
    showTypes[UnstructuredGrid::BAR] = getIntParameter("bar");
    showTypes[UnstructuredGrid::POLYLINE] = getIntParameter("polyline");
    showTypes[UnstructuredGrid::TRIANGLE] = getIntParameter("triangle");
    showTypes[UnstructuredGrid::QUAD] = getIntParameter("quad");
    showTypes[UnstructuredGrid::TETRAHEDRON] = getIntParameter("tetrahedron");
    showTypes[UnstructuredGrid::PYRAMID] = getIntParameter("pyramid");
    showTypes[UnstructuredGrid::PRISM] = getIntParameter("prism");
    showTypes[UnstructuredGrid::HEXAHEDRON] = getIntParameter("hexahedron");
    showTypes[UnstructuredGrid::POLYGON] = getIntParameter("polygon");
    showTypes[UnstructuredGrid::POINT] = showTypes[UnstructuredGrid::POLYHEDRON] = getIntParameter("polyhedron");
    const bool shownor = getIntParameter("normalcells");
    const bool showgho = getIntParameter("ghostcells");
    const bool bars = getIntParameter("make_bars");

    const Integer cellnrmin = m_CellNrMin->getValue();
    const Integer cellnrmax = m_CellNrMax->getValue();
    coRestraint showCell;
    showCell.add(m_cells->getValue());

    vistle::Scalar scaleFactor = m_cellScale->getValue();

    auto obj = expect<Object>("grid_in");
    auto split = splitContainerObject(obj);
    auto grid = split.geometry;
    if (!grid) {
        sendError("did not receive an input object");
        return true;
    }
    Object::const_ptr input = grid;
    DataBase::ptr data;
    auto mapped = split.mapped;
    bool perElement = mapped && mapped->guessMapping() == DataBase::Element;

    std::vector<Index> remap;
    vistle::Lines::ptr lines;
    bool haveOutput = false;
    if (auto *entry = m_cache.getOrLock(input->getName(), lines)) {
        lines.reset(new vistle::Lines(Object::Initialized));
        auto &ocl = lines->cl();
        auto &oel = lines->el();

        if (scaleFactor != 1.) {
            auto &ox = lines->x();
            auto &oy = lines->y();
            auto &oz = lines->z();
            if (auto unstr = UnstructuredGrid::as(grid)) {
                auto ix = unstr->x().data();
                auto iy = unstr->y().data();
                auto iz = unstr->z().data();
                const Index *icl = unstr->cl().data();

                Index begin = 0, end = unstr->getNumElements();
                if (cellnrmin >= 0) {
                    begin = std::max(cellnrmin, (Integer)begin);
                }
                if (cellnrmax + 1 > 0) {
                    end = std::min(cellnrmax + 1, (Integer)end);
                }

                for (Index index = begin; index < end; ++index) {
                    if (!showCell(index))
                        continue;

                    auto type = unstr->tl()[index];
                    const bool ghost = unstr->isGhost(index);

                    const bool show = ((showgho && ghost) || (shownor && !ghost));
                    if (!show)
                        continue;
                    if (!showTypes[type])
                        continue;

                    auto verts = unstr->cellVertices(index);
                    Vector3 center(0, 0, 0);
                    for (auto v: verts) {
                        Vector3 p(ix[v], iy[v], iz[v]);
                        center += p;
                    }
                    center *= 1.0f / verts.size();
                    auto baseidx = ox.size();
                    for (auto v: verts) {
                        if (!perElement)
                            remap.push_back(v);
                        Vector3 p = center + (Vector3(ix[v], iy[v], iz[v]) - center) * scaleFactor;
                        ox.push_back(p[0]);
                        oy.push_back(p[1]);
                        oz.push_back(p[2]);
                    }

                    const Index begin = unstr->el()[index], end = unstr->el()[index + 1];
                    switch (type) {
                    case UnstructuredGrid::QUAD:
                    case UnstructuredGrid::TRIANGLE:
                    case UnstructuredGrid::TETRAHEDRON:
                    case UnstructuredGrid::PYRAMID:
                    case UnstructuredGrid::PRISM:
                    case UnstructuredGrid::HEXAHEDRON: {
                        const auto numFaces = UnstructuredGrid::NumFaces[type];
                        for (int f = 0; f < numFaces; ++f) {
                            const int nCorners = UnstructuredGrid::FaceSizes[type][f];
                            if (bars) {
                                for (int i = 0; i < nCorners; ++i) {
                                    const Index nv = baseidx + UnstructuredGrid::FaceVertices[type][f][i];
                                    assert(nv < ox.size());
                                    ocl.push_back(nv);
                                    const Index nv1 =
                                        baseidx + UnstructuredGrid::FaceVertices[type][f][(i + 1) % nCorners];
                                    assert(nv1 < ox.size());
                                    ocl.push_back(nv1);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(index);
                                }
                            } else {
                                for (int i = 0; i <= nCorners; ++i) {
                                    const Index nv = baseidx + UnstructuredGrid::FaceVertices[type][f][i % nCorners];
                                    assert(nv < ox.size());
                                    ocl.push_back(nv);
                                }
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        }
                        break;
                    }
                    case UnstructuredGrid::POLYHEDRON: {
                        Index facestart = InvalidIndex;
                        Index term = 0;
                        for (Index i = begin; i < end; ++i) {
                            if (bars) {
                                auto idx = icl[i];
                                if (facestart == InvalidIndex) {
                                    facestart = i;
                                    term = idx;
                                } else if (term == idx) {
                                    facestart = InvalidIndex;
                                }
                                if (facestart != InvalidIndex) {
                                    auto relidx = std::find(verts.begin(), verts.end(), idx) - verts.begin();
                                    ocl.push_back(baseidx + relidx);
                                    auto idx1 = icl[i + 1];
                                    auto relidx1 = std::find(verts.begin(), verts.end(), idx1) - verts.begin();
                                    ocl.push_back(baseidx + relidx1);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(index);
                                }
                            } else {
                                auto idx = icl[i];
                                auto relidx = std::find(verts.begin(), verts.end(), idx) - verts.begin();
                                ocl.push_back(baseidx + relidx);
                                if (facestart == InvalidIndex) {
                                    facestart = i;
                                    term = idx;
                                } else if (term == idx) {
                                    facestart = InvalidIndex;
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(index);
                                }
                            }
                        }
                        break;
                    }
                    case UnstructuredGrid::BAR: {
                        ocl.push_back(baseidx);
                        ocl.push_back(baseidx + 1);
                        oel.push_back(ocl.size());
                        if (perElement)
                            remap.push_back(index);
                        break;
                    }
                    case UnstructuredGrid::POLYLINE: {
                        if (bars) {
                            for (Index i = 0; i + 1 < verts.size(); ++i) {
                                ocl.push_back(baseidx + i);
                                ocl.push_back(baseidx + i + 1);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        } else {
                            for (Index i = 0; i < verts.size(); ++i) {
                                ocl.push_back(baseidx + i);
                            }
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(index);
                        }
                        break;
                    }
                    }
                }
            } else if (auto str = StructuredGridBase::as(grid)) {
                const Index dims[3] = {str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2)};
                Index begin = 0, end = str->getNumElements();
                if (cellnrmin >= 0)
                    begin = std::max(cellnrmin, (Integer)begin);
                if (cellnrmax + 1 > 0)
                    end = std::min(cellnrmax + 1, (Integer)end);

                int d = StructuredGridBase::dimensionality(dims);
                const Byte type = d == 3   ? UnstructuredGrid::HEXAHEDRON
                                  : d == 2 ? UnstructuredGrid::QUAD
                                  : d == 1 ? UnstructuredGrid::BAR
                                           : UnstructuredGrid::POINT;

                for (Index index = begin; index < end; ++index) {
                    if (!showCell(index))
                        continue;
                    if (type == UnstructuredGrid::HEXAHEDRON && !showTypes[UnstructuredGrid::HEXAHEDRON])
                        continue;
                    if (type == UnstructuredGrid::QUAD && !showTypes[UnstructuredGrid::QUAD])
                        continue;
                    bool ghost = str->isGhostCell(index);
                    const bool show = ((showgho && ghost) || (shownor && !ghost));
                    if (!show)
                        continue;
                    auto verts = StructuredGridBase::cellVertices(index, dims);
                    auto center = str->cellCenter(index);
                    auto baseidx = ox.size();
                    for (auto v: verts) {
                        if (!perElement)
                            remap.push_back(v);
                        Vector3 p = center + (str->getVertex(v) - center) * scaleFactor;
                        ox.push_back(p[0]);
                        oy.push_back(p[1]);
                        oz.push_back(p[2]);
                    }
                    const auto numFaces = UnstructuredGrid::NumFaces[type];
                    for (int f = 0; f < numFaces; ++f) {
                        const int nCorners = UnstructuredGrid::FaceSizes[type][f];
                        if (bars) {
                            for (int i = 0; i < nCorners; ++i) {
                                const Index nv = baseidx + UnstructuredGrid::FaceVertices[type][f][i];
                                assert(nv < ox.size());
                                ocl.push_back(nv);
                                const Index nv1 = baseidx + UnstructuredGrid::FaceVertices[type][f][(i + 1) % nCorners];
                                assert(nv1 < ox.size());
                                ocl.push_back(nv1);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        } else {
                            for (int i = 0; i <= nCorners; ++i) {
                                const Index v = baseidx + UnstructuredGrid::FaceVertices[type][f][i % nCorners];
                                ocl.push_back(v);
                            }
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(index);
                        }
                    }
                }
            } else if (auto poly = Polygons::as(grid)) {
                if (showTypes[UnstructuredGrid::QUAD]) {
                    const Index *icl = poly->cl().data();
                    auto ix = poly->x().data();
                    auto iy = poly->y().data();
                    auto iz = poly->z().data();

                    Index begin = 0, end = poly->getNumElements();
                    if (cellnrmin >= 0)
                        begin = std::max(cellnrmin, (Integer)begin);
                    if (cellnrmax + 1 > 0)
                        end = std::min(cellnrmax + 1, (Integer)end);

                    for (Index index = begin; index < end; ++index) {
                        if (!showCell(index))
                            continue;
                        const bool ghost = poly->isGhost(index);
                        const bool show = ((showgho && ghost) || (shownor && !ghost));
                        if (!show)
                            continue;

                        const Index begin = poly->el()[index], end = poly->el()[index + 1];

                        std::vector<Index> verts;
                        for (Index i = begin; i < end; ++i) {
                            verts.push_back(icl[i]);
                        }
                        Vector3 center(0, 0, 0);
                        for (auto v: verts) {
                            Vector3 p(ix[v], iy[v], iz[v]);
                            center += p;
                        }
                        center *= 1.0f / verts.size();
                        auto baseidx = ox.size();
                        for (auto v: verts) {
                            if (!perElement)
                                remap.push_back(v);
                            Vector3 p = center + (Vector3(ix[v], iy[v], iz[v]) - center) * scaleFactor;
                            ox.push_back(p[0]);
                            oy.push_back(p[1]);
                            oz.push_back(p[2]);
                        }

                        if (bars) {
                            for (Index i = begin; i < end; ++i) {
                                ocl.push_back(baseidx + i - begin);
                                ocl.push_back(baseidx + (i - begin + 1) % (end - begin));
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        } else {
                            for (Index i = begin; i < end; ++i) {
                                ocl.push_back(baseidx + i - begin);
                            }
                            ocl.push_back(baseidx);
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(index);
                        }
                    }
                }
            } else if (auto quad = Quads::as(grid)) {
                if (showTypes[UnstructuredGrid::QUAD]) {
                    auto nelem = quad->getNumElements();
                    auto ix = quad->x().data();
                    auto iy = quad->y().data();
                    auto iz = quad->z().data();
                    const Index *icl = nullptr;
                    if (quad->getNumCorners() > 0) {
                        icl = quad->cl().data();
                    }
                    auto vert = [icl](Index idx) {
                        return icl ? icl[idx] : idx;
                    };
                    for (Index i = 0; i < nelem; ++i) {
                        if (!showCell(i))
                            continue;
                        const bool ghost = quad->isGhost(i);
                        const bool show = ((showgho && ghost) || (shownor && !ghost));
                        if (!show)
                            continue;

                        std::vector<Index> verts;
                        for (int j = 0; j < 4; ++j) {
                            verts.push_back(vert(i * 4 + j));
                        }
                        Vector3 center(0, 0, 0);
                        for (auto v: verts) {
                            Vector3 p(ix[v], iy[v], iz[v]);
                            center += p;
                        }
                        center *= 1.0f / verts.size();
                        auto baseidx = ox.size();
                        for (auto v: verts) {
                            if (!perElement)
                                remap.push_back(v);
                            Vector3 p = center + (Vector3(ix[v], iy[v], iz[v]) - center) * scaleFactor;
                            ox.push_back(p[0]);
                            oy.push_back(p[1]);
                            oz.push_back(p[2]);
                        }

                        auto emitVertex = [&ocl, baseidx](Index idx) {
                            ocl.push_back(baseidx + idx);
                        };
                        if (bars) {
                            for (int j = 0; j < 4; ++j) {
                                emitVertex(j);
                                emitVertex((j + 1) % 4);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(i);
                            }
                        } else {
                            for (int j = 0; j < 4; ++j) {
                                emitVertex(j);
                            }
                            emitVertex(0);
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(i);
                        }
                    }
                }
            } else if (auto tri = Triangles::as(grid)) {
                if (showTypes[UnstructuredGrid::TRIANGLE]) {
                    auto nelem = tri->getNumElements();
                    const Index *icl = nullptr;
                    auto ix = tri->x().data();
                    auto iy = tri->y().data();
                    auto iz = tri->z().data();
                    if (tri->getNumCorners() > 0) {
                        icl = tri->cl().data();
                    }
                    auto vert = [icl](Index idx) {
                        return icl ? icl[idx] : idx;
                    };
                    for (Index i = 0; i < nelem; ++i) {
                        if (!showCell(i))
                            continue;
                        const bool ghost = tri->isGhost(i);
                        const bool show = ((showgho && ghost) || (shownor && !ghost));
                        if (!show)
                            continue;
                        std::vector<Index> verts;
                        for (int j = 0; j < 3; ++j) {
                            verts.push_back(vert(i * 3 + j));
                        }
                        Vector3 center(0, 0, 0);
                        for (auto v: verts) {
                            Vector3 p(ix[v], iy[v], iz[v]);
                            center += p;
                        }
                        center *= 1.0f / verts.size();
                        auto baseidx = ox.size();
                        for (auto v: verts) {
                            if (!perElement)
                                remap.push_back(v);
                            Vector3 p = center + (Vector3(ix[v], iy[v], iz[v]) - center) * scaleFactor;
                            ox.push_back(p[0]);
                            oy.push_back(p[1]);
                            oz.push_back(p[2]);
                        }

                        auto emitVertex = [&ocl, baseidx](Index idx) {
                            ocl.push_back(baseidx + idx);
                        };
                        if (bars) {
                            for (int j = 0; j < 3; ++j) {
                                emitVertex(j);
                                emitVertex((j + 1) % 3);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(i);
                            }
                        } else {
                            for (int j = 0; j < 3; ++j) {
                                emitVertex(j);
                            }
                            emitVertex(0);
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(i);
                        }
                    }
                }
            }
        } else {
            if (auto unstr = UnstructuredGrid::as(grid)) {
                const Index *icl = unstr->cl().data();

                Index begin = 0, end = unstr->getNumElements();
                if (cellnrmin >= 0) {
                    begin = std::max(cellnrmin, (Integer)begin);
                }
                if (cellnrmax + 1 > 0) {
                    end = std::min(cellnrmax + 1, (Integer)end);
                }

                for (Index index = begin; index < end; ++index) {
                    if (!showCell(index))
                        continue;

                    auto type = unstr->tl()[index];
                    const bool ghost = unstr->isGhost(index);

                    const bool show = ((showgho && ghost) || (shownor && !ghost));
                    if (!show)
                        continue;
                    if (!showTypes[type])
                        continue;

                    const Index begin = unstr->el()[index], end = unstr->el()[index + 1];
                    switch (type) {
                    case UnstructuredGrid::QUAD:
                    case UnstructuredGrid::TRIANGLE:
                    case UnstructuredGrid::TETRAHEDRON:
                    case UnstructuredGrid::PYRAMID:
                    case UnstructuredGrid::PRISM:
                    case UnstructuredGrid::HEXAHEDRON: {
                        const auto numFaces = UnstructuredGrid::NumFaces[type];
                        for (int f = 0; f < numFaces; ++f) {
                            const int nCorners = UnstructuredGrid::FaceSizes[type][f];
                            if (bars) {
                                for (int i = 0; i < nCorners; ++i) {
                                    const Index nv = icl[begin + UnstructuredGrid::FaceVertices[type][f][i]];
                                    assert(nv < unstr->getNumVertices());
                                    ocl.push_back(nv);
                                    const Index nv1 =
                                        icl[begin + UnstructuredGrid::FaceVertices[type][f][(i + 1) % nCorners]];
                                    assert(nv1 < unstr->getNumVertices());
                                    ocl.push_back(nv1);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(index);
                                }
                            } else {
                                for (int i = 0; i <= nCorners; ++i) {
                                    const Index v = icl[begin + UnstructuredGrid::FaceVertices[type][f][i % nCorners]];
                                    ocl.push_back(v);
                                }
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        }
                        break;
                    }
                    case UnstructuredGrid::POLYHEDRON: {
                        Index facestart = InvalidIndex;
                        Index term = 0;
                        for (Index i = begin; i < end; ++i) {
                            if (bars) {
                                auto idx = icl[i];
                                if (facestart == InvalidIndex) {
                                    facestart = i;
                                    term = idx;
                                } else if (term == idx) {
                                    facestart = InvalidIndex;
                                }
                                if (facestart != InvalidIndex) {
                                    ocl.push_back(icl[i]);
                                    auto idx1 = icl[i + 1];
                                    ocl.push_back(idx1);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(index);
                                }
                            } else {
                                ocl.push_back(icl[i]);
                                if (facestart == InvalidIndex) {
                                    facestart = i;
                                    term = icl[i];
                                } else if (term == icl[i]) {
                                    facestart = InvalidIndex;
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(index);
                                }
                            }
                        }
                        break;
                    }
                    case UnstructuredGrid::BAR: {
                        ocl.push_back(icl[begin]);
                        ocl.push_back(icl[begin + 1]);
                        oel.push_back(ocl.size());
                        if (perElement)
                            remap.push_back(index);
                        break;
                    }
                    case UnstructuredGrid::POLYLINE: {
                        if (bars) {
                            for (Index i = begin; i + 1 < end; ++i) {
                                ocl.push_back(icl[i]);
                                ocl.push_back(icl[i + 1]);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        } else {
                            std::copy(icl + begin, icl + end, std::back_inserter(ocl));
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(index);
                        }
                        break;
                    }
                    }
                }
                lines->d()->x[0] = unstr->d()->x[0];
                lines->d()->x[1] = unstr->d()->x[1];
                lines->d()->x[2] = unstr->d()->x[2];
            } else if (auto str = StructuredGridBase::as(grid)) {
                const Index dims[3] = {str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2)};
                Index begin = 0, end = str->getNumElements();
                if (cellnrmin >= 0)
                    begin = std::max(cellnrmin, (Integer)begin);
                if (cellnrmax + 1 > 0)
                    end = std::min(cellnrmax + 1, (Integer)end);

                int d = StructuredGridBase::dimensionality(dims);
                const Byte type = d == 3   ? UnstructuredGrid::HEXAHEDRON
                                  : d == 2 ? UnstructuredGrid::QUAD
                                  : d == 1 ? UnstructuredGrid::BAR
                                           : UnstructuredGrid::POINT;

                for (Index index = begin; index < end; ++index) {
                    if (!showCell(index))
                        continue;
                    if (type == UnstructuredGrid::HEXAHEDRON && !showTypes[UnstructuredGrid::HEXAHEDRON])
                        continue;
                    if (type == UnstructuredGrid::QUAD && !showTypes[UnstructuredGrid::QUAD])
                        continue;
                    bool ghost = str->isGhostCell(index);
                    const bool show = ((showgho && ghost) || (shownor && !ghost));
                    if (!show)
                        continue;
                    auto verts = StructuredGridBase::cellVertices(index, dims);
                    const auto numFaces = UnstructuredGrid::NumFaces[type];
                    for (int f = 0; f < numFaces; ++f) {
                        const int nCorners = UnstructuredGrid::FaceSizes[type][f];
                        if (bars) {
                            for (int i = 0; i < nCorners; ++i) {
                                const Index nv = verts[UnstructuredGrid::FaceVertices[type][f][i]];
                                assert(nv < str->getNumVertices());
                                ocl.push_back(nv);
                                const Index nv1 = verts[UnstructuredGrid::FaceVertices[type][f][(i + 1) % nCorners]];
                                assert(nv1 < str->getNumVertices());
                                ocl.push_back(nv1);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        } else {
                            for (int i = 0; i <= nCorners; ++i) {
                                const Index v = verts[UnstructuredGrid::FaceVertices[type][f][i % nCorners]];
                                ocl.push_back(v);
                            }
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(index);
                        }
                    }
                }

                if (auto s = StructuredGrid::as(grid)) {
                    lines->d()->x[0] = s->d()->x[0];
                    lines->d()->x[1] = s->d()->x[1];
                    lines->d()->x[2] = s->d()->x[2];
                } else {
                    const Index numVert = str->getNumVertices();
                    lines->setSize(numVert);
                    auto x = lines->x().data();
                    auto y = lines->y().data();
                    auto z = lines->z().data();

                    for (Index i = 0; i < numVert; ++i) {
                        auto v = str->getVertex(i);
                        x[i] = v[0];
                        y[i] = v[1];
                        z[i] = v[2];
                    }
                }
            } else if (auto poly = Polygons::as(grid)) {
                if (showTypes[UnstructuredGrid::QUAD]) {
                    const Index *icl = poly->cl().data();

                    Index begin = 0, end = poly->getNumElements();
                    if (cellnrmin >= 0)
                        begin = std::max(cellnrmin, (Integer)begin);
                    if (cellnrmax + 1 > 0)
                        end = std::min(cellnrmax + 1, (Integer)end);

                    for (Index index = begin; index < end; ++index) {
                        if (!showCell(index))
                            continue;
                        const bool ghost = poly->isGhost(index);
                        const bool show = ((showgho && ghost) || (shownor && !ghost));
                        if (!show)
                            continue;

                        const Index begin = poly->el()[index], end = poly->el()[index + 1];
                        if (bars) {
                            for (Index i = begin; i < end; ++i) {
                                ocl.push_back(icl[i]);
                                ocl.push_back(icl[(i + 1 == end) ? begin : i + 1]);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(index);
                            }
                        } else {
                            for (Index i = begin; i < end; ++i) {
                                ocl.push_back(icl[i]);
                            }
                            ocl.push_back(icl[begin]);
                            oel.push_back(ocl.size());
                            if (perElement)
                                remap.push_back(index);
                        }
                    }
                }
                lines->d()->x[0] = poly->d()->x[0];
                lines->d()->x[1] = poly->d()->x[1];
                lines->d()->x[2] = poly->d()->x[2];
            } else if (auto quad = Quads::as(grid)) {
                if (showTypes[UnstructuredGrid::QUAD]) {
                    auto nelem = quad->getNumElements();
                    if (quad->getNumCorners() > 0) {
                        const Index *icl = quad->cl().data();
                        for (Index i = 0; i < nelem; ++i) {
                            if (!showCell(i))
                                continue;
                            const bool ghost = quad->isGhost(i);
                            const bool show = ((showgho && ghost) || (shownor && !ghost));
                            if (!show)
                                continue;
                            if (bars) {
                                for (int j = 0; j < 4; ++j) {
                                    ocl.push_back(icl[i * 4 + j]);
                                    ocl.push_back(icl[i * 4 + (j + 1) % 4]);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(i);
                                }
                            } else {
                                ocl.push_back(icl[i * 4]);
                                ocl.push_back(icl[i * 4 + 1]);
                                ocl.push_back(icl[i * 4 + 2]);
                                ocl.push_back(icl[i * 4 + 3]);
                                ocl.push_back(icl[i * 4]);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(i);
                            }
                        }
                    } else {
                        nelem = quad->getNumVertices() / 4;
                        for (Index i = 0; i < nelem; ++i) {
                            if (!showCell(i))
                                continue;
                            const bool ghost = quad->isGhost(i);
                            const bool show = ((showgho && ghost) || (shownor && !ghost));
                            if (!show)
                                continue;
                            if (bars) {
                                for (int j = 0; j < 4; ++j) {
                                    ocl.push_back(i * 4 + j);
                                    ocl.push_back(i * 4 + (j + 1) % 4);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(i);
                                }
                            } else {
                                ocl.push_back(i * 4);
                                ocl.push_back(i * 4 + 1);
                                ocl.push_back(i * 4 + 2);
                                ocl.push_back(i * 4 + 3);
                                ocl.push_back(i * 4);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(i);
                            }
                        }
                    }
                }
                lines->d()->x[0] = quad->d()->x[0];
                lines->d()->x[1] = quad->d()->x[1];
                lines->d()->x[2] = quad->d()->x[2];
            } else if (auto tri = Triangles::as(grid)) {
                if (showTypes[UnstructuredGrid::TRIANGLE]) {
                    auto nelem = tri->getNumElements();
                    if (tri->getNumCorners() > 0) {
                        const Index *icl = tri->cl().data();
                        for (Index i = 0; i < nelem; ++i) {
                            if (!showCell(i))
                                continue;
                            const bool ghost = tri->isGhost(i);
                            const bool show = ((showgho && ghost) || (shownor && !ghost));
                            if (!show)
                                continue;
                            if (bars) {
                                for (int j = 0; j < 3; ++j) {
                                    ocl.push_back(icl[i * 3 + j]);
                                    ocl.push_back(icl[i * 3 + (j + 1) % 3]);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(i);
                                }
                            } else {
                                ocl.push_back(icl[i * 3]);
                                ocl.push_back(icl[i * 3 + 1]);
                                ocl.push_back(icl[i * 3 + 2]);
                                ocl.push_back(icl[i * 3]);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(i);
                            }
                        }
                    } else {
                        nelem = tri->getNumVertices() / 3;
                        for (Index i = 0; i < nelem; ++i) {
                            if (!showCell(i))
                                continue;
                            const bool ghost = tri->isGhost(i);
                            const bool show = ((showgho && ghost) || (shownor && !ghost));
                            if (!show)
                                continue;
                            if (bars) {
                                for (int j = 0; j < 3; ++j) {
                                    ocl.push_back(i * 3 + j);
                                    ocl.push_back(i * 3 + (j + 1) % 3);
                                    oel.push_back(ocl.size());
                                    if (perElement)
                                        remap.push_back(i);
                                }
                            } else {
                                ocl.push_back(i * 3);
                                ocl.push_back(i * 3 + 1);
                                ocl.push_back(i * 3 + 2);
                                ocl.push_back(i * 3);
                                oel.push_back(ocl.size());
                                if (perElement)
                                    remap.push_back(i);
                            }
                        }
                    }
                }
                lines->d()->x[0] = tri->d()->x[0];
                lines->d()->x[1] = tri->d()->x[1];
                lines->d()->x[2] = tri->d()->x[2];
            }
        }

        lines->copyAttributes(grid);
        updateMeta(lines);
        m_cache.storeAndUnlock(entry, lines);

        haveOutput = !ocl.empty();
    }

    if (mapped) {
        if (!remap.empty() || !haveOutput) {
            data = mapped->cloneType();
            data->setSize(remap.size());
            for (Index i = 0; i < remap.size(); ++i) {
                data->copyEntry(i, mapped, remap[i]);
            }
            if (perElement) {
                data->setMapping(DataBase::Element);
                assert(data->getSize() == lines->getNumElements());
            } else {
                data->setMapping(DataBase::Vertex);
                assert(data->getSize() == lines->getNumVertices());
            }
        } else if (mapped->guessMapping() == DataBase::Vertex) {
            data = mapped->clone();
            data->setMapping(DataBase::Vertex);
            assert(data->getSize() == lines->getNumVertices());
        }
    }

    Object::ptr out;
    if (data) {
        updateMeta(data);
        data->setGrid(lines);
        out = data;
    } else {
        out = lines;
    }
    addObject("grid_out", out);

    return true;
}
