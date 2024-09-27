#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/util/enum.h>
#include <vistle/core/celltypes.h>
#include <vistle/alg/objalg.h>
#include <vistle/module/resultcache.h>

#include "SplitPolyhedra.h"

MODULE_MAIN(SplitPolyhedra)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Mode, (SplitToTetrahedra)(SplitToSimple)(RecoverSimple))

SplitPolyhedra::SplitPolyhedra(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    for (unsigned i = 0; i < NumPorts; ++i) {
        m_inPorts[i] = createInputPort("data_in" + std::to_string(i), "(data on) unstructured grid");
        m_outPorts[i] =
            createOutputPort("data_out" + std::to_string(i), "(data on) unstructured grid with simplified cells");
    }
    m_mode = addIntParameter("mode", "how to handle polyhedral and simple cells", SplitToTetrahedra, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_mode, Mode);
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


bool SplitPolyhedra::prepare()
{
    for (unsigned i = 0; i < NumPorts; ++i) {
        if (m_outPorts[i]->isConnected() && !m_inPorts[i]->isConnected()) {
            sendError("output port %s is connected, but input port %s is not", m_outPorts[i]->getName().c_str(),
                      m_inPorts[i]->getName().c_str());
        }
    }

    return true;
}

bool SplitPolyhedra::compute()
{
    auto mode = m_mode->getValue();
    bool perElement = false;
    UnstructuredGrid::const_ptr grid;
    std::array<DataBase::const_ptr, NumPorts> data;

    for (unsigned i = 0; i < NumPorts; ++i) {
        if (!m_inPorts[i]->isConnected())
            continue;
        auto in = accept<Object>(m_inPorts[i]);
        if (!in)
            continue;

        auto split = splitContainerObject(in);
        if (grid) {
            if (split.geometry && grid->getHandle() != split.geometry->getHandle()) {
                sendError("all inputs must have the same grid");
                return true;
            }
        } else {
            if (!split.geometry) {
                sendError("input does not have grid");
                return true;
            }
            grid = UnstructuredGrid::as(split.geometry);
            if (!grid) {
                sendError("input grid is not unstructured");
                return true;
            }
        }

        data[i] = split.mapped;

        if (mode == RecoverSimple)
            continue;

        if (split.mapped) {
            auto mapping = split.mapped->guessMapping();
            if (mapping == DataBase::Element)
                perElement = true;
        }
    }

    if (!grid) {
        sendError("no grid on any input");
        return true;
    }

    UnstructuredGrid::ptr simple;
    std::vector<Index> elementMapping;
    if (auto entry = m_grids.getOrLock(grid->getName(), simple)) {
        const Index *iel = &grid->el()[0];
        const Index *icl = &grid->cl()[0];
        const Byte *itl = &grid->tl()[0];
        const Index nelem = grid->getNumElements();
        simple = std::make_shared<UnstructuredGrid>(0, 0, 0);
        simple->d()->x[0] = grid->d()->x[0];
        simple->d()->x[1] = grid->d()->x[1];
        simple->d()->x[2] = grid->d()->x[2];
        auto &oel = simple->el();
        auto &otl = simple->tl();
        auto &ocl = simple->cl();
        auto &oGhost = simple->ghost();
        if (perElement) {
            elementMapping.reserve(nelem);
        }
        Index nPolySplit = 0, nSimpleCreated = 0, nBadSplit = 0, nSimpleRetained = 0;

        auto addElem = [&grid, &otl, &oGhost, &ocl, &oel, &elementMapping,
                        perElement](Index elem, Byte type = UnstructuredGrid::TETRAHEDRON) {
            otl.push_back(type);
            oel.push_back(ocl.size());
            oGhost.push_back(grid->isGhost(elem));
            if (perElement) {
                elementMapping.push_back(elem);
            }
        };

        auto addTetrasForFaceAndVertex = [&nSimpleCreated, &addElem, &ocl](const auto &face, Index V, Index elem) {
            auto minit = std::min_element(face.begin(), face.end());
            auto next = minit + 1 == face.end() ? face.begin() : minit + 1;
            auto prev = minit == face.begin() ? face.end() - 1 : minit - 1;
            auto v = *minit;
            while (next != prev) {
                ocl.push_back(v);
                ocl.push_back(*next);
                ++next;
                if (next == face.end())
                    next = face.begin();
                ocl.push_back(*next);
                ocl.push_back(V);
                ++nSimpleCreated;
                addElem(elem);
            }
        };

        for (Index elem = 0; elem < nelem; ++elem) {
            const Byte type = itl[elem];
            const Index begin = iel[elem], end = iel[elem + 1];
            assert(oel.size() == oGhost.size() + 1);
            assert(oel.size() == otl.size() + 1);
            assert(oel.back() == ocl.size());
            assert(!perElement || elementMapping.size() == otl.size());

            if (type == UnstructuredGrid::POLYHEDRON) {
                ++nPolySplit;
                std::vector<Index> verts; // vertices in cell
                verts.reserve(end - begin);
                std::vector<Index> fl; // face list (start indices)
                fl.reserve((end - begin) / 3);
                std::vector<Index> tri, quad; // list of triangle and quad faces
                tri.reserve((end - begin) / 3);
                quad.reserve((end - begin) / 4);
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
                    ++nSimpleCreated;
                    addElem(elem);
                    continue;
                } else if (mode == RecoverSimple || mode == SplitToSimple) {
                    if (nvert == 5 && nface == 5 && ntri == 4 && nquad == 1) {
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
                        ++nSimpleCreated;
                        addElem(elem, UnstructuredGrid::PYRAMID);
                        continue;
                    } else if (nvert == 6 && nface == 5 && ntri == 2 && nquad == 3) {
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
                        ++nSimpleCreated;
                        addElem(elem, UnstructuredGrid::PRISM);
                        continue;
                    } else if (nvert == 8 && nface == 6 && ntri == 0 && nquad == 6) {
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
                        ++nSimpleCreated;
                        addElem(elem, UnstructuredGrid::HEXAHEDRON);
                        continue;
                    }
                }
                if (mode == SplitToSimple || mode == SplitToTetrahedra) {
                    // try not to split quad faces, so that neighboring simple cells do not have be split

                    auto V = InvalidIndex;
                    if (mode == SplitToTetrahedra) {
                        V = verts[0]; // pick smallest vertex
                    } else {
                        // find a vertex around which to split: not adjacent to quads and
                        // - either only adjacent to triangles
                        // - or is the one with smallest index in every adjacent face
                        for (auto v: verts) {
                            bool good = true;
                            for (Index k = 0; k < nface; ++k) {
                                auto f = R(icl + fl[k], icl + fl[k + 1] - 1);
                                if (std::find(f.begin(), f.end(), v) == f.end())
                                    continue;
                                if (f.size() == 4) {
                                    good = false;
                                    break;
                                }
                                if (f.size() > 4) {
                                    auto minit = std::min_element(f.begin(), f.end());
                                    if (*minit != v) {
                                        good = false;
                                        break;
                                    }
                                }
                            }
                            if (good) {
                                V = v;
                                break;
                            }
                        }
                        if (V == InvalidIndex) {
                            V = verts[0];
                            ++nBadSplit;
                            if (nBadSplit < 100 || nBadSplit % 1000 == 0) {
                                std::cerr << "could not split polyhedron " << elem << ":";
                                for (Index k = 0; k < nface; ++k) {
                                    std::cerr << " [";
                                    auto f = R(icl + fl[k], icl + fl[k + 1] - 1);
                                    for (auto &v: f) {
                                        std::cerr << " " << v;
                                    }
                                    std::cerr << " ]";
                                }
                            }
                            std::cerr << std::endl;
                        }
                    }

                    std::vector<R<Index>> oppositeFaces;
                    for (Index k = 0; k < nface; ++k) {
                        auto f = R(icl + fl[k], icl + fl[k + 1] - 1);
                        if (std::find(f.begin(), f.end(), V) == f.end())
                            oppositeFaces.push_back(f);
                    }

                    for (auto &face: oppositeFaces) {
                        if (face.size() == 3) {
                            std::copy(face.begin(), face.end(), std::back_inserter(ocl));
                            ocl.push_back(V);
                            ++nSimpleCreated;
                            addElem(elem);
                        } else if (mode == SplitToSimple && face.size() == 4) {
                            std::copy(face.begin(), face.end(), std::back_inserter(ocl));
                            ocl.push_back(V);
                            ++nSimpleCreated;
                            addElem(elem, UnstructuredGrid::PYRAMID);
                        } else if (face.size() >= 3) {
                            addTetrasForFaceAndVertex(face, V, elem);
                        }
                    }
                    continue;
                }
            }

            if (mode == SplitToTetrahedra && type != UnstructuredGrid::TETRAHEDRON) {
                std::vector<Index> verts;
                std::copy(icl + begin, icl + end, std::back_inserter(verts));
                std::sort(verts.begin(), verts.end());
                auto V = verts[0]; // pick smallest vertex
                std::vector<std::vector<Index>> oppositeFaces;
                auto nface = UnstructuredGrid::NumFaces[type];
                for (int k = 0; k < nface; ++k) {
                    std::vector<Index> f;
                    for (unsigned i = 0; i < UnstructuredGrid::FaceSizes[type][k]; ++i) {
                        f.push_back(*(icl + begin + UnstructuredGrid::FaceVertices[type][k][i]));
                    }
                    if (std::find(f.begin(), f.end(), V) == f.end())
                        oppositeFaces.push_back(f);
                }

                for (auto &face: oppositeFaces) {
                    addTetrasForFaceAndVertex(face, V, elem);
                }
                continue;
            }

            // retain cell unmodified
            std::copy(icl + begin, icl + end, std::back_inserter(ocl));
            ++nSimpleRetained;
            addElem(elem, type);
        }

        std::ostringstream oss;
        oss << "split " << nPolySplit << " polyhedra into " << nSimpleCreated << " simple cells with " << nBadSplit
            << " bad splits, retained " << nSimpleRetained << " simple cells";
        sendInfo(oss.str());

        if (nBadSplit > 0) {
            std::ostringstream oss;
            oss << nBadSplit << " polyhedra could not be split properly: consider using SplitToTetrahedra mode";
            sendWarning(oss.str());
        }

        if (simple) {
            simple->copyAttributes(grid);
            updateMeta(simple);
        }

        m_grids.storeAndUnlock(entry, simple);
    }

    for (unsigned i = 0; i < NumPorts; ++i) {
        if (!m_outPorts[i]->isConnected())
            continue;

        if (data[i]) {
            DataBase::ptr ndata;
            if (perElement) {
                ndata = data[i]->cloneType();
                ndata->setSize(elementMapping.size());
                for (Index e = 0; e < elementMapping.size(); ++e) {
                    ndata->copyEntry(e, data[i], elementMapping[e]);
                }
                ndata->copyAttributes(data[i]);
            } else {
                ndata = data[i]->clone();
            }
            ndata->setGrid(simple);
            updateMeta(ndata);
            addObject(m_outPorts[i], ndata);
        } else {
            addObject(m_outPorts[i], simple);
        }
    }

    return true;
}
