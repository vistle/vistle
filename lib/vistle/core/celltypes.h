#ifndef VISTLE_CELLTYPES_H
#define VISTLE_CELLTYPES_H

#include <vtkm/CellShape.h>
#include <vtkm/CellClassification.h>

#include "scalar.h"
#include "index.h"

namespace vistle {

namespace cell {

enum CellType {
    // make sure that these types match those from COVISE: src/kernel/do/coDoUnstructuredGrid.h
    NONE = vtkm::CELL_SHAPE_EMPTY,
    POINT = vtkm::CELL_SHAPE_VERTEX,
    //POLY_VERTEX = 2,
    BAR = vtkm::CELL_SHAPE_LINE,
    POLYLINE = vtkm::CELL_SHAPE_POLY_LINE,
    TRIANGLE = vtkm::CELL_SHAPE_TRIANGLE,
    //TRIANGLE_STRIP = 6,
    POLYGON = vtkm::CELL_SHAPE_POLYGON,
    //PIXEL = 8,
    QUAD = vtkm::CELL_SHAPE_QUAD,
    TETRAHEDRON = vtkm::CELL_SHAPE_TETRA,
    POLYHEDRON = 11, // in COVISE, reserved for VOXEL in vtk-m
    HEXAHEDRON = vtkm::CELL_SHAPE_HEXAHEDRON,
    PRISM = vtkm::CELL_SHAPE_WEDGE,
    PYRAMID = vtkm::CELL_SHAPE_PYRAMID,
    NUM_TYPES = vtkm::NUMBER_OF_CELL_SHAPES, // keep last
};

enum CellClassification { NORMAL = vtkm::CellClassification::Normal, GHOST = vtkm::CellClassification::Ghost };

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
struct TypeData<POLYLINE> {
    const int Dimension = 1;
    const CellType type = POLYLINE;
};

template<>
struct TypeData<POLYGON> {
    const int Dimension = 2;
    const CellType type = POLYGON;
    const int NumFaces = 1;
};


template<>
struct TypeData<POLYHEDRON> {
    const CellType type = POLYHEDRON;
    const int Dimension = 3;
};

} // namespace cell
} // namespace vistle
#endif
