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
    DataBase::const_ptr data;
    StructuredGridBase::const_ptr grid = accept<StructuredGridBase>("data_in");
    if (!grid) {
        data = accept<DataBase>("data_in");
        grid = StructuredGridBase::as(data->grid());
    }

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

    UnstructuredGrid::ptr unstrGridOut(new UnstructuredGrid(numElements, numCorners, numVerticesTotal));

    // construct type list and element list
    for (Index i = 0; i < numElements; i++) {
        unstrGridOut->tl()[i] = UnstructuredGrid::HEXAHEDRON;
        unstrGridOut->el()[i] = i * M_NUM_CORNERS_HEXAHEDRON;
    }

    // add total at end
    unstrGridOut->el()[numElements] = numElements * M_NUM_CORNERS_HEXAHEDRON;


    // construct connectivity list (all hexahedra)
    const Index dim[3] = { grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2) };
    const Index nx = dim[0]-1;
    const Index ny = dim[1]-1;
    const Index nz = dim[2]-1;
    Index idx[3];
    Index el = 0;
    for (Index ix=0; ix<nx; ++ix) {
        idx[0] = ix;
        for (Index iy=0; iy<ny; ++iy) {
            idx[1] = iy;
            for (Index iz=0; iz<nz; ++iz) {
                idx[2] = iz;
                const Index v = UniformGrid::obtainCellIndex(idx, dim);

                const Index baseInsertionIndex = el * M_NUM_CORNERS_HEXAHEDRON;
                unstrGridOut->cl()[baseInsertionIndex] = v;                                       // 0            7 -------- 6
                unstrGridOut->cl()[baseInsertionIndex + 1] = v + 1;                               // 1           /|         /|
                unstrGridOut->cl()[baseInsertionIndex + 2] = v + dim[2] + 1;                      // 2          / |        / |
                unstrGridOut->cl()[baseInsertionIndex + 3] = v + dim[2];                          // 3         4 -------- 5  |
                unstrGridOut->cl()[baseInsertionIndex + 4] = v + dim[2] * dim[1];                 // 4         |  3-------|--2
                unstrGridOut->cl()[baseInsertionIndex + 5] = v + dim[2] * dim[1] + 1;             // 5         | /        | /
                unstrGridOut->cl()[baseInsertionIndex + 6] = v + dim[2] * (dim[1] + 1) + 1;       // 6         |/         |/
                unstrGridOut->cl()[baseInsertionIndex + 7] = v + dim[2] * (dim[1] + 1);           // 7         0----------1

                ++el;
            }
        }
    }

    // pass on conversion of x, y, z vectors to specific helper functions based on grid type
    if (auto uniformGrid = UniformGrid::as(grid)) {
        compute_uniformVecs(uniformGrid, unstrGridOut, numVertices);
    } else if (auto rectilinearGrid = RectilinearGrid::as(grid)) {
        compute_rectilinearVecs(rectilinearGrid, unstrGridOut, numVertices);
    } else if (auto structuredGrid = StructuredGrid::as(grid)) {
        compute_structuredVecs(structuredGrid, unstrGridOut, numVerticesTotal);
    } else {
        sendInfo("Error: Unable to convert Structured Grid Base object to a leaf data type");
        return true;
    }

    // output data
    unstrGridOut->copyAttributes(grid);
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
    const Index numElements = obj->getNumElements();
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
                ((max.x - min.x) / numElements),
                ((max.y - min.y) / numElements),
                ((max.z - min.z) / numElements)
                );


    // construct vertices
    const Index dim[3] = { numVertices.x, numVertices.y, numVertices.z };
    Index idx[3];
    for (Index i = 0; i < numVertices.x; i++) {
        idx[0] = i;
        for (Index j = 0; j < numVertices.y; j++) {
            idx[1] = j;
            for (Index k = 0; k < numVertices.z; k++) {
                idx[2] = k;
                const Index insertionIndex = UniformGrid::obtainCellIndex(idx, dim);

                unstrGridOut->x()[insertionIndex] = min.x + i * delta.x;
                unstrGridOut->y()[insertionIndex] = min.y + j * delta.y;
                unstrGridOut->z()[insertionIndex] = min.z + k * delta.z;
            }
        }
    }




    /* left off here: add connectivity list,
        also: ask martin about changing a pushed commit message & commit time of his lazygang scheduling, commit numElements_total

*/
    return;
}

// COMPUTE HELPER FUNCTION - PROCESS RECTILINEAR GRID OBJECT
//-------------------------------------------------------------------------
void StructuredToUnstructured::compute_rectilinearVecs(RectilinearGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                                       const Cartesian3<Index> numVertices) {

    const Index dim[3] = { numVertices.x, numVertices.y, numVertices.z };
    Index idx[3];
    for (Index i = 0; i < numVertices.x; i++) {
        idx[0] = i;
        for (Index j = 0; j < numVertices.y; j++) {
            idx[1] = j;
            for (Index k = 0; k < numVertices.z; k++) {
                idx[2] = k;
                const Index insertionIndex = UniformGrid::obtainCellIndex(idx, dim);

                unstrGridOut->x()[insertionIndex] = obj->coords(0)[i];
                unstrGridOut->y()[insertionIndex] = obj->coords(1)[j];
                unstrGridOut->z()[insertionIndex] = obj->coords(2)[k];
            }
        }
    }

    return;
}

// COMPUTE HELPER FUNCTION - PROCESS STRUCTURED GRID OBJECT
//-------------------------------------------------------------------------
void StructuredToUnstructured::compute_structuredVecs(StructuredGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                                      const Index numVerticesTotal) {

    for (Index i = 0; i < numVerticesTotal; i++) {
        unstrGridOut->x()[i] = obj->x()[i];
        unstrGridOut->y()[i] = obj->y()[i];
        unstrGridOut->z()[i] = obj->z()[i];
    }

    return;

}
