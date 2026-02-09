#include "ReadBackCuda.h"

ReadBackCuda::ReadBackCuda()
{}
ReadBackCuda::~ReadBackCuda()
{}
bool ReadBackCuda::init()
{
    return false;
}
bool ReadBackCuda::isInitialized() const
{
    return false;
}
bool ReadBackCuda::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                              GLint buf, GLenum type)
{
    return false;
}
bool ReadBackCuda::readpixelsyuv(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                                 GLint buf, int subx, int suby)
{
    return false;
}
bool ReadBackCuda::readdepthquant(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, int ps, GLubyte *bits,
                                  GLint buf, GLenum type)
{
    return false;
}
