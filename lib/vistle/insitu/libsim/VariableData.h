#ifndef VISLTLE_LIBSIM_VARIABLE_DATA_H
#define VISLTLE_LIBSIM_VARIABLE_DATA_H

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
vistle::Object::ptr get(const visit_handle &varHandle, message::SyncShmIDs &creator);
vistle::Vec<vistle::Scalar, 1>::ptr allocVarForCombinedMesh(const VariableInfo &varInfo, vistle::Object::ptr mesh,
                                                            message::SyncShmIDs &creator);

} // namespace VariableData

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif
