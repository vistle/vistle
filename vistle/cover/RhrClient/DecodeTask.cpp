#include "DecodeTask.h"
#include <vector>
#include <mutex>

#include <vistle/rhr/rfbext.h>
#include <vistle/rhr/compdecomp.h>
#include <vistle/core/messages.h>
#include <PluginUtil/MultiChannelDrawer.h>

#include <osg/Geometry>


using namespace opencover;
using namespace vistle;
using vistle::message::RemoteRenderMessage;


#define CERR std::cerr << "RhrClient:Decode: "


DecodeTask::DecodeTask(std::shared_ptr<const RemoteRenderMessage> msg, std::shared_ptr<buffer> payload)
: msg(msg), payload(payload), rgba(NULL), depth(NULL)
{}

bool DecodeTask::work()
{
    //CERR << "DecodeTask::execute" << std::endl;
    assert(msg->rhr().type == rfbTile);

    if (!depth && !rgba) {
        // dummy task for other node
        //CERR << "DecodeTask: no FB" << std::endl;
        return true;
    }

    const auto &tile = static_cast<const tileMsg &>(msg->rhr());

    if (tile.unzippedsize == 0) {
        CERR << "DecodeTask: no data, unzippedsize=" << tile.unzippedsize << std::endl;
        return false;
    }
    if (!payload) {
        CERR << "DecodeTask: no data, payload is NULL" << std::endl;
        return false;
    }

    if (tile.format == rfbColorRGBA) {
        assert(rgba);
    } else {
        assert(depth);
    }

    vistle::CompressionParameters param;
    auto &cp = param.rgba;
    auto &dp = param.depth;
    if (tile.compression & rfbTileDepthZfp) {
        dp.depthCodec = vistle::CompressionParameters::DepthZfp;
    }
    if (tile.compression & rfbTileDepthQuantize) {
        dp.depthCodec = vistle::CompressionParameters::DepthQuant;
    }
    if (tile.compression & rfbTileDepthQuantizePlanar) {
        dp.depthCodec = vistle::CompressionParameters::DepthQuantPlanar;
    }
    if (tile.compression & rfbTileDepthPredict) {
        dp.depthCodec = vistle::CompressionParameters::DepthPredict;
    }
    if (tile.compression & rfbTileDepthPredictPlanar) {
        dp.depthCodec = vistle::CompressionParameters::DepthPredictPlanar;
    }
    if (tile.format == rfbDepthFloat) {
        dp.depthFloat = true;
    }
    if (tile.compression == rfbTileJpeg) {
        cp.rgbaCodec = vistle::CompressionParameters::Jpeg_YUV444;
    } else if (tile.compression == rfbTilePredictRGB) {
        cp.rgbaCodec = vistle::CompressionParameters::PredictRGB;
    } else if (tile.compression == rfbTilePredictRGBA) {
        cp.rgbaCodec = vistle::CompressionParameters::PredictRGBA;
    } else {
        cp.rgbaCodec = vistle::CompressionParameters::Raw;
    }
    param.isDepth = tile.format != rfbColorRGBA;

    try {
        auto decompbuf = message::decompressPayload(*msg, *payload);
        if (ssize_t(decompbuf.size()) != tile.unzippedsize) {
            CERR << "DecodeTask: invalid data: unzipped size wrong: " << decompbuf.size() << " != " << tile.unzippedsize
                 << std::endl;
        }
        return decompressTile(param.isDepth ? depth : rgba, decompbuf, param, tile.x, tile.y, tile.width, tile.height,
                              tile.totalwidth);
    } catch (vistle::message::codec_error &ex) {
        CERR << "DecodeTask: codec error: " << ex.what() << ", info: " << ex.info() << std::endl;
        return false;
    }

    return false;
}
