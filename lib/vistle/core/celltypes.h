#ifndef VISTLE_CELLTYPES_H
#define VISTLE_CELLTYPES_H

#include "scalar.h"
#include "index.h"

namespace vistle {

namespace cell {

enum CellType {
    GHOST_BIT = 0x80,
    CONVEX_BIT = 0x40, //<! cell was checked to be convex
    TYPE_MASK = 0x3f,

    // make sure that these types match those from COVISE: src/kernel/do/coDoUnstructuredGrid.h
    NONE = 0,
    BAR = 1,
    TRIANGLE = 2,
    QUAD = 3,
    TETRAHEDRON = 4,
    PYRAMID = 5,
    PRISM = 6,
    HEXAHEDRON = 7,
    VPOLYHEDRON = 8, // not in COVISE: polyhedron with facestream as in VTK
    POLYGON = 9, // not in COVISE
    POINT = 10,
    CPOLYHEDRON = 11, // in COVISE, but has to be translated to VPOLYHEDRON
    POLYHEDRON = CPOLYHEDRON,
    NUM_TYPES = 12, // keep last
};

template<int type>
struct TypeData;

template<>
struct TypeData<NONE> {
    const CellType type = NONE;
    const int NumVertices = 0;
};

template<>
struct TypeData<POINT> {
    const CellType type = POINT;
    const int Dimension = 0;
    const int NumVertices = 1;
};

template<>
struct TypeData<BAR> {
    const CellType type = BAR;
    const int Dimension = 1;
    const int NumVertices = 2;
    const int NumEdges = 1;
};

template<>
struct TypeData<TRIANGLE> {
    const CellType type = TRIANGLE;
    const int Dimension = 2;
    const int NumVertices = 3;
    const int NumEdges = 3;
    const int NumFaces = 1;
};

template<>
struct TypeData<QUAD> {
    const CellType type = QUAD;
    const int Dimension = 2;
    const int NumVertices = 4;
    const int NumEdges = 4;
    const int NumFaces = 1;
};

template<>
struct TypeData<TETRAHEDRON> {
    const CellType type = TETRAHEDRON;
    const int Dimension = 3;
    const int NumVertices = 4;
    const int NumEdges = 4;
    const int NumFaces = 4;
};

template<>
struct TypeData<PYRAMID> {
    const CellType type = PYRAMID;
    const int Dimension = 3;
    const int NumVertices = 5;
    const int NumEdges = 8;
    const int NumFaces = 5;
};

template<>
struct TypeData<PRISM> {
    const CellType type = PRISM;
    const int Dimension = 3;
    const int NumVertices = 6;
    const int NumEdges = 9;
    const int NumFaces = 5;
};

template<>
struct TypeData<HEXAHEDRON> {
    const CellType type = HEXAHEDRON;
    const int Dimension = 3;
    const int NumVertices = 8;
    const int NumEdges = 12;
    const int NumFaces = 6;
};

template<>
struct TypeData<POLYGON> {
    const int Dimension = 2;
    const CellType type = POLYGON;
    const int NumFaces = 1;
};

template<>
struct TypeData<CPOLYHEDRON> {
    const CellType type = CPOLYHEDRON;
    const int Dimension = 3;
};

template<>
struct TypeData<VPOLYHEDRON> {
    const CellType type = VPOLYHEDRON;
    const int Dimension = 3;
};

} // namespace cell
} // namespace vistle
#endif
