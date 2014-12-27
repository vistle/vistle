#include <IceT.h>
#include <IceTMPI.h>

#include <kernel/coVRPluginSupport.h>
#include <kernel/coVRConfig.h>
#include <kernel/coVRMSController.h>
#include "CompositorIceT.h"

#include <osg/MatrixTransform>

#include <rhr/ReadBackCuda.h>


using namespace opencover;

static CompositorIceT *plugin = NULL;

CompositorIceT::CompositorIceT()
: m_rank(-1)
, m_size(-1)
, m_displayRank(0) // display on master
, m_useCuda(false)
, m_sparseReadback(true)
, m_rhr(false)
{
   assert(plugin == NULL);
   plugin = this;
   m_useCuda = true;
}


// this is called if the plugin is removed at runtime
CompositorIceT::~CompositorIceT()
{
   icetDestroyMPICommunicator(m_icetComm);
   plugin = NULL;
}

bool CompositorIceT::init()
{
   int initialized = 0;
   MPI_Initialized(&initialized);
   if (!initialized) {
      std::cerr << "CompositorIceT: No MPI support" << std::endl;
      return false;
   }

   MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &m_size);
   if (m_rank == m_displayRank && !m_rhr) {
      // menues can be anywhere - but are switched off for remote hybrid rendering
      m_sparseReadback = false;
   }
   m_icetComm = icetCreateMPICommunicator(MPI_COMM_WORLD);
   return true;
}

void CompositorIceT::checkResize(int win)
{
   if (win >= m_windows.size())
   {
      m_windows.resize(win+1);
   }
   Window &w = m_windows[win];

   osgViewer::GraphicsWindow *gwin = coVRConfig::instance()->windows[0].window;
   int x, y, width, height;
   gwin->getWindowRectangle(x, y, width, height);
   if (width != w.w || height != w.h) {
      if (w.w != -1) {
         icetDestroyContext(w.icetCtx);
      } else {
         if (m_useCuda) {
            w.cudaColor = new ReadBackCuda();
            if (!w.cudaColor->init()) {
               delete w.cudaColor;
               w.cudaColor = NULL;
            }

            w.cudaDepth = new ReadBackCuda();
            if (!w.cudaDepth->init()) {
               delete w.cudaDepth;
               w.cudaDepth = NULL;
            }
         }
      }
      w.icetCtx = icetCreateContext(m_icetComm);
      w.w = width;
      w.h = height;

      icetResetTiles();
      icetAddTile(0, 0, w.w, w.h, m_displayRank);
      if (m_rhr) {
         icetDisable(ICET_COMPOSITE_ONE_BUFFER);
      } else {
         icetEnable(ICET_COMPOSITE_ONE_BUFFER);
      }
      icetStrategy(ICET_STRATEGY_REDUCE);
      icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
      icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
      icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);

      icetDrawCallback(drawCallback);
   }
}

void CompositorIceT::composite(int windowNumber)
{
   if (windowNumber > 0)
      return;

   checkResize(windowNumber);
   icetSetContext(m_windows[windowNumber].icetCtx);

   IceTDouble mv[16], proj[16];
   if (m_sparseReadback) {
      const osg::Matrix &osg_mv = coVRConfig::instance()->screens[0].rightView;
      const osg::Matrix &osg_proj = coVRConfig::instance()->screens[0].rightProj;
      for (int i=0; i<16; ++i) {
         mv[i] = osg_mv(i/4, i%4);
         proj[i] = osg_proj(i/4, i%4);
      }

      osg::Node *root = cover->getObjectsRoot();
      osg::BoundingSphere bs = root->getBound();
      const float r = bs.radius() * cover->getScale();
      osg::Vec3 center = bs.center()*cover->getBaseMat();
      icetBoundingBoxf(center[0]-r, center[0]+r,
            center[1]-r, center[1]+r,
            center[2]-r, center[2]+r);
      //std::cerr << "world bounds: r= " << r << ", c=" << center[0] << " " << center[1] << " " << center[2] << std::endl;
   }

   IceTFloat bg[4] = { 0.0, 0.0, 0.0, 1.0 };

   IceTImage image = icetDrawFrame(proj, mv, bg);
   IceTSizeType width = icetImageGetWidth(image);
   IceTSizeType height = icetImageGetHeight(image);

   if (m_rank == m_displayRank && !icetImageIsNull(image)) {

      IceTUByte *color = icetImageGetColorub(image);
      glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, color);
      if (m_rhr) {
         IceTFloat *depth = icetImageGetDepthf(image);
         glDrawPixels(width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
      }
   }
}

void CompositorIceT::preSwapBuffers(int windowNumber)
{
   composite(windowNumber);
}

void CompositorIceT::postSwapBuffers(int windowNumber)
{
}

//! OpenGL framebuffer read-back
void CompositorIceT::readpixels(ReadBackCuda *cuda, GLint x, GLint y, GLint w, GLint pitch, GLint h,                                              
      GLenum format, int ps, GLubyte *bits, GLint buf, GLenum type)
{
   if (cuda) {
      cuda->readpixels(x, y, w, pitch, h, format, ps, bits, buf, type);
      return;
   }

   GLint readbuf=GL_BACK;
   glGetIntegerv(GL_READ_BUFFER, &readbuf);

   glReadBuffer(buf);
   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

   if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
   else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
   else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
   else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

   glPixelStorei(GL_PACK_ROW_LENGTH, pitch);

   // Clear previous error
   while (glGetError() != GL_NO_ERROR)
      ;

   glReadPixels(x, y, w, h, format, type, bits);

   glPopClientAttrib();
   glReadBuffer(readbuf);
}

void  CompositorIceT::drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image) {

   //std::cerr << "IceT draw CB: vp=" << viewport[0] << ", " << viewport[1] << ", " << viewport[2] << ", " <<  viewport[3] << std::endl;
   int vp_x = viewport[0];
   int vp_y = viewport[1];
   int vp_w = viewport[2];
   int vp_h = viewport[3];

   IceTSizeType width = icetImageGetWidth(image);
   IceTSizeType height = icetImageGetHeight(image);
   if (!plugin->m_sparseReadback) {
      vp_x = 0;
      vp_y = 0;
      vp_w = width;
      vp_h = height;
   }
 
   //IceTEnum cf = icetImageGetColorFormat(image);
   //IceTEnum df = icetImageGetDepthFormat(image);

   IceTUByte *color = icetImageGetColorub(image);
   IceTFloat *depth = icetImageGetDepthf(image);

   Window &w = plugin->m_windows[0];

   readpixels(w.cudaColor, vp_x, vp_y, vp_w, width, vp_h, GL_RGBA, 4, color+4*(vp_y*width+vp_x), GL_BACK, GL_UNSIGNED_BYTE);
   readpixels(w.cudaDepth, vp_x, vp_y, vp_w, width, vp_h, GL_DEPTH_COMPONENT, 4, reinterpret_cast<unsigned char *>(depth)+4*(vp_y*width+vp_x), GL_BACK, GL_FLOAT);
}

void CompositorIceT::expandBoundingSphere(osg::BoundingSphere &bs) {

   if (coVRMSController::instance()->isMaster()) {
      coVRMSController::SlaveData sd(sizeof(bs));
      coVRMSController::instance()->readSlaves(&sd);
      for (int i=0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
         osg::BoundingSphere *expand = static_cast<osg::BoundingSphere *>(sd.data[i]);
         bs.expandBy(*expand);
      }
      coVRMSController::instance()->sendSlaves(&bs, sizeof(bs));
   } else {
      coVRMSController::instance()->sendMaster(&bs, sizeof(bs));
      coVRMSController::instance()->readMaster(&bs, sizeof(bs));
   }
}

COVERPLUGIN(CompositorIceT)
