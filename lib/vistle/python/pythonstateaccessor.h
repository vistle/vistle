#ifndef PYTHON_STATEACCESSOR_H
#define PYTHON_STATEACCESSOR_H

#include "export.h"
#include <vistle/util/buffer.h>

namespace vistle {

class StateTracker;

namespace message {
class Message;
}

struct V_PYEXPORT PythonStateAccessor {
    virtual ~PythonStateAccessor();
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual StateTracker &state() = 0;
    virtual bool sendMessage(const vistle::message::Message &m, const buffer *payload = nullptr) = 0;
};

} // namespace vistle
#endif
