#ifndef VISTLE_COVER_PLUGIN_COMPOSITORICET_READBACKCUDA_H
#define VISTLE_COVER_PLUGIN_COMPOSITORICET_READBACKCUDA_H

/**\file
 * \brief class ReadBackCuda
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author Martin Aumüller <aumueller@uni-koeln.de>
 * \author (c) 2011 RRZK, 2013 HLRS
 */

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include <cstdlib>

struct cudaGraphicsResource;
typedef unsigned char uchar;

//! fast framebuffer read-back using CUDA
class ReadBackCuda {
public:
    ReadBackCuda();
    ~ReadBackCuda();

    bool init();
    bool isInitialized() const;

    bool readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits, GLint buf,
                    GLenum type = GL_UNSIGNED_BYTE);
    bool readpixelsyuv(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits, GLint buf,
                       int subx, int suby);
    bool readdepthquant(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                        GLint buf, GLenum type = GL_UNSIGNED_BYTE);

    static bool s_error; //!< whether a CUDA error occurred
private:
    bool initPbo(size_t raw, size_t compressed);
    bool pbo2cuda(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLint buf, GLenum type,
                  uchar **img);
    GLuint pboName; //!< id of pixel-buffer object used for CUDA/GL interoperation
    cudaGraphicsResource *imgRes; //!< corresponding CUDA resource
    uchar *outImg; //!< output buffer for converted colors
    size_t imgSize; //!< size of output image
    size_t compSize; //!< size of output buffer
    bool m_initialized; //!< whether class is initialized
};

#endif
