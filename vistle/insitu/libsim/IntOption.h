#ifndef VISTLE_LIBSIM_INT_OPTION_H
#define VISTLE_LIBSIM_INT_OPTION_H
#include <vistle/insitu/message/sharedOption.h>
#include <vistle/insitu/message/InSituMessage.h>

#include <vistle/core/scalar.h>

namespace vistle{
namespace insitu{
namespace libsim{
    enum class IntOptions
    {
        CombineGrids
        , ConstGrids
        , KeepTimesteps
        , NthTimestep
        , VtkFormat
    };

    typedef message::SharedOption<IntOptions, vistle::Integer> IntOption;
}
namespace message{
    DECLARE_ENGINE_MESSAGE_WITH_PARAM(LibSimIntOption, libsim::IntOption)
}
}
}

#endif