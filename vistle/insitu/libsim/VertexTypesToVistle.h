#ifndef VERTEX_TYPES_TO_VISTLE_H
#define VERTEX_TYPES_TO_VISTLE_H
#include <vistle/core/unstr.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>
namespace vistle {
namespace insitu {
namespace libsim {

extern vistle::UnstructuredGrid::Type vertexTypeToVistle(size_t type);

extern vistle::Index getNumVertices(vistle::UnstructuredGrid::Type type);

} // namespace libsim
} // namespace insitu
} // namespace vistle
#endif
