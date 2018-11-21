#ifndef TILEMESSAGE_H
#define TILEMESSAGE_H

#include <memory>
#include <vector>

namespace vistle {
struct tileMsg;

namespace message {
class RemoteRenderMessage;
}
}

struct TileMessage {
    TileMessage(std::shared_ptr<vistle::message::RemoteRenderMessage> msg, std::shared_ptr<std::vector<char>> payload);

    std::shared_ptr<vistle::message::RemoteRenderMessage> msg;
    const vistle::tileMsg &tile;
    std::shared_ptr<std::vector<char>> payload;
};

#endif
