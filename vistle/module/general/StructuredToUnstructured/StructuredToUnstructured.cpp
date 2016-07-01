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
    StructuredGridBase::const_ptr obj = expect<StructuredGridBase>("data_in");

    // assert existence of useable data
    if (!obj) {
       sendInfo("Error: Unusable Input Data");
       return true;
    }


    // BEGIN CONVERSION BETWEEN STRUCTURED AND UNSTRUCTURED:
    // * all Structured grids share the same algorithms for generating their tl, cl and el.
    // * x, y, z members are unique, however

    // instantiate output unstructured grid data object
    const Index numElements = obj->getNumElements_total();
    const Index numCorners = obj->getNumElements_total() * M_NUM_CORNERS_HEXAHEDRON;
    const Index numVerticesTotal = (obj->getNumElements_x() + 1) * (obj->getNumElements_y() + 1) * (obj->getNumElements_z() + 1);
    const Cartesian3<Index> numVertices = Cartesian3<Index>(
                obj->getNumElements_x() + 1,
                obj->getNumElements_y() + 1,
                obj->getNumElements_z() + 1
                );

    UnstructuredGrid::ptr unstrGridOut(new UnstructuredGrid(numElements, numCorners, numVerticesTotal));

    // construct type list and element list
    for (unsigned i = 0; i < numElements; i++) {
        unstrGridOut->tl()[i] = UnstructuredGrid::HEXAHEDRON;
        unstrGridOut->el()[i] = i * M_NUM_CORNERS_HEXAHEDRON;
    }

    // add total at end
    unstrGridOut->el()[numElements] = numElements * M_NUM_CORNERS_HEXAHEDRON;


    // construct connectivity list (all hexahedra)
    for (unsigned i = 0; i < numElements; i++) {
        const Index baseInsertionIndex = i * M_NUM_CORNERS_HEXAHEDRON;

        unstrGridOut->cl()[baseInsertionIndex] = i;                                                     // 0            7 -------- 6
        unstrGridOut->cl()[baseInsertionIndex + 1] = i + 1;                                             // 1           /|         /|
        unstrGridOut->cl()[baseInsertionIndex + 2] = i + numVertices.z + 1;                             // 2          / |        / |
        unstrGridOut->cl()[baseInsertionIndex + 3] = i + numVertices.z;                                 // 3         4 -------- 5  |
        unstrGridOut->cl()[baseInsertionIndex + 4] = i + numVertices.z * numVertices.y;                 // 4         |  3-------|--2
        unstrGridOut->cl()[baseInsertionIndex + 5] = i + numVertices.z * numVertices.y + 1;             // 5         | /        | /
        unstrGridOut->cl()[baseInsertionIndex + 6] = i + numVertices.z * (numVertices.y + 1) + 1;       // 6         |/         |/
        unstrGridOut->cl()[baseInsertionIndex + 7] = i + numVertices.z * (numVertices.y + 1);           // 7         0----------1

    }


    // pass on conversion of x, y, z vectors to specific helper functions based on grid type
    if (auto uniformGrid = UniformGrid::as(obj)) {
        compute_uniformVecs(uniformGrid, unstrGridOut, numVertices);
    } else if (auto rectilinearGrid = RectilinearGrid::as(obj)) {
        compute_rectilinearVecs(rectilinearGrid, unstrGridOut, numVertices);
    } else if (auto structuredGrid = StructuredGrid::as(obj)) {
        compute_structuredVecs(structuredGrid, unstrGridOut, numVerticesTotal);
    } else {
        sendInfo("Error: Unable to convert Structured Grid Base object to a leaf data type");
        return true;
    }

    // output data
    addObject("data_out", unstrGridOut);

   return true;
}

// COMPUTE HELPER FUNCTION - PROCESS UNIFORM GRID OBJECT
//-------------------------------------------------------------------------
void StructuredToUnstructured::compute_uniformVecs(UniformGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                                   const Cartesian3<Index> numVertices) {
    const Index numElements = obj->getNumElements_total();
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
    for (unsigned i = 0; i < numVertices.x; i++) {
        for (unsigned j = 0; j < numVertices.y; j++) {
            for (unsigned k = 0; k < numVertices.z; k++) {
                const Index insertionIndex = (i * numVertices.y + j) * numVertices.z + k;

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

    for (unsigned i = 0; i < numVertices.x; i++) {
        for (unsigned j = 0; j < numVertices.y; j++) {
            for (unsigned k = 0; k < numVertices.z; k++) {
                const Index insertionIndex = (i * numVertices.y + j) * numVertices.z + k;

                unstrGridOut->x()[insertionIndex] = obj->coords_x()[i];
                unstrGridOut->y()[insertionIndex] = obj->coords_y()[j];
                unstrGridOut->z()[insertionIndex] = obj->coords_z()[k];
            }
        }
    }

    return;
}

// COMPUTE HELPER FUNCTION - PROCESS STRUCTURED GRID OBJECT
//-------------------------------------------------------------------------
void StructuredToUnstructured::compute_structuredVecs(StructuredGrid::const_ptr obj, UnstructuredGrid::ptr unstrGridOut,
                                                      const Index numVerticesTotal) {

    for (unsigned i = 0; i < numVerticesTotal; i++) {
        unstrGridOut->x()[i] = obj->coords_x()[i];
        unstrGridOut->y()[i] = obj->coords_y()[i];
        unstrGridOut->z()[i] = obj->coords_z()[i];
    }

    return;

}
