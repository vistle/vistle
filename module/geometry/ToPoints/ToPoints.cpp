#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/alg/objalg.h>

#include "ToPoints.h"

MODULE_MAIN(ToPoints)

using namespace vistle;

ToPoints::ToPoints(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "grid or geometry");
    createOutputPort("grid_out", "unconnected points");

    addResultCache(m_gridCache);
    addResultCache(m_resultCache);
}

ToPoints::~ToPoints()
{}

bool ToPoints::compute()
{
    auto o = expect<Object>("grid_in");
    auto split = splitContainerObject(o);
    auto coords = Coords::as(split.geometry);
    if (!coords) {
        sendError("no coordinates on input");
        return true;
    }

    Object::ptr out;
    if (auto resultEntry = m_resultCache.getOrLock(o->getName(), out)) {
        Object::ptr outGrid;
        if (auto gridEntry = m_gridCache.getOrLock(coords->getName(), outGrid)) {
            Points::ptr points = Points::clone<Vec<Scalar, 3>>(coords);
            updateMeta(points);
            outGrid = points;
            m_gridCache.storeAndUnlock(gridEntry, outGrid);
        }
        if (split.mapped) {
            auto mapping = split.mapped->guessMapping();
            if (mapping != DataBase::Vertex) {
                sendError("data has to be mapped per vertex");
                out = outGrid;
            } else {
                auto data = split.mapped->clone();
                data->setGrid(outGrid);
                updateMeta(data);
                out = data;
            }
        } else {
            out = outGrid;
        }
        m_resultCache.storeAndUnlock(resultEntry, out);
    }

    addObject("grid_out", out);

    return true;
}
