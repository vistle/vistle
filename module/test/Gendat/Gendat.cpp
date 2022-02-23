#include <sstream>
#include <iomanip>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>
#include <vistle/core/points.h>
#include <vistle/core/spheres.h>
#include <vistle/core/coordswradius.h>
#include <vistle/util/enum.h>

#include "Gendat.h"

MODULE_MAIN(Gendat)

using namespace vistle;

// clang-format off
DEFINE_ENUM_WITH_STRING_CONVERSIONS(
    GeoMode,
    (Triangle_Geometry)(Quad_Geometry)(Polygon_Geometry)(Uniform_Grid)(Rectilinear_Grid)(Layer_Grid)(Structured_Grid)(Unstructured_Grid)(Point_Geometry)(Sphere_Geometry))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(
    DataMode,
    (One)(Dist_Origin)(Identity_X)(Identity_Y)(Identity_Z)(Sine_X)(Sine_Y)(Sine_Z)(Cosine_X)(Cosine_Y)(Cosine_Z)(Random))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(
        AnimDataMode,
        (Constant)(Add_Scale)(Divide_Scale)(Add_X)(Add_Y)(Add_Z))
// clang-format on

Gendat::Gendat(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
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

    m_size[0] = addIntParameter("size_x", "number of cells per block in x-direction", 10);
    m_size[1] = addIntParameter("size_y", "number of cells per block in y-direction", 10);
    m_size[2] = addIntParameter("size_z", "number of cells per block in z-direction", 10);

    m_blocks[0] = addIntParameter("blocks_x", "number of blocks in x-direction", 3);
    m_blocks[1] = addIntParameter("blocks_y", "number of blocks in y-direction", 3);
    m_blocks[2] = addIntParameter("blocks_z", "number of blocks in z-direction", 3);

    m_ghostLayerWidth = addIntParameter("ghost_layers", "number of ghost layers on all sides of a grid", 0);

    m_min = addVectorParameter("min", "minimum coordinates", ParamVector(-1., -1, -1.));
    m_max = addVectorParameter("max", "maximum coordinates", ParamVector(1., 1., 1.));

    m_steps = addIntParameter("timesteps", "number of timesteps", 0);
    setParameterRange(m_steps, Integer(0), Integer(999999));

    m_elementData = addIntParameter("element_data", "generate data per element/cell", false, Parameter::Boolean);

    m_animData = addIntParameter("anim_data", "data animation", Constant, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_animData, AnimDataMode);

    m_delay = addFloatParameter("delay", "wait after creating an object (s)", 0.);
    setParameterRange(m_delay, 0., 3.);

    observeParameter(m_blocks[0]);
    observeParameter(m_blocks[1]);
    observeParameter(m_blocks[2]);
    observeParameter(m_steps);

    setParallelizationMode(ParallelizeBlocks);
}

Gendat::~Gendat()
{}

bool Gendat::examine(const Parameter *)
{
    size_t nblocks = m_blocks[0]->getValue() * m_blocks[1]->getValue() * m_blocks[2]->getValue();
    setPartitions(nblocks);
    setTimesteps(m_steps->getValue());
    return nblocks > 0;
}

bool Gendat::read(Reader::Token &token, int timestep, int blockNum)
{
    Index steps = m_steps->getValue();
    if (steps > 0 && timestep < 0) {
        // don't generate constant data when animation has been requested
        return true;
    }

    Index blocks[3];
    for (int i = 0; i < 3; ++i) {
        blocks[i] = m_blocks[i]->getValue();
    }

    Index b = blockNum;
    Index bx = b % blocks[0];
    b /= blocks[0];
    Index by = b % blocks[1];
    b /= blocks[1];
    Index bz = b;

    block(token, bx, by, bz, blockNum, timestep);

    return true;
}

inline Scalar computeData(Scalar x, Scalar y, Scalar z, DataMode mode, Scalar scale, AnimDataMode anim, Index time)
{
    switch (anim) {
    case Constant:
        break;
    case Add_Scale:
        scale += Scalar(time);
        break;
    case Divide_Scale:
        scale /= Scalar(time + 1);
        break;
    case Add_X:
        x += Scalar(time);
        break;
    case Add_Y:
        y += Scalar(time);
        break;
    case Add_Z:
        z += Scalar(time);
        break;
    }

    switch (mode) {
    case One:
        return scale;
    case Dist_Origin:
        return sqrt(x * x + y * y + z * z) * scale;
    case Identity_X:
        return x * scale;
    case Identity_Y:
        return y * scale;
    case Identity_Z:
        return z * scale;
    case Sine_X:
        return sin(x * M_PI) * scale;
    case Sine_Y:
        return sin(y * M_PI) * scale;
    case Sine_Z:
        return sin(z * M_PI) * scale;
    case Cosine_X:
        return cos(x * M_PI) * scale;
    case Cosine_Y:
        return cos(y * M_PI) * scale;
    case Cosine_Z:
        return cos(z * M_PI) * scale;
    case Random:
        return rand() * scale;
    }

    return 0.;
}

void setDataCells(Scalar *d, const GridInterface *grid, DataMode mode, Scalar scale, AnimDataMode anim, Index time)
{
    Index numElem = grid->getNumElements();
    for (Index idx = 0; idx < numElem; ++idx) {
        auto c = grid->cellCenter(idx);
        d[idx] = computeData(c[0], c[1], c[2], mode, scale, anim, time);
    }
}

void setDataCoords(Scalar *d, Index numVert, const Scalar *xx, const Scalar *yy, const Scalar *zz, DataMode mode,
                   Scalar scale, AnimDataMode anim, Index time)
{
    for (Index idx = 0; idx < numVert; ++idx) {
        Scalar x = xx[idx], y = yy[idx], z = zz[idx];
        d[idx] = computeData(x, y, z, mode, scale, anim, time);
    }
}

void setDataUniform(Scalar *d, Index dim[3], Vector3 min, Vector3 max, DataMode mode, Scalar scale, AnimDataMode anim,
                    Index time)
{
    Vector3 dist = max - min;
    for (int c = 0; c < 3; ++c) {
        if (dim[c] > 1)
            dist[c] /= dim[c] - 1;
        else
            dist[c] = 0.;
    }
    for (Index i = 0; i < dim[0]; ++i) {
        for (Index j = 0; j < dim[1]; ++j) {
            for (Index k = 0; k < dim[2]; ++k) {
                Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                Scalar x = min[0] + i * dist[0];
                Scalar y = min[1] + j * dist[1];
                Scalar z = min[2] + k * dist[2];

                d[idx] = computeData(x, y, z, mode, scale, anim, time);
            }
        }
    }
}

void setStructuredGridGhostLayers(StructuredGridBase::ptr ptr, Index ghostWidth[3][2])
{
    for (Index i = 0; i < 3; ++i) {
        ptr->setNumGhostLayers(i, StructuredGridBase::Bottom, ghostWidth[i][0]);
        ptr->setNumGhostLayers(i, StructuredGridBase::Top, ghostWidth[i][1]);
    }
}

void setStructuredGridGlobalOffsets(StructuredGridBase::ptr ptr, Index offset[3])
{
    for (Index i = 0; i < 3; ++i) {
        ptr->setGlobalIndexOffset(i, offset[i]);
    }
}

void Gendat::block(Reader::Token &token, Index bx, Index by, Index bz, vistle::Index block, vistle::Index time) const
{
    Index dim[3];
    Vector3 dist, bdist;
    Index maxBlocks[3];
    Index currBlock[3] = {bx, by, bz};
    Vector3 gmin = m_min->getValue(), gmax = m_max->getValue();
    Vector3 bmin;
    Vector3 min, max;

    for (int c = 0; c < 3; ++c) {
        dim[c] = m_size[c]->getValue() + 1;
        maxBlocks[c] = m_blocks[c]->getValue();
        bdist[c] = (gmax[c] - gmin[c]) / maxBlocks[c];
        bmin[c] = gmin[c] + currBlock[c] * bdist[c];
        if (m_size[c]->getValue() > 0) {
            dist[c] = bdist[c] / m_size[c]->getValue();
        } else {
            bdist[c] = 0.;
            dist[c] = 0.;
        }
        min[c] = bmin[c];
        max[c] = bmin[c] + bdist[c];
    }
    GeoMode geoMode = (GeoMode)m_geoMode->getValue();
    Index numVert = dim[0] * dim[1] * dim[2];
    Index numCells = (dim[0] - 1) * (dim[1] - 1) * (dim[2] - 1);
    bool elementData = m_elementData->getValue();

    Object::ptr geoOut;

    // output data: first if statement separates coord-derived objects
    if (geoMode == Triangle_Geometry || geoMode == Quad_Geometry || geoMode == Polygon_Geometry) {
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
        } else if (geoMode == Quad_Geometry) {
            Quads::ptr q(new Quads(4, 4));
            geo = q;

            q->cl()[0] = 0;
            q->cl()[1] = 1;
            q->cl()[2] = 3;
            q->cl()[3] = 2;
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
        numCells = geo->getInterface<ElementInterface>()->getNumElements();

        geo->x()[0] = min[0];
        geo->y()[0] = min[1];
        geo->z()[0] = min[2];

        geo->x()[1] = max[0];
        geo->y()[1] = min[1];
        geo->z()[1] = min[2];

        geo->x()[2] = min[0];
        geo->y()[2] = max[1];
        geo->z()[2] = min[2];

        geo->x()[3] = max[0];
        geo->y()[3] = max[1];
        geo->z()[3] = min[2];
        geoOut = geo;
    } else {
        // obtain dimensions of current block while taking into consideration ghost cells
        Index ghostWidth[3][2];
        Index offsets[3];

        for (unsigned i = 0; i < 3; i++) {
            offsets[i] = 0;
            if (currBlock[i] > 0)
                offsets[i] = currBlock[i] * (dim[i] - 1) - m_ghostLayerWidth->getValue();

            ghostWidth[i][0] = (currBlock[i] == 0) ? 0 : m_ghostLayerWidth->getValue();
            ghostWidth[i][1] = (currBlock[i] == maxBlocks[i] - 1) ? 0 : m_ghostLayerWidth->getValue();

            dim[i] += ghostWidth[i][0] + ghostWidth[i][1];
        }
        numVert = dim[0] * dim[1] * dim[2];
        numCells = (dim[0] - 1) * (dim[1] - 1) * (dim[2] - 1);

        Index numCellVert = 1;
        if (dim[2] > 1) {
            numCellVert = 8;
        } else if (dim[1] > 1) {
            numCellVert = 4;
        } else if (dim[0] > 1) {
            numCellVert = 2;
        }

        for (int c = 0; c < 3; ++c) {
            min[c] -= ghostWidth[c][0] * dist[c];
            max[c] += ghostWidth[c][1] * dist[c];
        }

        if (geoMode == Uniform_Grid) {
            UniformGrid::ptr u(new UniformGrid(dim[0], dim[1], dim[2]));

            // generate test data
            for (unsigned i = 0; i < 3; i++) {
                u->min()[i] = min[i];
                u->max()[i] = max[i];
            }
            setStructuredGridGhostLayers(u, ghostWidth);
            setStructuredGridGlobalOffsets(u, offsets);

            geoOut = u;

        } else if (geoMode == Rectilinear_Grid) {
            RectilinearGrid::ptr r(new RectilinearGrid(dim[0], dim[1], dim[2]));

            // generate test data
            for (int c = 0; c < 3; ++c) {
                for (Index i = 0; i < dim[c]; ++i) {
                    r->coords(c)[i] = min[c] + i * dist[c];
                }
            }
            setStructuredGridGhostLayers(r, ghostWidth);
            setStructuredGridGlobalOffsets(r, offsets);

            geoOut = r;

        } else if (geoMode == Layer_Grid) {
            LayerGrid::ptr lg(new LayerGrid(dim[0], dim[1], dim[2]));
            setStructuredGridGhostLayers(lg, ghostWidth);
            setStructuredGridGlobalOffsets(lg, offsets);
            for (unsigned i = 0; i < 2; i++) {
                lg->min()[i] = min[i];
                lg->max()[i] = max[i];
            }
            geoOut = lg;

        } else if (geoMode == Structured_Grid) {
            StructuredGrid::ptr s(new StructuredGrid(dim[0], dim[1], dim[2]));
            setStructuredGridGhostLayers(s, ghostWidth);
            setStructuredGridGlobalOffsets(s, offsets);
            geoOut = s;

        } else if (geoMode == Unstructured_Grid) {
            const Index nx = std::max(dim[0] - 1, Index(1));
            const Index ny = std::max(dim[1] - 1, Index(1));
            const Index nz = std::max(dim[2] - 1, Index(1));
            Index numElem = nx * ny * nz;
            UnstructuredGrid::ptr u(new UnstructuredGrid(numElem, numElem * numCellVert, numVert));
            Index elem = 0;
            Index idx = 0;
            Index *cl = u->cl().data();
            Index *el = u->el().data();
            Byte *tl = u->tl().data();

            unsigned type = UnstructuredGrid::POINT;
            if (dim[2] > 1) {
                type = UnstructuredGrid::HEXAHEDRON;
            } else if (dim[1] > 1) {
                type = UnstructuredGrid::QUAD;
            } else if (dim[0] > 1) {
                type = UnstructuredGrid::BAR;
            }

            for (Index ix = 0; ix < nx; ++ix) {
                for (Index iy = 0; iy < ny; ++iy) {
                    for (Index iz = 0; iz < nz; ++iz) {
                        cl[idx++] = UniformGrid::vertexIndex(ix, iy, iz, dim);
                        if (dim[0] > 1) {
                            cl[idx++] = UniformGrid::vertexIndex(ix + 1, iy, iz, dim);
                            if (dim[1] > 1) {
                                cl[idx++] = UniformGrid::vertexIndex(ix + 1, iy + 1, iz, dim);
                                cl[idx++] = UniformGrid::vertexIndex(ix, iy + 1, iz, dim);
                                if (dim[2] > 1) {
                                    cl[idx++] = UniformGrid::vertexIndex(ix, iy, iz + 1, dim);
                                    cl[idx++] = UniformGrid::vertexIndex(ix + 1, iy, iz + 1, dim);
                                    cl[idx++] = UniformGrid::vertexIndex(ix + 1, iy + 1, iz + 1, dim);
                                    cl[idx++] = UniformGrid::vertexIndex(ix, iy + 1, iz + 1, dim);
                                }
                            }
                        }

                        tl[elem] = type;
                        if ((ix < ghostWidth[0][0] || ix + ghostWidth[0][1] >= nx) ||
                            (iy < ghostWidth[1][0] || iy + ghostWidth[1][1] >= ny) ||
                            (iz < ghostWidth[2][0] || iz + ghostWidth[2][1] >= nz)) {
                            tl[elem] |= UnstructuredGrid::GHOST_BIT;
                        }

                        ++elem;
                        el[elem] = idx;
                    }
                }
            }

            if (checkConvexity())
                u->checkConvexity();

            geoOut = u;
        } else if (geoMode == Point_Geometry) {
            Points::ptr p(new Points(numVert));
            geoOut = p;
        } else if (geoMode == Sphere_Geometry) {
            Spheres::ptr s(new Spheres(numVert));
            geoOut = s;
        }

        if (auto lg = LayerGrid::as(geoOut)) {
            Scalar *z = lg->z().data();
            for (Index i = 0; i < dim[0]; ++i) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index k = 0; k < dim[2]; ++k) {
                        Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                        z[idx] = min[2] + k * dist[2];
                    }
                }
            }
        }

        if (auto coords = Coords::as(geoOut)) {
            Scalar *x = coords->x().data();
            Scalar *y = coords->y().data();
            Scalar *z = coords->z().data();
            for (Index i = 0; i < dim[0]; ++i) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index k = 0; k < dim[2]; ++k) {
                        Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                        x[idx] = min[0] + i * dist[0];
                        y[idx] = min[1] + j * dist[1];
                        z[idx] = min[2] + k * dist[2];
                    }
                }
            }
        }

        if (auto rad = CoordsWithRadius::as(geoOut)) {
            Scalar *r = rad->r().data();
            for (Index i = 0; i < dim[0]; ++i) {
                for (Index j = 0; j < dim[1]; ++j) {
                    for (Index k = 0; k < dim[2]; ++k) {
                        Index idx = StructuredGrid::vertexIndex(i, j, k, dim);
                        r[idx] = dist[0] * 0.2;
                    }
                }
            }
        }
    }

    Index numData = elementData ? numCells : numVert;

    Vec<Scalar, 1>::ptr scalar;
    if (isConnected("data_out0")) {
        scalar.reset(new Vec<Scalar, 1>(numData));
        scalar->setBlock(block);
        if (time >= 0)
            scalar->setTimestep(time);
    }
    Vec<Scalar, 3>::ptr vector;
    if (isConnected("data_out1")) {
        vector.reset(new Vec<Scalar, 3>(numData));
        vector->setBlock(block);
        if (time >= 0)
            vector->setTimestep(time);
    }

    geoOut->refresh();

    const int dtime = time < 0 ? 0 : time;
    if (elementData) {
        const GridInterface *grid = geoOut->getInterface<GridInterface>();
        if (scalar) {
            setDataCells(scalar->x().data(), grid, (DataMode)m_dataModeScalar->getValue(),
                         m_dataScaleScalar->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
        }
        if (vector) {
            setDataCells(vector->x().data(), grid, (DataMode)m_dataMode[0]->getValue(), m_dataScale[0]->getValue(),
                         (AnimDataMode)m_animData->getValue(), dtime);
            setDataCells(vector->y().data(), grid, (DataMode)m_dataMode[1]->getValue(), m_dataScale[1]->getValue(),
                         (AnimDataMode)m_animData->getValue(), dtime);
            setDataCells(vector->z().data(), grid, (DataMode)m_dataMode[2]->getValue(), m_dataScale[2]->getValue(),
                         (AnimDataMode)m_animData->getValue(), dtime);
        }
    } else if (auto coord = Coords::as(geoOut)) {
        const Scalar *xx = coord->x().data();
        const Scalar *yy = coord->y().data();
        const Scalar *zz = coord->z().data();

        if (scalar) {
            setDataCoords(scalar->x().data(), numVert, xx, yy, zz, (DataMode)m_dataModeScalar->getValue(),
                          m_dataScaleScalar->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
        }
        if (vector) {
            setDataCoords(vector->x().data(), numVert, xx, yy, zz, (DataMode)m_dataMode[0]->getValue(),
                          m_dataScale[0]->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
            setDataCoords(vector->y().data(), numVert, xx, yy, zz, (DataMode)m_dataMode[1]->getValue(),
                          m_dataScale[1]->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
            setDataCoords(vector->z().data(), numVert, xx, yy, zz, (DataMode)m_dataMode[2]->getValue(),
                          m_dataScale[2]->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
        }
    } else {
        if (scalar) {
            setDataUniform(scalar->x().data(), dim, min, max, (DataMode)m_dataModeScalar->getValue(),
                           m_dataScaleScalar->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
        }
        if (vector) {
            setDataUniform(vector->x().data(), dim, min, max, (DataMode)m_dataMode[0]->getValue(),
                           m_dataScale[0]->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
            setDataUniform(vector->y().data(), dim, min, max, (DataMode)m_dataMode[1]->getValue(),
                           m_dataScale[1]->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
            setDataUniform(vector->z().data(), dim, min, max, (DataMode)m_dataMode[2]->getValue(),
                           m_dataScale[2]->getValue(), (AnimDataMode)m_animData->getValue(), dtime);
        }
    }

    if (geoOut) {
        geoOut->updateInternals();
        geoOut->setBlock(block);
        if (time >= 0)
            geoOut->setTimestep(time);
        token.addObject("grid_out", geoOut);
        if (scalar) {
            scalar->setMapping(elementData ? DataBase::Element : DataBase::Vertex);
            scalar->setGrid(geoOut);
        }
        if (vector) {
            vector->setMapping(elementData ? DataBase::Element : DataBase::Vertex);
            vector->setGrid(geoOut);
        }
    }

    if (scalar) {
        scalar->addAttribute("_species", "scalar");
        token.addObject("data_out0", scalar);
    }
    if (vector) {
        vector->addAttribute("_species", "vector");
        token.addObject("data_out1", vector);
    }

    auto delay = m_delay->getValue();
    if (delay > 0.) {
        usleep(int32_t(delay * 1e6));
    }
}
