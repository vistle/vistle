#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/structuredgridbase.h>
#include <core/quads.h>
#include <core/lines.h>
#include <core/points.h>
#include <util/enum.h>

#include "IndexManifolds.h"

MODULE_MAIN(IndexManifolds)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Direction, (X)(Y)(Z))

IndexManifolds::IndexManifolds(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("reduce geometry to underlying points", name, moduleID, comm) {

   p_data_in = createInputPort("data_in0");
   p_surface_out = createOutputPort("surface_out0");
   p_line_out = createOutputPort("line_out0");
   p_point_out = createOutputPort("point_out0");

   p_coord = addIntVectorParameter("coord", "coordinates of point on surface/line", IntParamVector(0,0,0));
   setParameterMinimum(p_coord, IntParamVector(0,0,0));
   p_direction = addIntParameter("direction", "normal on surface and direction of line", X, Parameter::Choice);
   V_ENUM_SET_CHOICES(p_direction, Direction);
}

IndexManifolds::~IndexManifolds() {

}

bool IndexManifolds::compute(std::shared_ptr<PortTask> task) const
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

    Direction dir = static_cast<Direction>(p_direction->getValue());
    Direction dir1 = dir==X ? Y : X;
    Direction dir2 = dir==Z ? Y : Z;
    auto coord = p_coord->getValue();
    Index c[3];
    Index off[3];
    Index bghost[3], tghost[3];
    Index dims[3];
    for (int d=0; d<3; ++d) {
        dims[d] = str->getNumDivisions(d);
        c[d] = coord[d];
        off[d] = str->getGlobalIndexOffset(d);
        if (c[d] < off[d])
            c[d] = InvalidIndex;
        else
            c[d] -= off[d];
        bghost[d] = str->getNumGhostLayers(d, StructuredGridBase::Bottom);
        tghost[d] = str->getNumGhostLayers(d, StructuredGridBase::Top);
    }

    if (off[dir]+bghost[dir] <= coord[dir] && off[dir]+str->getNumDivisions(dir) > coord[dir]+tghost[dir]) {
        // compute surface
        Index nvert1 = dims[dir1] - bghost[dir1] - tghost[dir1];
        Index nvert2 = dims[dir2] - bghost[dir2] - tghost[dir2];
        Index nvert = nvert1*nvert2;
        Index nquad = (nvert1-1) * (nvert2-1);

        Quads::ptr surface(new Quads(nquad*4, nvert));
        surface->copyAttributes(ingrid);
        DataBase::ptr outdata;
        if (data) {
            outdata = data->cloneType();
            outdata->setSize(nvert);
            outdata->setGrid(surface);
            outdata->copyAttributes(data);
        }

        Scalar *x[3];
        for (int d=0; d<3; ++d) {
            x[d] = surface->x(d).data();
        }
        Index *cl = surface->cl().data();
        Index cc[3]{c[0], c[1], c[2]};
        Index ii=0, vv=0;
        cc[dir1] = bghost[dir1];
        for (Index i=0; i<nvert1; ++i) {
            cc[dir2] = bghost[dir2];
            for (Index j=0; j<nvert2; ++j) {
                if (i+1<nvert1 && j+1<nvert2)
                {
                    cl[ii++] = cc[dir1]*nvert2 + cc[dir2];
                    cl[ii++] = (cc[dir1]+1)*nvert2 + cc[dir2];
                    cl[ii++] = (cc[dir1]+1)*nvert2 + cc[dir2]+1;
                    cl[ii++] = cc[dir1]*nvert2 + cc[dir2]+1;
                }
                Index v = StructuredGridBase::vertexIndex(cc[0], cc[1], cc[2], dims);
                auto p = str->getVertex(v);
                for (int d=0; d<3; ++d)
                    x[d][vv] = p[d];
                if (outdata)
                    outdata->copyEntry(vv, data, v);

                ++vv;
                ++cc[dir2];
            }
            ++cc[dir1];
        }
        assert(ii == nquad*4);
        assert(vv == nvert);
        if (outdata)
            task->addObject(p_surface_out, outdata);
        else
            task->addObject(p_surface_out, surface);
    }

    if (off[dir1]+bghost[dir1] <= coord[dir1] && off[dir1]+str->getNumDivisions(dir1) > coord[dir1]+tghost[dir1]
        && off[dir2]+bghost[dir2] <= coord[dir2] && off[dir2]+str->getNumDivisions(dir2) > coord[dir2]+tghost[dir2]) {
        // compute line
        Index nvert = dims[dir] - bghost[dir] - tghost[dir];

        Lines::ptr line(new Lines(1, nvert, nvert));
        line->el()[0] = 0;
        line->el()[1] = nvert;
        line->copyAttributes(ingrid);
        DataBase::ptr outdata;
        if (data) {
            outdata = data->cloneType();
            outdata->setSize(nvert);
            outdata->setGrid(line);
            outdata->copyAttributes(data);
        }

        Index cc[3]{c[0], c[1], c[2]};
        cc[dir] = bghost[dir];
        for (Index i=0; i<nvert; ++i) {
            Index v = StructuredGridBase::vertexIndex(cc[0], cc[1], cc[2], dims);
            auto p = str->getVertex(v);

            for (int d=0; d<3; ++d) {
                line->x(d)[cc[dir]] = p[d];
            }
            if (outdata)
                outdata->copyEntry(cc[dir], data, v);
            line->cl()[i] = i;

            ++cc[dir];
        }
        if (outdata)
            task->addObject(p_line_out, outdata);
        else
            task->addObject(p_line_out, line);
    }

    if (off[0]+bghost[0] <= coord[0] && off[0]+str->getNumDivisions(0) > coord[0]+tghost[0]
        && off[1]+bghost[1] <= coord[1] && off[1]+str->getNumDivisions(1) > coord[1]+tghost[1]
        && off[2]+bghost[2] <= coord[2] && off[2]+str->getNumDivisions(2) > coord[2]+tghost[2]) {
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
        for (int d=0; d<3; ++d)
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
