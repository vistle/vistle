//-------------------------------------------------------------------------
// TO UNSTRUCTURED H
// * converts a grid to an unstructured grid
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------


#include "ToUnstructured.h"
#include <vistle/alg/objalg.h>
#include <vistle/module/resultcache.h>

using namespace vistle;

const vistle::Index M_NUM_CORNERS_HEXAHEDRON = 8;
const vistle::Index M_NUM_CORNERS_QUAD = 4;
const vistle::Index M_NUM_CORNERS_BAR = 2;
//const vistle::Index M_NUM_CORNERS_POINT = 1;

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(ToUnstructured)


//-------------------------------------------------------------------------
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// CONSTRUCTOR
//-------------------------------------------------------------------------
ToUnstructured::ToUnstructured(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    // create ports
    createInputPort("data_in", "(data on) structured, rectilinear, or uniform grid");
    createOutputPort("data_out", "(data on) equivalent unstructured grid");

    // add module parameters
    //none


    // policies
    // none

    // additional operations
    // none

    addResultCache(m_cache);
}

// DESTRUCTOR
//-------------------------------------------------------------------------
ToUnstructured::~ToUnstructured()
{}

// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool ToUnstructured::compute()
{
    // acquire input data object
    Object::const_ptr input = accept<Object>("data_in");
    auto split = splitContainerObject(input);
    Object::const_ptr gridObj = split.geometry;
    DataBase::const_ptr data = split.mapped;

    UnstructuredGrid::ptr unstrGridOut;
    if (auto entry = m_cache.getOrLock(gridObj->getName(), unstrGridOut)) {
        StructuredGridBase::const_ptr grid = StructuredGridBase::as(gridObj);
        if (auto unstr = UnstructuredGrid::as(gridObj)) {
            unstrGridOut = unstr->clone();
        } else if (!grid) {
            sendError("unusable input data: neither structured nor unstructured grid");
        } else {
            // BEGIN CONVERSION BETWEEN STRUCTURED AND UNSTRUCTURED:
            // * all Structured grids share the same algorithms for generating their tl, cl and el.
            // * x, y, z members are unique, however

            const Index dim[3] = {grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2)};
            const Index nx = dim[0] - 1;
            const Index ny = dim[1] - 1;
            const Index nz = dim[2] - 1;

            Byte CellType = UnstructuredGrid::POINT;
            if (nz > 0) {
                CellType = UnstructuredGrid::HEXAHEDRON;
            } else if (ny > 0) {
                CellType = UnstructuredGrid::QUAD;
            } else if (nx > 0) {
                CellType = UnstructuredGrid::BAR;
            }

            int NumCellCorners = UnstructuredGrid::NumVertices[CellType];

            // instantiate output unstructured grid data object
            const Index numElements = grid->getNumElements();
            const Index numCorners = grid->getNumElements() * NumCellCorners;
            const Index numVerticesTotal =
                grid->getNumDivisions(0) * grid->getNumDivisions(1) * grid->getNumDivisions(2);
            const Cartesian3<Index> numVertices =
                Cartesian3<Index>(grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2));

            auto structuredGrid = StructuredGrid::as(gridObj);
            unstrGridOut.reset(new UnstructuredGrid(numElements, numCorners, structuredGrid ? 0 : numVerticesTotal));

            // construct type list and element list
            for (Index i = 0; i < numElements; i++) {
                unstrGridOut->tl()[i] = CellType;
                unstrGridOut->setGhost(i, grid->isGhostCell(i));
                unstrGridOut->el()[i] = i * NumCellCorners;
            }

            // add total at end
            unstrGridOut->el()[numElements] = numElements * NumCellCorners;

            // construct connectivity list (all hexahedra)
            if (nz > 0) {
                // 3-dim
                auto *cl = unstrGridOut->cl().data();
                Index el = 0;
                for (Index ix = 0; ix < nx; ++ix) {
                    for (Index iy = 0; iy < ny; ++iy) {
                        for (Index iz = 0; iz < nz; ++iz) {
                            const Index baseInsertionIndex = el * M_NUM_CORNERS_HEXAHEDRON;
                            // clang-format off
                            cl[baseInsertionIndex] = UniformGrid::vertexIndex(ix, iy, iz, dim);                 // 0       7 -------- 6
                            cl[baseInsertionIndex + 1] = UniformGrid::vertexIndex(ix + 1, iy, iz, dim);         // 1      /|         /|
                            cl[baseInsertionIndex + 2] = UniformGrid::vertexIndex(ix + 1, iy + 1, iz, dim);     // 2     / |        / |
                            cl[baseInsertionIndex + 3] = UniformGrid::vertexIndex(ix, iy + 1, iz, dim);         // 3    4 -------- 5  |
                            cl[baseInsertionIndex + 4] = UniformGrid::vertexIndex(ix, iy, iz + 1, dim);         // 4    |  3-------|--2
                            cl[baseInsertionIndex + 5] = UniformGrid::vertexIndex(ix + 1, iy, iz + 1, dim);     // 5    | /        | /
                            cl[baseInsertionIndex + 6] = UniformGrid::vertexIndex(ix + 1, iy + 1, iz + 1, dim); // 6    |/         |/
                            cl[baseInsertionIndex + 7] = UniformGrid::vertexIndex(ix, iy + 1, iz + 1, dim);     // 7    0----------1
                            // clang-format on

                            ++el;
                        }
                    }
                }
            } else if (ny > 0) {
                // 2-dim
                auto *cl = unstrGridOut->cl().data();
                Index el = 0;
                for (Index ix = 0; ix < nx; ++ix) {
                    for (Index iy = 0; iy < ny; ++iy) {
                        const Index baseInsertionIndex = el * M_NUM_CORNERS_QUAD;
                        cl[baseInsertionIndex] = UniformGrid::vertexIndex(ix, iy, 0, dim);
                        cl[baseInsertionIndex + 1] = UniformGrid::vertexIndex(ix + 1, iy, 0, dim);
                        cl[baseInsertionIndex + 2] = UniformGrid::vertexIndex(ix + 1, iy + 1, 0, dim);
                        cl[baseInsertionIndex + 3] = UniformGrid::vertexIndex(ix, iy + 1, 0, dim);

                        ++el;
                    }
                }
            } else if (nx > 0) {
                // 1-dim
                auto *cl = unstrGridOut->cl().data();
                Index el = 0;
                for (Index ix = 0; ix < nx; ++ix) {
                    const Index baseInsertionIndex = el * M_NUM_CORNERS_BAR;
                    cl[baseInsertionIndex] = UniformGrid::vertexIndex(ix, 0, 0, dim);
                    cl[baseInsertionIndex + 1] = UniformGrid::vertexIndex(ix + 1, 0, 0, dim);

                    ++el;
                }
            } else {
                // 0-dim
                unstrGridOut->cl()[0] = 0;
            }

            // pass on conversion of x, y, z vectors to specific helper functions based on grid type
            if (auto uniformGrid = UniformGrid::as(gridObj)) {
                compute_uniformVecs(uniformGrid, unstrGridOut, numVertices);
            } else if (auto layerGrid = LayerGrid::as(gridObj)) {
                compute_layerVecs(layerGrid, unstrGridOut, numVertices);
            } else if (auto rectilinearGrid = RectilinearGrid::as(gridObj)) {
                compute_rectilinearVecs(rectilinearGrid, unstrGridOut, numVertices);
            } else if (auto structuredGrid = StructuredGrid::as(gridObj)) {
                for (int c = 0; c < 3; ++c) {
                    unstrGridOut->d()->x[c] = structuredGrid->d()->x[c];
                }
            } else {
                sendError("Unable to convert Structured Grid Base object to a leaf data type");
            }

            unstrGridOut->copyAttributes(gridObj);
        }

        updateMeta(unstrGridOut);
        m_cache.storeAndUnlock(entry, unstrGridOut);
    }

    // output data
    if (!unstrGridOut) {
    } else if (data) {
        auto outdata = data->clone();
        outdata->copyAttributes(data);
        outdata->setGrid(unstrGridOut);
        updateMeta(unstrGridOut);
        addObject("data_out", outdata);
    } else {
        addObject("data_out", unstrGridOut);
    }

    return true;
}

// COMPUTE HELPER FUNCTION - PROCESS UNIFORM GRID OBJECT
//-------------------------------------------------------------------------
void ToUnstructured::compute_uniformVecs(UniformGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                         const Cartesian3<Index> numVertices)
{
    const Cartesian3<Scalar> min = Cartesian3<Scalar>(obj->min()[0], obj->min()[1], obj->min()[2]);
    const Cartesian3<Scalar> max = Cartesian3<Scalar>(obj->max()[0], obj->max()[1], obj->max()[2]);
    Cartesian3<Index> div = numVertices;
    if (div.x <= 2)
        div.x = 2;
    if (div.y <= 2)
        div.y = 2;
    if (div.z <= 2)
        div.z = 2;
    const Cartesian3<Scalar> delta = Cartesian3<Scalar>(
        ((max.x - min.x) / (div.x - 1)), ((max.y - min.y) / (div.y - 1)), ((max.z - min.z) / (div.z - 1)));


    // construct vertices
    const Index dim[3] = {numVertices.x, numVertices.y, numVertices.z};
    for (Index i = 0; i < numVertices.x; i++) {
        for (Index j = 0; j < numVertices.y; j++) {
            for (Index k = 0; k < numVertices.z; k++) {
                const Index insertionIndex = UniformGrid::vertexIndex(i, j, k, dim);

                unstrGridOut->x()[insertionIndex] = min.x + i * delta.x;
                unstrGridOut->y()[insertionIndex] = min.y + j * delta.y;
                unstrGridOut->z()[insertionIndex] = min.z + k * delta.z;
            }
        }
    }

    return;
}

void ToUnstructured::compute_layerVecs(LayerGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                       const Cartesian3<Index> numVertices)
{
    std::array<Scalar, 2> min = {obj->min()[0], obj->min()[1]};
    std::array<Scalar, 2> max = {obj->max()[0], obj->max()[1]};
    Cartesian3<Index> div = numVertices;
    if (div.x <= 2)
        div.x = 2;
    if (div.y <= 2)
        div.y = 2;
    if (div.z <= 2)
        div.z = 2;
    std::array<Scalar, 2> delta = {((max[0] - min[0]) / (div.x - 1)), ((max[1] - min[1]) / (div.y - 1))};

    // construct vertices
    const Index dim[3] = {numVertices.x, numVertices.y, numVertices.z};
    for (Index i = 0; i < numVertices.x; i++) {
        for (Index j = 0; j < numVertices.y; j++) {
            for (Index k = 0; k < numVertices.z; k++) {
                const Index insertionIndex = UniformGrid::vertexIndex(i, j, k, dim);

                unstrGridOut->x()[insertionIndex] = min[0] + i * delta[0];
                unstrGridOut->y()[insertionIndex] = min[1] + j * delta[1];
            }
        }
    }
    unstrGridOut->d()->x[2] = obj->d()->x[0];
}

// COMPUTE HELPER FUNCTION - PROCESS RECTILINEAR GRID OBJECT
//-------------------------------------------------------------------------
void ToUnstructured::compute_rectilinearVecs(RectilinearGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                             const Cartesian3<Index> numVertices)
{
    const Index dim[3] = {numVertices.x, numVertices.y, numVertices.z};
    for (Index i = 0; i < numVertices.x; i++) {
        for (Index j = 0; j < numVertices.y; j++) {
            for (Index k = 0; k < numVertices.z; k++) {
                const Index insertionIndex = UniformGrid::vertexIndex(i, j, k, dim);

                unstrGridOut->x()[insertionIndex] = obj->coords(0)[i];
                unstrGridOut->y()[insertionIndex] = obj->coords(1)[j];
                unstrGridOut->z()[insertionIndex] = obj->coords(2)[k];
            }
        }
    }

    return;
}
