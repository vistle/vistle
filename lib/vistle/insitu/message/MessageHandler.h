#ifndef VISTLE_INSITU_MESSAGE_HANDLER_H
#define VISTLE_INSITU_MESSAGE_HANDLER_H

#include "InSituMessage.h"

namespace vistle {
namespace insitu {
namespace message {

class MessageHandler {
public:
    template<typename SomeMessage>
    bool send(const SomeMessage &msg) const
    {
        if (msg.type == InSituMessageType::Invalid) {
            std::cerr << "in situ MessageHandler: can not send invalid message!" << std::endl;
            return false;
        }
        vistle::vecostreambuf<vistle::buffer> buf;
        vistle::oarchive ar(buf);
        ar &msg;
        vistle::buffer vec = buf.get_vector();
        return sendMessage(msg.type, vec);
    }

    bool isInitialized() const;

    virtual insitu::message::Message recv() = 0;
    virtual insitu::message::Message tryRecv() = 0;

protected:
    virtual bool sendMessage(InSituMessageType type, const vistle::buffer &msg) const = 0;
};
} // namespace message
} // namespace insitu
} // namespace vistle


#endif // VISTLE_INSITU_MESSAGE_HANDLER_H
