#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vector>

class FrameBuffer {

 public:
   typedef struct {
      unsigned char r, g, b, a;
   } uchar4;

   FrameBuffer(int w, int h, FrameBuffer::uchar4 *rgba=nullptr, float *depth=nullptr)
      : m_width(w)
      , m_height(h)
      , m_rgbaP(nullptr)
      , m_depthP(nullptr)
      {
         resize(m_width, m_height, rgba, depth);
      }

   void resize(int w, int h, uchar4 *rgba=nullptr, float *depth=nullptr);
   uchar4 &rgba(int x, int y) const {
      return m_rgbaP[x+m_width*y];
   }
   float &depth(int x, int y) const {
      return m_depthP[x+m_width*y];
   }
   int width() const;
   int height() const;

 private:
   int m_width, m_height;
   std::vector<uchar4> m_rgba;
   std::vector<float> m_depth;
   uchar4 *m_rgbaP;
   float *m_depthP;
};

#endif
