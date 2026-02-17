/**\file
 * \brief class ReadBackCuda
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author Martin Aumüller <aumueller@uni-koeln.de>
 * \author (c) 2011 RRZK, 2013 HLRS
 *
 * \copyright GPL2+
 */

#include <GL/glew.h>
#include "ReadBackCuda.h"

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>

#include <cstdio>
#include <cassert>

#include <vistle/rhr/depthquant.h>
#include <vistle/rhr/rfbext.h>

using namespace vistle;

//! use BGRA for pixel transfer
#define USE_BGRA

static const int TileSizeX = 16;
static const int TileSizeY = 16;

bool ReadBackCuda::s_error = false;

//! check for CUDA errors
bool cucheck(const char *msg, cudaError_t err)
{
#if 0
   if(err == cudaSuccess)
   {
      cudaThreadSynchronize();
      err = cudaGetLastError();
   }
#endif

    if (err == cudaSuccess)
        return true;

    fprintf(stderr, "%s: %s\n", msg, cudaGetErrorString(err));

    ReadBackCuda::s_error = true;

    return false;
}

//! ctor
ReadBackCuda::ReadBackCuda(): pboName(0), imgRes(NULL), outImg(NULL), imgSize(0), compSize(0), m_initialized(false)
{}

//! GL initialization, requires valid GL context
bool ReadBackCuda::init()
{
    if (s_error)
        return false;

    assert(m_initialized == false);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
        return false;
    }

    bool canUsePbo = glGenBuffers && glDeleteBuffers && glBufferData && glBindBuffer;
    if (!canUsePbo) {
        fprintf(stderr, "PBOs not available\n");
        return false;
    }

    m_initialized = true;

    return true;
}

//! query initialization state
bool ReadBackCuda::isInitialized() const
{
    return m_initialized && !s_error;
}

//! dtor
ReadBackCuda::~ReadBackCuda()
{
    if (imgRes)
        cucheck("unreg buf", cudaGraphicsUnregisterResource(imgRes));
    cucheck("cufree out", cudaFree(outImg));
    if (pboName != 0)
        glDeleteBuffers(1, &pboName);
}

//! set up PBO for image size to be read back
bool ReadBackCuda::initPbo(size_t raw, size_t compressed)
{
    if (s_error)
        return false;

    if (imgSize != raw || compSize != compressed || pboName == 0) {
        if (pboName == 0) {
            glGenBuffers(1, &pboName);
        } else {
            cucheck("unreg buf", cudaGraphicsUnregisterResource(imgRes));
            cucheck("cufree out", cudaFree(outImg));
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName);
        glBufferData(GL_PIXEL_PACK_BUFFER, raw, NULL, GL_STREAM_COPY);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        imgSize = raw;

        cucheck("reg buf", cudaGraphicsGLRegisterBuffer(&imgRes, pboName, cudaGraphicsMapFlagsNone));
        cucheck("malloc out", cudaMalloc(&outImg, compressed));
    }

    return pboName != 0;
}

//! convert single color from RGB to YUV
__device__ void colorconvert(uchar r, uchar g, uchar b, uchar *y, uchar *u, uchar *v)
{
    *y = 0.299f * r + 0.587f * g + 0.114f * b;
    *u = 128.f + 0.5f * r - 0.418688f * g - 0.081312f * b;
    *v = 128.f - 0.168736f * r - 0.331265f * g + 0.5f * b;
}

//! convert single color from RGB to YUV
__device__ void colorconvert(uchar r, uchar g, uchar b, uchar *y, float *u, float *v)
{
    *y = 0.299f * r + 0.587f * g + 0.114f * b;
    *u += 128.f + 0.5f * r - 0.418688f * g - 0.081312f * b;
    *v += 128.f - 0.168736f * r - 0.331265f * g + 0.5f * b;
}

//! convert buffer from RGB to YUV
__global__ void rgb2yuv(const uchar *inimg, uchar *outimg, int w, int h, int subx, int suby)
{
    if (subx < 1)
        subx = 1;
    if (suby < 1)
        suby = 1;
#ifdef USE_BGRA
    const int bpp = 4;
#else
    const int bpp = 3;
#endif

    const int wsub = (w + subx - 1) / subx;
    const int hsub = (h + suby - 1) / suby;

    uchar *ybase = outimg;
    uchar *ubase = &ybase[w * h];

    int x = threadIdx.x + blockIdx.x * TileSizeX;
    int y = threadIdx.y + blockIdx.y * TileSizeY;

    uchar *vbase = &ubase[wsub * hsub];

    float u = 0.f, v = 0.f;

    uchar *uu = &ubase[x + y * wsub];
    uchar *vv = &vbase[x + y * wsub];

    x *= subx;
    y *= suby;

    if (x >= w || y >= h)
        return;

    for (int iy = 0; iy < suby; ++iy) {
        const int yy = y + iy;
        for (int ix = 0; ix < subx; ++ix) {
            const int xx = x + ix;
            uchar *cy = &ybase[xx + yy * w];
            const uchar r = inimg[(xx + yy * w) * bpp + 0];
            const uchar g = inimg[(xx + yy * w) * bpp + 1];
            const uchar b = inimg[(xx + yy * w) * bpp + 2];

            colorconvert(r, g, b, cy, &u, &v);
        }
    }
    *uu = u / (subx * suby);
    *vv = v / (subx * suby);
}

template<int nelem>
__device__ void findgap(uint32_t *__restrict__ data, uint32_t *__restrict__ order, uint32_t maxidx, uint32_t midval,
                        uint32_t &__restrict__ mididx)
{
    if (threadIdx.x == nelem - 1) {
        mididx = maxidx / 2;
    }
    if (threadIdx.x + 1 < maxidx) {
        if (data[order[threadIdx.x]] <= midval && data[order[threadIdx.x + 1]] > midval)
            mididx = threadIdx.x;
    }
}

template<int nelem>
__device__ void sort(uint32_t *__restrict__ data, uint32_t *__restrict__ ranks, uint32_t *__restrict__ order,
                     uint32_t Far, uint32_t &__restrict__ maxidx)
{
    const int halfwarp = 16;
#define SYNC() \
    if (nelem > halfwarp) \
    __syncthreads()

    int rank = 0;
    for (int i = 0; i < nelem; ++i) {
        rank += data[i] < data[threadIdx.x];
    }
    ranks[threadIdx.x] = rank;
    SYNC();

    if (nelem > halfwarp) {
        int add = 0;
        for (int i = 0; i < nelem - 1; ++i) {
            if (threadIdx.x > i)
                add += ranks[threadIdx.x] == ranks[i];
        }
        SYNC();
        ranks[threadIdx.x] += add;
    } else {
        for (int i = 0; i < nelem - 1; ++i) {
            if (threadIdx.x > i)
                ranks[threadIdx.x] += ranks[threadIdx.x] == ranks[i];
        }
    }

    order[ranks[threadIdx.x]] = threadIdx.x;
    SYNC();

    if (threadIdx.x == 0)
        maxidx = nelem - 1;
    SYNC();
    if (threadIdx.x > 0) {
        if (data[order[threadIdx.x]] == Far && data[order[threadIdx.x - 1]] < Far)
            maxidx = threadIdx.x - 1;
    }
    SYNC();

#undef SYNC
}

//! dxt-like compresesion for depth values
template<int bpp, class Quant, bool RGBA>
__global__ void depthquant(const uchar *inimg, uchar *outimg, int w, int h)
{
    const int edge = Quant::edge;
    const int scalebits = Quant::scale_bits;
    const int quantbits = Quant::bits_per_pixel;
    const uint32_t Far = bpp == 4 ? 0x00ffffffU : (1U << (bpp * 8)) - 1;
    const uint32_t mask = (1U << quantbits) - 1;

    int x = (threadIdx.x % edge) + (threadIdx.y % (TileSizeX / edge)) * edge + blockIdx.x * TileSizeX;
    if (x >= w)
        x = w - 1;
    int y = (threadIdx.x / edge) + (threadIdx.y / (TileSizeY / edge)) * edge + blockIdx.y * TileSizeY;
    if (y >= h)
        y = h - 1;

    extern __shared__ uint32_t shared[];
    const int tileoff = blockDim.x * threadIdx.y;
    uint32_t *depths = &shared[tileoff + 0];
    uint32_t *ranks = &depths[blockDim.x * blockDim.y];
    uint32_t *orders = &ranks[blockDim.x * blockDim.y];
    uint32_t *maxidx = &shared[blockDim.x * blockDim.y * 3 + threadIdx.y];

    const uint32_t depth = get_depth < RGBA ? DepthRGBA : DepthInteger, bpp > (inimg, x, y, w, h);

    orders[threadIdx.x] = 0;
    ranks[threadIdx.x] = 0;
    depths[threadIdx.x] = depth;
    sort<edge * edge>(depths, ranks, orders, Far, *maxidx);
    uint32_t mindepth = depths[orders[0]];
    uint32_t maxdepth = depths[orders[*maxidx]];
    const bool haveFar = depths[orders[edge * edge - 1]] == Far;
    const uint32_t range = maxdepth - mindepth;
    const float qscale = haveFar ? mask - 0.5f : mask + 0.5f;

    uint32_t quant = 0;
    if (scalebits == 0) {
        if (depth == Far) {
            quant = mask;
        } else if (range > 0) {
            quant = ((depth - mindepth) * qscale) / range;
        }
    } else {
        uint32_t *mididx = &maxidx[blockDim.y];
        const uint32_t scalemask = (1U << scalebits) - 1U;
        const uint32_t depthmask = ~scalemask;
        mindepth &= depthmask;
        maxdepth |= scalemask;
        const uint32_t range = maxdepth - mindepth;
        const uint32_t midval = (mindepth + maxdepth) >> 1;
        findgap<edge * edge>(depths, orders, *maxidx, midval, *mididx);
        const uint32_t lowermid = depths[orders[*mididx]];
        const uint32_t uppermid = depths[orders[(*mididx) + 1]];
        int lowerscale = 1, upperscale = 1;
        if (range > 1) {
            if (lowermid > mindepth)
                lowerscale = range / (lowermid - mindepth) / 2;
            if (maxdepth > uppermid)
                upperscale = range / (maxdepth - uppermid) / 2;
            if (lowerscale == 0)
                lowerscale = 1;
            if (upperscale == 0)
                upperscale = 1;
            if (lowerscale > 1 << scalebits)
                lowerscale = 1 << scalebits;
            if (upperscale > 1 << scalebits)
                upperscale = 1 << scalebits;
        }

        quant = 0;
        if (depth == Far) {
            quant = mask;
        } else if (range > 0) {
            if (depth <= midval) {
                quant = ((depth - mindepth) * (lowerscale * mask + 0.5f)) / range;
            } else if (haveFar) {
                quant = ((maxdepth - depth) * (upperscale * (mask - 1.5f))) / range;
                quant = mask - 1 - quant;
            } else {
                quant = ((maxdepth - depth) * (upperscale * mask + 0.5f)) / range;
                quant = mask - quant;
            }
        }

        --lowerscale;
        --upperscale;

        mindepth &= depthmask;
        mindepth |= lowerscale;
        maxdepth &= depthmask;
        maxdepth |= upperscale;
    }
    depths[threadIdx.x] = quant;

    Quant &sq = ((Quant *)outimg)[(h - 1 - y) / edge * w / edge + x / edge];
    if (threadIdx.x == 0) {
        if (haveFar) {
            setdepth(sq, 0, maxdepth);
            setdepth(sq, 1, mindepth);
        } else {
            setdepth(sq, 0, mindepth);
            setdepth(sq, 1, maxdepth);
        }

        dq_clearbits(sq);
        for (int iy = 0; iy < edge; ++iy) {
            for (int ix = 0; ix < edge; ++ix) {
                int i = iy * edge + ix;
                dq_setbits(sq, i * quantbits, quantbits, depths[(edge - 1 - iy) * edge + ix]);
            }
        }
    }
}


//! retrieve image in YUV format, chrominance planes are optionally sub-sampled
bool ReadBackCuda::readpixelsyuv(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                                 GLint buf, int subx, int suby)
{
    size_t subsize = w * h + 2 * ((w + subx - 1) / subx) * ((h + suby - 1) / suby);
    if (!initPbo(pitch * h * 4, subsize))
        return false;

    uchar *img = NULL;
#ifdef USE_BGRA
    if (!pbo2cuda(x, y, w, pitch, h, GL_BGRA, ps, buf, GL_UNSIGNED_INT_8_8_8_8_REV, &img))
        return false;
#else
    if (!pbo2cuda(x, y, w, pitch, h, GL_BGR, ps, buf, GL_UNSIGNED_BYTE, &img))
        return false;
#endif

    dim3 grid(((w + TileSizeX - 1) / TileSizeX + subx - 1) / subx, ((h + TileSizeY - 1) / TileSizeY + suby - 1) / suby);
    dim3 block(TileSizeX, TileSizeY);

    //fprintf(stderr, "rgb2yuv: x=%d, y=%d, w=%d, h=%d, sub=(%d %d)\n", x, y, w, h, subx, suby);

    rgb2yuv<<<grid, block>>>(img, outImg, w, h, subx, suby);

    cucheck("memcpy", cudaMemcpy(bits, outImg, subsize, cudaMemcpyDeviceToHost));
    cucheck("unmap res", cudaGraphicsUnmapResources(1, &imgRes, NULL));

    //_prof_rb.endframe(w*h, 0, stereo? 0.5 : 1);
    //checkgl("Read Pixels");

    return true;
}

//! RGB read-back
bool ReadBackCuda::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                              GLint buf, GLenum type)
{
    //fprintf(stderr, "readpixels: w=%d, pitch=%d\n", w, pitch);
    if (!initPbo(pitch * h * ps, 0))
        return false;

    uchar *img = NULL;
    if (!pbo2cuda(x, y, w, pitch, h, format, ps, buf, type, &img))
        return false;

    if (pitch != w) {
        cucheck("memcpy2d", cudaMemcpy2D(bits, pitch * ps, img, pitch * ps, w * ps, h, cudaMemcpyDeviceToHost));
    } else {
        cucheck("memcpy", cudaMemcpy(bits, img, pitch * h * ps, cudaMemcpyDeviceToHost));
    }
    cucheck("unmap res", cudaGraphicsUnmapResources(1, &imgRes, NULL));

    //_prof_rb.endframe(w*h, 0, stereo? 0.5 : 1);
    //checkgl("Read Pixels");

    return true;
}

bool ReadBackCuda::pbo2cuda(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLint buf,
                            GLenum type, uchar **img)
{
    if (s_error)
        return false;

    GLint readbuf = GL_BACK;
    glGetIntegerv(GL_READ_BUFFER, &readbuf);

    glReadBuffer(buf);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    if (pitch % 8 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 8);
    else if (pitch % 4 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    else if (pitch % 2 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 2);
    else if (pitch % 1 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glPixelStorei(GL_PACK_ROW_LENGTH, pitch);

    int e = glGetError();
    while (e != GL_NO_ERROR)
        e = glGetError(); // Clear previous error
    //_prof_rb.startframe();

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName);
    glBufferData(GL_PIXEL_PACK_BUFFER, pitch * h * ps, NULL, GL_STREAM_COPY);
    glReadPixels(x, y, w, h, format, type, NULL);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    glPopClientAttrib();
    glReadBuffer(readbuf);

    if (!cucheck("map res", cudaGraphicsMapResources(1, &imgRes, NULL)))
        return false;

    size_t sz;
    return cucheck("get map ptr", cudaGraphicsResourceGetMappedPointer((void **)img, &sz, imgRes));
}

//! compressed depth read-back
bool ReadBackCuda::readdepthquant(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                                  GLint buf, GLenum type)
{
    size_t compressed = 0;
    int edge = 1;
    if (ps <= 2) {
        edge = DepthQuantize16::edge;
        compressed = (w + edge - 1) / edge * (h + edge - 1) / edge;
        compressed *= sizeof(DepthQuantize16);
    } else {
        edge = DepthQuantize24::edge;
        compressed = (w + edge - 1) / edge * (h + edge - 1) / edge;
        compressed *= sizeof(DepthQuantize24);
    }

#if 0
   if (w%edge || h%edge)
      return false;
#endif

    if (!initPbo(pitch * h * ps, compressed))
        return false;

    uchar *img = NULL;
    if (!pbo2cuda(x, y, w, pitch, h, format, ps, buf, type, &img))
        return false;

    dim3 grid((w + TileSizeX - 1) / TileSizeX, (h + TileSizeY - 1) / TileSizeY);
    dim3 block(TileSizeX, TileSizeY);

    const size_t sharedsize =
        3 * sizeof(uint32_t) * TileSizeX * TileSizeY + 2 * sizeof(uint32_t) * TileSizeX * TileSizeY / (edge * edge);

    if (format == GL_BGRA) {
        assert(ps == 4);
        depthquant<4, DepthQuantize24, true><<<grid, block, sharedsize>>>(img, outImg, w, h);
    } else {
        switch (ps) {
        case 1:
            depthquant<1, DepthQuantize16, false><<<grid, block, sharedsize>>>(img, outImg, w, h);
            break;
        case 2:
            depthquant<2, DepthQuantize16, false><<<grid, block, sharedsize>>>(img, outImg, w, h);
            break;
        case 3:
            depthquant<3, DepthQuantize24, false><<<grid, block, sharedsize>>>(img, outImg, w, h);
            break;
        case 4:
            depthquant<4, DepthQuantize24, false><<<grid, block, sharedsize>>>(img, outImg, w, h);
            break;
        default:
            fprintf(stderr, "pixel depth not supported\n");
            return false;
        }
    }

    cucheck("depth memcpy", cudaMemcpy(bits, outImg, compressed, cudaMemcpyDeviceToHost));
    cucheck("depth unmap res", cudaGraphicsUnmapResources(1, &imgRes, NULL));

    return true;
}
