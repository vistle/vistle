#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/util/coRestraint.h>
#include <vistle/util/enum.h>
#include <vistle/alg/objalg.h>
#include <vistle/core/geometry.h>

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

    bool operator()(Index element) { return m_restraint(m_marker[element]); }

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
    p_in[0] = createInputPort("threshold_in");
    p_out[0] = createOutputPort("threshold_out");
    for (unsigned i = 1; i < NUMPORTS; ++i) {
        p_in[i] = createInputPort("data_in" + std::to_string(i));
        p_out[i] = createOutputPort("data_out" + std::to_string(i));
    }

    p_reuse = addIntParameter("reuse_coordinates", "do not renumber vertices and reuse coordinate and data arrays",
                              false, Parameter::Boolean);
    p_invert = addIntParameter("invert_selection", "invert cell selection", false, Parameter::Boolean);
#ifdef CELLSELECT
    p_restraint = addStringParameter("selection", "values to select", "");
#else
    p_operation = addIntParameter("operation", "selection operation", AnyLess, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_operation, Operation);
    p_threshold = addFloatParameter("threshold", "selection threshold", 0);
#endif
}

Threshold::~Threshold()
{}

bool Threshold::prepare()
{
#ifdef CELLSELECT
    m_restraint.clear();
    m_restraint.add(p_restraint->getValue());
#endif
    m_invert = p_invert->getValue();
    return true;
}

template<class T, int Dim>
typename Vec<T, Dim>::ptr remapData(typename Vec<T, Dim>::const_ptr in, const Threshold::ElementsMapping &em,
                                    const Threshold::VerticesMapping &vm)
{
    if (in->guessMapping() == DataBase::Vertex) {
        typename Vec<T, Dim>::ptr out(new Vec<T, Dim>(vm.size()));

        const T *data_in[Dim];
        T *data_out[Dim];
        for (int d = 0; d < Dim; ++d) {
            data_in[d] = &in->x(d)[0];
            data_out[d] = out->x(d).data();
        }

        for (const auto &v: vm) {
            Index f = v.first;
            Index s = v.second;
            for (int d = 0; d < Dim; ++d) {
                data_out[d][s] = data_in[d][f];
            }
        }
        return out;
    } else if (in->guessMapping() == DataBase::Element) {
        typename Vec<T, Dim>::ptr out(new Vec<T, Dim>(em.size()));

        const T *data_in[Dim];
        T *data_out[Dim];
        for (int d = 0; d < Dim; ++d) {
            data_in[d] = &in->x(d)[0];
            data_out[d] = out->x(d).data();
        }

        Index s = 0;
        for (const auto &e: em) {
            Index f = e;
            for (int d = 0; d < Dim; ++d) {
                data_out[d][s] = data_in[d][f];
            }
            ++s;
        }
        return out;
    }

    return nullptr;
}

bool Threshold::compute(std::shared_ptr<BlockTask> task) const
{
    auto obj = task->expect<Object>(p_in[0]);
    auto split = splitContainerObject(obj);
    auto &grid = split.geometry;
    auto &mapped = split.mapped;
    auto ugrid = UnstructuredGrid::as(grid);
    auto poly = Polygons::as(grid);

    DataBase::const_ptr data = mapped;
    if (!ugrid && !poly) {
        sendError("no valid grid received on data_in");
        return true;
    }
    Indexed::const_ptr grid_in = ugrid ? Indexed::as(ugrid) : Indexed::as(poly);
    assert(grid_in);

#ifdef CELLSELECT
    CellSelector select(m_restraint, data);
#else
    CellSelector select((Operation)p_operation->getValue(), p_threshold->getValue(), data);
#endif

    auto outgrid = grid_in->cloneType();

    VerticesMapping vm;
    ElementsMapping em;
    const Index *icl = &grid_in->cl()[0];
    const Index *iel = &grid_in->el()[0];
    const Byte *itl = nullptr;
    if (ugrid) {
        itl = &ugrid->tl()[0];
    }

    Index nelem = grid_in->getNumElements();
    Index ncorn = 0;
    for (Index e = 0; e < nelem; ++e) {
        const Index begin = iel[e], end = iel[e + 1];
        if (m_invert ^ select(e)) {
            em.push_back(e);
            ncorn += end - begin;
        }
    }

    outgrid->el().resize(em.size() + 1);
    outgrid->cl().resize(ncorn);
    auto *el = &outgrid->el()[0];
    auto *cl = &outgrid->cl()[0];
    Byte *tl = nullptr;
    if (ugrid && !em.empty()) {
        auto otl = &std::dynamic_pointer_cast<UnstructuredGrid>(outgrid)->tl();
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
            *tl = itl[e];
            ++tl;
        }
    }
    *el = cidx;

    renumberVertices(grid_in, outgrid, vm);
    outgrid->setMeta(grid_in->meta());
    outgrid->copyAttributes(grid_in);

    for (unsigned i = 0; i < NUMPORTS; ++i) {
        if (!p_out[i]->isConnected())
            continue;
        if (!p_in[i]->isConnected()) {
            sendError("require input on %s for %s", p_in[i]->getName().c_str(), p_out[i]->getName().c_str());
            return true;
        }
        auto din = i > 0 ? splitContainerObject(task->expect<Object>(p_in[i])) : split;
        if (din.geometry->getHandle() != grid_in->getHandle()) {
            sendError("grids do not match");
            return true;
        }
        auto &data = din.mapped;
        if (!data) {
            task->addObject(p_out[i], outgrid);
        } else if (vm.empty()) {
            DataBase::ptr dout = data->clone();
            dout->setGrid(outgrid);
            updateMeta(dout);
            task->addObject(p_out[i], dout);
        } else {
            DataBase::ptr data_obj_out;
            if (auto data_in = Vec<Scalar, 3>::as(data)) {
                data_obj_out = remapData<Scalar, 3>(data_in, em, vm);
            } else if (auto data_in = Vec<Scalar, 1>::as(data)) {
                data_obj_out = remapData<Scalar, 1>(data_in, em, vm);
            } else if (auto data_in = Vec<Index, 3>::as(data)) {
                data_obj_out = remapData<Index, 3>(data_in, em, vm);
            } else if (auto data_in = Vec<Index, 1>::as(data)) {
                data_obj_out = remapData<Index, 1>(data_in, em, vm);
            } else if (auto data_in = Vec<Byte, 3>::as(data)) {
                data_obj_out = remapData<Byte, 3>(data_in, em, vm);
            } else if (auto data_in = Vec<Byte, 1>::as(data)) {
                data_obj_out = remapData<Byte, 1>(data_in, em, vm);
            } else {
                std::cerr << "WARNING: No valid 1D or 3D data on input port" << std::endl;
            }

            if (data_obj_out) {
                data_obj_out->setGrid(outgrid);
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

        const Scalar *xcoord = &coords->x()[0];
        const Scalar *ycoord = &coords->y()[0];
        const Scalar *zcoord = &coords->z()[0];
        auto &px = poly->x();
        auto &py = poly->y();
        auto &pz = poly->z();
        px.resize(c);
        py.resize(c);
        pz.resize(c);

        for (const auto &v: vm) {
            Index f = v.first;
            Index s = v.second;
            px[s] = xcoord[f];
            py[s] = ycoord[f];
            pz[s] = zcoord[f];
        }
    }
}

MODULE_MAIN(Threshold)
