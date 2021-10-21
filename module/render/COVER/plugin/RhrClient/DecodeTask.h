#ifndef RHR_DECODETASK_H
#define RHR_DECODETASK_H

#include <future>
#include <memory>
#include <vector>

#include <vistle/util/buffer.h>

#include <PluginUtil/MultiChannelDrawer.h>

namespace vistle {
namespace message {
class RemoteRenderMessage;
}
} // namespace vistle


//! Task structure for submitting to Intel Threading Building Blocks work //queue
struct DecodeTask {
    DecodeTask(std::shared_ptr<const vistle::message::RemoteRenderMessage> msg,
               std::shared_ptr<vistle::buffer> payload);
    bool work();

    std::future<bool> result;
    std::shared_ptr<const vistle::message::RemoteRenderMessage> msg;
    std::shared_ptr<vistle::buffer> payload;
    std::shared_ptr<opencover::MultiChannelDrawer::ViewData> viewData;
    char *rgba = nullptr, *depth = nullptr;
};
#endif
