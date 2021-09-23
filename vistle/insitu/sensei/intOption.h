#ifndef VISTLE_SENSEI_INT_OPTION_H
#define VISTLE_SENSEI_INT_OPTION_H
#include <vistle/insitu/message/InSituMessage.h>
#include <vistle/insitu/message/sharedOption.h>

#include <vistle/core/scalar.h>
namespace vistle {
namespace insitu {
namespace sensei {
enum class IntOptions { NthTimestep, KeepTimesteps, LastDummy };

typedef message::SharedOption<IntOptions, vistle::Integer> IntOption;
} // namespace sensei
namespace message {
DEFINE_IN_SITU_MESSAGE(SenseiIntOption, sensei::IntOption)
}
} // namespace insitu
} // namespace vistle

#endif
