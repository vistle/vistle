#ifndef ADD_OBJECT_MSQ_H
#define ADD_OBJECT_MSQ_H
#include "export.h"
#include "moduleInfo.h"
#include <vistle/core/object.h>
#include <vistle/core/messagequeue.h>

namespace vistle {
namespace message {}
namespace insitu {
namespace message {
// Queue to send addObject messages to LibSImController module
class V_INSITUMESSAGEEXPORT AddObjectMsq {
public:
    AddObjectMsq(const ModuleInfo &moduleInfo, size_t rank);
    ~AddObjectMsq() = default;
    AddObjectMsq(AddObjectMsq &other) = delete;
    AddObjectMsq(AddObjectMsq &&other) = default;
    AddObjectMsq operator=(AddObjectMsq &other) = delete;
    AddObjectMsq &operator=(AddObjectMsq &&other) = default;

    void addObject(const std::string &port, vistle::Object::const_ptr obj);
    void sendObjects();

private:
    std::unique_ptr<vistle::message::MessageQueue> m_sendMessageQueue;
    ModuleInfo m_moduleInfo;
    size_t m_rank = 0;
    std::vector<vistle::message::Buffer> m_cache;
};
} // namespace message
} // namespace insitu
} // namespace vistle
#endif // !ADD_OBJECT_MSQ_H
