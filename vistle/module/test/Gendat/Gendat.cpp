#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>
#include <core/triangles.h>
#include <core/polygons.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>
#include <core/unstr.h>
#include <util/enum.h>

#include "Gendat.h"

MODULE_MAIN(Gendat)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(GeoMode,
                                    (Triangle_Geometry)
                                    (Polygon_Geometry)
                                    (Uniform_Grid)
                                    (Rectilinear_Grid)
                                    (Structured_Grid)
                                    (Unstructured_Grid))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(DataMode,
                                    (One)
                                    (Dist_Origin)
                                    (Identity_X)
                                    (Identity_Y)
                                    (Identity_Z)
                                    (Sine_X)
                                    (Sine_Y)
                                    (Sine_Z)
                                    (Cosine_X)
                                    (Cosine_Y)
                                    (Cosine_Z)
                                    (Random))

Gendat::Gendat(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Gendat", shmname, name, moduleID) {

   createOutputPort("grid_out", "only grid");
   createOutputPort("data_out0", "scalar data");
   createOutputPort("data_out1", "vector data");

   std::vector<std::string> choices;
   m_geoMode = addIntParameter("geo_mode", "geometry generation mode", Unstructured_Grid, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_geoMode, GeoMode);

   m_dataModeScalar = addIntParameter("data_mode_scalar", "data generation mode", Dist_Origin, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_dataModeScalar, DataMode);
   m_dataScaleScalar = addFloatParameter("data_scale_scalar", "data scale factor (scalar)", 1.);

   m_dataMode[0] = addIntParameter("data_mode_vec_x", "data generation mode", Identity_X, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_dataMode[0], DataMode);
   m_dataScale[0] = addFloatParameter("data_scale_vec_x", "data scale factor", 1.);

   m_dataMode[1] = addIntParameter("data_mode_vec_y", "data generation mode", Identity_Y, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_dataMode[1], DataMode);
   m_dataScale[1] = addFloatParameter("data_scale_vec_y", "data scale factor", 1.);

   m_dataMode[2] = addIntParameter("data_mode_vec_z", "data generation mode", Identity_Z, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_dataMode[2], DataMode);
   m_dataScale[2] = addFloatParameter("data_scale_vec_z", "data scale factor", 1.);

   m_size[0] = addIntParameter("size_x", "number of cells/block in x-direction", 10);
   m_size[1] = addIntParameter("size_y", "number of cells/block in y-direction", 10);
   m_size[2] = addIntParameter("size_z", "number of cells/block in z-direction", 10);

   m_blocks[0] = addIntParameter("blocks_x", "number of blocks in x-direction", 3);
   m_blocks[1] = addIntParameter("blocks_y", "number of blocks in y-direction", 3);
   m_blocks[2] = addIntParameter("blocks_z", "number of blocks in z-direction", 3);

   m_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 0);
}

Gendat::~Gendat() {

}

inline Scalar computeData(Scalar x, Scalar y, Scalar z, DataMode mode, Scalar scale) {
    switch(mode) {
    case One: return scale;
    case Dist_Origin: return sqrt(x*x+y*y+z*z)*scale;
    case Identity_X: return x*scale;
    case Identity_Y: return y*scale;
    case Identity_Z: return z*scale;
    case Sine_X: return sin(x*M_PI)*scale;
    case Sine_Y: return sin(y*M_PI)*scale;
    case Sine_Z: return sin(z*M_PI)*scale;
    case Cosine_X: return cos(x*M_PI)*scale;
    case Cosine_Y: return cos(y*M_PI)*scale;
    case Cosine_Z: return cos(z*M_PI)*scale;
    case Random: return rand()*scale;
    }

    return 0.;
}

void setDataCoords(Scalar *d, Index numVert, const Scalar *xx, const Scalar *yy, const Scalar *zz, DataMode mode, Scalar scale) {
    for (Index idx=0; idx<numVert; ++idx) {
        Scalar x = xx[idx], y=yy[idx], z=zz[idx];
        d[idx] = computeData(x, y, z, mode, scale);
    }
}

void setDataUniform(Scalar *d, Index dim[3], Vector min, Vector max, DataMode mode, Scalar scale) {
    Vector dist=max-min;
    for (int c=0; c<3; ++c) {
        dist[c] /= dim[c]-1;
    }
    for (Index i=0; i<dim[0]; ++i) {
        for (Index j=0; j<dim[1]; ++j) {
            for (Index k=0; k<dim[2]; ++k) {
                Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                Scalar x = min[0]+i*dist[0];
                Scalar y = min[1]+j*dist[1];
                Scalar z = min[2]+k*dist[2];

                d[idx] = computeData(x, y, z, mode, scale);
            }
        }
    }
}

void setStructuredGridGhostLayers(StructuredGridBase::ptr ptr, Index ghostWidth[3][2]) {
    for (Index i=0; i<3; ++i) {
            ptr->setNumGhostLayers(i, StructuredGridBase::Bottom, ghostWidth[i][0]);
            ptr->setNumGhostLayers(i, StructuredGridBase::Top, ghostWidth[i][1]);
    }
}

void Gendat::block(Index bx, Index by, Index bz, vistle::Index b) {

    Index dim[3];
    Vector dist;
    Index maxBlocks[3];
    Index currBlock[3] = {bx, by, bz};

    for (int i=0; i<3; ++i) {
        dim[i] = m_size[i]->getValue()+1;
        dist[i] = 1./m_size[i]->getValue();
        maxBlocks[i] = m_blocks[i]->getValue();
    }
    GeoMode geoMode = (GeoMode)m_geoMode->getValue();
    Index numVert = dim[0]*dim[1]*dim[2];

    Object::ptr geoOut;

    Vector min(bx, by, bz), max=min+Vector(1.,1.,1.);

    // output data: first if statement seperates coord-derived objects
    if (geoMode == Triangle_Geometry || geoMode == Polygon_Geometry) {
        Coords::ptr geo;
        if (geoMode == Triangle_Geometry) {

            Triangles::ptr t(new Triangles(6, 4));
            geo = t;

            t->cl()[0] = 0;
            t->cl()[1] = 1;
            t->cl()[2] = 3;

            t->cl()[3] = 0;
            t->cl()[4] = 3;
            t->cl()[5] = 2;
        } else if (geoMode == Polygon_Geometry) {

            Polygons::ptr p(new Polygons(1, 4, 4));
            geo = p;

            p->cl()[0] = 0;
            p->cl()[1] = 1;
            p->cl()[2] = 3;
            p->cl()[3] = 2;

            p->el()[0] = 0;
            p->el()[1] = 4;
        }
        numVert = geo->getSize();

        geo->x()[0] = min[0];
        geo->y()[0] = min[1];
        geo->z()[0] = min[2];
;
        geo->x()[1] = max[0];
        geo->y()[1] = min[1];
        geo->z()[1] = min[2];

        geo->x()[2] = min[0];
        geo->y()[2] = max[1];
        geo->z()[2] = min[2];

        geo->x()[3] = max[0];
        geo->y()[3] = max[1];
        geo->z()[3] = min[0];
        geoOut = geo;
    } else {
        // obtain dimenstions of current block while taking into consideration ghost cells
        Index ghostWidth[3][2];

        for (unsigned i = 0; i < 3; i++) {
            ghostWidth[i][0] = (currBlock[i] == 0) ? 0 : m_ghostLayerWidth->getValue();
            ghostWidth[i][1] = (currBlock[i] == maxBlocks[i] - 1) ? 0 : m_ghostLayerWidth->getValue();


            dim[i] += ghostWidth[i][0] + ghostWidth[i][1];
        }

        numVert = dim[0]*dim[1]*dim[2];

        if (geoMode == Uniform_Grid) {
            UniformGrid::ptr u(new UniformGrid(dim[0], dim[1], dim[2]));

            // generate test data
            for (unsigned i = 0; i < 3; i++) {
                u->min()[i] = min[i];
                u->max()[i] = max[i];
            }
            setStructuredGridGhostLayers(u, ghostWidth);

            geoOut = u;

        } else if (geoMode == Rectilinear_Grid) {
            RectilinearGrid::ptr r(new RectilinearGrid(dim[0], dim[1], dim[2]));

            // generate test data
            for (int c=0; c<3; ++c) {
                for (unsigned i = 0; i < dim[c]; ++i) {
                    r->coords(c)[i] = min[c]+i*dist[c];
                }
            }
            setStructuredGridGhostLayers(r, ghostWidth);

            geoOut = r;

        } else if (geoMode == Structured_Grid) {
            StructuredGrid::ptr s(new StructuredGrid(dim[0], dim[1], dim[2]));

            Scalar *x = s->x().data();
            Scalar *y = s->y().data();
            Scalar *z = s->z().data();
            for (Index i=0; i<dim[0]; ++i) {
                for (Index j=0; j<dim[1]; ++j) {
                    for (Index k=0; k<dim[2]; ++k) {
                        Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                        x[idx] = min[0]+i*dist[0];
                        y[idx] = min[1]+j*dist[1];
                        z[idx] = min[2]+k*dist[2];
                    }
                }
            }
            setStructuredGridGhostLayers(s, ghostWidth);

            geoOut = s;

        } else if (geoMode == Unstructured_Grid) {
            Index numElem = (dim[0]-1)*(dim[1]-1)*(dim[2]-1);
            UnstructuredGrid::ptr u(new UnstructuredGrid(numElem, numElem*8, numVert));
            Scalar *x = u->x().data();
            Scalar *y = u->y().data();
            Scalar *z = u->z().data();
            for (Index i=0; i<dim[0]; ++i) {
                for (Index j=0; j<dim[1]; ++j) {
                    for (Index k=0; k<dim[2]; ++k) {
                        Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                        x[idx] = min[0]+i*dist[0];
                        y[idx] = min[1]+j*dist[1];
                        z[idx] = min[2]+k*dist[2];
                    }
                }
            }
            const Index nx = dim[0]-1;
            const Index ny = dim[1]-1;
            const Index nz = dim[2]-1;
            Index elem = 0;
            Index idx = 0;
            Index *cl = u->cl().data();
            Index *el = u->el().data();
            unsigned char *tl = u->tl().data();

            for (Index ix=0; ix<nx; ++ix) {
                for (Index iy=0; iy<ny; ++iy) {
                    for (Index iz=0; iz<nz; ++iz) {
                        cl[idx++] = UniformGrid::vertexIndex(ix,   iy,   iz,   dim);       // 0       7 -------- 6
                        cl[idx++] = UniformGrid::vertexIndex(ix+1, iy,   iz,   dim);       // 1      /|         /|
                        cl[idx++] = UniformGrid::vertexIndex(ix+1, iy+1, iz,   dim);       // 2     / |        / |
                        cl[idx++] = UniformGrid::vertexIndex(ix,   iy+1, iz,   dim);       // 3    4 -------- 5  |
                        cl[idx++] = UniformGrid::vertexIndex(ix,   iy,   iz+1, dim);       // 4    |  3-------|--2
                        cl[idx++] = UniformGrid::vertexIndex(ix+1, iy,   iz+1, dim);       // 5    | /        | /
                        cl[idx++] = UniformGrid::vertexIndex(ix+1, iy+1, iz+1, dim);       // 6    |/         |/
                        cl[idx++] = UniformGrid::vertexIndex(ix,   iy+1, iz+1, dim);       // 7    0----------1

                        if ((ix < ghostWidth[0][0] || ix >= nx-ghostWidth[0][1])
                                || (iy < ghostWidth[1][0] || iy >= ny-ghostWidth[1][1])
                                || (iz < ghostWidth[2][0] || iz >= nz-ghostWidth[2][1])) {
                            tl[elem] = UnstructuredGrid::GHOST_HEXAHEDRON;
                        } else {
                            tl[elem] = UnstructuredGrid::HEXAHEDRON;
                        }

                        ++elem;
                        el[elem] = idx;
                    }
                }
            }
            geoOut = u;
        }
    }

    Vec<Scalar,1>::ptr scalar(new Vec<Scalar,1>(numVert));
    scalar->setBlock(b);
    Vec<Scalar,3>::ptr vector(new Vec<Scalar,3>(numVert));
    vector->setBlock(b);

    if (auto coord = Coords::as(geoOut)) {
        const Scalar *xx = coord->x().data();
        const Scalar *yy = coord->y().data();
        const Scalar *zz = coord->z().data();

        setDataCoords(scalar->x().data(), numVert, xx, yy, zz, (DataMode)m_dataModeScalar->getValue(), m_dataScaleScalar->getValue());
        setDataCoords(vector->x().data(), numVert, xx, yy, zz, (DataMode)m_dataMode[0]->getValue(), m_dataScale[0]->getValue());
        setDataCoords(vector->y().data(), numVert, xx, yy, zz, (DataMode)m_dataMode[1]->getValue(), m_dataScale[1]->getValue());
        setDataCoords(vector->z().data(), numVert, xx, yy, zz, (DataMode)m_dataMode[2]->getValue(), m_dataScale[2]->getValue());
    } else {
        setDataUniform(scalar->x().data(), dim, min, max, (DataMode)m_dataModeScalar->getValue(), m_dataScaleScalar->getValue());
        setDataUniform(vector->x().data(), dim, min, max, (DataMode)m_dataMode[0]->getValue(), m_dataScale[0]->getValue());
        setDataUniform(vector->y().data(), dim, min, max, (DataMode)m_dataMode[1]->getValue(), m_dataScale[1]->getValue());
        setDataUniform(vector->z().data(), dim, min, max, (DataMode)m_dataMode[2]->getValue(), m_dataScale[2]->getValue());
    }

    if (geoOut) {
        geoOut->setBlock(b);
        addObject("grid_out", geoOut);
        scalar->setGrid(geoOut);
        vector->setGrid(geoOut);
    }

    addObject("data_out0", scalar);
    addObject("data_out1", vector);
}

bool Gendat::compute() {

    Index blocks[3];
    for (int i=0; i<3; ++i) {
        blocks[i] = m_blocks[i]->getValue();
    }

    Index b = 0;
    for (Index bx=0; bx<blocks[0]; ++bx) {
        for (Index by=0; by<blocks[1]; ++by) {
            for (Index bz=0; bz<blocks[2]; ++bz) {
                if (b % size() == rank()) {
                    block(bx, by, bz, b);
                }
                ++b;
            }
        }
    }

    return true;
}
