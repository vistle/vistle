#include "SelectVertices.h"

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
#include <vistle/alg/fields.h>


using namespace vistle;

SelectVertices::SelectVertices(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    p_in[0] = createInputPort("data_in", "input ");
    p_out[0] = createOutputPort("data_out", "output data");
    linkPorts(p_in[0], p_out[0]);
    for (unsigned i = 1; i < NUMPORTS; ++i) {
        p_in[i] = createInputPort("data_in" + std::to_string(i), "additional input data");
        p_out[i] = createOutputPort("data_out" + std::to_string(i), "additional output data");
        linkPorts(p_in[i], p_out[i]);
        setPortOptional(p_in[i], true);
    }

    p_invert = addIntParameter("invert_selection", "invert vertex selection", false, Parameter::Boolean);
    p_restraint = addStringParameter("selection", "vertices to select", "", Parameter::Restraint);
    p_blockRestraint = addStringParameter("block_selection", "blocks to select", "all", Parameter::Restraint);

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
        DataBase::ptr data_obj_out = remapData(data, vm);

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
