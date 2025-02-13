#ifndef VISTLE_INSITU_LIBSIM_VERTEXTYPESTOVISTLE_H
#define VISTLE_INSITU_LIBSIM_VERTEXTYPESTOVISTLE_H

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
