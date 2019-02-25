#include "DecodeTask.h"
#include <vector>
#include <mutex>

#include <rhr/rfbext.h>
#include <core/messages.h>
#include <PluginUtil/MultiChannelDrawer.h>

#include <osg/Geometry>


#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>

namespace {

struct TjDecomp {

    TjDecomp()
        : handle(tjInitDecompress())
    {}

    ~TjDecomp() {
        tjDestroy(handle);
    }

   tjhandle handle;
};

std::mutex tjMutex;
std::vector<std::shared_ptr<TjDecomp>> tjContexts;
}
#endif


using namespace opencover;
using namespace vistle;
using vistle::message::RemoteRenderMessage;


#define CERR std::cerr << "RhrClient:Decode: "


DecodeTask::DecodeTask(std::shared_ptr<const RemoteRenderMessage> msg, std::shared_ptr<std::vector<char>> payload)
: msg(msg)
, payload(payload)
, rgba(NULL)
, depth(NULL)
{}

bool DecodeTask::work() {
    //CERR << "DecodeTask::execute" << std::endl;
    assert(msg->rhr().type == rfbTile);

    if (!depth && !rgba) {
        // dummy task for other node
        //CERR << "DecodeTask: no FB" << std::endl;
        return true;
    }

    const auto &tile = static_cast<const tileMsg &>(msg->rhr());

    size_t sz = tile.unzippedsize;
    int bpp = 0;
    if (tile.format == rfbColorRGBA) {
        assert(rgba);
        bpp = 4;
    } else {
        assert(depth);
        switch (tile.format) {
        case rfbDepthFloat: bpp=4; break;
        case rfbDepth8Bit: bpp=1; break;
        case rfbDepth16Bit: bpp=2; break;
        case rfbDepth24Bit: bpp=4; break;
        case rfbDepth32Bit: bpp=4; break;
        }
    }

    if (!payload || sz==0) {
        CERR << "DecodeTask: no data" << std::endl;
        return false;
    }

    auto decompbuf = message::decompressPayload(*msg, *payload);

    if (tile.format!=rfbColorRGBA && (tile.compression & rfbTileDepthZfp)) {

#ifndef HAVE_ZFP
        CERR << "DecodeTask: not compiled with zfp support, cannot decompress" << std::endl;
        return false;
#else
        if (tile.format==rfbDepthFloat) {
            zfp_type type = zfp_type_float;
            bitstream *stream = stream_open(decompbuf.data(), decompbuf.size());
            zfp_stream *zfp = zfp_stream_open(stream);
            zfp_stream_rewind(zfp);
            zfp_field *field = zfp_field_2d(depth, type, tile.width, tile.height);
            if (!zfp_read_header(zfp, field, ZFP_HEADER_FULL)) {
                CERR << "DecodeTask: reading zfp compression parameters failed" << std::endl;
            }
            if (field->type != type) {
                CERR << "DecodeTask: zfp type not float" << std::endl;
            }
            if (field->nx != tile.width || field->ny != tile.height) {
                CERR << "DecodeTask: zfp size mismatch: " << field->nx << "x" << field->ny << " != " << tile.width << "x" << tile.height << std::endl;
            }
            zfp_field_set_pointer(field, depth+(tile.y*tile.totalwidth+tile.x)*bpp);
            zfp_field_set_stride_2d(field, 1, tile.totalwidth);
            if (!zfp_decompress(zfp, field)) {
                CERR << "DecodeTask: zfp decompression failed" << std::endl;
            }
            zfp_stream_close(zfp);
            zfp_field_free(field);
            stream_close(stream);
        } else {
            CERR << "DecodeTask: zfp not in float format, cannot decompress" << std::endl;
            return false;
        }
#endif
    } else  if (tile.format!=rfbColorRGBA && (tile.compression & rfbTileDepthQuantize)) {

        depthdequant(depth, decompbuf.data(), bpp==4 ? DepthFloat : DepthInteger, bpp, tile.x, tile.y, tile.width, tile.height, tile.totalwidth);
    } else if (tile.compression == rfbTileJpeg) {

#ifdef HAVE_TURBOJPEG
        std::shared_ptr<TjDecomp> tjc;
        {
            std::lock_guard<std::mutex> locker(tjMutex);
            if (tjContexts.empty()) {
                tjc = std::make_shared<TjDecomp>();
            } else {
                tjc = tjContexts.back();
                tjContexts.pop_back();
            }
        }
        char *dest = tile.format==rfbColorRGBA ? rgba : depth;
        int w=-1, h=-1;
        tjDecompressHeader(tjc->handle, reinterpret_cast<unsigned char *>(decompbuf.data()), tile.size, &w, &h);
        dest += (tile.y*tile.totalwidth+tile.x)*bpp;
        int ret = tjDecompress(tjc->handle, reinterpret_cast<unsigned char *>(decompbuf.data()), tile.size, reinterpret_cast<unsigned char *>(dest), tile.width, tile.totalwidth*bpp, tile.height, bpp, TJPF_BGR);
        if (ret == -1) {
            CERR << "DecodeTask: JPEG error: " << tjGetErrorStr() << std::endl;
            return false;
        }
#else
        CERR << "DecodeTask: no JPEG support" << std::endl;
        return false;
#endif
        std::lock_guard<std::mutex> locker(tjMutex);
        tjContexts.push_back(tjc);
    } else {

        char *dest = tile.format==rfbColorRGBA ? rgba : depth;
        for (int yy=0; yy<tile.height; ++yy) {
            memcpy(dest+((tile.y+yy)*tile.totalwidth+tile.x)*bpp, decompbuf.data()+tile.width*yy*bpp, tile.width*bpp);
        }
    }

    //CERR << "DecodeTask: finished" << std::endl;
    return true;
}
