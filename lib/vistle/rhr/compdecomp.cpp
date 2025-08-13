#include "compdecomp.h"
#include "depthquant.h"
#include "predict.h"
#include <iostream>
#include <mutex>
#include <memory>

#include <zfp.h>

#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>


namespace vistle {

struct TjComp {
    TjComp(): handle(tjInitCompress()) {}

    ~TjComp() { tjDestroy(handle); }

    tjhandle handle;
};

struct TjDecomp {
    TjDecomp(): handle(tjInitDecompress()) {}

    ~TjDecomp() { tjDestroy(handle); }

    tjhandle handle;
};

std::mutex tjMutex;
std::vector<std::shared_ptr<TjComp>> tjCompContexts;
std::vector<std::shared_ptr<TjDecomp>> tjDecompContexts;
} // namespace vistle
#endif

#define CERR std::cerr << "CompDecomp: "


namespace vistle {

buffer copyTile(const char *img, int x, int y, int w, int h, int stride, int bpp)
{
    size_t size = w * h * bpp;
    buffer tilebuf(size);
    for (int yy = 0; yy < h; ++yy) {
        memcpy(tilebuf.data() + yy * bpp * w, img + ((yy + y) * stride + x) * bpp, w * bpp);
    }
    return tilebuf;
}

buffer compressDepth(const float *depth, int x, int y, int w, int h, int stride,
                     vistle::DepthCompressionParameters &param)
{
    const char *zbuf = reinterpret_cast<const char *>(depth);
    switch (param.depthCodec) {
    case vistle::CompressionParameters::DepthZfp: {
        buffer result;
        zfp_type type = zfp_type_float;
        zfp_field *field = zfp_field_2d(const_cast<float *>(depth + y * stride + x), type, w, h);
        zfp_field_set_stride_2d(field, 0, stride);

        zfp_stream *zfp = zfp_stream_open(nullptr);
        switch (param.depthZfpMode) {
        default:
            CERR << "invalid ZfpMode " << param.depthZfpMode << std::endl;
            // FALLTHRU
        case CompressionParameters::ZfpFixedRate:
            zfp_stream_set_rate(zfp, 6, type, 2, 0);
            break;
        case CompressionParameters::ZfpPrecision:
            zfp_stream_set_precision(zfp, 16);
            break;
        case CompressionParameters::ZfpAccuracy:
            zfp_stream_set_accuracy(zfp, 1. / 1024.);
            break;
        }
        size_t bufsize = zfp_stream_maximum_size(zfp, field);
        buffer zfpbuf(bufsize);
        bitstream *stream = stream_open(zfpbuf.data(), bufsize);
        zfp_stream_set_bit_stream(zfp, stream);

        zfp_stream_rewind(zfp);
        zfp_write_header(zfp, field, ZFP_HEADER_FULL);
        size_t zfpsize = zfp_compress(zfp, field);
        if (zfpsize == 0) {
            CERR << "zfp compression failed" << std::endl;
            param.depthCodec = vistle::CompressionParameters::DepthRaw;
        } else {
            zfpbuf.resize(zfpsize);
            result = std::move(zfpbuf);
        }
        zfp_field_free(field);
        zfp_stream_close(zfp);
        stream_close(stream);
        return result;
    }
    case vistle::CompressionParameters::DepthQuant: {
        const int ds = 3; //msg.format == rfbDepth16Bit ? 2 : 3;
        size_t size = depthquant_size(DepthFloat, ds, w, h);
        buffer qbuf(size);
        depthquant(qbuf.data(), zbuf, DepthFloat, ds, x, y, w, h, stride);
#ifdef QUANT_ERROR
        buffer dequant(sizeof(float) * w * h);
        depthdequant(dequant.data(), qbuf, DepthFloat, ds, 0, 0, w, h);
        //depthquant(qbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride); // test depthcompare
        depthcompare(zbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride);
#endif
        return qbuf;
    }
    case vistle::CompressionParameters::DepthQuantPlanar: {
        const int ds = 3; //msg.format == rfbDepth16Bit ? 2 : 3;
        size_t size = depthquant_size(DepthFloat, ds, w, h);
        buffer qbuf(size);
        depthquant_planar(qbuf.data(), zbuf, DepthFloat, ds, x, y, w, h, stride);
#ifdef QUANT_ERROR
        buffer dequant(sizeof(float) * w * h);
        depthdequant_planar(dequant.data(), qbuf, DepthFloat, ds, 0, 0, w, h);
        //depthquant_planar(qbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride); // test depthcompare
        depthcompare(zbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride);
#endif
        return qbuf;
    }
    case vistle::CompressionParameters::DepthPredict: {
        size_t size = w * h * 3;
        buffer pbuf(size);
        transform_predict((unsigned char *)pbuf.data(), depth + stride * y + x, w, h, stride);
        return pbuf;
    }
    case vistle::CompressionParameters::DepthPredictPlanar: {
        size_t size = w * h * 3;
        buffer pbuf(size);
        transform_predict_planar((unsigned char *)pbuf.data(), depth + stride * y + x, w, h, stride);
        return pbuf;
    }
    case vistle::CompressionParameters::DepthRaw: {
        break;
    }
    }

    param.depthCodec = vistle::CompressionParameters::DepthRaw;

    return copyTile(reinterpret_cast<const char *>(depth), x, y, w, h, stride, sizeof(float));
}


buffer compressRgba(const unsigned char *rgba, int x, int y, int w, int h, int stride,
                    vistle::RgbaCompressionParameters &param)
{
    buffer result;

    const int bpp = 4;
    if (param.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV411 ||
        param.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV444) {
#ifdef HAVE_TURBOJPEG
        TJSAMP sampling = param.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV411 ? TJSAMP_420 : TJSAMP_444;
        std::shared_ptr<TjComp> tjc;
        std::unique_lock<std::mutex> locker(tjMutex);
        if (tjCompContexts.empty()) {
            tjc = std::make_shared<TjComp>();
        } else {
            tjc = tjCompContexts.back();
            tjCompContexts.pop_back();
        }
        locker.unlock();
        size_t maxsize = tjBufSize(w, h, sampling);
        buffer jpegbuf(maxsize);
        unsigned long sz = 0;
        auto col = rgba + (stride * y + x) * bpp;
#ifdef TIMING
        double start = vistle::Clock::time();
#endif
        int ret = tjCompress(tjc->handle, const_cast<unsigned char *>(col), w, stride * bpp, h, bpp,
                             reinterpret_cast<unsigned char *>(jpegbuf.data()), &sz,
                             param.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV411, 90, TJ_BGR);
        jpegbuf.resize(sz);
        locker.lock();
        tjCompContexts.push_back(tjc);
#ifdef TIMING
        double dur = vistle::Clock::time() - start;
        std::cerr << "JPEG compression: " << dur << "s, " << msg.width * (msg.height / dur) / 1e6 << " MPix/s"
                  << std::endl;
#endif
        if (ret >= 0) {
            result = std::move(jpegbuf);
            return result;
        }
#endif
        param.rgbaCodec = vistle::CompressionParameters::PredictRGB;
    }
    if (param.rgbaCodec == vistle::CompressionParameters::PredictRGB) {
        result.resize(w * h * 3);
        transform_predict<3, true, true>(reinterpret_cast<unsigned char *>(result.data()),
                                         rgba + (y * stride + x) * bpp, w, h, stride);
        return result;
    }
    if (param.rgbaCodec == vistle::CompressionParameters::PredictRGBA) {
        result.resize(w * h * 4);
        transform_predict<4, true, true>(reinterpret_cast<unsigned char *>(result.data()),
                                         rgba + (y * stride + x) * bpp, w, h, stride);
        return result;
    }

    return copyTile(reinterpret_cast<const char *>(rgba), x, y, w, h, stride, bpp);
}

buffer compressTile(const char *input, int x, int y, int w, int h, int stride, vistle::CompressionParameters &param)
{
    if (param.isDepth) {
        return compressDepth(reinterpret_cast<const float *>(input), x, y, w, h, stride, param.depth);
    }
    return compressRgba(reinterpret_cast<const unsigned char *>(input), x, y, w, h, stride, param.rgba);
}

bool decompressTile(char *dest, const buffer &input, CompressionParameters param, int x, int y, int w, int h,
                    int stride)
{
    int bpp = 4;
    if (param.isDepth) {
        auto &p = param.depth;

        if (p.depthCodec == vistle::CompressionParameters::DepthZfp) {
            if (!p.depthFloat) {
                CERR << "zfp not in float format, cannot decompress" << std::endl;
                return false;
            }
            bpp = 4;

            auto &decompbuf = input;
            auto depth = reinterpret_cast<float *>(dest);
            bool good = true;

            zfp_type type = zfp_type_float;
            bitstream *stream = stream_open(const_cast<char *>(decompbuf.data()), decompbuf.size());
            zfp_stream *zfp = zfp_stream_open(stream);
            zfp_stream_rewind(zfp);
            zfp_field *field = zfp_field_2d(depth, type, w, h);
            if (!zfp_read_header(zfp, field, ZFP_HEADER_FULL)) {
                CERR << "docompressTile: reading zfp compression parameters failed" << std::endl;
                good = false;
            }
            if (field->type != type) {
                CERR << "docompressTile: zfp type not float" << std::endl;
                good = false;
            }
            if (int(field->nx) != w || int(field->ny) != h) {
                CERR << "docompressTile: zfp size mismatch: " << field->nx << "x" << field->ny << " != " << w << "x"
                     << h << std::endl;
                good = false;
            }
            zfp_field_set_pointer(field, depth + (y * stride + x));
            zfp_field_set_stride_2d(field, 1, stride);
            if (!zfp_decompress(zfp, field)) {
                CERR << "docompressTile: zfp decompression failed" << std::endl;
                good = false;
            }
            zfp_stream_close(zfp);
            zfp_field_free(field);
            stream_close(stream);

            return good;
        }

        if (p.depthCodec == vistle::CompressionParameters::DepthQuant) {
            depthdequant(dest, input.data(), bpp == 4 ? DepthFloat : DepthInteger, bpp, x, y, w, h, stride);
            return true;
        }

        if (p.depthCodec == vistle::CompressionParameters::DepthQuantPlanar) {
            depthdequant_planar(dest, input.data(), bpp == 4 ? DepthFloat : DepthInteger, bpp, x, y, w, h, stride);
            return true;
        }

        if (p.depthCodec == vistle::CompressionParameters::DepthPredict) {
            transform_unpredict(reinterpret_cast<float *>(dest) + y * stride + x, (unsigned char *)input.data(), w, h,
                                stride);
            return true;
        }

        if (p.depthCodec == vistle::CompressionParameters::DepthPredictPlanar) {
            transform_unpredict_planar(reinterpret_cast<float *>(dest) + y * stride + x, (unsigned char *)input.data(),
                                       w, h, stride);
            return true;
        }

    } else {
        auto &p = param.rgba;

        if (p.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV411 ||
            p.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV444) {
#ifdef HAVE_TURBOJPEG
            std::shared_ptr<TjDecomp> tjd;
            {
                std::unique_lock<std::mutex> locker(tjMutex);
                if (tjDecompContexts.empty()) {
                    tjd = std::make_shared<TjDecomp>();
                } else {
                    tjd = tjDecompContexts.back();
                    tjDecompContexts.pop_back();
                }
            }
            int ww = -1, hh = -1;
            const unsigned char *rgba = reinterpret_cast<const unsigned char *>(input.data());
            tjDecompressHeader(tjd->handle, const_cast<unsigned char *>(rgba), input.size(), &ww, &hh);
            dest += (y * stride + x) * bpp;
            int ret = tjDecompress(tjd->handle, const_cast<unsigned char *>(rgba), input.size(),
                                   reinterpret_cast<unsigned char *>(dest), w, stride * bpp, h, bpp, TJPF_BGR);
            std::unique_lock<std::mutex> locker(tjMutex);
            tjDecompContexts.push_back(tjd);
            if (ret == -1) {
                CERR << "docompressTile: JPEG error: " << tjGetErrorStr() << std::endl;
                return false;
            }
            return true;
#else
            CERR << "docompressTile: no JPEG support" << std::endl;
            return false;
#endif
        }
        if (p.rgbaCodec == vistle::CompressionParameters::PredictRGB) {
            assert(bpp == 4);
            transform_unpredict<3, true, true>(reinterpret_cast<unsigned char *>(dest) + (y * stride + x) * bpp,
                                               (unsigned char *)input.data(), w, h, stride);
            return true;
        }
        if (p.rgbaCodec == vistle::CompressionParameters::PredictRGBA) {
            assert(bpp == 4);
            transform_unpredict<4, true, true>(reinterpret_cast<unsigned char *>(dest) + (y * stride + x) * bpp,
                                               (unsigned char *)input.data(), w, h, stride);
            return true;
        }
    }

    for (int yy = 0; yy < h; ++yy) {
        memcpy(dest + ((y + yy) * stride + x) * bpp, input.data() + w * yy * bpp, w * bpp);
    }

    return true;
}

} // namespace vistle
