#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/module/resultcache.h>
#include <vistle/alg/objalg.h>

#include "DomainSurface.h"

#define USE_SET

using namespace vistle;

DomainSurface::DomainSurface(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in");
    createOutputPort("data_out");
    addIntParameter("ghost", "Show ghostcells", 0, Parameter::Boolean);
    addIntParameter("tetrahedron", "Show tetrahedron", 1, Parameter::Boolean);
    addIntParameter("pyramid", "Show pyramid", 1, Parameter::Boolean);
    addIntParameter("prism", "Show prism", 1, Parameter::Boolean);
    addIntParameter("hexahedron", "Show hexahedron", 1, Parameter::Boolean);
    addIntParameter("polyhedron", "Show polyhedron", 1, Parameter::Boolean);
    addIntParameter("triangle", "Show triangle", 0, Parameter::Boolean);
    addIntParameter("quad", "Show quad", 0, Parameter::Boolean);
    addIntParameter("reuseCoordinates", "Re-use the unstructured grids coordinate list and data-object", 0,
                    Parameter::Boolean);
    addIntParameter("save_memory", "Less memory intensive algorithm", 1, Parameter::Boolean);

    addResultCache(m_cache);
}

DomainSurface::~DomainSurface()
{}

template<class T, int Dim>
typename Vec<T, Dim>::ptr remapData(typename Vec<T, Dim>::const_ptr in, const DomainSurface::DataMapping &dm)
{
    typename Vec<T, Dim>::ptr out(new Vec<T, Dim>(dm.size()));

    const T *data_in[Dim];
    T *data_out[Dim];
    for (int d = 0; d < Dim; ++d) {
        data_in[d] = &in->x(d)[0];
        data_out[d] = out->x(d).data();
    }

    for (Index i = 0; i < dm.size(); ++i) {
        for (int d = 0; d < Dim; ++d) {
            data_out[d][i] = data_in[d][dm[i]];
        }
    }

    return out;
}

bool DomainSurface::compute(std::shared_ptr<BlockTask> task) const
{
    //DomainSurface Polygon
    auto container = task->expect<Object>("data_in");
    auto split = splitContainerObject(container);
    DataBase::const_ptr data = split.mapped;
    StructuredGridBase::const_ptr sgrid = StructuredGridBase::as(split.geometry);
    UnstructuredGrid::const_ptr ugrid = UnstructuredGrid::as(split.geometry);
    if (!ugrid && !sgrid) {
        sendError("no grid and no data received");
        return true;
    }
    Object::const_ptr grid_in =
        ugrid ? Object::as(ugrid) : std::dynamic_pointer_cast<const Object, const StructuredGridBase>(sgrid);
    assert(grid_in);

    bool haveElementData = false;
    if (data && data->guessMapping(grid_in) == DataBase::Element) {
        haveElementData = true;
    }

    Object::ptr surface;
    DataMapping vm;
    DataMapping em;
    if (ugrid) {
        auto poly = createSurface(ugrid, em, haveElementData);
        surface = poly;
        if (!poly)
            return true;
        renumberVertices(ugrid, poly, vm);
    } else if (sgrid) {
        auto quad = createSurface(sgrid, em, haveElementData);
        surface = quad;
        if (!quad)
            return true;
        if (auto coords = Coords::as(grid_in)) {
            renumberVertices(coords, quad, vm);
        } else {
            createVertices(sgrid, quad, vm);
        }
    }

    surface->setMeta(grid_in->meta());
    surface->copyAttributes(grid_in);
    updateMeta(surface);

    if (auto entry = m_cache.getOrLock(grid_in->getName(), surface)) {
        m_cache.storeAndUnlock(entry, surface);
    }

    if (!data) {
        surface = surface->clone();
        updateMeta(surface);
        task->addObject("data_out", surface);
        return true;
    }

    if (!haveElementData && data->guessMapping(grid_in) != DataBase::Vertex) {
        sendError("data mapping not per vertex and not per element");
        return true;
    }

    if (!haveElementData && vm.empty()) {
        DataBase::ptr dout = data->clone();
        dout->setGrid(surface);
        updateMeta(dout);
        task->addObject("data_out", dout);
        return true;
    }

    DataBase::ptr data_obj_out;
    const auto &dm = haveElementData ? em : vm;
    if (auto data_in = Vec<Scalar, 3>::as(data)) {
        data_obj_out = remapData<Scalar, 3>(data_in, dm);
    } else if (auto data_in = Vec<Scalar, 1>::as(data)) {
        data_obj_out = remapData<Scalar, 1>(data_in, dm);
    } else if (auto data_in = Vec<Index, 3>::as(data)) {
        data_obj_out = remapData<Index, 3>(data_in, dm);
    } else if (auto data_in = Vec<Index, 1>::as(data)) {
        data_obj_out = remapData<Index, 1>(data_in, dm);
    } else if (auto data_in = Vec<Byte, 3>::as(data)) {
        data_obj_out = remapData<Byte, 3>(data_in, dm);
    } else if (auto data_in = Vec<Byte, 1>::as(data)) {
        data_obj_out = remapData<Byte, 1>(data_in, dm);
    } else {
        std::cerr << "WARNING: No valid 1D or 3D element data on input Port" << std::endl;
    }

    if (data_obj_out) {
        data_obj_out->setGrid(surface);
        data_obj_out->setMeta(data->meta());
        data_obj_out->copyAttributes(data);
        updateMeta(data_obj_out);
        task->addObject("data_out", data_obj_out);
    }

    return true;
}

Quads::ptr DomainSurface::createSurface(vistle::StructuredGridBase::const_ptr grid, DomainSurface::DataMapping &em,
                                        bool haveElementData) const
{
    auto sgrid = std::dynamic_pointer_cast<const StructuredGrid, const StructuredGridBase>(grid);

    Quads::ptr m_grid_out(new Quads(0, 0));
    auto &pcl = m_grid_out->cl();
    Index dims[3] = {grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2)};

    for (int d = 0; d < 3; ++d) {
        int d1 = d == 0 ? 1 : 0;
        int d2 = d == d1 + 1 ? d1 + 2 : d1 + 1;
        assert(d != d1);
        assert(d != d2);
        assert(d1 != d2);

        Index b1 = grid->getNumGhostLayers(d1, StructuredGridBase::Bottom);
        Index e1 = grid->getNumDivisions(d1);
        if (grid->getNumGhostLayers(d1, StructuredGridBase::Top) + 1 < e1)
            e1 -= grid->getNumGhostLayers(d1, StructuredGridBase::Top) + 1;
        else
            e1 = 0;
        Index b2 = grid->getNumGhostLayers(d2, StructuredGridBase::Bottom);
        Index e2 = grid->getNumDivisions(d2);
        if (grid->getNumGhostLayers(d2, StructuredGridBase::Top) + 1 < e2)
            e2 -= grid->getNumGhostLayers(d2, StructuredGridBase::Top) + 1;
        else
            e2 = 0;

        if (grid->getNumGhostLayers(d, StructuredGridBase::Bottom) == 0) {
            for (Index i1 = b1; i1 < e1; ++i1) {
                for (Index i2 = b2; i2 < e2; ++i2) {
                    Index idx[3]{0, 0, 0};
                    idx[d1] = i1;
                    idx[d2] = i2;
                    if (haveElementData) {
                        em.emplace_back(grid->cellIndex(idx, dims));
                    }
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d2] = i2 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                }
            }
        }
        if (grid->getNumDivisions(d) > 1 && grid->getNumGhostLayers(d, StructuredGridBase::Top) == 0) {
            for (Index i1 = b1; i1 < e1; ++i1) {
                for (Index i2 = b2; i2 < e2; ++i2) {
                    Index idx[3]{0, 0, 0};
                    idx[d] = grid->getNumDivisions(d) - 1;
                    idx[d1] = i1;
                    idx[d2] = i2;
                    if (haveElementData) {
                        --idx[d];
                        em.emplace_back(grid->cellIndex(idx, dims));
                        idx[d] = grid->getNumDivisions(d) - 1;
                    }
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d2] = i2 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                }
            }
        }
    }

    return m_grid_out;
}

void DomainSurface::renumberVertices(Coords::const_ptr coords, Indexed::ptr poly, DataMapping &vm) const
{
    const bool reuseCoord = getIntParameter("reuseCoordinates");

    if (reuseCoord) {
        poly->d()->x[0] = coords->d()->x[0];
        poly->d()->x[1] = coords->d()->x[1];
        poly->d()->x[2] = coords->d()->x[2];
    } else {
        vm.clear();

        std::map<Index, Index> mapped;
        for (Index &v: poly->cl()) {
            if (mapped.emplace(v, vm.size()).second) {
                Index vv = vm.size();
                vm.push_back(v);
                v = vv;
            } else {
                v = mapped[v];
            }
        }
        mapped.clear();

        const Scalar *xcoord = &coords->x()[0];
        const Scalar *ycoord = &coords->y()[0];
        const Scalar *zcoord = &coords->z()[0];
        auto &px = poly->x();
        auto &py = poly->y();
        auto &pz = poly->z();
        px.resize(vm.size());
        py.resize(vm.size());
        pz.resize(vm.size());

        for (Index i = 0; i < vm.size(); ++i) {
            px[i] = xcoord[vm[i]];
            py[i] = ycoord[vm[i]];
            pz[i] = zcoord[vm[i]];
        }
    }
}

void DomainSurface::renumberVertices(Coords::const_ptr coords, Quads::ptr quad, DataMapping &vm) const
{
    const bool reuseCoord = getIntParameter("reuseCoordinates");

    if (reuseCoord) {
        quad->d()->x[0] = coords->d()->x[0];
        quad->d()->x[1] = coords->d()->x[1];
        quad->d()->x[2] = coords->d()->x[2];
    } else {
        vm.clear();

        std::map<Index, Index> mapped;
        for (Index &v: quad->cl()) {
            if (mapped.emplace(v, vm.size()).second) {
                Index vv = vm.size();
                vm.push_back(v);
                v = vv;
            } else {
                v = mapped[v];
            }
        }
        mapped.clear();

        const Scalar *xcoord = &coords->x()[0];
        const Scalar *ycoord = &coords->y()[0];
        const Scalar *zcoord = &coords->z()[0];
        auto &px = quad->x();
        auto &py = quad->y();
        auto &pz = quad->z();
        px.resize(vm.size());
        py.resize(vm.size());
        pz.resize(vm.size());

        for (Index i = 0; i < vm.size(); ++i) {
            px[i] = xcoord[vm[i]];
            py[i] = ycoord[vm[i]];
            pz[i] = zcoord[vm[i]];
        }
    }
}

void DomainSurface::createVertices(StructuredGridBase::const_ptr grid, Quads::ptr quad, DataMapping &vm) const
{
    vm.clear();
    std::map<Index, Index> mapped;
    for (Index &v: quad->cl()) {
        if (mapped.emplace(v, vm.size()).second) {
            Index vv = vm.size();
            vm.push_back(v);
            v = vv;
        } else {
            v = mapped[v];
        }
    }
    mapped.clear();

    auto &px = quad->x();
    auto &py = quad->y();
    auto &pz = quad->z();
    px.resize(vm.size());
    py.resize(vm.size());
    pz.resize(vm.size());

    for (Index i = 0; i < vm.size(); ++i) {
        Vector3 p = grid->getVertex(vm[i]);
        px[i] = p[0];
        py[i] = p[1];
        pz[i] = p[2];
    }
}

struct Face {
    Index elem = InvalidIndex;
    Index face = InvalidIndex;
    std::array<Index, 3> verts;

    Face(Index e, Index f, Index sz, const Index *vl, const Index *cl): elem(e), face(f)
    {
        if (sz == 0)
            return;
        Index smallIdx = 0;
        Index smallVert = vl[cl[smallIdx]];
        for (Index idx = smallIdx + 1; idx < sz; ++idx) {
            if (smallVert > vl[cl[idx]]) {
                smallIdx = idx;
                smallVert = vl[cl[idx]];
            }
        }

        unsigned next = (smallIdx + 1) % sz;
        unsigned prev = (smallIdx + sz - 1) % sz;
        if (vl[cl[next]] > vl[cl[prev]]) {
            for (Index i = 0; i < sz && i < 3; ++i) {
                verts[i] = vl[cl[(i + smallIdx) % sz]];
            }
        } else {
            for (Index i = 0; i < sz && i < 3; ++i) {
                verts[i] = vl[cl[(smallIdx + sz - i) % sz]];
            }
        }
    }

    Face(Index e, Index f, Index sz, const Index *v): elem(e), face(f)
    {
        if (sz == 0)
            return;
        Index smallIdx = 0;
        Index smallVert = v[smallIdx];
        for (Index idx = smallIdx + 1; idx < sz; ++idx) {
            if (smallVert > v[idx]) {
                smallIdx = idx;
                smallVert = v[idx];
            }
        }

        unsigned next = (smallIdx + 1) % sz;
        unsigned prev = (smallIdx + sz - 1) % sz;
        if (v[next] > v[prev]) {
            for (Index i = 0; i < sz && i < 3; ++i) {
                verts[i] = v[(i + smallIdx) % sz];
            }
        } else {
            for (Index i = 0; i < sz && i < 3; ++i) {
                verts[i] = v[(smallIdx + sz - i) % sz];
            }
        }
    }

    Face(Index e, Index f, const std::vector<Index> &v): Face(e, f, v.size(), v.data()) {}

    bool operator==(const Face &other) const { return std::equal(verts.begin(), verts.end(), other.verts.begin()); }

    bool operator<(const Face &other) const
    {
        auto mm = std::mismatch(verts.begin(), verts.end(), other.verts.begin());
        if (mm.first == verts.end())
            return false;
        return *mm.first < *mm.second;
    }
};

std::ostream &operator<<(std::ostream &os, const Face &f)
{
    os << f.verts.size() << "(";
    for (auto it = f.verts.begin(); it != f.verts.end(); ++it) {
        if (it != f.verts.begin())
            os << " ";
        os << *it;
    }
    os << ")" << std::endl;
    return os;
}

#ifdef USE_SET
typedef std::set<Face> FaceSet;
#else
struct FaceHash {
    size_t operator()(const Face &f) const
    {
        const unsigned N = 3;
        const size_t primes[N] = {21619127, 731372359, 16267148063931119};
        size_t h = 0;
        for (unsigned i = 0; i < N && i < f.verts.size(); ++i) {
            h += primes[i] * f.verts[i];
        }
        return std::hash<size_t>()(h);
    }
};

typedef std::unordered_set<Face, FaceHash> FaceSet;
#endif

Polygons::ptr DomainSurface::createSurface(vistle::UnstructuredGrid::const_ptr m_grid_in,
                                           DomainSurface::DataMapping &em, bool haveElementData) const
{
    const bool useVertexOwners = getIntParameter("save_memory");
    const bool showgho = getIntParameter("ghost");
    const bool showtet = getIntParameter("tetrahedron");
    const bool showpyr = getIntParameter("pyramid");
    const bool showpri = getIntParameter("prism");
    const bool showhex = getIntParameter("hexahedron");
    const bool showpol = getIntParameter("polyhedron");
    const bool showtri = getIntParameter("triangle");
    const bool showqua = getIntParameter("quad");

    const Index num_elem = m_grid_in->getNumElements();
    const Index *el = &m_grid_in->el()[0];
    const Index *cl = &m_grid_in->cl()[0];
    const Byte *tl = &m_grid_in->tl()[0];
    Polygons::ptr m_grid_out(new Polygons(0, 0, 0));
    auto &pl = m_grid_out->el();
    auto &pcl = m_grid_out->cl();

    auto processElement = [&](Index i, FaceSet &visibleFaces) {
        const Index elStart = el[i], elEnd = el[i + 1];
        const Byte t = tl[i] & UnstructuredGrid::TYPE_MASK;
        if (t == UnstructuredGrid::VPOLYHEDRON) {
            Index faceNum = 0;
            Index j = elStart;
            while (j < elEnd) {
                Index numVert = cl[j];
                if (numVert >= 3) {
                    auto face = &cl[j + 1];
                    auto it_ok = visibleFaces.emplace(i, faceNum, numVert, face);
                    if (!it_ok.second) {
                        // found duplicate, hence inner face: remove
                        visibleFaces.erase(it_ok.first);
                    }
                }
                j += numVert + 1;
                ++faceNum;
            }
            if (j != elEnd) {
                std::cerr << "WARNING: Polyhedron incomplete: " << i << std::endl;
            }
        } else if (t == UnstructuredGrid::CPOLYHEDRON) {
            Index faceNum = 0;
            Index facestart = InvalidIndex;
            Index term = 0;
            for (Index j = elStart; j < elEnd; ++j) {
                if (facestart == InvalidIndex) {
                    facestart = j;
                    term = cl[j];
                } else if (cl[j] == term) {
                    Index numVert = j - facestart;
                    if (numVert >= 3) {
                        auto face = &cl[facestart];
                        auto it_ok = visibleFaces.emplace(i, faceNum, numVert, face);
                        if (!it_ok.second) {
                            // found duplicate, hence inner face: remove
                            visibleFaces.erase(it_ok.first);
                        }
                    }
                    facestart = InvalidIndex;
                    ++faceNum;
                }
            }
        } else {
            const auto numFaces = UnstructuredGrid::NumFaces[t];
            const auto &faces = UnstructuredGrid::FaceVertices[t];
            for (int f = 0; f < numFaces; ++f) {
                const auto &face = faces[f];
                const auto facesize = UnstructuredGrid::FaceSizes[t][f];
                auto it_ok = visibleFaces.emplace(i, f, facesize, cl + elStart, face);
                //std::cerr << "Face: " << *it_ok.first << std::endl;
                if (!it_ok.second) {
                    // found duplicate, hence inner face: remove
                    visibleFaces.erase(it_ok.first);
                }
            }
        }
    };

    auto addFace = [&](Index i, Index f) {
        auto elStart = el[i], elEnd = el[i + 1];
        Byte t = tl[i] & UnstructuredGrid::TYPE_MASK;
        switch (t) {
        case UnstructuredGrid::VPOLYHEDRON: {
            Index faceNum = 0;
            Index j = elStart;
            while (j < elEnd) {
                Index numVert = cl[j];
                if (faceNum == f && numVert >= 3) {
                    auto face = &cl[j + 1];
                    const Index *begin = &face[0], *end = &face[numVert];
                    auto rbegin = std::reverse_iterator<const Index *>(end),
                         rend = std::reverse_iterator<const Index *>(begin);
                    std::copy(rbegin, rend, std::back_inserter(pcl));
                    break;
                }
                j += numVert + 1;
                ++faceNum;
            }
            break;
        }
        case UnstructuredGrid::CPOLYHEDRON: {
            Index faceNum = 0;
            Index facestart = InvalidIndex;
            Index term = 0;
            for (Index j = elStart; j < elEnd; ++j) {
                if (facestart == InvalidIndex) {
                    facestart = j;
                    term = cl[j];
                } else if (cl[j] == term) {
                    Index numVert = j - facestart;
                    if (faceNum == f && numVert >= 3) {
                        auto face = &cl[facestart];
                        const Index *begin = &face[0], *end = &face[numVert];
                        auto rbegin = std::reverse_iterator<const Index *>(end),
                             rend = std::reverse_iterator<const Index *>(begin);
                        std::copy(rbegin, rend, std::back_inserter(pcl));
                        break;
                    }
                    facestart = InvalidIndex;
                    ++faceNum;
                }
            }
            break;
        }
        case UnstructuredGrid::PYRAMID:
        case UnstructuredGrid::PRISM:
        case UnstructuredGrid::TETRAHEDRON:
        case UnstructuredGrid::HEXAHEDRON:
        case UnstructuredGrid::TRIANGLE:
        case UnstructuredGrid::QUAD: {
            auto verts = &cl[elStart];
            const auto &faces = UnstructuredGrid::FaceVertices[t];
            const auto facesize = UnstructuredGrid::FaceSizes[t][f];
            const auto &face = faces[f];
            for (unsigned j = 0; j < facesize; ++j) {
                pcl.push_back(verts[face[j]]);
            }
            break;
        }
        }
        pl.push_back(pcl.size());
        if (haveElementData) {
            em.emplace_back(i);
        }
    };

    auto addToOutput = [&](const FaceSet &visibleFaces) mutable {
        for (const auto &f: visibleFaces) {
            const auto &i = f.elem;
            if (i == InvalidIndex)
                continue;

            bool ghost = tl[i] & UnstructuredGrid::GHOST_BIT;
            if (!showgho && ghost)
                continue;

            Byte t = tl[i] & UnstructuredGrid::TYPE_MASK;
            bool show = true;
            switch (t) {
            case UnstructuredGrid::VPOLYHEDRON:
            case UnstructuredGrid::CPOLYHEDRON:
                show = showpol;
                break;
            case UnstructuredGrid::PYRAMID:
                show = showpyr;
                break;
            case UnstructuredGrid::PRISM:
                show = showpri;
                break;
            case UnstructuredGrid::TETRAHEDRON:
                show = showtet;
                break;
            case UnstructuredGrid::HEXAHEDRON:
                show = showhex;
                break;
            case UnstructuredGrid::TRIANGLE:
                show = showtri;
                break;
            case UnstructuredGrid::QUAD:
                show = showqua;
                break;
            }
            if (!show)
                continue;

            addFace(i, f.face);
        }
    };

    if (!useVertexOwners) {
        FaceSet visibleFaces;
        for (Index i = 0; i < num_elem; ++i) {
            processElement(i, visibleFaces);
        }
        addToOutput(visibleFaces);
    } else {
        UnstructuredGrid::VertexOwnerList::const_ptr vol = m_grid_in->getVertexOwnerList();
        auto nf = m_grid_in->getNeighborFinder();
        for (Index i = 0; i < num_elem; ++i) {
            const Index elStart = el[i], elEnd = el[i + 1];
            bool ghost = tl[i] & UnstructuredGrid::GHOST_BIT;
            if (!showgho && ghost)
                continue;
            Byte t = tl[i] & UnstructuredGrid::TYPE_MASK;
            if (t == UnstructuredGrid::VPOLYHEDRON) {
                if (showpol) {
                    Index faceNum = 0;
                    Index j = elStart;
                    while (j < elEnd) {
                        Index numVert = cl[j];
                        if (numVert >= 3) {
                            auto face = &cl[j + 1];
                            Index neighbour = nf.getNeighborElement(i, face[0], face[1], face[2]);
                            if (neighbour == InvalidIndex) {
                                addFace(i, faceNum);
                            }
                        }
                        j += numVert + 1;
                        ++faceNum;
                    }
                    if (j != elEnd) {
                        std::cerr << "WARNING: Polyhedron incomplete: " << i << std::endl;
                    }
                }
            } else if (t == UnstructuredGrid::CPOLYHEDRON) {
                if (showpol) {
                    Index faceNum = 0;
                    Index facestart = InvalidIndex;
                    Index term = 0;
                    for (Index j = elStart; j < elEnd; ++j) {
                        if (facestart == InvalidIndex) {
                            facestart = j;
                            term = cl[j];
                        } else if (cl[j] == term) {
                            Index numVert = j - facestart;
                            if (numVert >= 3) {
                                auto face = &cl[facestart];
                                Index neighbour = nf.getNeighborElement(i, face[0], face[1], face[2]);
                                if (neighbour == InvalidIndex) {
                                    addFace(i, faceNum);
                                }
                            }
                            facestart = InvalidIndex;
                            ++faceNum;
                        }
                    }
                }
            } else {
                bool show = false;
                switch (t) {
                case UnstructuredGrid::PYRAMID:
                    show = showpyr;
                    break;
                case UnstructuredGrid::PRISM:
                    show = showpri;
                    break;
                case UnstructuredGrid::TETRAHEDRON:
                    show = showtet;
                    break;
                case UnstructuredGrid::HEXAHEDRON:
                    show = showhex;
                    break;
                case UnstructuredGrid::TRIANGLE:
                    show = showtri;
                    break;
                case UnstructuredGrid::QUAD:
                    show = showqua;
                    break;
                default:
                    break;
                }

                if (show) {
                    const auto numFaces = UnstructuredGrid::NumFaces[t];
                    const auto &faces = UnstructuredGrid::FaceVertices[t];
                    for (int f = 0; f < numFaces; ++f) {
                        const auto &face = faces[f];
                        Index neighbour = 0;
                        if (UnstructuredGrid::Dimensionality[t] == 3)
                            neighbour = nf.getNeighborElement(i, cl[elStart + face[0]], cl[elStart + face[1]],
                                                              cl[elStart + face[2]]);
                        if (UnstructuredGrid::Dimensionality[t] == 2 || neighbour == InvalidIndex) {
                            addFace(i, f);
                        }
                    }
                }
            }
        }
    }

    if (m_grid_out->getNumElements() == 0) {
        return Polygons::ptr();
    }

    return m_grid_out;
}

//bool DomainSurface::checkNormal(Index v1, Index v2, Index v3, Scalar x_center, Scalar y_center, Scalar z_center) {
//   Scalar *xcoord = m_grid_in->x().data();
//   Scalar *ycoord = m_grid_in->y().data();
//   Scalar *zcoord = m_grid_in->z().data();
//   Scalar a[3], b[3], c[3], n[3];

//   // compute normal of a=v2v1 and b=v2v3
//   a[0] = xcoord[v1] - xcoord[v2];
//   a[1] = ycoord[v1] - ycoord[v2];
//   a[2] = zcoord[v1] - zcoord[v2];
//   b[0] = xcoord[v3] - xcoord[v2];
//   b[1] = ycoord[v3] - ycoord[v2];
//   b[2] = zcoord[v3] - zcoord[v2];
//   n[0] = a[1] * b[2] - b[1] * a[2];
//   n[1] = a[2] * b[0] - b[2] * a[0];
//   n[2] = a[0] * b[1] - b[0] * a[1];

//   // compute vector from base-point to volume-center
//   c[0] = x_center - xcoord[v2];
//   c[1] = y_center - ycoord[v2];
//   c[2] = z_center - zcoord[v2];
//   // look if normal is correct or not
//   if ((c[0] * n[0] + c[1] * n[1] + c[2] * n[2]) > 0)
//       return false;
//   else
//       return true;
//}


MODULE_MAIN(DomainSurface)
