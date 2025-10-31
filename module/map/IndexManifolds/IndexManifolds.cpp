#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/quads.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/util/enum.h>
#include <vistle/alg/objalg.h>

#include "IndexManifolds.h"

MODULE_MAIN(IndexManifolds)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Direction, (X)(Y)(Z))

IndexManifolds::IndexManifolds(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    p_data_in = createInputPort("data_in0", "(data on) structured grid");
    p_surface_out = createOutputPort("surface_out0", "2D slice of input");
    p_line_out = createOutputPort("line_out0", "1D line of input");
    p_point_out = createOutputPort("point_out0", "0D point of input");
    linkPorts(p_data_in, p_surface_out);
    linkPorts(p_data_in, p_line_out);
    linkPorts(p_data_in, p_point_out);

    p_coord = addIntVectorParameter("coord", "coordinates of point on surface/line", IntParamVector(0, 0, 0));
    setParameterMinimum(p_coord, IntParamVector(0, 0, 0));
    p_direction = addIntParameter("direction", "normal on surface and direction of line", Z, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_direction, Direction);

    addResultCache(m_surfaceCache);
}

bool IndexManifolds::changeParameter(const vistle::Parameter *p)
{
    if (p == p_coord || p == p_direction) {
        Direction dir = static_cast<Direction>(p_direction->getValue());
        auto c = p_coord->getValue();
        std::stringstream str;
        str << toString(dir) << "=" << c[dir];
        setItemInfo(str.str());
    }
    return Module::changeParameter(p);
}

bool IndexManifolds::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto obj = task->expect<Object>("data_in0");
    if (!obj) {
        sendError("no input");
        return true;
    }

    auto split = splitContainerObject(obj);

    Object::const_ptr ingrid = split.geometry;
    auto data = split.mapped;

    if (!ingrid) {
        sendError("no grid on input");
        return true;
    }

    auto str = StructuredGridBase::as(ingrid);
    if (!str) {
        sendError("no structured grid on input");
        return true;
    }

    bool elementData = data && data->guessMapping() == DataBase::Element;

    Direction dir = static_cast<Direction>(p_direction->getValue());
    Direction dir1 = dir == X ? Y : X;
    Direction dir2 = dir == Z ? Y : Z;
    Index coord[3];
    Index c[3];
    Index off[3];
    Index bghost[3], tghost[3];
    Index dims[3];
    for (int d = 0; d < 3; ++d) {
        dims[d] = str->getNumDivisions(d);
        coord[d] = p_coord->getValue()[d];
        c[d] = coord[d];
        off[d] = str->getGlobalIndexOffset(d);
        if (c[d] < off[d])
            c[d] = InvalidIndex;
        else
            c[d] -= off[d];
        bghost[d] = str->getNumGhostLayers(d, StructuredGridBase::Bottom);
        tghost[d] = str->getNumGhostLayers(d, StructuredGridBase::Top);
    }

    if (isConnected(*p_surface_out) && off[dir] + bghost[dir] <= coord[dir] &&
        off[dir] + str->getNumDivisions(dir) > coord[dir] + tghost[dir]) {
        // compute surface
        Index nvert1 = dims[dir1] - bghost[dir1] - tghost[dir1];
        Index nvert2 = dims[dir2] - bghost[dir2] - tghost[dir2];
        Index nvert = nvert1 * nvert2;
        Index nquad = (nvert1 - 1) * (nvert2 - 1);

        Quads::ptr surface;
        auto cacheEntry = m_surfaceCache.getOrLock(ingrid->getName(), surface);
        if (!cacheEntry) {
            if (!data)
                surface = surface->clone();
        } else {
            surface.reset(new Quads(nquad * 4, nvert));
            surface->copyAttributes(ingrid);

            Scalar *x[3];
            for (int d = 0; d < 3; ++d) {
                x[d] = surface->x(d).data();
            }
            Index *cl = surface->cl().data();
            Index ii = 0;
            for (Index i = 0; i < nvert1 - 1; ++i) {
                for (Index j = 0; j < nvert2 - 1; ++j) {
                    cl[ii++] = i * nvert2 + j;
                    cl[ii++] = (i + 1) * nvert2 + j;
                    cl[ii++] = (i + 1) * nvert2 + j + 1;
                    cl[ii++] = i * nvert2 + j + 1;
                }
            }
            assert(ii == nquad * 4);

            Index cc[3]{c[0], c[1], c[2]};
            cc[dir1] = bghost[dir1];
            Index vv = 0;
#ifndef NDEBUG
            Index cell = 0;
#endif
            for (Index i = 0; i < nvert1; ++i) {
                cc[dir2] = bghost[dir2];
                for (Index j = 0; j < nvert2; ++j) {
                    Index v = StructuredGridBase::vertexIndex(cc[0], cc[1], cc[2], dims);
                    auto p = str->getVertex(v);
                    for (int d = 0; d < 3; ++d)
                        x[d][vv] = p[d];

                    ++vv;
                    ++cc[dir2];
                }
                ++cc[dir1];
            }
            assert(vv == nvert);
            assert(cell == 0 || cell == nquad);

            updateMeta(surface);
            m_surfaceCache.storeAndUnlock(cacheEntry, surface);
        }

        if (!data) {
            task->addObject(p_surface_out, surface);
        } else {
            DataBase::ptr outdata = data->cloneType();
            outdata->setMapping(elementData ? DataBase::Element : DataBase::Vertex);
            outdata->setGrid(surface);
            outdata->setSize(elementData ? nquad : nvert);

            Index cc[3]{c[0], c[1], c[2]};
            cc[dir1] = bghost[dir1];
            Index vv = 0;
            Index cell = 0;
            for (Index i = 0; i < nvert1; ++i) {
                cc[dir2] = bghost[dir2];
                for (Index j = 0; j < nvert2; ++j) {
                    Index v = StructuredGridBase::vertexIndex(cc[0], cc[1], cc[2], dims);
                    if (elementData) {
                        if (i + 1 < nvert1 && j + 1 < nvert2) {
                            Index c = StructuredGridBase::cellIndex(cc[0], cc[1], cc[2], dims);
                            outdata->copyEntry(cell, data, c);
                            ++cell;
                        }
                    } else {
                        outdata->copyEntry(vv, data, v);
                    }

                    ++vv;
                    ++cc[dir2];
                }
                ++cc[dir1];
            }
            assert(vv == nvert);
            assert(cell == 0 || cell == nquad);

            updateMeta(outdata);
            task->addObject(p_surface_out, outdata);
        }
    }

    if (isConnected(*p_line_out) && off[dir1] + bghost[dir1] <= coord[dir1] &&
        off[dir1] + str->getNumDivisions(dir1) > coord[dir1] + tghost[dir1] &&
        off[dir2] + bghost[dir2] <= coord[dir2] &&
        off[dir2] + str->getNumDivisions(dir2) > coord[dir2] + tghost[dir2]) {
        // compute line
        Index nvert = dims[dir] - bghost[dir] - tghost[dir];

        Lines::ptr line(new Lines(1, nvert, nvert));
        line->el()[0] = 0;
        line->el()[1] = nvert;
        line->copyAttributes(ingrid);
        DataBase::ptr outdata;
        if (data) {
            outdata = data->cloneType();
            outdata->setSize(elementData ? nvert - 1 : nvert);
            outdata->setGrid(line);
        }

        Index cell = 0;
        Index cc[3]{c[0], c[1], c[2]};
        cc[dir] = bghost[dir];
        for (Index i = 0; i < nvert; ++i) {
            Index v = StructuredGridBase::vertexIndex(cc[0], cc[1], cc[2], dims);
            auto p = str->getVertex(v);

            for (int d = 0; d < 3; ++d) {
                line->x(d)[cc[dir]] = p[d];
            }
            if (outdata) {
                if (elementData) {
                    if (i + 1 < nvert) {
                        Index c = StructuredGridBase::cellIndex(cc[0], cc[1], cc[2], dims);
                        outdata->copyEntry(cell, data, c);
                        ++cell;
                    }
                } else {
                    outdata->copyEntry(cc[dir], data, v);
                }
            }
            line->cl()[i] = i;

            ++cc[dir];
        }
        if (outdata) {
            updateMeta(outdata);
            task->addObject(p_line_out, outdata);
        } else {
            task->addObject(p_line_out, line);
            updateMeta(line);
        }
    }

    if (isConnected(*p_point_out) && off[0] + bghost[0] <= coord[0] &&
        off[0] + str->getNumDivisions(0) > coord[0] + tghost[0] && off[1] + bghost[1] <= coord[1] &&
        off[1] + str->getNumDivisions(1) > coord[1] + tghost[1] && off[2] + bghost[2] <= coord[2] &&
        off[2] + str->getNumDivisions(2) > coord[2] + tghost[2]) {
        // get point
        Index v = StructuredGridBase::vertexIndex(c[0], c[1], c[2], dims);
        auto p = str->getVertex(v);

        Points::ptr point(new Points(1));
        point->copyAttributes(ingrid);
        DataBase::ptr outdata;
        if (data) {
            outdata = data->cloneType();
            outdata->setSize(1);
            outdata->setGrid(point);
        }
        for (int d = 0; d < 3; ++d)
            point->x(d)[0] = p[d];
        if (outdata)
            outdata->copyEntry(0, data, v);

        if (outdata) {
            updateMeta(outdata);
            task->addObject(p_point_out, outdata);
        } else {
            task->addObject(p_point_out, point);
            updateMeta(point);
        }
    }

    return true;
}
