//-------------------------------------------------------------------------
// STRUCTURED TO UNSTRUCTURED H
// * converts a structured grid to an unstructured grid
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------


#include "StructuredToUnstructured.h"

using namespace vistle;

//-------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------
MODULE_MAIN(StructuredToUnstructured)


//-------------------------------------------------------------------------
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// CONSTRUCTOR
//-------------------------------------------------------------------------
StructuredToUnstructured::StructuredToUnstructured(const std::string &shmname, const std::string &name, int moduleID)
   : Module("StructuredToUnstructured", shmname, name, moduleID) {

   // create ports
   createInputPort("data_in");
   createOutputPort("data_out");

   // add module parameters
   //none


   // policies
   // none

   // additional operations
   // none
}

// DESTRUCTOR
//-------------------------------------------------------------------------
StructuredToUnstructured::~StructuredToUnstructured() {

}

// COMPUTE FUNCTION
//-------------------------------------------------------------------------
bool StructuredToUnstructured::compute() {

    // acquire input data object
    Object::const_ptr input = accept<Object>("data_in");
    Object::const_ptr gridObj = input;
    DataBase::const_ptr data = DataBase::as(gridObj);
    if (data && data->grid()) {
        gridObj = data->grid();
    }
    if (UnstructuredGrid::as(gridObj)) {
        passThroughObject("data_out", input);
        return true;
    }

    StructuredGridBase::const_ptr grid = StructuredGridBase::as(gridObj);
    // assert existence of useable data
    if (!grid) {
       sendInfo("Error: Unusable Input Data");
       return true;
    }


    // BEGIN CONVERSION BETWEEN STRUCTURED AND UNSTRUCTURED:
    // * all Structured grids share the same algorithms for generating their tl, cl and el.
    // * x, y, z members are unique, however

    // instantiate output unstructured grid data object
    const Index numElements = grid->getNumElements();
    const Index numCorners = grid->getNumElements() * M_NUM_CORNERS_HEXAHEDRON;
    const Index numVerticesTotal = grid->getNumDivisions(0) * grid->getNumDivisions(1) * grid->getNumDivisions(2);
    const Cartesian3<Index> numVertices = Cartesian3<Index>(
                grid->getNumDivisions(0),
                grid->getNumDivisions(1),
                grid->getNumDivisions(2)
                );

    auto structuredGrid = StructuredGrid::as(gridObj);
    UnstructuredGrid::ptr unstrGridOut(new UnstructuredGrid(numElements, numCorners, structuredGrid ? 0 : numVerticesTotal));

    // construct type list and element list
    for (Index i = 0; i < numElements; i++) {
        unstrGridOut->tl()[i] = grid->isGhostCell(i) ? UnstructuredGrid::GHOST_HEXAHEDRON : UnstructuredGrid::HEXAHEDRON;
        unstrGridOut->el()[i] = i * M_NUM_CORNERS_HEXAHEDRON;
    }

    // add total at end
    unstrGridOut->el()[numElements] = numElements * M_NUM_CORNERS_HEXAHEDRON;


    // construct connectivity list (all hexahedra)
    const Index dim[3] = { grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2) };
    const Index nx = dim[0]-1;
    const Index ny = dim[1]-1;
    const Index nz = dim[2]-1;
    Index el = 0;
    for (Index ix=0; ix<nx; ++ix) {
        for (Index iy=0; iy<ny; ++iy) {
            for (Index iz=0; iz<nz; ++iz) {
                const Index baseInsertionIndex = el * M_NUM_CORNERS_HEXAHEDRON;
                unstrGridOut->cl()[baseInsertionIndex]   = UniformGrid::vertexIndex(ix,   iy,   iz,   dim);       // 0       7 -------- 6
                unstrGridOut->cl()[baseInsertionIndex+1] = UniformGrid::vertexIndex(ix+1, iy,   iz,   dim);       // 1      /|         /|
                unstrGridOut->cl()[baseInsertionIndex+2] = UniformGrid::vertexIndex(ix+1, iy+1, iz,   dim);       // 2     / |        / |
                unstrGridOut->cl()[baseInsertionIndex+3] = UniformGrid::vertexIndex(ix,   iy+1, iz,   dim);       // 3    4 -------- 5  |
                unstrGridOut->cl()[baseInsertionIndex+4] = UniformGrid::vertexIndex(ix,   iy,   iz+1, dim);       // 4    |  3-------|--2
                unstrGridOut->cl()[baseInsertionIndex+5] = UniformGrid::vertexIndex(ix+1, iy,   iz+1, dim);       // 5    | /        | /
                unstrGridOut->cl()[baseInsertionIndex+6] = UniformGrid::vertexIndex(ix+1, iy+1, iz+1, dim);       // 6    |/         |/
                unstrGridOut->cl()[baseInsertionIndex+7] = UniformGrid::vertexIndex(ix,   iy+1, iz+1, dim);       // 7    0----------1

                ++el;
            }
        }
    }

    // pass on conversion of x, y, z vectors to specific helper functions based on grid type
    if (auto uniformGrid = UniformGrid::as(gridObj)) {
        compute_uniformVecs(uniformGrid, unstrGridOut, numVertices);
    } else if (auto rectilinearGrid = RectilinearGrid::as(gridObj)) {
        compute_rectilinearVecs(rectilinearGrid, unstrGridOut, numVertices);
    } else if (auto structuredGrid = StructuredGrid::as(gridObj)) {
        for (int c=0; c<3; ++c) {
            unstrGridOut->d()->x[c] = structuredGrid->d()->x[c];
        }
    } else {
        sendInfo("Error: Unable to convert Structured Grid Base object to a leaf data type");
        return true;
    }

    // output data
    unstrGridOut->copyAttributes(gridObj);
    if (data) {
        auto outdata = data->clone();
        outdata->copyAttributes(data);
        outdata->setGrid(unstrGridOut);
        addObject("data_out", outdata);
    } else {
        addObject("data_out", unstrGridOut);
    }

   return true;
}

// COMPUTE HELPER FUNCTION - PROCESS UNIFORM GRID OBJECT
//-------------------------------------------------------------------------
void StructuredToUnstructured::compute_uniformVecs(UniformGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                                   const Cartesian3<Index> numVertices) {
    const Cartesian3<Scalar> min = Cartesian3<Scalar>(
                obj->min()[0],
                obj->min()[1],
                obj->min()[2]
                );
    const Cartesian3<Scalar> max = Cartesian3<Scalar>(
                obj->max()[0],
                obj->max()[1],
                obj->max()[2]
                );
    const Cartesian3<Scalar> delta = Cartesian3<Scalar>(
                ((max.x - min.x) / (numVertices.x-1)),
                ((max.y - min.y) / (numVertices.y-1)),
                ((max.z - min.z) / (numVertices.z-1))
                );


    // construct vertices
    const Index dim[3] = { numVertices.x, numVertices.y, numVertices.z };
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

// COMPUTE HELPER FUNCTION - PROCESS RECTILINEAR GRID OBJECT
//-------------------------------------------------------------------------
void StructuredToUnstructured::compute_rectilinearVecs(RectilinearGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                                       const Cartesian3<Index> numVertices) {

    const Index dim[3] = { numVertices.x, numVertices.y, numVertices.z };
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
