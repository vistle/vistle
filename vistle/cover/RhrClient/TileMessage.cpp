#include "TileMessage.h"

#include <cassert>
#include <core/message.h>
#include <rhr/rfbext.h>

using namespace vistle;

TileMessage::TileMessage(std::shared_ptr<message::RemoteRenderMessage> msg, std::shared_ptr<buffer> payload)
    : msg(msg), tile(static_cast<const tileMsg &>(msg->rhr())), payload(payload) {

    assert(msg->rhr().type == rfbTile);
    assert(msg->payloadSize()==0 || (payload && payload->size()==msg->payloadSize()));
}
