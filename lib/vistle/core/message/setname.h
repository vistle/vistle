#ifndef VISTLE_CORE_MESSAGE_SETNAME_H
#define VISTLE_CORE_MESSAGE_SETNAME_H

#include "../message.h"

namespace vistle {
namespace message {

//! set display name of a module
class V_COREEXPORT SetName: public MessageBase<SetName, SETNAME> {
public:
    SetName(int module, const std::string &name);
    int module() const;
    const char *name() const;

private:
    int m_module;
    description_t m_name;
};

} // namespace message
} // namespace vistle
#endif
