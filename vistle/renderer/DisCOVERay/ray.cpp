//#undef NDEBUG

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

#include <rhr/vncserver.h>
#include <renderer/vnccontroller.h>
#include <renderer/parrendmgr.h>
#include "rayrenderobject.h"
#include "common.h"

#ifdef HAVE_ISPC
#include "render_ispc.h"

#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

#ifdef USE_TBB
#include <tbb/parallel_for.h>
#else
//#include <future>
#endif

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

   static RayCaster &the() {

      return *s_instance;
   }

   bool render() override;

   bool parameterChanged(const Parameter *p) override;

   ParallelRemoteRenderManager m_renderManager;

   // parameters
   IntParameter *m_packetSize;
   int rayPacketSize;
   IntParameter *m_renderTileSizeParam;
   int m_tilesize;
   IntParameter *m_shading;
   bool m_doShade;
   IntParameter *m_uvVisParam;
   bool m_uvVis;
   IntParameter *m_useIspcParam;
   bool m_useIspc;
   FloatParameter *m_pointSizeParam;

   // object lifetime management
   boost::shared_ptr<RenderObject> addObject(int sender, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture) override;

   void removeObject(boost::shared_ptr<RenderObject> ro) override;

   std::vector<ispc::RenderObjectData *> instances;

   std::vector<boost::shared_ptr<RayRenderObject>> static_geometry;
   std::vector<std::vector<boost::shared_ptr<RayRenderObject>>> anim_geometry;

   RTCScene m_scene;

   size_t m_timestep;

   int m_currentView; //!< holds no. of view currently being rendered - not a problem is IceT is not reentrant anyway
   static void drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
   void renderRect(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
};

RayCaster *RayCaster::s_instance = nullptr;


RayCaster::RayCaster(const std::string &shmname, const std::string &name, int moduleId)
: Renderer("RayCaster", shmname, name, moduleId)
, m_renderManager(this, RayCaster::drawCallback)
, rayPacketSize(8)
, m_tilesize(64)
, m_doShade(true)
, m_uvVis(false)
, m_useIspc(false)
, m_timestep(0)
, m_currentView(-1)
{
#if 0
    std::cerr << "Ray: " << getpid() << std::endl;
    sleep(10);
#endif

   vassert(s_instance == nullptr);
   s_instance = this;

#ifdef HAVE_ISPC
   m_useIspc = true;

   /* from embree examples: for best performance set FTZ and DAZ flags in MXCSR control and status * register */
   _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
   _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

   m_packetSize = addIntParameter("ray_packet_size", "size of a ray packet (1, 4, 8, or 16)", (Integer)rayPacketSize);
   setParameterRange(m_packetSize, (Integer)1, (Integer)16);
   m_shading = addIntParameter("shading", "shade and light objects", (Integer)m_doShade, Parameter::Boolean);
   m_uvVisParam = addIntParameter("uv_visualization", "show u/v coordinates", (Integer)m_uvVis, Parameter::Boolean);
   m_renderTileSizeParam = addIntParameter("render_tile_size", "edge length of square tiles used during rendering", m_tilesize);
   setParameterRange(m_renderTileSizeParam, (Integer)1, (Integer)16384);
   m_useIspcParam = addIntParameter("use_ispc", "use SIMD implementation with ISPC", (Integer)m_useIspc, Parameter::Boolean);
   m_pointSizeParam = addFloatParameter("point_size", "size of points", RayRenderObject::pointSize);
   setParameterRange(m_pointSizeParam, (Float)0, (Float)1e6);

   rtcInit("verbose=0");
   rtcSetErrorFunction(rtcErrorCallback);
   m_scene = rtcNewScene(RTC_SCENE_DYNAMIC|sceneFlags, intersections);
   rtcCommit(m_scene);
}


RayCaster::~RayCaster() {

   vassert(s_instance == this);
   s_instance = nullptr;
   rtcDeleteScene(m_scene);
   rtcExit();
}


bool RayCaster::parameterChanged(const Parameter *p) {

    m_renderManager.handleParam(p);

    if (p == m_shading) {

       m_doShade = m_shading->getValue();
       m_renderManager.setModified();
    } else if (p == m_uvVisParam) {

        m_uvVis = m_uvVisParam->getValue();
        m_renderManager.setModified();
    } else if (p == m_packetSize) {

        rayPacketSize = m_packetSize->getValue();
        if (rayPacketSize != 1 && rayPacketSize != 4 && rayPacketSize != 8 && rayPacketSize != 16) {
           rayPacketSize = 1;
           m_packetSize->setValue(rayPacketSize);
           std::cerr << "invalid ray packet size, defaulting to " << rayPacketSize << std::endl;
        }
    } else if (p == m_useIspcParam) {

       m_useIspc = m_useIspcParam->getValue();
    } else if (p == m_pointSizeParam) {

       RayRenderObject::pointSize = m_pointSizeParam->getValue();
    }

   return Renderer::parameterChanged(p);
}

template<class RayPacket>
static void setRay(RayPacket &rayPacket, int i, const RTCRay &ray) {

   rayPacket.orgx[i] = ray.org[0];
   rayPacket.orgy[i] = ray.org[1];
   rayPacket.orgz[i] = ray.org[2];

   rayPacket.dirx[i] = ray.dir[0];
   rayPacket.diry[i] = ray.dir[1];
   rayPacket.dirz[i] = ray.dir[2];

   rayPacket.tnear[i] = ray.tnear;
   rayPacket.tfar[i] = ray.tfar;

   rayPacket.geomID[i] = ray.geomID;
   rayPacket.primID[i] = ray.primID;
   rayPacket.instID[i] = ray.instID;

   rayPacket.mask[i] = ray.mask;
   rayPacket.time[i] = ray.time;
}

template<>
void setRay<RTCRay>(RTCRay &packet, int i, const RTCRay &ray) {

    packet = ray;
}


template<class RayPacket>
static RTCRay getRay(const RayPacket &rays, int i) {

   RTCRay ray;

   ray.org[0] = rays.orgx[i];
   ray.org[1] = rays.orgy[i];
   ray.org[2] = rays.orgz[i];

   ray.dir[0] = rays.dirx[i];
   ray.dir[1] = rays.diry[i];
   ray.dir[2] = rays.dirz[i];

   ray.tnear = rays.tnear[i];
   ray.tfar = rays.tfar[i];

   ray.geomID = rays.geomID[i];
   ray.primID = rays.primID[i];
   ray.instID = rays.instID[i];

   ray.mask = rays.mask[i];
   ray.time = rays.time[i];

   ray.Ng[0] = rays.Ngx[i];
   ray.Ng[1] = rays.Ngy[i];
   ray.Ng[2] = rays.Ngz[i];

   ray.u = rays.u[i];
   ray.v = rays.v[i];

   return ray;
}

template<>
RTCRay getRay<RTCRay>(const RTCRay &rays, int i) {
    return rays;
}


static RTCRay makeRay(const Vector3 &origin, const Vector3 &direction, float tNear, float tFar) {

   RTCRay ray;
   ray.org[0] = origin[0];
   ray.org[1] = origin[1];
   ray.org[2] = origin[2];
   ray.dir[0] = direction[0];
   ray.dir[1] = direction[1];
   ray.dir[2] = direction[2];
   ray.tnear = tNear;
   ray.tfar = tFar;
   ray.geomID = RTC_INVALID_GEOMETRY_ID;
   ray.primID = RTC_INVALID_GEOMETRY_ID;
   ray.instID = RTC_INVALID_GEOMETRY_ID;
   ray.mask = RayEnabled;
   ray.time = 0.0f;
   return ray;
}

namespace {
   void packetIntersect(const void *validMask, RTCScene scene, RTCRay &ray) {
      rtcIntersect(scene, ray);
   }

   void packetIntersect(const void *validMask, RTCScene scene, RTCRay4 &ray) {
      rtcIntersect4(validMask, scene, ray);
   }

   void packetIntersect(const void *validMask, RTCScene scene, RTCRay8 &ray) {
      rtcIntersect8(validMask, scene, ray);
   }

   void packetIntersect(const void *validMask, RTCScene scene, RTCRay16 &ray) {
      rtcIntersect16(validMask, scene, ray);
   }
}

template<class RayPacket>
struct RayPacketTraits;

template<>
struct RayPacketTraits<RTCRay> {
    static const int sizeX = 1;
    static const int sizeY = 1;
    static const int size = sizeX*sizeY;
};

template<>
struct RayPacketTraits<RTCRay4> {
    static const int sizeX = 2;
    static const int sizeY = 2;
    static const int size = sizeX*sizeY;
};

template<>
struct RayPacketTraits<RTCRay8> {
    static const int sizeX = 4;
    static const int sizeY = 2;
    static const int size = sizeX*sizeY;
};

template<>
struct RayPacketTraits<RTCRay16> {
    static const int sizeX = 4;
    static const int sizeY = 4;
    static const int size = sizeX*sizeY;
};

struct TileTask {
   TileTask(const RayCaster &rc, const ParallelRemoteRenderManager::PerViewState &vd, int tile=-1)
   : rc(rc)
   , vd(vd)
   , tile(tile)
   , tilesize(rc.m_tilesize)
   , rayPacketSize(rc.rayPacketSize)
   {
   }

   void shadeRay(const RTCRay &ray, int x, int y) const;

   void render(int tile) const;

   template<class RayPacket>
   void packetRender(int tile) const;

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
   const int rayPacketSize;
   int xoff, yoff;
   float *depth;
   unsigned char *rgba;
};


void TileTask::render(int tile) const {

   const int tx = tile%ntx;
   const int ty = tile/ntx;

#ifdef HAVE_ISPC
   if (rc.m_useIspc) {
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
   } else
#endif
   {
      if (rayPacketSize == 16) {
         packetRender<RTCRay16>(tile);
      } else if (rayPacketSize == 8) {
         packetRender<RTCRay8>(tile);
      } else if (rayPacketSize == 4) {
         packetRender<RTCRay4>(tile);
      } else if (rayPacketSize == 1) {
         packetRender<RTCRay>(tile);
      } else {
         vassert("unsupported ray packet size" == 0);
      }
   }
}


template<class RayPacket>
void TileTask::packetRender(int tile) const {
    RTCRay ray;
    RayPacket packet;
    RayPacketTraits<RayPacket> traits;
    const int packetSizeX = traits.sizeX;
    const int packetSizeY = traits.sizeY;
    unsigned RTCORE_ALIGN(32) validMask[traits.size];

    const int tx = tile%ntx;
    const int ty = tile/ntx;
    const int x0=xoff+tx*tilesize, x1=std::min(x0+tilesize, xlim);
    const int y0=yoff+ty*tilesize, y1=std::min(y0+tilesize, ylim);
    const Vector toLowerBottom = lowerBottom - origin;
    for (int yy=y0; yy<y1; yy += packetSizeY) {
       for (int xx=x0; xx<x1; xx += packetSizeX) {

          int idx = 0;
          for (int y=yy; y<yy+packetSizeY; ++y) {
             for (int x=xx; x<xx+packetSizeX; ++x) {

                if (x>=xlim || y>=ylim) {
                   validMask[idx] = RayDisabled;
                } else {
                   validMask[idx] = RayEnabled;
                   const Vector rd = toLowerBottom + x*dx + y*dy;
                   ray = makeRay(origin, rd, tNear, tFar);
                   setRay(packet, idx, ray);
                }

                ++idx;
             }
          }

          packetIntersect(validMask, rc.m_scene, packet);

          idx = 0;
          for (int y=yy; y<yy+packetSizeY; ++y) {
             for (int x=xx; x<xx+packetSizeX; ++x) {

                if (validMask[idx] == RayEnabled) {
                   ray = getRay(packet, idx);
                   shadeRay(ray, x, y);
                }

                ++idx;
             }
          }
       }
    }
}

void TileTask::shadeRay(const RTCRay &ray, int x, int y) const {

    const float ambientFactor = 0.2f;
    const Vector4 specColor(0.4f, 0.4f, 0.4f, 1.0f);
    const float specExp = 16.f;
    const Vector4 ambient(0.2f, 0.2f, 0.2f, 1.0f);
    const bool twoSided = true;

   Vector4 shaded(0, 0, 0, 0);
   float zValue = 1.;
   if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {
      Vector3 viewDir(ray.dir[0], ray.dir[1], ray.dir[2]);
      const Vector3 pos = origin + ray.tfar * viewDir;
      viewDir.normalize();
      const Vector4 pos4(pos[0], pos[1], pos[2], 1);
      const float win2 = depthTransform2.dot(pos4);
      const float win3 = depthTransform3.dot(pos4);
      zValue= (win2/win3+1.f)*0.5f;

      if (rc.m_doShade) {

         vassert(ray.instID < (int)rc.instances.size());
         auto rod = rc.instances[ray.instID];
         vassert(rod->geomId == ray.geomID);

         Vector4 color = rc.m_renderManager.m_defaultColor;
         if (rod->hasSolidColor) {
            for (int c=0; c<4; ++c) {
               color[c] = rod->solidColor[c];
            }
         }
         if (rod->indexBuffer && rod->texData && rod->texCoords) {

            const float &u = ray.u;
            const float &v = ray.v;
            const float w = 1.f - u - v;
            const Index v0 = rod->indexBuffer[ray.primID].v0;
            const Index v1 = rod->indexBuffer[ray.primID].v1;
            const Index v2 = rod->indexBuffer[ray.primID].v2;

            const float tc0 = rod->texCoords[v0];
            const float tc1 = rod->texCoords[v1];
            const float tc2 = rod->texCoords[v2];
            float tc = w*tc0 + u*tc1 + v*tc2;
            //vassert(tc >= 0.f);
            //vassert(tc <= 1.f);
            if (tc < 0.f)
               tc = 0.f;
            if (tc > 1.f)
               tc = 1.f;
            unsigned idx = tc * rod->texWidth;
            if (idx >= rod->texWidth)
               idx = rod->texWidth-1;
            const unsigned char *c = &rod->texData[idx*4];
            for (int i=0; i<4; ++i)
               color[i] = c[i];
         }

         if (rod->lighted) {
            Vector4 ambientColor = color;
            ambientColor.block<3,1>(0,0) *= ambientFactor;
            Vector3 normal(ray.Ng[0], ray.Ng[1], ray.Ng[2]);
            normal.normalize();
            if (twoSided && normal.dot(viewDir) > 0.f)
               normal *= -1.f;
            shaded += ambientColor.cwiseProduct(ambient);
            for (const auto &light: vd.lights) {
               if (light.enabled) {
                  const Vector3 lv = light.isDirectional
                     ? light.transformedPosition.block<3,1>(0,0).normalized()
                     : (light.transformedPosition.block<3,1>(0,0)-pos).normalized();
                  float atten = 1.f;
                  if (!light.isDirectional) {
                     atten = light.attenuation[0];
                     if (light.attenuation[1]>0.f || light.attenuation[2]>0.f) {
                        const float d = (modelView * (light.transformedPosition-pos4)).block<3,1>(0,0).norm();
                        atten += (light.attenuation[1] + light.attenuation[2]*d)*d;
                     }
                     atten = 1.f/atten;
                  }
                  shaded += ambientColor.cwiseProduct(atten*light.ambient);
                  const float ldot = std::max(0.f, normal.dot(lv));
                  shaded += color.cwiseProduct(atten*ldot*light.diffuse);
                  if (ldot > 0.f) {
                     const Vector3 halfway = (lv-viewDir).normalized();
                     const float hdot = std::max(0.f, normal.dot(halfway));
                     if (hdot > 0) {
                        shaded += specColor.cwiseProduct(atten*powf(hdot, specExp)*light.specular);
                     }
                  }
               }
            }
         } else {
            shaded = color;
         }
         for (int i=0; i<4; ++i)
            if (shaded[i] > 255)
               shaded[i] = 255;

      } else {

         shaded = rc.m_renderManager.m_defaultColor;
      }
   }

   depth[y*imgWidth+x] = zValue;
   unsigned char *rgba = this->rgba+(y*imgWidth+x)*4;
   for (int i=0; i<4; ++i) {
      rgba[i] = shaded[i];
   }
}



bool RayCaster::render() {

    // ensure that previous frame is completed
    bool immed_resched = m_renderManager.finishFrame();

    //vistle::StopWatch timer("render");

    const size_t numTimesteps = anim_geometry.size();
    if (!m_renderManager.prepareFrame(numTimesteps)) {
       return immed_resched;
    }

    // switch time steps in embree scene
    if (m_timestep != m_renderManager.timestep()) {
       if (anim_geometry.size() > m_timestep) {
          for (auto &ro: anim_geometry[m_timestep])
             if (ro->data->scene)
                rtcDisable(m_scene, ro->data->instId);
       }
       m_timestep = m_renderManager.timestep();
       if (anim_geometry.size() > m_timestep) {
          for (auto &ro: anim_geometry[m_timestep])
             if (ro->data->scene)
                rtcEnable(m_scene, ro->data->instId);
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

#if 1
       IceTImage img = icetDrawFrame(proj, mv, bg);
#else
       IceTInt viewport[4] = {0, 0, width, height};
       renderRect(proj, mv, bg, viewport, image);
       IceTImage img = icetCompositeImage(mapColor[pbo], mapDepth[pbo], viewport, proj, mv, bg);
#endif

       m_renderManager.finishCurrentView(img, false);
    }
    m_currentView = -1;

    return true;
}

void RayCaster::renderRect(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image) {

   //StopWatch timer("RayCaster::render()");

#if 0
   IceTSizeType width = icetImageGetWidth(image);
   IceTSizeType height = icetImageGetHeight(image);
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

   vistle::Matrix4 MV, P;
   for (int i=0; i<4; ++i) {
      for (int j=0; j<4; ++j) {
         MV(i,j) = mv[j*4+i];
         P(i,j) = proj[j*4+i];
      }
   }

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
   renderTile.rgba = icetImageGetColorub(image);
   renderTile.depth = icetImageGetDepthf(image);
   renderTile.depthTransform2 = depthTransform.row(2);
   renderTile.depthTransform3 = depthTransform.row(3);
   renderTile.ntx = ntx;
   renderTile.xoff = viewport[0];
   renderTile.yoff = viewport[1];
   renderTile.xlim = viewport[0] + viewport[2];
   renderTile.ylim = viewport[1] + viewport[3];
   renderTile.imgWidth = icetImageGetWidth(image);
   renderTile.imgHeight = icetImageGetHeight(image);
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

   m_renderManager.updateRect(m_currentView, viewport, image);

   int err = rtcGetError();
   if (err != 0) {
      std::cerr << "RTC error: " << err << std::endl;
   }
}


void RayCaster::removeObject(boost::shared_ptr<RenderObject> vro) {

   auto ro = boost::static_pointer_cast<RayRenderObject>(vro);
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


boost::shared_ptr<RenderObject> RayCaster::addObject(int sender, const std::string &senderPort,
                                 vistle::Object::const_ptr container,
                                 vistle::Object::const_ptr geometry,
                                 vistle::Object::const_ptr normals,
                                 vistle::Object::const_ptr colors,
                                 vistle::Object::const_ptr texture) {

   boost::shared_ptr<RayRenderObject> ro(new RayRenderObject(sender, senderPort, container, geometry, normals, colors, texture));

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

      float identity[16];
      for (int i=0; i<16; ++i) {
         identity[i] = (i/4 == i%4) ? 1. : 0.;
      }
      rtcSetTransform(m_scene, rod->instId, RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, identity);
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

   RayCaster::the().renderRect(proj, mv, bg, viewport, image);
}


MODULE_MAIN(RayCaster)
