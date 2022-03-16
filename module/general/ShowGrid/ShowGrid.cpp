#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/lines.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/module/resultcache.h>
#include <vistle/alg/objalg.h>

#include "ShowGrid.h"

MODULE_MAIN(ShowGrid)

using namespace vistle;

ShowGrid::ShowGrid(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    createInputPort("grid_in");
    createOutputPort("grid_out");

    addIntParameter("normalcells", "Show normal (non ghost) cells", 1, Parameter::Boolean);
    addIntParameter("ghostcells", "Show ghost cells", 0, Parameter::Boolean);
    addIntParameter("convex", "Show convex cells", 1, Parameter::Boolean);
    addIntParameter("nonconvex", "Show non-convex cells", 1, Parameter::Boolean);

    addIntParameter("tetrahedron", "Show tetrahedron", 1, Parameter::Boolean);
    addIntParameter("pyramid", "Show pyramid", 1, Parameter::Boolean);
    addIntParameter("prism", "Show prism", 1, Parameter::Boolean);
    addIntParameter("hexahedron", "Show hexahedron", 1, Parameter::Boolean);
    addIntParameter("polyhedron", "Show polyhedron", 1, Parameter::Boolean);
    addIntParameter("quad", "Show quad", 1, Parameter::Boolean);
    addIntParameter("triangle", "Show triangle", 1, Parameter::Boolean);
    m_CellNrMin = addIntParameter("Show Cells from Cell Nr. :", "Show Cell Nr.", -1);
    m_CellNrMax = addIntParameter("Show Cells to Cell Nr. :", "Show Cell Nr.", -1);

    addResultCache(m_cache);
}

ShowGrid::~ShowGrid()
{}

bool ShowGrid::compute()
{
    const bool shownor = getIntParameter("normalcells");
    const bool showgho = getIntParameter("ghostcells");
    const bool showconv = getIntParameter("convex");
    const bool shownonconv = getIntParameter("nonconvex");

    const bool showtet = getIntParameter("tetrahedron");
    const bool showpyr = getIntParameter("pyramid");
    const bool showpri = getIntParameter("prism");
    const bool showhex = getIntParameter("hexahedron");
    const bool showpol = getIntParameter("polyhedron");
    const bool showqua = getIntParameter("quad");
    const bool showtri = getIntParameter("triangle");
    const Integer cellnrmin = getIntParameter("Show Cells from Cell Nr. :");
    const Integer cellnrmax = getIntParameter("Show Cells to Cell Nr. :");


    auto obj = expect<Object>("grid_in");
    auto split = splitContainerObject(obj);
    auto grid = split.geometry;
    if (!grid) {
        sendError("did not receive an input object");
        return true;
    }

    vistle::Lines::ptr out;
    if (auto *entry = m_cache.getOrLock(grid->getName(), out)) {
        out.reset(new vistle::Lines(Object::Initialized));
        auto &ocl = out->cl();
        auto &oel = out->el();

        if (auto unstr = UnstructuredGrid::as(grid)) {
            const Index *icl = &unstr->cl()[0];

            Index begin = 0, end = unstr->getNumElements();
            if (cellnrmin >= 0)
                begin = std::max(cellnrmin, (Integer)begin);
            if (cellnrmax >= 0)
                end = std::min(cellnrmax + 1, (Integer)end);

            for (Index index = begin; index < end; ++index) {
                auto type = unstr->tl()[index];
                const bool ghost = type & UnstructuredGrid::GHOST_BIT;
                const bool conv = type & UnstructuredGrid::CONVEX_BIT;

                const bool show =
                    ((showgho && ghost) || (shownor && !ghost)) && ((showconv && conv) || (shownonconv && !conv));
                if (!show)
                    continue;
                type &= vistle::UnstructuredGrid::TYPE_MASK;
                switch (type) {
                case UnstructuredGrid::TETRAHEDRON:
                    if (!showtet) {
                        continue;
                    }
                    break;
                case UnstructuredGrid::PYRAMID:
                    if (!showpyr) {
                        continue;
                    }
                    break;
                case UnstructuredGrid::PRISM:
                    if (!showpri) {
                        continue;
                    }
                    break;
                case UnstructuredGrid::HEXAHEDRON:
                    if (!showhex) {
                        continue;
                    }
                    break;
                case UnstructuredGrid::VPOLYHEDRON:
                case UnstructuredGrid::CPOLYHEDRON:
                    if (!showpol) {
                        continue;
                    }
                    break;
                case UnstructuredGrid::QUAD:
                    if (!showqua) {
                        continue;
                    }
                    break;
                case UnstructuredGrid::TRIANGLE:
                    if (!showtri) {
                        continue;
                    }
                    break;
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
                        for (int i = 0; i <= nCorners; ++i) {
                            const Index v = icl[begin + UnstructuredGrid::FaceVertices[type][f][i % nCorners]];
                            ocl.push_back(v);
                        }
                        oel.push_back(ocl.size());
                    }
                    break;
                }
                case UnstructuredGrid::VPOLYHEDRON: {
                    for (Index i = begin; i < end; i += icl[i] + 1) {
                        const Index numFaceVert = icl[i];
                        for (Index k = i + 1; k < i + numFaceVert + 1; ++k)
                            ocl.push_back(icl[k]);
                        ocl.push_back(icl[i + 1]);
                        oel.push_back(ocl.size());
                    }
                    break;
                }
                case UnstructuredGrid::CPOLYHEDRON: {
                    Index facestart = InvalidIndex;
                    Index term = 0;
                    for (Index i = begin; i < end; ++i) {
                        ocl.push_back(icl[i]);
                        if (facestart == InvalidIndex) {
                            facestart = i;
                            term = icl[i];
                        } else if (term == icl[i]) {
                            oel.push_back(ocl.size());
                            facestart = InvalidIndex;
                        }
                    }
                    break;
                }
                }
            }
            out->d()->x[0] = unstr->d()->x[0];
            out->d()->x[1] = unstr->d()->x[1];
            out->d()->x[2] = unstr->d()->x[2];
        } else if (auto str = StructuredGridBase::as(grid)) {
            const Index dims[3] = {str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2)};
            Index begin = 0, end = str->getNumElements();
            if (cellnrmin >= 0)
                begin = std::max(cellnrmin, (Integer)begin);
            if (cellnrmax >= 0)
                end = std::min(cellnrmax + 1, (Integer)end);

            int d = StructuredGridBase::dimensionality(dims);
            const Byte type = d == 3   ? UnstructuredGrid::HEXAHEDRON
                              : d == 2 ? UnstructuredGrid::QUAD
                              : d == 1 ? UnstructuredGrid::BAR
                                       : UnstructuredGrid::POINT;

            for (Index index = begin; index < end; ++index) {
                if (type == UnstructuredGrid::HEXAHEDRON && !showhex)
                    continue;
                if (type == UnstructuredGrid::QUAD && !showqua)
                    continue;
                bool ghost = str->isGhostCell(index);
                const bool show = ((showgho && ghost) || (shownor && !ghost));
                if (!show)
                    continue;
                auto verts = StructuredGridBase::cellVertices(index, dims);
                const auto numFaces = UnstructuredGrid::NumFaces[type];
                for (int f = 0; f < numFaces; ++f) {
                    const int nCorners = UnstructuredGrid::FaceSizes[type][f];
                    for (int i = 0; i <= nCorners; ++i) {
                        const Index v = verts[UnstructuredGrid::FaceVertices[type][f][i % nCorners]];
                        ocl.push_back(v);
                    }
                    oel.push_back(ocl.size());
                }
            }

            if (auto s = StructuredGrid::as(grid)) {
                out->d()->x[0] = s->d()->x[0];
                out->d()->x[1] = s->d()->x[1];
                out->d()->x[2] = s->d()->x[2];
            } else {
                const Index numVert = str->getNumVertices();
                out->setSize(numVert);
                auto x = &out->x()[0];
                auto y = &out->y()[0];
                auto z = &out->z()[0];

                for (Index i = 0; i < numVert; ++i) {
                    auto v = str->getVertex(i);
                    x[i] = v[0];
                    y[i] = v[1];
                    z[i] = v[2];
                }
            }
        }

        out->copyAttributes(grid);
        m_cache.storeAndUnlock(entry, out);
    }
    addObject("grid_out", out);

    return true;
}
