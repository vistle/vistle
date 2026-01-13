#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/alg/fields.h>

#include "SplitDimensions.h"

using namespace vistle;

SplitDimensions::SplitDimensions(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    p_in = createInputPort("data_in", "(data on) unstructured grid");
    p_out[3] = createOutputPort("data_out_3d", "3-dimensional elements of input");
    p_out[2] = createOutputPort("data_out_2d", "2-dimensional elements of input (triangles and quads)");
    p_out[1] = createOutputPort("data_out_1d", "1-dimensional elements of input (bars)");
    p_out[0] = createOutputPort("data_out_0d", "0-dimensional elements of input (points)");
    for (int i = 0; i < 4; ++i) {
        linkPorts(p_in, p_out[i]);
    }

    p_reuse = addIntParameter("reuse_coordinates", "do not renumber vertices and reuse coordinate and data arrays",
                              false, Parameter::Boolean);
}

bool SplitDimensions::compute(const std::shared_ptr<BlockTask> &task) const
{
    DataBase::const_ptr data;
    StructuredGridBase::const_ptr sgrid;
    UnstructuredGrid::const_ptr ugrid;
    ugrid = task->accept<UnstructuredGrid>("data_in");
    sgrid = task->accept<StructuredGridBase>("data_in");
    if (!ugrid && !sgrid) {
        data = task->expect<DataBase>("data_in");
        if (!data) {
            sendError("no grid and no data received");
            return true;
        }
        if (!data->grid()) {
            sendError("no grid attached to data");
            return true;
        }
        ugrid = UnstructuredGrid::as(data->grid());
        sgrid = StructuredGridBase::as(data->grid());
        if (!ugrid && !sgrid) {
            sendError("no valid grid attached to data");
            return true;
        }
    }
    Object::const_ptr grid_in =
        ugrid ? Object::as(ugrid) : std::dynamic_pointer_cast<const Object, const StructuredGridBase>(sgrid);
    assert(grid_in);

    if (data && data->guessMapping(grid_in) != DataBase::Vertex) {
        sendError("data mapping not per vertex");
        return true;
    }

    vistle::Points::ptr out0d(new vistle::Points(size_t(0)));
    if (isConnected(*p_out[0])) {
        out0d.reset(new vistle::Points(size_t(0)));
    }

    vistle::Indexed::ptr out[4];
    vistle::Lines::ptr out1d;
    shm<Index>::array *ocl1 = nullptr, *oel1 = nullptr;
    if (isConnected(*p_out[1])) {
        out1d.reset(new vistle::Lines(0, 0, 0));
        out[1] = out1d;
        ocl1 = &out1d->cl();
        oel1 = &out1d->el();
    }

    vistle::Polygons::ptr out2d;
    shm<Index>::array *ocl2 = nullptr, *oel2 = nullptr;
    if (isConnected(*p_out[2])) {
        out2d.reset(new vistle::Polygons(0, 0, 0));
        out[2] = out2d;
        ocl2 = &out2d->cl();
        oel2 = &out2d->el();
    }

    vistle::UnstructuredGrid::ptr out3d;
    shm<Index>::array *ocl3 = nullptr, *oel3 = nullptr;
    shm<unsigned char>::array *otl3 = nullptr;
    if (isConnected(*p_out[3])) {
        out3d.reset(new vistle::UnstructuredGrid(0, 0, 0));
        out[3] = out3d;
        ocl3 = &out3d->cl();
        oel3 = &out3d->el();
        otl3 = &out3d->tl();
    }

    VerticesMapping vm[4];

    if (ugrid) {
        const Index *icl = ugrid->cl().data();
        const Index *iel = ugrid->el().data();

        Index nelem = ugrid->getNumElements();
        Index npoint = 0;
        for (Index e = 0; e < nelem; ++e) {
            auto type = ugrid->tl()[e];

            const Index begin = iel[e], end = iel[e + 1];

            switch (type) {
            case cell::POINT:
                if (out0d) {
                    Index v = icl[begin];
                    vm[0][v] = npoint++;
                    out0d->d()->x[0]->push_back(ugrid->x()[v]);
                    out0d->d()->x[1]->push_back(ugrid->y()[v]);
                    out0d->d()->x[2]->push_back(ugrid->y()[v]);
                }
                break;
            case cell::BAR:
            case cell::POLYLINE:
                if (oel1 && !ugrid->isGhost(e)) {
                    for (Index i = begin; i < end; ++i) {
                        ocl1->push_back(icl[i]);
                    }
                    oel1->push_back(ocl1->size());
                }
                break;
            case cell::TRIANGLE:
            case cell::QUAD:
            case cell::POLYGON:
                if (oel2 && !ugrid->isGhost(e)) {
                    for (Index i = begin; i < end; ++i) {
                        ocl2->push_back(icl[i]);
                    }
                    oel2->push_back(ocl2->size());
                }
                break;
            case cell::TETRAHEDRON:
            case cell::PYRAMID:
            case cell::PRISM:
            case cell::HEXAHEDRON:
            case cell::POLYHEDRON:
                if (oel3) {
                    otl3->push_back(type);
                    for (Index i = begin; i < end; ++i) {
                        ocl3->push_back(icl[i]);
                    }
                    oel3->push_back(ocl3->size());
                }
                break;
            }
        }
    }

    if (ugrid) {
        if (out0d) {
            renumberVertices(ugrid, out0d, vm[0]);
            std::cerr << "remapped from " << ugrid->getNumCorners() << " to " << vm[0].size() << " vertices"
                      << std::endl;
            out0d->setMeta(grid_in->meta());
            out0d->copyAttributes(grid_in);
        }
        for (int d = 1; d <= 3; ++d) {
            if (out[d]) {
                renumberVertices(ugrid, out[d], vm[d]);
                std::cerr << "remapped from " << ugrid->getNumCorners() << " to " << vm[d].size() << " vertices"
                          << std::endl;
                out[d]->setMeta(grid_in->meta());
                out[d]->copyAttributes(grid_in);
            }
        }
#if 0
   } else if (sgrid) {
       auto quad = createSurface(sgrid);
       surface = quad;
       if (!quad)
           return true;
       if (auto coords = Coords::as(grid_in)) {
           renumberVertices(coords, quad, vm);
       } else {
           createVertices(sgrid, quad, vm);
       }
#endif
    }

    Object::ptr obj;
    for (int d = 0; d <= 3; ++d) {
        if (d == 0)
            obj = out0d;
        else
            obj = out[d];
        if (!obj)
            continue;
        if (!data) {
            updateMeta(obj);
            task->addObject(p_out[d], obj);
        } else if (vm[d].empty()) {
            DataBase::ptr dout = data->clone();
            dout->setGrid(out[d]);
            updateMeta(dout);
            task->addObject(p_out[d], dout);
        } else {
            DataBase::ptr data_obj_out = remapData(data, vm[d]);

            if (data_obj_out) {
                data_obj_out->setGrid(obj);
                data_obj_out->setMeta(data->meta());
                data_obj_out->copyAttributes(data);
                updateMeta(data_obj_out);
                task->addObject(p_out[d], data_obj_out);
            }
        }
    }

    return true;
}

Quads::ptr SplitDimensions::createSurface(vistle::StructuredGridBase::const_ptr grid) const
{
    auto sgrid = std::dynamic_pointer_cast<const StructuredGrid, const StructuredGridBase>(grid);

    Quads::ptr m_grid_out(new Quads(0, 0));
    auto &pcl = m_grid_out->cl();
    Index dims[3] = {grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2)};

    for (int d = 0; d < 3; ++d) {
        int d1 = d == 0 ? 1 : 0;
        int d2 = d == d1 + 1 ? d1 + 2 : d1 + 1;
        assert(d != d1);
        assert(d != d2);
        assert(d1 != d2);

        Index b1 = grid->getNumGhostLayers(d1, StructuredGridBase::Bottom);
        Index e1 = grid->getNumDivisions(d1);
        if (grid->getNumGhostLayers(d1, StructuredGridBase::Top) + 1 < e1)
            e1 -= grid->getNumGhostLayers(d1, StructuredGridBase::Top) + 1;
        else
            e1 = 0;
        Index b2 = grid->getNumGhostLayers(d2, StructuredGridBase::Bottom);
        Index e2 = grid->getNumDivisions(d2);
        if (grid->getNumGhostLayers(d2, StructuredGridBase::Top) + 1 < e2)
            e2 -= grid->getNumGhostLayers(d2, StructuredGridBase::Top) + 1;
        else
            e2 = 0;

        if (grid->getNumGhostLayers(d, StructuredGridBase::Bottom) == 0) {
            for (Index i1 = b1; i1 < e1; ++i1) {
                for (Index i2 = b2; i2 < e2; ++i2) {
                    Index idx[3]{0, 0, 0};
                    idx[d1] = i1;
                    idx[d2] = i2;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d2] = i2 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                }
            }
        }
        if (grid->getNumDivisions(d) > 1 && grid->getNumGhostLayers(d, StructuredGridBase::Top) == 0) {
            for (Index i1 = b1; i1 < e1; ++i1) {
                for (Index i2 = b2; i2 < e2; ++i2) {
                    Index idx[3]{0, 0, 0};
                    idx[d] = grid->getNumDivisions(d) - 1;
                    idx[d1] = i1;
                    idx[d2] = i2;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d2] = i2 + 1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                    idx[d1] = i1;
                    pcl.push_back(grid->vertexIndex(idx, dims));
                }
            }
        }
    }

    return m_grid_out;
}

void SplitDimensions::renumberVertices(Coords::const_ptr coords, Points::ptr points, VerticesMapping &vm) const
{
    const Scalar *xcoord = coords->x().data();
    const Scalar *ycoord = coords->y().data();
    const Scalar *zcoord = coords->z().data();
    auto &px = points->x();
    auto &py = points->y();
    auto &pz = points->z();
    auto c = vm.size();
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

void SplitDimensions::renumberVertices(Coords::const_ptr coords, Indexed::ptr poly, VerticesMapping &vm) const
{
    const bool reuseCoord = p_reuse->getValue();

    if (reuseCoord) {
        poly->d()->x[0] = coords->d()->x[0];
        poly->d()->x[1] = coords->d()->x[1];
        poly->d()->x[2] = coords->d()->x[2];
    } else {
        vm.clear();
        Index c = 0;
        for (Index &v: poly->cl()) {
            if (vm.emplace(v, c).second) {
                v = c;
                ++c;
            } else {
                v = vm[v];
            }
        }

        const Scalar *xcoord = coords->x().data();
        const Scalar *ycoord = coords->y().data();
        const Scalar *zcoord = coords->z().data();
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

void SplitDimensions::renumberVertices(Coords::const_ptr coords, Quads::ptr quad, VerticesMapping &vm) const
{
    const bool reuseCoord = p_reuse->getValue();

    if (reuseCoord) {
        quad->d()->x[0] = coords->d()->x[0];
        quad->d()->x[1] = coords->d()->x[1];
        quad->d()->x[2] = coords->d()->x[2];
    } else {
        vm.clear();
        Index c = 0;
        for (Index &v: quad->cl()) {
            if (vm.emplace(v, c).second) {
                v = c;
                ++c;
            } else {
                v = vm[v];
            }
        }

        const Scalar *xcoord = coords->x().data();
        const Scalar *ycoord = coords->y().data();
        const Scalar *zcoord = coords->z().data();
        auto &px = quad->x();
        auto &py = quad->y();
        auto &pz = quad->z();
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

void SplitDimensions::createVertices(StructuredGridBase::const_ptr grid, Quads::ptr quad, VerticesMapping &vm) const
{
    vm.clear();
    Index c = 0;
    for (Index &v: quad->cl()) {
        if (vm.emplace(v, c).second) {
            v = c;
            ++c;
        } else {
            v = vm[v];
        }
    }

    auto &px = quad->x();
    auto &py = quad->y();
    auto &pz = quad->z();
    px.resize(c);
    py.resize(c);
    pz.resize(c);

    for (const auto &v: vm) {
        Index f = v.first;
        Index s = v.second;
        Vector3 p = grid->getVertex(f);
        px[s] = p[0];
        py[s] = p[1];
        pz[s] = p[2];
    }
}

MODULE_MAIN(SplitDimensions)
