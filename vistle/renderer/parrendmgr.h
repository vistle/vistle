#ifndef VISTLE_PARRENDMGR_H
#define VISTLE_PARRENDMGR_H

#include <IceT.h>
#include <IceTMPI.h>
#include "vnccontroller.h"

namespace vistle {

class V_RENDEREREXPORT ParallelRemoteRenderManager {
public:
   typedef void (*IceTDrawCallback)(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
   struct PerViewState;

   ParallelRemoteRenderManager(Module *module, IceTDrawCallback drawCallback);
   ~ParallelRemoteRenderManager();

   bool handleParam(const Parameter *p);
   bool prepareFrame(size_t numTimesteps);
   size_t timestep() const;
   size_t numViews() const;
   void setCurrentView(size_t i);
   void finishCurrentView(const IceTImage &img);
   void getViewMat(size_t viewIdx, IceTDouble *mat) const;
   void getProjMat(size_t viewIdx, IceTDouble *mat) const;
   const PerViewState &viewData(size_t viewIdx) const;
   void updateRect(size_t viewIdx, const IceTInt *viewport, IceTImage image);
   void setModified();
   void setLocalBounds(const Vector3 &min, const Vector3 &max);
   int rootRank() const {
      return m_displayRank==-1 ? 0 : m_displayRank;
   }

   Module *m_module;
   IceTDrawCallback m_drawCallback;
   int m_displayRank;
   VncController m_vncControl;
   IntParameter *m_continuousRendering;

   IntParameter *m_colorRank;
   Vector4 m_defaultColor;

   size_t m_timestep;
   Vector3 boundMin, boundMax;

   int m_updateBounds;
   int m_doRender;

   struct PerViewState {
       // synchronized across all ranks

      int width, height;
      Matrix4 view;
      Matrix4 proj;
      std::vector<VncServer::Light> lights;
      VncServer::ViewParameters vncParam;

      PerViewState()
      : width(0)
      , height(0)
      {

         view.Identity();
         proj.Identity();
      }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & width;
         ar & height;
         ar & view;
         ar & proj;
         ar & lights;
      }
   };

   //! serializable description of one IceT tile - usable by boost::mpi
   struct DisplayTile {

      int rank;
      int x, y, width, height;

      DisplayTile()
      : rank(-1)
      , x(0)
      , y(0)
      , width(0)
      , height(0)
      {}

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & x;
         ar & y;
         ar & width;
         ar & height;
      }

   };

   //! state shared among all views
   struct GlobalState {
      unsigned timestep;
      unsigned numTimesteps;
      Vector3 bMin, bMax;

      GlobalState()
      : timestep(0)
      , numTimesteps(0)
      {
      }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & timestep;
         ar & numTimesteps;
      }
   };
   struct GlobalState m_state;

   std::vector<PerViewState> m_viewData; // synchronized from rank 0 to slaves
   int m_currentView; //!< holds no. of view currently being rendered - not a problem is IceT is not reentrant anyway

   //! per view IceT state
   struct IceTData {

       bool ctxValid;
       int width, height; // dimensions of local tile
       IceTContext ctx;

       IceTData()
       : ctxValid(false)
       , width(0)
       , height(0)
       {
           ctx = 0;
       }
   };
   std::vector<IceTData> m_icet; // managed locally
   IceTCommunicator m_icetComm; // common for all contexts
};

}
#endif


