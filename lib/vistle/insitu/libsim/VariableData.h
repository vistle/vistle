#ifndef VISTLE_INSITU_LIBSIM_VARIABLEDATA_H
#define VISTLE_INSITU_LIBSIM_VARIABLEDATA_H

#include "VariableInfo.h"
#include <vistle/core/object.h>
#include <vistle/core/vec.h>

#include <memory>
class visit_handle;
namespace vistle {
class UnstructuredGrid;
namespace insitu {
namespace message {
class SyncShmIDs;
}
namespace libsim {
namespace VariableData {
vistle::Object::ptr get(const visit_handle &varHandle);
vistle::Vec<vistle::Scalar, 1>::ptr allocVarForCombinedMesh(const VariableInfo &varInfo, vistle::Object::ptr mesh);

} // namespace VariableData

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif
