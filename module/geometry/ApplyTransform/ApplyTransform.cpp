#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/alg/objalg.h>

#include "ApplyTransform.h"

MODULE_MAIN(ApplyTransform)

using namespace vistle;

ApplyTransform::ApplyTransform(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in", "grid or geometry");
    createOutputPort("grid_out", "unconnected points");

    addResultCache(m_gridCache);
    addResultCache(m_resultCache);
}

ApplyTransform::~ApplyTransform()
{}

bool ApplyTransform::compute()
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
            auto T = coords->getTransform();
            auto clone = coords->clone();
            if (!T.isIdentity()) {
                clone->resetCoords();
                clone->setTransform(vistle::Matrix4::Identity());
                Index ncoords = coords->getSize();
                clone->setSize(ncoords);
                auto *x = coords->x().data(), *y = coords->y().data(), *z = coords->z().data();
                auto *nx = clone->x().data(), *ny = clone->y().data(), *nz = clone->z().data();
                for (Index i = 0; i < ncoords; ++i) {
                    auto p = T * Vector4(x[i], y[i], z[i], Scalar(1));
                    nx[i] = p[0] / p[3];
                    ny[i] = p[1] / p[3];
                    nz[i] = p[2] / p[3];
                }
            }
            updateMeta(clone);
            outGrid = clone;
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
