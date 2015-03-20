#include "parrendmgr.h"
#include "renderobject.h"
#include "renderer.h"

namespace mpi = boost::mpi;

namespace vistle {

ParallelRemoteRenderManager::ParallelRemoteRenderManager(Renderer *module, IceTDrawCallback drawCallback)
: m_module(module)
, m_drawCallback(drawCallback)
, m_displayRank(0)
, m_vncControl(module, m_displayRank)
, m_defaultColor(Vector4(127, 127, 127, 255))
, m_updateBounds(1)
, m_doRender(1)
, m_currentView(-1)
{
   m_continuousRendering = m_module->addIntParameter("continuous_rendering", "render even though nothing has changed", 0, Parameter::Boolean);
   m_colorRank = m_module->addIntParameter("color_rank", "different colors on each rank", 0, Parameter::Boolean);
   m_defaultColor = Vector4(127, 127, 127, 255);
   m_icetComm = icetCreateMPICommunicator((MPI_Comm)m_module->comm());
}

ParallelRemoteRenderManager::~ParallelRemoteRenderManager() {

   for (auto &icet: m_icet) {
      if (icet.ctxValid)
         icetDestroyContext(icet.ctx);
   }
   icetDestroyMPICommunicator(m_icetComm);
}


void ParallelRemoteRenderManager::setModified() {

   m_doRender = 1;
}

void ParallelRemoteRenderManager::setLocalBounds(const Vector3 &min, const Vector3 &max) {

   localBoundMin = min;
   localBoundMax = max;
   m_updateBounds = 1;
}

bool ParallelRemoteRenderManager::handleParam(const Parameter *p) {

    setModified();

    if (p == m_colorRank) {

      if (m_colorRank->getValue()) {

         const int r = m_module->rank()%3;
         const int g = (m_module->rank()/3)%3;
         const int b = (m_module->rank()/9)%3;
         m_defaultColor = Vector4(63+r*64, 63+g*64, 63+b*64, 255);
      } else {

         m_defaultColor = Vector4(127, 127, 127, 255);
      }
      return true;
    }

    return m_vncControl.handleParam(p);
}

bool ParallelRemoteRenderManager::prepareFrame(size_t numTimesteps) {

   m_state.numTimesteps = numTimesteps;
   m_state.numTimesteps = mpi::all_reduce(m_module->comm(), m_state.numTimesteps, mpi::maximum<unsigned>());

   if (m_updateBounds) {
      Vector min, max;
      m_module->getBounds(min, max);
      setLocalBounds(min, max);
   }

   m_updateBounds = mpi::all_reduce(m_module->comm(), m_updateBounds, mpi::maximum<int>());
   if (m_updateBounds) {
      mpi::all_reduce(m_module->comm(), localBoundMin.data(), 3, m_state.bMin.data(), mpi::minimum<Scalar>());
      mpi::all_reduce(m_module->comm(), localBoundMax.data(), 3, m_state.bMax.data(), mpi::maximum<Scalar>());
   }

   auto vnc = m_vncControl.server();
   if (vnc) {
      if (m_updateBounds) {
         Vector center = 0.5*(m_state.bMin+m_state.bMax);
         Scalar radius = (m_state.bMax-m_state.bMin).norm()*0.5;
         vnc->setBoundingSphere(center, radius);
      }

      vnc->setNumTimesteps(m_state.numTimesteps);

      vnc->preFrame();
      if (m_module->rank() == rootRank()) {

          if (m_state.timestep != vnc->timestep())
              m_doRender = 1;
      }
      m_state.timestep = vnc->timestep();

      for (int i=m_viewData.size(); i<vnc->numViews(); ++i) {
          std::cerr << "new view no. " << i << std::endl;
          m_doRender = 1;
          m_viewData.emplace_back();
      }

      for (int i=0; i<vnc->numViews(); ++i) {

          PerViewState &vd = m_viewData[i];

          const Matrix4 view = vnc->viewMat(i) * vnc->transformMat(i) * vnc->scaleMat(i);
          if (vd.width != vnc->width(i)
                  || vd.height != vnc->height(i)
                  || vd.proj != vnc->projMat(i)
                  || vd.view != view) {
              m_doRender = 1;
          }

          vd.vncParam = vnc->getViewParameters(i);

          vd.width = vnc->width(i);
          vd.height = vnc->height(i);
          vd.view = view;
          vd.proj = vnc->projMat(i);

          vd.lights = vnc->lights;
          const Matrix4 lightTransform = vnc->viewMat(i).inverse();
          for (auto &light: vd.lights) {
              Vector4 transformedPosition = lightTransform * light.position;
              transformedPosition /= transformedPosition[3];
              light.transformedPosition = Vector3(transformedPosition[0], transformedPosition[1], transformedPosition[2]);
          }
      }
   }
   m_updateBounds = 0;

   m_doRender = mpi::all_reduce(m_module->comm(), m_doRender, mpi::maximum<int>());

   if (m_continuousRendering->getValue())
      m_doRender = 1;

   bool doRender = m_doRender;
   m_doRender = 0;

   if (doRender) {

      mpi::broadcast(m_module->comm(), m_state, rootRank());
      mpi::broadcast(m_module->comm(), m_viewData, rootRank());

      if (vnc && m_module->rank() != rootRank()) {
          for (size_t i=0; i<m_viewData.size(); ++i) {
              const PerViewState &vd = m_viewData[i];
              vnc->resize(i, vd.width, vd.height);
          }
      }
   }

   return doRender;
}

size_t ParallelRemoteRenderManager::timestep() const {
   return m_state.timestep;
}

size_t ParallelRemoteRenderManager::numViews() const {
   return m_viewData.size();
}

void ParallelRemoteRenderManager::setCurrentView(size_t i) {

   vassert(m_currentView == -1);
   auto vnc = m_vncControl.server();

   int resetTiles = 0;
   while (m_icet.size() <= i) {
      resetTiles = 1;
      m_icet.emplace_back();
      auto &icet = m_icet.back();
      icet.ctx = icetCreateContext(m_icetComm);
      icet.ctxValid = true;
   }
   vassert(i < m_icet.size());
   auto &icet = m_icet[i];
   //std::cerr << "setting IceT context: " << icet.ctx << " (valid: " << icet.ctxValid << ")" << std::endl;
   icetSetContext(icet.ctx);
   m_currentView = i;

   if (vnc) {
      if (icet.width != vnc->width(i) || icet.height != vnc->height(i))
         resetTiles = 1;
   }
   resetTiles = mpi::all_reduce(m_module->comm(), resetTiles, mpi::maximum<int>());

   if (resetTiles) {
      std::cerr << "resetting IceT tiles for view " << i << "..." << std::endl;
      icet.width = icet.height = 0;
      if (m_module->rank() == rootRank()) {
         icet.width =  vnc->width(i);
         icet.height = vnc->height(i);
         std::cerr << "IceT dims on rank " << m_module->rank() << ": " << icet.width << "x" << icet.height << std::endl;
      }

      DisplayTile localTile;
      localTile.x = 0;
      localTile.y = 0;
      localTile.width = icet.width;
      localTile.height = icet.height;

      std::vector<DisplayTile> icetTiles;
      mpi::all_gather(m_module->comm(), localTile, icetTiles);
      vassert(icetTiles.size() == (unsigned)m_module->size());

      icetResetTiles();

      for (int i=0; i<m_module->size(); ++i) {
         if (icetTiles[i].width>0 && icetTiles[i].height>0)
            icetAddTile(icetTiles[i].x, icetTiles[i].y, icetTiles[i].width, icetTiles[i].height, i);
      }

      icetDisable(ICET_COMPOSITE_ONE_BUFFER); // include depth buffer in compositing result
      icetStrategy(ICET_STRATEGY_REDUCE);
      icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
      icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
      icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);

      icetDrawCallback(m_drawCallback);
   }

   icetBoundingBoxf(localBoundMin[0], localBoundMax[0],
         localBoundMin[1], localBoundMax[1],
         localBoundMin[2], localBoundMax[2]);
}

void ParallelRemoteRenderManager::finishCurrentView(const IceTImage &img) {

   size_t i = m_currentView;
   vassert(i >= 0);
   vassert(i < numViews());
   if (m_module->rank() == rootRank()) {
      auto vnc = m_vncControl.server();
      vassert(vnc);
      const int bpp = 4;
      const int w = vnc->width(i);
      const int h = vnc->height(i);
      vassert(w == icetImageGetWidth(img));
      vassert(h == icetImageGetHeight(img));

      const IceTUByte *color = icetImageGetColorcub(img);
      const IceTFloat *depth = icetImageGetDepthcf(img);

      if (color && depth) {
         for (int y=0; y<h; ++y) {
            memcpy(vnc->rgba(i)+w*bpp*y, color+w*(h-1-y)*bpp, bpp*w);
            memcpy(vnc->depth(i)+w*y, depth+w*(h-1-y), sizeof(float)*w);
         }

         vnc->invalidate(i, 0, 0, vnc->width(i), vnc->height(i), m_viewData[i].vncParam, i==m_viewData.size()-1);
      }
   }
   m_currentView = -1;
}

void ParallelRemoteRenderManager::getViewMat(size_t viewIdx, IceTDouble *mat) const {
   const PerViewState &vd = m_viewData[viewIdx];
   for (int i=0; i<16; ++i) {
      mat[i] = vd.view.data()[i];
   }
}

void ParallelRemoteRenderManager::getProjMat(size_t viewIdx, IceTDouble *mat) const {
   const PerViewState &vd = m_viewData[viewIdx];
   for (int i=0; i<16; ++i) {
      mat[i] = vd.proj.data()[i];
   }
}

const ParallelRemoteRenderManager::PerViewState &ParallelRemoteRenderManager::viewData(size_t viewIdx) const {

   return m_viewData[viewIdx];
}

void ParallelRemoteRenderManager::updateRect(size_t viewIdx, const IceTInt *viewport, const IceTImage image) {

   auto vnc = m_vncControl.server();
   if (vnc && m_module->rank() != rootRank()) {
      // observe what slaves are rendering
      const int bpp = 4;
      const int w = vnc->width(m_currentView);
      const int h = vnc->height(m_currentView);
      vassert(w == icetImageGetWidth(image));
      vassert(h == icetImageGetHeight(image));

      const IceTUByte *color = icetImageGetColorcub(image);

      memset(vnc->rgba(m_currentView), 0, w*h*bpp);

      for (int y=viewport[1]; y<viewport[1]+viewport[3]; ++y) {
         memcpy(vnc->rgba(m_currentView)+(w*y+viewport[0])*bpp, color+(w*(h-1-y)+viewport[0])*bpp, bpp*viewport[2]);
      }

      vnc->invalidate(m_currentView, 0, 0, vnc->width(m_currentView), vnc->height(m_currentView), vnc->getViewParameters(m_currentView), true);
   }
}

void ParallelRemoteRenderManager::addObject(boost::shared_ptr<RenderObject> ro) {
   m_updateBounds = 1;
}

void ParallelRemoteRenderManager::removeObject(boost::shared_ptr<RenderObject> ro) {
   m_updateBounds = 1;
}

}
