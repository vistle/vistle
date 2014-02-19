#include "framebuffer.h"
#include <core/assert.h>

void FrameBuffer::resize(int width, int height, FrameBuffer::uchar4 *rgba, float *depth) {

   if (rgba || depth) {

      vassert(rgba && depth);
      m_rgba.clear();
      m_depth.clear();
      m_rgbaP = rgba;
      m_depthP = depth;
   } else {
      m_depth.resize(width*height);
      m_depthP = m_depth.data();
      m_rgba.resize(width*height);
      m_rgbaP = m_rgba.data();
   }
}

int FrameBuffer::width() const {

   return m_width;
}

int FrameBuffer::height() const {

   return m_height;
}
