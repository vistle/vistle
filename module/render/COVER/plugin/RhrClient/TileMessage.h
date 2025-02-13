#ifndef VISTLE_COVER_PLUGIN_RHRCLIENT_TILEMESSAGE_H
#define VISTLE_COVER_PLUGIN_RHRCLIENT_TILEMESSAGE_H

#include <memory>
#include <vector>

#include <vistle/util/buffer.h>

namespace vistle {
struct tileMsg;

namespace message {
class RemoteRenderMessage;
}
} // namespace vistle

struct TileMessage {
    TileMessage(std::shared_ptr<vistle::message::RemoteRenderMessage> msg, std::shared_ptr<vistle::buffer> payload);

    std::shared_ptr<vistle::message::RemoteRenderMessage> msg;
    const vistle::tileMsg &tile;
    std::shared_ptr<vistle::buffer> payload;
};
#endif
