#ifndef VERTEX_TYPES_TO_VISTLE_H
#define VERTEX_TYPES_TO_VISTLE_H
#include <core/unstr.h>
#include "VisItDataTypes.h"
namespace insitu {


	static    vistle::UnstructuredGrid::Type vertexTypeToVistle(int type)
	{
		static vistle::UnstructuredGrid::Type libsim_to_vistle[VISIT_NUM_CELL_TYPES];
		static bool init = false;
		if (!init)
		{
			for (size_t i = 0; i < vistle::UnstructuredGrid::Type::NUM_TYPES; i++)
			{
				libsim_to_vistle[i] = vistle::UnstructuredGrid::Type::NONE;
			}
			libsim_to_vistle[VISIT_CELL_BEAM] = vistle::UnstructuredGrid::Type::BAR;
			libsim_to_vistle[VISIT_CELL_TRI] = vistle::UnstructuredGrid::Type::TRIANGLE;
			libsim_to_vistle[VISIT_CELL_QUAD] = vistle::UnstructuredGrid::Type::QUAD;
			libsim_to_vistle[VISIT_CELL_TET] = vistle::UnstructuredGrid::Type::TETRAHEDRON;
			libsim_to_vistle[VISIT_CELL_PYR] = vistle::UnstructuredGrid::Type::PYRAMID;
			libsim_to_vistle[VISIT_CELL_WEDGE] = vistle::UnstructuredGrid::Type::PRISM;
			libsim_to_vistle[VISIT_CELL_HEX] = vistle::UnstructuredGrid::Type::HEXAHEDRON;
			libsim_to_vistle[VISIT_CELL_POINT] = vistle::UnstructuredGrid::Type::POINT;
			libsim_to_vistle[VISIT_CELL_POLYHEDRON] = vistle::UnstructuredGrid::Type::POLYHEDRON;

			libsim_to_vistle[VISIT_CELL_QUADRATIC_EDGE] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_TRI] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_QUAD] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_TET] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_PYR] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_WEDGE] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_HEX] = vistle::UnstructuredGrid::Type::NONE;

			libsim_to_vistle[VISIT_CELL_BIQUADRATIC_TRI] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_BIQUADRATIC_QUAD] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_TRIQUADRATIC_HEX] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_LINEAR_QUAD] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_QUADRATIC_LINEAR_WEDGE] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_BIQUADRATIC_QUADRATIC_WEDGE] = vistle::UnstructuredGrid::Type::NONE;
			libsim_to_vistle[VISIT_CELL_BIQUADRATIC_QUADRATIC_HEX] = vistle::UnstructuredGrid::Type::NONE;
			init = true;
		}
		return libsim_to_vistle[type];
	}

	static vistle::Index getNumVertices(vistle::UnstructuredGrid::Type type)
	{
		static vistle::Index num_verts_of_elem[vistle::UnstructuredGrid::Type::NUM_TYPES];
		static bool  init = false;
		if (!init)
		{
			num_verts_of_elem[vistle::UnstructuredGrid::Type::NONE] = 0;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::BAR] = 2;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::TRIANGLE] = 3;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::QUAD] = 4;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::TETRAHEDRON] = 4;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::PYRAMID] = 5;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::PRISM] = 6;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::HEXAHEDRON] = 8;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::POLYGON] = -1;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::VPOLYHEDRON] =
			num_verts_of_elem[vistle::UnstructuredGrid::Type::POINT] = 1;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::CPOLYHEDRON] = -1;
			num_verts_of_elem[vistle::UnstructuredGrid::Type::NUM_TYPES] = -1;

			init = true;
		}
		return num_verts_of_elem[type];
	}
}

//GHOST_BIT = cell::GHOST_BIT,
//CONVEX_BIT = cell::CONVEX_BIT,
//TYPE_MASK = cell::TYPE_MASK,
//
//POLYHEDRON = cell::POLYHEDRON,
//
//NONE = cell::NONE,
//BAR = cell::BAR,
//TRIANGLE = cell::TRIANGLE,
//QUAD = cell::QUAD,
//TETRAHEDRON = cell::TETRAHEDRON,
//PYRAMID = cell::PYRAMID,
//PRISM = cell::PRISM,
//HEXAHEDRON = cell::HEXAHEDRON,
//POLYGON = cell::POLYGON,
//VPOLYHEDRON = cell::VPOLYHEDRON,
//POINT = cell::POINT,
//CPOLYHEDRON = cell::CPOLYHEDRON,
//NUM_TYPES = cell::NUM_TYPES,


///* Cell Types */
//constexpr int VISIT_CELL_BEAM = 0;
//constexpr int VISIT_CELL_TRI = 1;
//constexpr int VISIT_CELL_QUAD = 2;
//constexpr int VISIT_CELL_TET = 3;
//constexpr int VISIT_CELL_PYR = 4;
//constexpr int VISIT_CELL_WEDGE = 5;
//constexpr int VISIT_CELL_HEX = 6;
//constexpr int VISIT_CELL_POINT = 7;
//constexpr int VISIT_CELL_POLYHEDRON = 8;
//
//constexpr int VISIT_CELL_QUADRATIC_EDGE = 20;
//constexpr int VISIT_CELL_QUADRATIC_TRI = 21;
//constexpr int VISIT_CELL_QUADRATIC_QUAD = 22;
//constexpr int VISIT_CELL_QUADRATIC_TET = 23;
//constexpr int VISIT_CELL_QUADRATIC_PYR = 24;
//constexpr int VISIT_CELL_QUADRATIC_WEDGE = 25;
//constexpr int VISIT_CELL_QUADRATIC_HEX = 26;
//
//constexpr int VISIT_CELL_BIQUADRATIC_TRI = 27;
//constexpr int VISIT_CELL_BIQUADRATIC_QUAD = 28;
//constexpr int VISIT_CELL_TRIQUADRATIC_HEX = 29;
//constexpr int VISIT_CELL_QUADRATIC_LINEAR_QUAD = 30;
//constexpr int VISIT_CELL_QUADRATIC_LINEAR_WEDGE = 31;
//constexpr int VISIT_CELL_BIQUADRATIC_QUADRATIC_WEDGE = 32;
//constexpr int VISIT_CELL_BIQUADRATIC_QUADRATIC_HEX = 33;
#endif