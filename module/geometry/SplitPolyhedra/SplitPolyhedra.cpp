#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/unstr.h>

#include "SplitPolyhedra.h"

MODULE_MAIN(SplitPolyhedra)

using namespace vistle;

SplitPolyhedra::SplitPolyhedra(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in", "unstructured grid");
    createOutputPort("grid_out", "unstructured grid with tesselated polyhedra");
}

SplitPolyhedra::~SplitPolyhedra()
{}

template<typename T>
struct R {
    R(const T *begin, const T *end): m_begin(begin), m_end(end) {}
    R(T *begin, size_t size): m_begin(begin), m_end(begin + size) {}
    const T *begin() const { return m_begin; }
    const T *end() const { return m_end; }
    size_t size() const { return m_end - m_begin; }
    const T &operator[](size_t i) const { return *(m_begin + i); }
    const T *m_begin, *m_end;
};


bool SplitPolyhedra::compute()
{
    auto data = expect<DataBase>("grid_in");
    if (!data) {
        sendError("input does not an unstructured grid and does not have a grid");
        return true;
    }
    auto obj = data->grid();
    if (!obj) {
        obj = data;
        data.reset();
    }

    auto grid = UnstructuredGrid::as(obj);
    if (!grid) {
        sendError("input does not have an unstructured grid");
        return true;
    }

    const Index *iel = &grid->el()[0];
    const Index *icl = &grid->cl()[0];
    const Byte *itl = &grid->tl()[0];
    const Index nelem = grid->getNumElements();
    auto simple = std::make_shared<UnstructuredGrid>(0, 0, 0);
    simple->d()->x[0] = grid->d()->x[0];
    simple->d()->x[1] = grid->d()->x[1];
    simple->d()->x[2] = grid->d()->x[2];
    auto &oel = simple->el();
    auto &otl = simple->tl();
    auto &ocl = simple->cl();
    auto &oGhost = simple->isGhost();
    auto &oConvex = simple->convexityList();
    for (Index elem = 0; elem < nelem; ++elem) {
        Byte type = itl[elem];

        if (type == UnstructuredGrid::POLYHEDRON) {
            const Index begin = iel[elem], end = iel[elem + 1];
            std::vector<Index> verts; // vertices in cell
            std::vector<Index> fl; // face list (start indices)
            std::vector<Index> tri, quad; // list of triangle and quad faces
            Index first = InvalidIndex;
            Index facebegin = InvalidIndex;
            for (Index i = begin; i < end; ++i) {
                if (first == InvalidIndex) {
                    first = icl[i];
                    facebegin = i;
                    fl.push_back(facebegin);
                } else {
                    verts.push_back(icl[i]);
                    if (icl[i] == first) {
                        if (i - facebegin == 3) {
                            tri.push_back(facebegin);
                        } else if (i - facebegin == 4) {
                            quad.push_back(facebegin);
                        }
                        first = InvalidIndex;
                        facebegin = InvalidIndex;
                    }
                }
            }
            assert(first == InvalidIndex);
            assert(facebegin == InvalidIndex);
            std::sort(verts.begin(), verts.end());
            auto vend = std::unique(verts.begin(), verts.end());
            verts.resize(vend - verts.begin());
            Index nvert = verts.size();
            Index nface = fl.size();
            fl.push_back(end);

            // try to recover simple cells
            Index ntri = tri.size(), nquad = quad.size();
            if (nvert == 4 && nface == 4 && ntri == 4 && nquad == 0) {
                otl.push_back(UnstructuredGrid::TETRAHEDRON);
                oGhost.push_back(grid->getIsGhost(elem));
                oConvex.push_back(grid->isConvex(elem));
                const auto T = R(icl + tri[0], 3);
                assert(T.size() == 3);
                for (auto c: T)
                    ocl.push_back(c);
                for (auto v: verts) {
                    if (std::find(T.begin(), T.end(), v) == T.end()) {
                        ocl.push_back(v);
                        break;
                    }
                }
                oel.push_back(ocl.size());
            } else if (nvert == 5 && nface == 5 && ntri == 4 && nquad == 1) {
                otl.push_back(UnstructuredGrid::PYRAMID);
                oGhost.push_back(grid->getIsGhost(elem));
                oConvex.push_back(grid->isConvex(elem));
                const auto Q = R(icl + quad[0], 4);
                assert(Q.size() == 4);
                for (auto c: Q)
                    ocl.push_back(c);
                for (auto v: verts) {
                    if (std::find(Q.begin(), Q.end(), v) == Q.end()) {
                        ocl.push_back(v);
                        break;
                    }
                }
                oel.push_back(ocl.size());
            } else if (nvert == 6 && nface == 5 && ntri == 2 && nquad == 3) {
                otl.push_back(UnstructuredGrid::PRISM);
                oGhost.push_back(grid->getIsGhost(elem));
                oConvex.push_back(grid->isConvex(elem));
                auto Tb = R(icl + tri[0], 3);
                auto Tt = R(icl + tri[1], 3);

                // find a side face containing first vertex of bottom
                Index v0 = Tb[0];
                Index ns = 0;
                Index s0 = InvalidIndex;
                for (; ns < nquad; ++ns) {
                    auto Qs = R(icl + quad[ns], 4);
                    auto it = std::find(Qs.begin(), Qs.end(), v0);
                    if (it != Qs.end()) {
                        s0 = it - Qs.begin();
                        break;
                    }
                }
                auto Qs = R(icl + quad[ns], 4); // side face

                // determine bottom-top-alignment
                Index opposite; // to v0
                if (Qs[(s0 + 3) % 4] == Tb[1]) {
                    opposite = Qs[(s0 + 1) % 4];
                } else {
                    assert(Qs[(s0 + 1) % 4] == Tb[2]);
                    opposite = Qs[(s0 + 3) % 4];
                }
                Index offset = InvalidIndex;
                for (Index t = 0; t < 3; ++t) {
                    if (Tt[t] == opposite) {
                        offset = t;
                        break;
                    }
                }
                assert(offset != InvalidIndex);

                for (Index b = 0; b < 3; ++b)
                    ocl.push_back(Tb[b]);
                for (Index t = 0; t < 3; ++t)
                    ocl.push_back(Tt[(offset + 3 - t) % 3]);
                oel.push_back(ocl.size());
            } else if (nvert == 8 && nface == 6 && ntri == 0 && nquad == 6) {
                otl.push_back(UnstructuredGrid::HEXAHEDRON);
                oGhost.push_back(grid->getIsGhost(elem));
                oConvex.push_back(grid->isConvex(elem));
                auto Qb = R(icl + quad[0], 4); // bottom face
                // find top face
                Index nt = 1;
                for (; nt < nquad; ++nt) {
                    bool disjoint = true;
                    auto Qt = R(icl + quad[nt], 4);
                    for (Index b = 0; b < 4; ++b) {
                        Index v = Qb[b];
                        if (std::find(Qt.begin(), Qt.end(), v) != Qt.end()) {
                            disjoint = false;
                            break;
                        }
                    }
                    if (disjoint)
                        break;
                }
                auto Qt = R(icl + quad[nt], 4); // top face

                // find a side face containing first vertex of bottom
                Index v0 = Qb[0];
                Index ns = 1;
                Index s0 = InvalidIndex;
                for (; ns < nquad; ++ns) {
                    auto Qs = R(icl + quad[ns], 4);
                    auto it = std::find(Qs.begin(), Qs.end(), v0);
                    if (it != Qs.end()) {
                        s0 = it - Qs.begin();
                        break;
                    }
                }
                auto Qs = R(icl + quad[ns], 4); // side face

                // determine bottom-top-alignment
                Index offset = InvalidIndex;
                Index opposite; // to v0
                if (Qs[(s0 + 3) % 4] == Qb[1]) {
                    opposite = Qs[(s0 + 1) % 4];
                } else {
                    assert(Qs[(s0 + 1) % 4] == Qb[3]);
                    opposite = Qs[(s0 + 3) % 4];
                }
                for (Index t = 0; t < 4; ++t) {
                    if (Qt[t] == opposite) {
                        offset = t;
                        break;
                    }
                }
                assert(offset != InvalidIndex);

                for (Index b = 0; b < 4; ++b)
                    ocl.push_back(Qb[b]);
                for (Index t = 0; t < 4; ++t)
                    ocl.push_back(Qt[(offset + 4 - t) % 4]);
                oel.push_back(ocl.size());
            } else {
                // split complex cells into tetrahedra/pyramids
                std::vector<R<Index>> oppositeFaces;
                auto V = verts[0]; // pick smallest vertex
                for (Index k = 0; k < nface; ++k) {
                    auto f = R(icl + fl[k], icl + fl[k + 1] - 1);
                    if (std::find(f.begin(), f.end(), V) == f.end())
                        oppositeFaces.push_back(f);
                }

                for (auto &face: oppositeFaces) {
                    if (face.size() == 3) {
                        otl.push_back(UnstructuredGrid::TETRAHEDRON);
                        oGhost.push_back(grid->getIsGhost(elem));
                        oConvex.push_back(grid->isConvex(elem));
                        std::copy(face.begin(), face.end(), std::back_inserter(ocl));
                        ocl.push_back(V);
                        oel.push_back(ocl.size());
                    } else if (face.size() == 4) {
                        otl.push_back(UnstructuredGrid::PYRAMID);
                        oGhost.push_back(grid->getIsGhost(elem));
                        oConvex.push_back(grid->isConvex(elem));
                        std::copy(face.begin(), face.end(), std::back_inserter(ocl));
                        ocl.push_back(V);
                        oel.push_back(ocl.size());
                    } else {
                        auto minit = std::min_element(face.begin(), face.end());
                        auto next = minit + 1 == face.end() ? face.begin() : minit + 1;
                        auto prev = minit == face.begin() ? face.end() - 1 : minit;
                        auto v = *minit;
                        // split in same order as for face from neighboring cell
                        if (*prev > *next) {
                            while (next != prev) {
                                otl.push_back(UnstructuredGrid::TETRAHEDRON);
                                oGhost.push_back(grid->getIsGhost(elem));
                                oConvex.push_back(grid->isConvex(elem));
                                ocl.push_back(v);
                                ocl.push_back(*next);
                                ++next;
                                if (next == face.end())
                                    next = face.begin();
                                ocl.push_back(*next);
                                ocl.push_back(V);
                                oel.push_back(ocl.size());
                            }
                        } else {
                            while (prev != next) {
                                otl.push_back(UnstructuredGrid::TETRAHEDRON);
                                oGhost.push_back(grid->getIsGhost(elem));
                                oConvex.push_back(grid->isConvex(elem));
                                ocl.push_back(v);
                                ocl.push_back(*prev);
                                if (prev == face.begin())
                                    prev = face.end();
                                --prev;
                                ocl.push_back(*prev);
                                ocl.push_back(V);
                                oel.push_back(ocl.size());
                            }
                        }
                    }
                }
            }
        } else {
            // retain simple cells
            otl.push_back(type);
            oGhost.push_back(grid->getIsGhost(elem));
            oConvex.push_back(grid->isConvex(elem));
            const Index begin = iel[elem], end = iel[elem + 1];
            std::copy(icl + begin, icl + end, std::back_inserter(ocl));
            oel.push_back(ocl.size());
        }
    }

    if (simple) {
        simple->setMeta(obj->meta());
        simple->copyAttributes(obj);
        updateMeta(simple);
    }

    if (data) {
        auto ndata = data->clone();
        ndata->setMeta(data->meta());
        ndata->copyAttributes(data);
        ndata->setGrid(simple);
        updateMeta(ndata);
        addObject("grid_out", ndata);
    } else {
        addObject("grid_out", simple);
    }

    return true;
}
