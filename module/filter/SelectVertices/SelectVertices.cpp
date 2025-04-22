#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/points.h>
#include <vistle/core/coords.h>
#include <vistle/util/coRestraint.h>
#include <vistle/util/enum.h>
#include <vistle/alg/objalg.h>
#include <vistle/core/geometry.h>

#include "SelectVertices.h"

using namespace vistle;

SelectVertices::SelectVertices(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    p_in[0] = createInputPort("data_in", "input ");
    p_out[0] = createOutputPort("data_out", "output data");
    for (unsigned i = 1; i < NUMPORTS; ++i) {
        p_in[i] = createInputPort("data_in" + std::to_string(i), "additional input data");
        p_out[i] = createOutputPort("data_out" + std::to_string(i), "additional output data");
    }

    p_invert = addIntParameter("invert_selection", "invert vertex selection", false, Parameter::Boolean);
    p_restraint = addStringParameter("selection", "vertices to select", "");
    p_blockRestraint = addStringParameter("block_selection", "blocks to select", "all");

    addResultCache(m_gridCache);
}

SelectVertices::~SelectVertices()
{}

bool SelectVertices::prepare()
{
    m_invert = p_invert->getValue();

    m_restraint.clear();
    m_restraint.add(p_restraint->getValue());

    m_blockRestraint.clear();
    m_blockRestraint.add(p_blockRestraint->getValue());

    return true;
}

template<class T, int Dim>
typename Vec<T, Dim>::ptr remapData(typename Vec<T, Dim>::const_ptr in, const SelectVertices::VerticesMapping &vm)
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
            assert(f < in->getSize());
            assert(s < out->getSize());
            for (int d = 0; d < Dim; ++d) {
                data_out[d][s] = data_in[d][f];
            }
        }

        out->copyAttributes(in);
        return out;
    }

    return nullptr;
}

bool SelectVertices::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr grid;
    int block = -1;
    std::array<DataBase::const_ptr, NUMPORTS> d_in;
    for (unsigned i = 0; i < NUMPORTS; ++i) {
        if (p_in[i]->isConnected()) {
            auto obj = task->expect<Object>(p_in[0]);
            auto split = splitContainerObject(obj);
            if (!grid) {
                grid = split.geometry;
            } else if (grid->getHandle() != split.geometry->getHandle()) {
                sendError("multiple geometries");
                return true;
            }
            d_in[i] = split.mapped;

            if (block < 0) {
                block = split.block;
            }
        }
        if (!p_out[i]->isConnected())
            continue;
        if (!p_in[i]->isConnected()) {
            sendError("require input on %s for %s", p_in[i]->getName().c_str(), p_out[i]->getName().c_str());
            return true;
        }
        if (d_in[i] && d_in[i]->guessMapping(grid) != DataBase::Vertex) {
            sendError("data mapping on %s must be per-vertex", p_in[i]->getName().c_str());
            return true;
        }
    }
    if (!grid) {
        sendError("no geometry on any input port");
        return true;
    }

    if (!m_blockRestraint(block)) {
        return true;
    }

    CachedResult cachedResult;
    if (auto cacheEntry = m_gridCache.getOrLock(grid->getName(), cachedResult)) {
        const auto &select = m_restraint;

        auto points = std::make_shared<Points>(0);
        VerticesMapping &vm = cachedResult.vm;
        auto geo = grid->getInterface<GeometryInterface>();
        Index nvert = geo->getNumVertices();

        auto &x = points->x();
        auto &y = points->y();
        auto &z = points->z();

        Index idx = 0;
        for (Index i = 0; i < nvert; ++i) {
            if (m_invert ^ select(i)) {
                vm[i] = idx;
                ++idx;

                auto coord = geo->getVertex(i);

                x.push_back(coord[0]);
                y.push_back(coord[1]);
                z.push_back(coord[2]);
            }
        }
        assert(idx == vm.size());
        assert(points->getNumPoints() == idx);

        points->copyAttributes(grid);
        updateMeta(points);

        cachedResult.points = points;
        m_gridCache.storeAndUnlock(cacheEntry, cachedResult);
    }

    auto &points = cachedResult.points;
    auto &vm = cachedResult.vm;
    for (unsigned i = 0; i < NUMPORTS; ++i) {
        if (!p_out[i]->isConnected())
            continue;

        auto &data = d_in[i];
        DataBase::ptr data_obj_out;
        if (auto data_in = Vec<Scalar, 3>::as(data)) {
            data_obj_out = remapData<Scalar, 3>(data_in, vm);
        } else if (auto data_in = Vec<Scalar, 1>::as(data)) {
            data_obj_out = remapData<Scalar, 1>(data_in, vm);
        } else if (auto data_in = Vec<Index, 3>::as(data)) {
            data_obj_out = remapData<Index, 3>(data_in, vm);
        } else if (auto data_in = Vec<Index, 1>::as(data)) {
            data_obj_out = remapData<Index, 1>(data_in, vm);
        } else if (auto data_in = Vec<Byte, 3>::as(data)) {
            data_obj_out = remapData<Byte, 3>(data_in, vm);
        } else if (auto data_in = Vec<Byte, 1>::as(data)) {
            data_obj_out = remapData<Byte, 1>(data_in, vm);
        } else {
            std::cerr << "WARNING: No valid 1D or 3D data on input port" << std::endl;
        }

        if (data_obj_out) {
            assert(points->getSize() == data_obj_out->getSize());
            data_obj_out->setGrid(points);
            updateMeta(data_obj_out);
            task->addObject(p_out[i], data_obj_out);
        } else {
            task->addObject(p_out[i], points);
        }
    }

    return true;
}


MODULE_MAIN(SelectVertices)
