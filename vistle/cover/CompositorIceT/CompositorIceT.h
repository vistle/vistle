#ifndef COMPOSITOR_ICET_H
#define COMPOSITOR_ICET_H

/**\file
 * \brief CompositorIceT plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013, 2014 HLRS
 *
 * \copyright GPL2+
 */

#include <vector>
#include <IceTMPI.h>

#include <cover/coVRPlugin.h>

class ReadBackCuda;

//! Implement depth compositor for sort-last parallel rendering based on IceT
class CompositorIceT : public opencover::coVRPlugin
{
 public:
   //! constructor
   CompositorIceT();
   //! destructor
   ~CompositorIceT();
   //! called during plug-in initialization
   bool init();

   //! called after a frame is rendered, but before being copied to the front buffer - used for read-back
   void preSwapBuffers(int windowNumber);
   //! called after a frame is copied to the front buffer - can be used for read-back if results do not have to be displayed on render host
   void postSwapBuffers(int windowNumber);
   //! make common scene bounding sphere available to master
   void expandBoundingSphere(osg::BoundingSphere &bs);                          

 private:
   //! store state of a window being composited
   struct Window
   {
      Window(): w(-1), h(-1), cudaColor(NULL), cudaDepth(NULL) {}

      int w; //!< window width
      int h; //!< window height
      IceTContext icetCtx; //!< compositing context
      ReadBackCuda *cudaColor, *cudaDepth; //!< helpers for CUDA read-back
   };

   int m_rank, m_size; //!< MPI rank and communicator size
   int m_displayRank; //!< MPI rank of display node
   bool m_useCuda; //!< whether CUDA shall be used for framebuffer read-back
   bool m_sparseReadback; //!< whether local scene bounds shall be used for accelerating read-back and compositing
   bool m_rhr; //!< whether remote hybrid rendering is in use - otherwise depth is not needed after compositing
   std::vector<Window> m_windows; //< all windows being composited
   IceTCommunicator m_icetComm; //!< IceT communicator

   //! do the actual composting work for window with index 'win'
   void composite(int win);
   //! check whether the window with index 'win' has bee resized since last frame
   void checkResize(int win);
   //! wrapper for pixel read-back (either pure OpenGL or CUDA)
   static void readpixels(ReadBackCuda *cuda, GLint x, GLint y, GLint w, GLint pitch, GLint h,                                              
         GLenum format, int ps, GLubyte *bits, GLint buf, GLenum type);
   //! draw callback for IceT
   static void drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
};
#endif
