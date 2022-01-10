#ifndef VISTLE_LIBSIM_INT_OPTION_H
#define VISTLE_LIBSIM_INT_OPTION_H
#include <vistle/insitu/message/sharedOption.h>
#include <vistle/insitu/message/InSituMessage.h>

#include <vistle/core/scalar.h>

namespace vistle {
namespace insitu {
namespace libsim {
const char *IntParams[] = {"CombineGrids", "ConstGrids", "KeepTimesteps", "NthTimestep", "VtkFormat"};
} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif
