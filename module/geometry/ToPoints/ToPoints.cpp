#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/indexed.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/points.h>
#include <vistle/alg/objalg.h>

#include "ToPoints.h"

MODULE_MAIN(ToPoints)

using namespace vistle;

ToPoints::ToPoints(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    auto pin = createInputPort("grid_in", "grid or geometry");
    auto pout = createOutputPort("grid_out", "unconnected points");
    linkPorts(pin, pout);

    m_allCoordinates =
        addIntParameter("all_coordinates", "include all or only referenced coordinates", false, Parameter::Boolean);

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
        std::vector<Index> verts;
        bool indexed = false;
        if (!m_allCoordinates->getValue()) {
            Index nconn = 0;
            const Index *cl = nullptr;
            if (auto tri = Triangles::as(split.geometry)) {
                cl = tri->cl().data();
                nconn = tri->cl().size();
            } else if (auto quads = Quads::as(split.geometry)) {
                cl = quads->cl().data();
                nconn = quads->cl().size();
            } else if (auto indexed = Indexed::as(split.geometry)) {
                cl = indexed->cl().data();
                nconn = indexed->cl().size();
            }

            if (cl && nconn > 0) {
                indexed = true;
                verts.reserve(nconn);
                std::copy(cl, cl + nconn, std::back_inserter(verts));
                std::sort(verts.begin(), verts.end());
                auto end = std::unique(verts.begin(), verts.end());
                verts.resize(end - verts.begin());
            }
        }

        Object::ptr outGrid;
        if (auto gridEntry = m_gridCache.getOrLock(coords->getName(), outGrid)) {
            if (indexed) {
                const Index *cl = verts.data();
                size_t nconn = verts.size();
                auto points = std::make_shared<Points>(nconn);
                auto x = points->x().data(), y = points->y().data(), z = points->z().data();
                auto ix = coords->x(), iy = coords->y(), iz = coords->z();
                for (Index i = 0; i < nconn; ++i) {
                    Index ii = cl[i];
                    x[i] = ix[ii];
                    y[i] = iy[ii];
                    z[i] = iz[ii];
                }
                outGrid = points;
                outGrid->copyAttributes(coords);
            } else {
                Points::ptr points = Points::clone<Vec<Scalar, 3>>(coords);
                outGrid = points;
            }
            updateMeta(outGrid);
            m_gridCache.storeAndUnlock(gridEntry, outGrid);
        }
        if (split.mapped) {
            auto mapping = split.mapped->guessMapping();
            if (mapping != DataBase::Vertex) {
                sendError("data has to be mapped per vertex");
                out = outGrid;
            } else {
                DataBase::ptr data;
                if (indexed) {
                    const Index *cl = verts.data();
                    size_t nconn = verts.size();
                    auto odata = split.mapped->cloneType();
                    data = odata;
                    odata->setSize(nconn);
                    for (Index i = 0; i < nconn; ++i) {
                        Index ii = cl[i];
                        odata->copyEntry(i, split.mapped, ii);
                    }
                } else {
                    auto clone = split.mapped->clone();
                    data = clone;
                }
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
