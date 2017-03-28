#include <IceT.h>
#include <IceTMPI.h>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <boost/mpi.hpp>

#include <renderer/renderer.h>
#include <core/texture1d.h>
#include <core/message.h>
#include <core/assert.h>

#include <util/stopwatch.h>

#include <renderer/parrendmgr.h>
#include "rayrenderobject.h"
#include "common.h"

#include "render_ispc.h"
#include <xmmintrin.h>
#include <pmmintrin.h>

#ifdef USE_TBB
#include <tbb/parallel_for.h>
#else
//#include <future>
#endif

#define ICET_CALLBACK

namespace mpi = boost::mpi;

using namespace vistle;

const float Epsilon = 1e-9f;

class RayCaster: public vistle::Renderer {

   static RayCaster *s_instance;

 public:
   static void rtcErrorCallback(RTCError code, const char *desc) {

      std::string err = "Error: Unknown RTC error.";

      switch (code) {
         case RTC_NO_ERROR: err = "No error occurred."; break;
         case RTC_UNKNOWN_ERROR: err = "An unknown error has occurred."; break;
         case RTC_INVALID_ARGUMENT: err = "An invalid argument was specified."; break;
         case RTC_INVALID_OPERATION: err = "The operation is not allowed for the specified object."; break;
         case RTC_OUT_OF_MEMORY: err = "There is not enough memory left to complete the operation."; break;
         case RTC_UNSUPPORTED_CPU: err = "The CPU is not supported as it does not support SSE2."; break;
         case RTC_CANCELLED: err = "The operation got cancelled by an Memory Monitor Callback or Progress Monitor Callback function."; break;
      }

      std::cerr << "RTC error: " << desc << " - " << err << std::endl;
   }

   struct RGBA {
      unsigned char r, g, b, a;
   };

   RayCaster(const std::string &shmname, const std::string &name, int moduleId);
   ~RayCaster();
   void prepareQuit() override;

   static RayCaster &the() {

      return *s_instance;
   }

   bool render() override;

   bool changeParameter(const Parameter *p) override;
   void connectionAdded(const Port *from, const Port *to) override;

   ParallelRemoteRenderManager m_renderManager;

   Port *m_outPort;

   // parameters
   IntParameter *m_renderTileSizeParam;
   int m_tilesize;
   IntParameter *m_shading;
   bool m_doShade;
   IntParameter *m_uvVisParam;
   bool m_uvVis;
   FloatParameter *m_pointSizeParam;

   // object lifetime management
   std::shared_ptr<RenderObject> addObject(int sender, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr texture) override;

   void removeObject(std::shared_ptr<RenderObject> ro) override;

   std::vector<ispc::RenderObjectData *> instances;

   std::vector<std::shared_ptr<RayRenderObject>> static_geometry;
   std::vector<std::vector<std::shared_ptr<RayRenderObject>>> anim_geometry;

   RTCDevice m_device;
   RTCScene m_scene;

   size_t m_timestep;

   int m_currentView; //!< holds no. of view currently being rendered - not a problem is IceT is not reentrant anyway
   static void drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
   void renderRect(const vistle::Matrix4 &proj, const vistle::Matrix4 &mv, const IceTInt *viewport,
                   int width, int height, unsigned char *rgba, float *depth);
};

RayCaster *RayCaster::s_instance = nullptr;


RayCaster::RayCaster(const std::string &shmname, const std::string &name, int moduleId)
: Renderer("RayCaster", shmname, name, moduleId)
#ifdef ICET_CALLBACK
, m_renderManager(this, RayCaster::drawCallback)
#else
, m_renderManager(this, nullptr)
#endif
, m_tilesize(TileSize)
, m_doShade(true)
, m_uvVis(false)
, m_timestep(0)
, m_currentView(-1)
{
#if 0
    std::cerr << "Ray: " << getpid() << std::endl;
    sleep(10);
#endif

   vassert(s_instance == nullptr);
   s_instance = this;

   /* from embree examples: for best performance set FTZ and DAZ flags in MXCSR control and status * register */
   _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
   _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

   m_outPort = createOutputPort("image_out", "connect to COVER");

   m_shading = addIntParameter("shading", "shade and light objects", (Integer)m_doShade, Parameter::Boolean);
   m_uvVisParam = addIntParameter("uv_visualization", "show u/v coordinates", (Integer)m_uvVis, Parameter::Boolean);
   m_renderTileSizeParam = addIntParameter("render_tile_size", "edge length of square tiles used during rendering", m_tilesize);
   setParameterRange(m_renderTileSizeParam, (Integer)1, (Integer)TileSize);
   m_pointSizeParam = addFloatParameter("point_size", "size of points", RayRenderObject::pointSize);
   setParameterRange(m_pointSizeParam, (Float)0, (Float)1e6);

   m_device = rtcNewDevice("verbose=0");
   if (!m_device) {
       std::cerr << "Ray: failed to create device" << std::endl;
       throw(vistle::exception("failed to create Embree device"));
   }
   rtcDeviceSetErrorFunction(m_device, rtcErrorCallback);
   m_scene = rtcDeviceNewScene(m_device, RTC_SCENE_DYNAMIC|sceneFlags, intersections);
   rtcCommit(m_scene);
}


RayCaster::~RayCaster() {

   vassert(s_instance == this);
   s_instance = nullptr;
}

void RayCaster::prepareQuit() {

   removeAllObjects();

   rtcDeleteScene(m_scene);
   rtcDeleteDevice(m_device);

   Renderer::prepareQuit();
}

void RayCaster::connectionAdded(const Port *from, const Port *to) {

    if (from == m_outPort) {
        if (rank() == 0) {
            std::cerr << "sending rhr config object" << std::endl;
            Module::addObject(m_outPort, m_renderManager.getConfigObject());
        }
    }
}


bool RayCaster::changeParameter(const Parameter *p) {

    m_renderManager.handleParam(p);

    if (p == m_shading) {

       m_doShade = m_shading->getValue();
       m_renderManager.setModified();
    } else if (p == m_uvVisParam) {

        m_uvVis = m_uvVisParam->getValue();
        m_renderManager.setModified();
    } else if (p == m_pointSizeParam) {

       RayRenderObject::pointSize = m_pointSizeParam->getValue();
    } else if (p == m_renderTileSizeParam) {

        m_tilesize = m_renderTileSizeParam->getValue();
    }

   return Renderer::changeParameter(p);
}

struct TileTask {
   TileTask(const RayCaster &rc, const ParallelRemoteRenderManager::PerViewState &vd, int tile=-1)
   : rc(rc)
   , vd(vd)
   , tile(tile)
   , tilesize(rc.m_tilesize)
   {
   }

   void render(int tile) const;

   void operator()(int tile) const {
      render(tile);
   }

   void operator()() const {
      vassert(tile >= 0);
      render(tile);
   }

   const RayCaster &rc;
   const ParallelRemoteRenderManager::PerViewState &vd;
   const int tile;
   const int tilesize;
   int imgWidth, imgHeight;
   int xlim, ylim;
   int ntx;
   Vector4 depthTransform2, depthTransform3;
   Vector3 lowerBottom, dx, dy;
   Vector3 origin;
   Matrix4 modelView;
   float tNear, tFar;
   int xoff, yoff;
   float *depth;
   unsigned char *rgba;
};


void TileTask::render(int tile) const {

    const int tx = tile%ntx;
    const int ty = tile/ntx;

    ispc::SceneData sceneData;
    sceneData.scene = (ispc::__RTCScene *)rc.m_scene;
    for (int i=0; i<4; ++i) {
        auto row = modelView.block<1,4>(i,0);
        sceneData.modelView[i].x = row[0];
        sceneData.modelView[i].y = row[1];
        sceneData.modelView[i].z = row[2];
        sceneData.modelView[i].w = row[3];
    }
    sceneData.doShade = rc.m_doShade;
    sceneData.uvVis = rc.m_uvVis;
    sceneData.defaultColor.x = rc.m_renderManager.m_defaultColor[0];
    sceneData.defaultColor.y = rc.m_renderManager.m_defaultColor[1];
    sceneData.defaultColor.z = rc.m_renderManager.m_defaultColor[2];
    sceneData.defaultColor.w = rc.m_renderManager.m_defaultColor[3];
    sceneData.ro = rc.instances.data();
    std::vector<ispc::Light> lights;
    for (const auto &light: vd.lights) {
        ispc::Light l;
        l.enabled = light.enabled;
        l.transformedPosition.x = light.transformedPosition[0];
        l.transformedPosition.y = light.transformedPosition[1];
        l.transformedPosition.z = light.transformedPosition[2];
        for (int c=0; c<3; ++c) {
            l.attenuation[c] = light.attenuation[c];
        }
        l.isDirectional = light.isDirectional;

        l.ambient.x = light.ambient[0];
        l.ambient.y = light.ambient[1];
        l.ambient.z = light.ambient[2];
        l.ambient.w = light.ambient[3];

        l.diffuse.x = light.diffuse[0];
        l.diffuse.y = light.diffuse[1];
        l.diffuse.z = light.diffuse[2];
        l.diffuse.w = light.diffuse[3];

        l.specular.x = light.specular[0];
        l.specular.y = light.specular[1];
        l.specular.z = light.specular[2];
        l.specular.w = light.specular[3];

        lights.push_back(l);
    }
    sceneData.numLights = lights.size();
    sceneData.lights = lights.data();

    ispc::TileData data;
    data.rgba = rgba;
    data.depth = depth;
    data.imgWidth = imgWidth;

    data.x0=xoff+tx*tilesize;
    data.x1=std::min(data.x0+tilesize, xlim);
    data.y0=yoff+ty*tilesize;
    data.y1=std::min(data.y0+tilesize, ylim);

    data.origin.x = origin[0];
    data.origin.y = origin[1];
    data.origin.z = origin[2];

    data.dx.x = dx[0];
    data.dx.y = dx[1];
    data.dx.z = dx[2];

    data.dy.x = dy[0];
    data.dy.y = dy[1];
    data.dy.z = dy[2];

    const Vector toLowerBottom = lowerBottom - origin;
    data.corner.x = toLowerBottom[0];
    data.corner.y = toLowerBottom[1];
    data.corner.z = toLowerBottom[2];

    data.tNear = tNear;
    data.tFar = tFar;

    data.depthTransform2.x = depthTransform2[0];
    data.depthTransform2.y = depthTransform2[1];
    data.depthTransform2.z = depthTransform2[2];
    data.depthTransform2.w = depthTransform2[3];

    data.depthTransform3.x = depthTransform3[0];
    data.depthTransform3.y = depthTransform3[1];
    data.depthTransform3.z = depthTransform3[2];
    data.depthTransform3.w = depthTransform3[3];


    ispcRenderTile(&sceneData, &data);
}


bool RayCaster::render() {

    // ensure that previous frame is completed
    bool immed_resched = m_renderManager.finishFrame(m_timestep);

    //vistle::StopWatch timer("render");

    const size_t numTimesteps = anim_geometry.size();
    if (!m_renderManager.prepareFrame(numTimesteps)) {
       return immed_resched;
    }

    // switch time steps in embree scene
    if (m_timestep != m_renderManager.timestep() || m_renderManager.sceneChanged()) {
       if (anim_geometry.size() > m_timestep && m_timestep != m_renderManager.timestep()) {
          for (auto &ro: anim_geometry[m_timestep])
             if (ro->data->scene)
                rtcDisable(m_scene, ro->data->instId);
       }
       m_timestep = m_renderManager.timestep();
       if (anim_geometry.size() > m_timestep) {
           for (auto &ro: anim_geometry[m_timestep])
               if (ro->data->scene) {
                   if (m_renderManager.isVariantVisible(ro->variant)) {
                       rtcEnable(m_scene, ro->data->instId);
                   } else {
                       rtcDisable(m_scene, ro->data->instId);
                   }
               }
       }
       if (m_renderManager.sceneChanged()) {
           for (auto &ro: static_geometry) {
               if (ro->data->scene) {
                   if (m_renderManager.isVariantVisible(ro->variant)) {
                       rtcEnable(m_scene, ro->data->instId);
                   } else {
                       rtcDisable(m_scene, ro->data->instId);
                   }
               }
           }
       }
       rtcCommit(m_scene);
    }

    for (size_t i=0; i<m_renderManager.numViews(); ++i) {
       m_renderManager.setCurrentView(i);
       m_currentView = i;

       auto &vd = m_renderManager.viewData(i);
       //std::cerr << "rendering view " << i << ", proj=" << vd.proj << std::endl;
       const vistle::Matrix4 lightTransform = vd.model.inverse();
       for (auto &light: vd.lights) {
          light.transformedPosition = lightTransform * light.position;
          if (fabs(light.transformedPosition[3]) > Epsilon) {
             light.isDirectional = false;
             light.transformedPosition /= light.transformedPosition[3];
          } else {
             light.isDirectional = true;
          }
#if 0
          if (light.enabled)
             std::cerr << "light pos " << light.position.transpose() << " -> " << light.transformedPosition.transpose() << std::endl;
#endif
       }
       IceTDouble proj[16], mv[16];
       m_renderManager.getModelViewMat(i, mv);
       m_renderManager.getProjMat(i, proj);
       IceTFloat bg[4] = { 0., 0., 0., 0. };

#ifdef ICET_CALLBACK
       IceTImage img = icetDrawFrame(proj, mv, bg);
#else
       vistle::Matrix4 MV, P;
       for (int i=0; i<4; ++i) {
           for (int j=0; j<4; ++j) {
               MV(i,j) = mv[j*4+i];
               P(i,j) = proj[j*4+i];
           }
       }

       IceTInt viewport[4] = {0, 0, vd.width, vd.height};
       unsigned char *rgba = m_renderManager.rgba(i);
       float *depth = m_renderManager.depth(i);
       renderRect(P, MV, viewport, vd.width, vd.height, rgba, depth);
       IceTImage img = icetCompositeImage(rgba, depth, viewport, proj, mv, bg);
#endif

       m_renderManager.finishCurrentView(img, m_timestep, false);
    }
    m_currentView = -1;

    return true;
}

void RayCaster::renderRect(const vistle::Matrix4 &P, const vistle::Matrix4 &MV, const IceTInt *viewport,
                           int width, int height, unsigned char *rgba, float *depth) {

   //StopWatch timer("RayCaster::render()");

#if 0
   std::cerr << "IceT draw CB: vp=" << viewport[0] << ", " << viewport[1] << ", " << viewport[2] << ", " <<  viewport[3]
             << ", img: " << width << "x" << height
             << std::endl;
#endif

   const int w = viewport[2];
   const int h = viewport[3];
   const int ts = m_tilesize;
   const int wt = ((w+ts-1)/ts)*ts;
   const int ht = ((h+ts-1)/ts)*ts;
   const int ntx = wt/ts;
   const int nty = ht/ts;

   //std::cerr << "PROJ:" << P << std::endl << std::endl;

   const vistle::Matrix4 MVP = P * MV;
   const auto inv = MVP.inverse();

   const Vector4 ro4 = MV.inverse().col(3);
   const Vector ro = ro4.block<3,1>(0,0)/ro4[3];

   const Vector4 lbn4 = inv * Vector4(-1, -1, -1, 1);
   Vector lbn(lbn4[0], lbn4[1], lbn4[2]);
   lbn /= lbn4[3];

   const Vector4 lbf4 = inv * Vector4(-1, -1, 1, 1);
   Vector lbf(lbf4[0], lbf4[1], lbf4[2]);
   lbf /= lbf4[3];

   const Vector4 rbn4 = inv * Vector4(1, -1, -1, 1);
   Vector rbn(rbn4[0], rbn4[1], rbn4[2]);
   rbn /= rbn4[3];

   const Vector4 ltn4 = inv * Vector4(-1, 1, -1, 1);
   Vector ltn(ltn4[0], ltn4[1], ltn4[2]);
   ltn /= ltn4[3];

   const Scalar tFar = (lbf-ro).norm()/(lbn-ro).norm();
   const Matrix4 depthTransform = MVP;

   TileTask renderTile(*this, m_renderManager.viewData(m_currentView));
   renderTile.rgba = rgba;
   renderTile.depth = depth;
   renderTile.depthTransform2 = depthTransform.row(2);
   renderTile.depthTransform3 = depthTransform.row(3);
   renderTile.ntx = ntx;
   renderTile.xoff = viewport[0];
   renderTile.yoff = viewport[1];
   renderTile.xlim = viewport[0] + viewport[2];
   renderTile.ylim = viewport[1] + viewport[3];
   renderTile.imgWidth = width;
   renderTile.imgHeight = height;
   renderTile.dx = (rbn-lbn)/renderTile.imgWidth;
   renderTile.dy = (ltn-lbn)/renderTile.imgHeight;
   renderTile.lowerBottom = lbn + 0.5*renderTile.dx + 0.5*renderTile.dy;
   renderTile.origin = ro;
   renderTile.tNear = 1.;
   renderTile.tFar = tFar;
   renderTile.modelView = MV;
#ifdef USE_TBB
   tbb::parallel_for(0, ntx*nty, 1, renderTile);
#else
#pragma omp parallel for schedule(dynamic)
   for (int t=0; t<ntx*nty; ++t) {
      renderTile(t);
   }
#endif

#if 0
   std::vector<std::future<void>> tiles;

   for (int t=0; t<ntx*nty; ++t) {
      TileTask renderTile(*this, t);
      renderTile.rgba = rgba;
      renderTile.depth = depth;
      renderTile.depthTransform = depthTransform;
      renderTile.ntx = ntx;
      renderTile.xoff = x0;
      renderTile.yoff = y0;
      renderTile.width = x1;
      renderTile.height = y1;
      renderTile.dx = (rbn-lbn)/w;
      renderTile.dy = (ltn-lbn)/h;
      renderTile.lowerBottom = lbn + 0.5*renderTile.dx + 0.5*renderTile.dy;
      renderTile.tNear = 1.;
      renderTile.tFar = zFar/zNear;
      tiles.emplace_back(std::async(std::launch::async, renderTile));
   }

   for (auto &task: tiles) {
      task.get();
   }
#endif

#ifdef ICET_CALLBACK
   m_renderManager.updateRect(m_currentView, viewport);
#endif

   int err = rtcDeviceGetError(m_device);
   if (err != 0) {
      std::cerr << "RTC error: " << err << std::endl;
   }
}


void RayCaster::removeObject(std::shared_ptr<RenderObject> vro) {

   auto ro = std::static_pointer_cast<RayRenderObject>(vro);
   auto rod = ro->data.get();

   if (rod->scene) {
      rtcDeleteGeometry(m_scene, rod->instId);
      rtcCommit(m_scene);

      instances[rod->instId] = nullptr;
   }

   const int t = ro->timestep;
   auto &objlist = t>=0 ? anim_geometry[t] : static_geometry;

   if (t == -1 || size_t(t) == m_timestep) {
      m_renderManager.setModified();
   }

   auto it = std::find(objlist.begin(), objlist.end(), ro);
   if (it != objlist.end()) {
      std::swap(*it, objlist.back());
      objlist.pop_back();
   }

   while (!anim_geometry.empty() && anim_geometry.back().empty())
      anim_geometry.pop_back();

   m_renderManager.removeObject(ro);
}


std::shared_ptr<RenderObject> RayCaster::addObject(int sender, const std::string &senderPort,
                                 vistle::Object::const_ptr container,
                                 vistle::Object::const_ptr geometry,
                                 vistle::Object::const_ptr normals,
                                 vistle::Object::const_ptr texture) {

   std::shared_ptr<RayRenderObject> ro(new RayRenderObject(m_device, sender, senderPort, container, geometry, normals, texture));

   const int t = ro->timestep;
   if (t == -1) {
      static_geometry.push_back(ro);
   } else {
      if (anim_geometry.size() <= size_t(t))
         anim_geometry.resize(t+1);
      anim_geometry[t].push_back(ro);
   }

   auto rod = ro->data.get();
   if (rod->scene) {
      rod->instId = rtcNewInstance(m_scene, rod->scene);
      if (instances.size() <= rod->instId)
         instances.resize(rod->instId+1);
      vassert(!instances[rod->instId]);
      instances[rod->instId] = rod;

      float transform[16];
      auto geoTransform = geometry->getTransform();
      for (int i=0; i<16; ++i) {
          transform[i] = geoTransform(i%4, i/4);
      }
      rtcSetTransform(m_scene, rod->instId, RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, transform);
      if (t == -1 || size_t(t) == m_timestep) {
         rtcEnable(m_scene, rod->instId);
         m_renderManager.setModified();
      } else {
         rtcDisable(m_scene, rod->instId);
      }
      rtcCommit(m_scene);
   }

   m_renderManager.addObject(ro);

   return ro;
}

void  RayCaster::drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image) {

    vistle::Matrix4 MV, P;
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            MV(i,j) = mv[j*4+i];
            P(i,j) = proj[j*4+i];
        }
    }

    unsigned char *rgba = icetImageGetColorub(image);
    float *depth = icetImageGetDepthf(image);
    int width = icetImageGetWidth(image);
    int height = icetImageGetHeight(image);

    RayCaster::the().renderRect(P, MV, viewport, width, height, rgba, depth);
}


MODULE_MAIN(RayCaster)
