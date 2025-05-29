#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/lines.h>
#include <vistle/util/coRestraint.h>
#include <vistle/util/enum.h>
#include <vistle/alg/objalg.h>
#include <vistle/alg/fields.h>
#include <vistle/core/geometry.h>
#include <vistle/core/structuredgridbase.h>

#include "Threshold.h"

using namespace vistle;

namespace {

#ifdef CELLSELECT
class CellSelector {
public:
    CellSelector(const coRestraint &restraint, DataBase::const_ptr &data): m_restraint(restraint), m_data(data)
    {
        if (auto idx = Vec<Index>::as(m_data)) {
            m_marker = &idx->x()[0];
        }
    }

    bool operator()(Index element) { return m_restraint(m_marker ? m_marker[element] : element); }

private:
    coRestraint m_restraint;
    DataBase::const_ptr m_data;
    const Index *m_marker = nullptr;
};
#else

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Operation, (AnyLess)(AllLess)(Contains)(AnyGreater)(AllGreater))
class CellSelector {
public:
    CellSelector(Operation op, Float threshold, DataBase::const_ptr &data)
    : m_operation(op)
    , m_threshold(threshold)
    , m_data(data)
    , m_mapping(data->guessMapping())
    , m_grid(data->grid())
    , m_elif(m_grid->getInterface<ElementInterface>())
    {
        if (auto scal = Vec<Scalar>::as(m_data)) {
            m_scalar = &scal->x()[0];
        }
        if (auto idx = Vec<Index>::as(m_data)) {
            m_index = &idx->x()[0];
        }
        if (auto bt = Vec<Byte>::as(m_data)) {
            m_byte = &bt->x()[0];
        }
    }

    bool operator()(Index element)
    {
        if (m_mapping != DataBase::Vertex && m_mapping != DataBase::Element)
            return false;
        if (!m_scalar && !m_index && !m_byte)
            return false;

        std::vector<Float> vals;
        vals.reserve(8);
        switch (m_mapping) {
        case DataBase::Vertex:
            for (auto v: m_elif->cellVertices(element)) {
                if (m_index)
                    vals.push_back(m_index[v]);
                if (m_byte)
                    vals.push_back(m_byte[v]);
                if (m_scalar)
                    vals.push_back(m_scalar[v]);
            }
            break;
        case DataBase::Element:
            if (m_index)
                vals.push_back(m_index[element]);
            if (m_byte)
                vals.push_back(m_byte[element]);
            if (m_scalar)
                vals.push_back(m_scalar[element]);
            break;
        default:
            return false;
        }

        switch (m_operation) {
        case AnyLess:
            for (const auto v: vals) {
                if (v < m_threshold)
                    return true;
            }
            return false;
        case AllLess:
            for (const auto v: vals) {
                if (v >= m_threshold)
                    return false;
            }
            return true;
        case Contains: {
            unsigned count = 0;
            for (const auto v: vals) {
                if (v == m_threshold)
                    return true;
                if (v < m_threshold)
                    ++count;
            }
            return count != 0 && count != vals.size();
        }
        case AnyGreater:
            for (const auto v: vals) {
                if (v > m_threshold)
                    return true;
            }
            return false;
        case AllGreater:
            for (const auto v: vals) {
                if (v <= m_threshold)
                    return false;
            }
            return true;
        default:
            assert("operation not implemented" == nullptr);
            break;
        }
        return false;
    }

private:
    Operation m_operation;
    Float m_threshold;
    DataBase::const_ptr m_data;
    DataBase::Mapping m_mapping;
    Object::const_ptr m_grid;
    const ElementInterface *m_elif = nullptr;
    const Index *m_index = nullptr;
    const Byte *m_byte = nullptr;
    const Scalar *m_scalar = nullptr;
};
#endif
} // namespace

Threshold::Threshold(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
#ifdef CELLSELECT
    p_in[0] = createInputPort("data_in", "input data");
    p_out[0] = createOutputPort("data_out", "output data");
#else
    p_in[0] = createInputPort("threshold_in", "scalar data deciding over elements rejection");
    p_out[0] = createOutputPort("threshold_out", "threshold input filtered to remaining elements");
#endif
    linkPorts(p_in[0], p_out[0]);
    for (unsigned i = 1; i < NUMPORTS; ++i) {
        p_in[i] = createInputPort("data_in" + std::to_string(i), "additional input data");
        p_out[i] = createOutputPort("data_out" + std::to_string(i), "additional output data");
        linkPorts(p_in[i], p_out[i]);
        setPortOptional(p_in[i], true);
    }

    p_reuse = addIntParameter("reuse_coordinates",
                              "do not renumber vertices and reuse coordinate and data arrays (if possible)", false,
                              Parameter::Boolean);
    p_invert = addIntParameter("invert_selection", "invert cell selection", false, Parameter::Boolean);
#ifdef CELLSELECT
    p_restraint = addStringParameter("selection", "values to select", "", Parameter::Restraint);
#else
    p_operation = addIntParameter("operation", "selection operation", AnyLess, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_operation, Operation);
    p_threshold = addFloatParameter("threshold", "selection threshold", 0);
#endif

    addResultCache(m_gridCache);
}

Threshold::~Threshold()
{}

bool Threshold::changeParameter(const vistle::Parameter *p)
{
#ifndef CELLSELECT
    auto val = p_threshold->getValue();
    std::stringstream str;
    str << std::setw(0) << std::setprecision(3) << val;
    setItemInfo(str.str());
#endif
    return Module::changeParameter(p);
}

bool Threshold::prepare()
{
#ifdef CELLSELECT
    m_restraint.clear();
    m_restraint.add(p_restraint->getValue());
#endif
    m_invert = p_invert->getValue();
    return true;
}

bool Threshold::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto obj = task->expect<Object>(p_in[0]);
    auto split = splitContainerObject(obj);
    auto &grid = split.geometry;
    auto &mapped = split.mapped;
    auto ugrid = UnstructuredGrid::as(grid);
    auto poly = Polygons::as(grid);
    auto line = Lines::as(grid);
    auto coords = Coords::as(grid);
    auto sgrid = StructuredGridBase::as(grid);
    auto grid_in = grid->getInterface<ElementInterface>();

    if (!ugrid && !poly && !line && !sgrid) {
        sendError("no valid grid received on threshold_in");
        return true;
    }
    Indexed::const_ptr indexed = ugrid ? Indexed::as(ugrid) : poly ? Indexed::as(poly) : Indexed::as(line);
    assert(indexed || sgrid);
    assert(grid_in);

    Object::const_ptr data = mapped;
#ifdef CELLSELECT
    if (mapped) {
        if (!Vec<Index>::as(mapped)) {
            sendError("only Index data supported on threshold_in");
            return true;
        }
        if (mapped->guessMapping() != DataBase::Element) {
            sendError("per-element mapping required on threshold_in");
            return true;
        }
    } else {
        data = grid;
    }
#else
    if (!mapped) {
        sendError("no mapped data received on threshold_in");
        return true;
    }
#endif

    CachedResult cachedResult;
    if (auto cacheEntry = m_gridCache.getOrLock(data->getName(), cachedResult)) {
#ifdef CELLSELECT
        CellSelector select(m_restraint, mapped);
#else
        CellSelector select((Operation)p_operation->getValue(), p_threshold->getValue(), mapped);
#endif

        Object::ptr outgeo;
        Indexed::ptr outgrid;
        Quads::ptr outquads;
        VerticesMapping &vm = cachedResult.vm;
        ElementsMapping &em = cachedResult.em;
        const Index *icl = nullptr;
        const Index *iel = nullptr;
        const Byte *itl = nullptr;
        if (indexed) {
            outgrid = indexed->cloneType();
            outgeo = outgrid;

            icl = &indexed->cl()[0];
            iel = &indexed->el()[0];
            if (ugrid) {
                itl = &ugrid->tl()[0];
            }
        } else if (sgrid) {
            Index dims[]{sgrid->getNumDivisions(0), sgrid->getNumDivisions(1), sgrid->getNumDivisions(2)};
            int dim = sgrid->dimensionality(dims);
            if (dim == 3) {
                outgrid = std::make_shared<UnstructuredGrid>(0, 0, 0);
                outgeo = outgrid;
            } else {
                outquads = std::make_shared<Quads>(0, 0);
                outgeo = outquads;
            }
        }

        Index nelem = grid_in->getNumElements();
        Index ncorn = 0;
        for (Index e = 0; e < nelem; ++e) {
            if (m_invert ^ select(e)) {
                em.push_back(e);
                if (iel) {
                    const Index begin = iel[e], end = iel[e + 1];
                    ncorn += end - begin;
                } else if (outquads) {
                    ncorn += 4;
                } else {
                    ncorn += grid_in->cellNumVertices(e);
                }
            }
        }

        if (outgrid) {
            outgrid->el().resize(em.size() + 1);
            outgrid->cl().resize(ncorn);
            auto *el = &outgrid->el()[0];
            auto *cl = &outgrid->cl()[0];
            Byte *tl = nullptr;
            auto outugrid = UnstructuredGrid::as(outgrid);
            if (outugrid && !em.empty()) {
                auto otl = &outugrid->tl();
                otl->resize(em.size());
                tl = &otl->at(0);
            }

            Index cidx = 0;
            for (const auto &e: em) {
                *el = cidx;
                ++el;

                const Index begin = iel[e], end = iel[e + 1];
                for (Index i = begin; i < end; ++i) {
                    cl[cidx] = icl[i];
                    ++cidx;
                }
                if (tl) {
                    if (itl)
                        *tl = itl[e];
                    else
                        *tl = UnstructuredGrid::HEXAHEDRON;
                    ++tl;
                }

                *el = cidx;
            }

        } else if (outquads) {
            outquads->cl().resize(em.size() * 4);
            auto *cl = &outquads->cl()[0];

            Index cidx = 0;
            for (const auto &e: em) {
                const auto vert = grid_in->cellVertices(e);
                for (int i = 0; i < 4; ++i) {
                    cl[cidx] = vert[i];
                    ++cidx;
                }
            }
        }
        if (outquads) {
            renumberVertices<Quads>(sgrid, outquads, vm);
        } else if (coords) {
            assert(outgrid);
            renumberVertices(coords, outgrid, vm);
        } else {
            assert(outgrid);
            renumberVertices<Indexed>(sgrid, outgrid, vm);
        }
        updateMeta(outgeo);

        cachedResult.grid = outgeo;
        m_gridCache.storeAndUnlock(cacheEntry, cachedResult);
    }
    auto &outgeo = cachedResult.grid;
    auto &em = cachedResult.em;
    auto &vm = cachedResult.vm;

    for (unsigned i = 0; i < NUMPORTS; ++i) {
        if (!p_out[i]->isConnected())
            continue;
        if (!p_in[i]->isConnected()) {
            sendError("require input on %s for %s", p_in[i]->getName().c_str(), p_out[i]->getName().c_str());
            return true;
        }
        auto din = i > 0 ? splitContainerObject(task->expect<Object>(p_in[i])) : split;
        if (din.geometry->getHandle() != grid->getHandle()) {
            sendError("grids do not match");
            return true;
        }
        auto &data = din.mapped;
        if (!data) {
            task->addObject(p_out[i], outgeo);
        } else if (vm.empty()) {
            DataBase::ptr dout = data->clone();
            dout->setGrid(outgeo);
            updateMeta(dout);
            task->addObject(p_out[i], dout);
        } else {
            DataBase::ptr data_obj_out;
            if (data->guessMapping(grid) == DataBase::Vertex) {
                data_obj_out = remapData(data, vm);
            } else {
                data_obj_out = remapData(data, em, true);
            }

            if (data_obj_out) {
                data_obj_out->setGrid(outgeo);
                data_obj_out->setMeta(data->meta());
                data_obj_out->copyAttributes(data);
                updateMeta(data_obj_out);
                task->addObject(p_out[i], data_obj_out);
            }
        }
    }

    return true;
}


void Threshold::renumberVertices(Coords::const_ptr coords, Indexed::ptr poly, VerticesMapping &vm) const
{
    const bool reuseCoord = p_reuse->getValue();

    if (reuseCoord) {
        poly->d()->x[0] = coords->d()->x[0];
        poly->d()->x[1] = coords->d()->x[1];
        poly->d()->x[2] = coords->d()->x[2];
    } else {
        Index c = 0;
        for (Index &v: poly->cl()) {
            if (vm.emplace(v, c).second) {
                v = c;
                ++c;
            } else {
                v = vm[v];
            }
        }

        auto &px = poly->x();
        auto &py = poly->y();
        auto &pz = poly->z();
        px.resize(c);
        py.resize(c);
        pz.resize(c);

        const Scalar *xcoord = &coords->x()[0];
        const Scalar *ycoord = &coords->y()[0];
        const Scalar *zcoord = &coords->z()[0];

        for (const auto &v: vm) {
            Index f = v.first;
            Index s = v.second;
            px[s] = xcoord[f];
            py[s] = ycoord[f];
            pz[s] = zcoord[f];
        }
    }
}

template<class Geometry>
void Threshold::renumberVertices(StructuredGridBase::const_ptr sgrid, typename Geometry::ptr geo,
                                 VerticesMapping &vm) const
{
    Index c = 0;
    for (Index &v: geo->cl()) {
        if (vm.emplace(v, c).second) {
            v = c;
            ++c;
        } else {
            v = vm[v];
        }
    }

    auto &px = geo->x();
    auto &py = geo->y();
    auto &pz = geo->z();
    px.resize(c);
    py.resize(c);
    pz.resize(c);

    for (const auto &v: vm) {
        Index f = v.first;
        Index s = v.second;
        auto p = sgrid->getVertex(f);
        px[s] = p[0];
        py[s] = p[1];
        pz[s] = p[2];
    }
}

MODULE_MAIN(Threshold)
