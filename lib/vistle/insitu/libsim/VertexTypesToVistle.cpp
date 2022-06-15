#include "VertexTypesToVistle.h"

vistle::UnstructuredGrid::Type vistle::insitu::libsim::vertexTypeToVistle(size_t type)
{
    static vistle::UnstructuredGrid::Type libsim_to_vistle[VISIT_NUM_CELL_TYPES];
    static bool init = false;
    if (!init) {
        for (size_t i = 0; i < vistle::UnstructuredGrid::Type::NUM_TYPES; i++) {
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
    if (type >= VISIT_NUM_CELL_TYPES)
        return vistle::UnstructuredGrid::Type::NONE;
    return libsim_to_vistle[type];
}

vistle::Index vistle::insitu::libsim::getNumVertices(vistle::UnstructuredGrid::Type type)
{
    static vistle::Index num_verts_of_elem[vistle::UnstructuredGrid::Type::NUM_TYPES];
    static bool init = false;
    if (!init) {
        num_verts_of_elem[vistle::UnstructuredGrid::Type::NONE] = 0;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::BAR] = 2;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::TRIANGLE] = 3;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::QUAD] = 4;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::TETRAHEDRON] = 4;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::PYRAMID] = 5;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::PRISM] = 6;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::HEXAHEDRON] = 8;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::POLYGON] = 0;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::POINT] = 1;
        num_verts_of_elem[vistle::UnstructuredGrid::Type::POLYHEDRON] = 0;
        init = true;
    }
    return num_verts_of_elem[type];
}
