#ifndef VISTLE_LIBSIM_INT_OPTION_H
#define VISTLE_LIBSIM_INT_OPTION_H
#include <vistle/insitu/message/sharedOption.h>
#include <vistle/insitu/message/InSituMessage.h>

#include <vistle/core/scalar.h>

namespace vistle {
namespace insitu {
namespace libsim {
enum class IntOptions { CombineGrids, ConstGrids, KeepTimesteps, NthTimestep, VtkFormat, LastDummy };

typedef message::SharedOption<IntOptions, vistle::Integer> IntOption;
} // namespace libsim
namespace message {
DEFINE_IN_SITU_MESSAGE(LibSimIntOption, libsim::IntOption)
}
} // namespace insitu
} // namespace vistle

#endif
