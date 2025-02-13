#ifndef VISTLE_INSITU_MESSAGE_SHAREDPARAM_H
#define VISTLE_INSITU_MESSAGE_SHAREDPARAM_H

#include "export.h"
#include <vistle/core/scalar.h>
#include <vistle/core/archives_config.h>
#include <string>
#include <vector>

namespace vistle {
namespace insitu {
namespace message {


struct IntParam {
    std::string name;
    vistle::Integer value = 0;
    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &name;
        ar &value;
    }
};

int V_INSITUMESSAGEEXPORT getIntParamValue(const std::vector<IntParam> &c, const std::string &name);

void V_INSITUMESSAGEEXPORT updateIntParam(std::vector<IntParam> &c, const IntParam &p);

} // namespace message
} // namespace insitu
} // namespace vistle


#endif
