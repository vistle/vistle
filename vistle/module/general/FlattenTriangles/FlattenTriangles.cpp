#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/normals.h>
#include <vistle/core/polygons.h>
#include <vistle/core/spheres.h>
#include <vistle/core/tubes.h>
#include <vistle/core/texture1d.h>

#include "FlattenTriangles.h"

MODULE_MAIN(FlattenTriangles)

using namespace vistle;

FlattenTriangles::FlattenTriangles(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in");
    createOutputPort("grid_out");
}

FlattenTriangles::~FlattenTriangles()
{}

template<int Dim>
struct Flatten {
    Triangles::const_ptr tri;
    Quads::const_ptr quad;
    DataBase::const_ptr object;
    DataBase::ptr result;
    Index N = 0;
    const Index *cl = nullptr;
    Flatten(Triangles::const_ptr tri, DataBase::const_ptr obj, DataBase::ptr result)
    : tri(tri), object(obj), result(result), N(tri->getNumCorners()), cl(&tri->cl()[0])
    {}
    Flatten(Quads::const_ptr quad, DataBase::const_ptr obj, DataBase::ptr result)
    : quad(quad), object(obj), result(result), N(quad->getNumCorners()), cl(&quad->cl()[0])
    {}
    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, Dim> V;
        typename V::const_ptr in(V::as(object));
        if (!in)
            return;
        typename V::ptr out(std::dynamic_pointer_cast<V>(result));
        if (!out)
            return;
        assert(out->getSize() == N);
        if (out->getSize() != N)
            return;

        for (int c = 0; c < Dim; ++c) {
            auto din = &in->x(c)[0];
            auto dout = out->x(c).data();

            for (Index i = 0; i < N; ++i) {
                dout[i] = din[cl[i]];
            }
        }
    }
};

DataBase::ptr flatten(Triangles::const_ptr tri, DataBase::const_ptr src, DataBase::ptr result)
{
    boost::mpl::for_each<Scalars>(Flatten<1>(tri, src, result));
    boost::mpl::for_each<Scalars>(Flatten<3>(tri, src, result));
    return result;
}

DataBase::ptr flatten(Quads::const_ptr quad, DataBase::const_ptr src, DataBase::ptr result)
{
    boost::mpl::for_each<Scalars>(Flatten<1>(quad, src, result));
    boost::mpl::for_each<Scalars>(Flatten<3>(quad, src, result));
    return result;
}

bool FlattenTriangles::compute()
{
    auto data = expect<DataBase>("grid_in");
    if (!data) {
        return true;
    }
    auto grid = data->grid();
    if (!grid) {
        grid = data;
        data.reset();
    }

    auto intri = Triangles::as(grid);
    auto inquad = Quads::as(grid);
    if (!intri && !inquad) {
        sendError("did not receive Triangles ");
        return true;
    }

    Coords::ptr outgrid;
    if (intri) {
        if (intri->getNumCorners() == 0) {
            // already flat
            auto ntri = intri->clone();
            addObject("grid_out", ntri);
            return true;
        }

        Triangles::ptr tri = intri->cloneType();
        tri->setSize(intri->getNumCorners());
        flatten(intri, intri, tri);
        tri->setMeta(intri->meta());
        tri->copyAttributes(intri);
        outgrid = tri;
    } else if (inquad) {
        if (inquad->getNumCorners() == 0) {
            // already flat
            auto nquad = inquad->clone();
            addObject("grid_out", nquad);
            return true;
        }

        Quads::ptr q = inquad->cloneType();
        q->setSize(inquad->getNumCorners());
        flatten(inquad, inquad, q);
        q->setMeta(inquad->meta());
        q->copyAttributes(inquad);
        outgrid = q;
    }

    if (data) {
        DataBase::ptr dout = data->clone();
        dout->resetArrays();
        dout->setSize(outgrid->getSize());
        if (intri) {
            flatten(intri, data, dout);
            dout->setGrid(outgrid);
        } else if (inquad) {
            flatten(inquad, data, dout);
            dout->setGrid(outgrid);
        }
        addObject("grid_out", dout);
    } else {
        addObject("grid_out", outgrid);
    }

    return true;
}
