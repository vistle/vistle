#ifndef VISTLE_SENSEI_INT_OPTION_H
#define VISTLE_SENSEI_INT_OPTION_H
#include <vistle/insitu/message/InSituMessage.h>
#include <vistle/insitu/message/sharedOption.h>

#include <vistle/core/scalar.h>
namespace vistle {
namespace insitu {
namespace sensei {
enum class IntOptions { NthTimestep, KeepTimesteps };

typedef message::SharedOption<IntOptions, vistle::Integer> IntOption;
} // namespace sensei
namespace message {
DECLARE_ENGINE_MESSAGE_WITH_PARAM(SenseiIntOption, sensei::IntOption)
}
} // namespace insitu
} // namespace vistle

#endif