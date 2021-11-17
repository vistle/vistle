#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/quads.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/util/enum.h>

#include "IndexManifolds.h"

MODULE_MAIN(IndexManifolds)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Direction, (X)(Y)(Z))

IndexManifolds::IndexManifolds(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    p_data_in = createInputPort("data_in0");
    p_surface_out = createOutputPort("surface_out0");
    p_line_out = createOutputPort("line_out0");
    p_point_out = createOutputPort("point_out0");

    p_coord = addIntVectorParameter("coord", "coordinates of point on surface/line", IntParamVector(0, 0, 0));
    setParameterMinimum(p_coord, IntParamVector(0, 0, 0));
    p_direction = addIntParameter("direction", "normal on surface and direction of line", Z, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_direction, Direction);
}

IndexManifolds::~IndexManifolds()
{}

bool IndexManifolds::compute(std::shared_ptr<BlockTask> task) const
{
    auto data = task->accept<DataBase>("data_in0");
    Object::const_ptr ingrid;
    if (data) {
        ingrid = data->grid();
    } else {
        ingrid = task->accept<Object>("data_in0");
    }

    if (!ingrid) {
        sendError("no grid on input");
        return true;
    }

    auto str = StructuredGridBase::as(ingrid);
    if (!str) {
        sendError("no structured grid on input");
        return true;
    }

    bool elementData = data->guessMapping() == DataBase::Element;

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

        Quads::ptr surface(new Quads(nquad * 4, nvert));
        surface->copyAttributes(ingrid);
        DataBase::ptr outdata;
        if (data) {
            outdata = data->cloneType();
            outdata->setSize(elementData ? nquad : nvert);
            outdata->setGrid(surface);
            outdata->copyAttributes(data);
        }

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
        Index cell = 0;
        for (Index i = 0; i < nvert1; ++i) {
            cc[dir2] = bghost[dir2];
            for (Index j = 0; j < nvert2; ++j) {
                Index v = StructuredGridBase::vertexIndex(cc[0], cc[1], cc[2], dims);
                auto p = str->getVertex(v);
                for (int d = 0; d < 3; ++d)
                    x[d][vv] = p[d];
                if (outdata) {
                    if (elementData) {
                        if (i + 1 < nvert1 && j + 1 < nvert2) {
                            Index c = StructuredGridBase::cellIndex(cc[0], cc[1], cc[2], dims);
                            outdata->copyEntry(cell, data, c);
                            ++cell;
                        }
                    } else {
                        outdata->copyEntry(vv, data, v);
                    }
                }

                ++vv;
                ++cc[dir2];
            }
            ++cc[dir1];
        }
        assert(vv == nvert);
        assert(cell == 0 || cell == nquad);

        if (outdata)
            task->addObject(p_surface_out, outdata);
        else
            task->addObject(p_surface_out, surface);
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
            outdata->copyAttributes(data);
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
        if (outdata)
            task->addObject(p_line_out, outdata);
        else
            task->addObject(p_line_out, line);
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
            outdata->copyAttributes(data);
        }
        for (int d = 0; d < 3; ++d)
            point->x(d)[0] = p[d];
        if (outdata)
            outdata->copyEntry(0, data, v);

        if (outdata)
            task->addObject(p_point_out, outdata);
        else
            task->addObject(p_point_out, point);
    }

    return true;
}
