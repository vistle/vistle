//#undef NDEBUG

#include <IceT.h>
#include <IceTMPI.h>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <boost/mpi.hpp>

#include <renderer/renderer.h>
#include <core/geometry.h>
#include <core/texture1d.h>
#include <core/message.h>
#include <core/assert.h>

#include <util/stopwatch.h>

#include <rhr/vncserver.h>
#include <renderer/vnccontroller.h>
#include <renderer/parrendmgr.h>
#include "rayrenderobject.h"


#ifdef USE_TBB
#include <tbb/parallel_for.h>
#else
//#include <future>
#endif

namespace mpi = boost::mpi;

using namespace vistle;

class RayCaster: public vistle::Renderer {

   static RayCaster *s_instance;

 public:
   struct RGBA {
      unsigned char r, g, b, a;
   };

   RayCaster(const std::string &shmname, const std::string &name, int moduleId);
   ~RayCaster();

   static RayCaster &the() {

      return *s_instance;
   }

   void render();

   bool parameterChanged(const Parameter *p);

   ParallelRemoteRenderManager m_renderManager;

   int rayPacketSize;

   // parameters
   IntParameter *m_renderTileSizeParam;
   int m_tilesize;
   IntParameter *m_shading;
   bool m_doShade;

   // object lifetime management
   boost::shared_ptr<RenderObject> addObject(int sender, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture) override;

   void removeObject(boost::shared_ptr<RenderObject> ro) override;

   std::vector<boost::shared_ptr<RayRenderObject>> instances;

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
, rayPacketSize(MaxPacketSize)
, m_tilesize(64)
, m_doShade(true)
, m_timestep(0)
, m_currentView(-1)
{
#if 0
    std::cerr << "Ray: " << getpid() << std::endl;
    sleep(10);
#endif

   vassert(s_instance == nullptr);
   s_instance = this;

   m_shading = addIntParameter("shading", "shade and light objects", (Integer)m_doShade, Parameter::Boolean);
   m_renderTileSizeParam = addIntParameter("render_tile_size", "edge length of square tiles used during rendering", m_tilesize);
   setParameterRange(m_renderTileSizeParam, (Integer)1, (Integer)16384);

   rtcInit("verbose=0");
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


struct TileTask {
   TileTask(const RayCaster &rc, const ParallelRemoteRenderManager::PerViewState &vd, int tile=-1)
   : rc(rc)
   , vd(vd)
   , tile(tile)
   , tilesize(rc.m_tilesize)
   , rayPacketSize(rc.rayPacketSize)
   , packetSizeX((rayPacketSize+1)/2)
   , packetSizeY(rayPacketSize/packetSizeX)
   {

   }

   void shadeRay(const RTCRay &ray, int x, int y) const;

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
   float tNear, tFar;
   const int rayPacketSize;
   const int packetSizeX, packetSizeY;
   int xoff, yoff;
   float *depth;
   unsigned char *rgba;
};


void TileTask::render(int tile) const {

   unsigned RTCORE_ALIGN(32) validMask[MaxPacketSize];
   RTCRay ray;
   RTCRay4 ray4;
   RTCRay8 ray8;

   const int tx = tile%ntx;
   const int ty = tile/ntx;
   const int x0=xoff+tx*tilesize, x1=x0+tilesize;
   const int y0=yoff+ty*tilesize, y1=y0+tilesize;
   const Vector toLowerBottom = lowerBottom - origin;
   for (int yy=y0; yy<y1; yy += packetSizeY) {
      for (int xx=x0; xx<x1; xx += packetSizeX) {

         if (yy >= ylim)
            continue;
         if (xx >= xlim)
            continue;
         
         int idx = 0;
         for (int y=yy; y<yy+packetSizeY; ++y) {
            for (int x=xx; x<xx+packetSizeX; ++x) {

               const Vector rd = toLowerBottom + x*dx + y*dy;
               ray = makeRay(origin, rd, tNear, tFar);

               if (x>=xlim || y>=ylim) {
                  validMask[idx] = RayDisabled;
               } else {
                  validMask[idx] = RayEnabled;
               }

               if (rayPacketSize==8) {
                  setRay(ray8, idx, ray);
               } else if (rayPacketSize==4) {
                  setRay(ray4, idx, ray);
               }

            ++idx;
            }
         }

         if (rayPacketSize==8) {
            rtcIntersect8(validMask, rc.m_scene, ray8);
         } else if (rayPacketSize==4) {
            rtcIntersect4(validMask, rc.m_scene, ray4);
         } else {
            rtcIntersect(rc.m_scene, ray);
         }

         idx = 0;
         for (int y=yy; y<yy+packetSizeY; ++y) {
            for (int x=xx; x<xx+packetSizeX; ++x) {

               if (validMask[idx] == RayEnabled) {
                  if (rayPacketSize==8) {
                     ray = getRay(ray8, idx);
                  } else if (rayPacketSize==4) {
                     ray = getRay(ray4, idx);
                  }
                  shadeRay(ray, x, y);
               }

            ++idx;
            }
         }
      }
   }
}

void TileTask::shadeRay(const RTCRay &ray, int x, int y) const {

   Vector4 shaded(0, 0, 0, 0);
   float zValue = 1.;
   if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {
      //std::cerr << "intersection at " << x << "," << y << ": " << ray.tfar*zNear << std::endl;
      if (rc.m_doShade) {

         vassert(ray.instID < (int)rc.instances.size());
         auto ro = rc.instances[ray.instID];
         vassert(ro->geomId == ray.geomID);

         Vector4 color = rc.m_renderManager.m_defaultColor;
         if (ro->hasSolidColor) {
            color = ro->solidColor;
         }
         if (ro->indexBuffer && ro->texData && ro->texCoords) {

            const float &u = ray.u;
            const float &v = ray.v;
            const float w = 1.f - u - v;
            const Index v0 = ro->indexBuffer[ray.primID].v0;
            const Index v1 = ro->indexBuffer[ray.primID].v1;
            const Index v2 = ro->indexBuffer[ray.primID].v2;

            const float tc0 = ro->texCoords[v0];
            const float tc1 = ro->texCoords[v1];
            const float tc2 = ro->texCoords[v2];
            float tc = w*tc0 + u*tc1 + v*tc2;
            //vassert(tc >= 0.f);
            //vassert(tc <= 1.f);
            if (tc < 0.f)
               tc = 0.f;
            if (tc > 1.f)
               tc = 1.f;
            unsigned idx = tc * ro->texWidth;
            if (idx >= ro->texWidth)
               idx = ro->texWidth-1;
            const unsigned char *c = &ro->texData[idx*4];
            color[0] = c[0];
            color[1] = c[1];
            color[2] = c[2];
            color[3] = c[3];
         }

         const Vector3 pos = Vector3(ray.org[0], ray.org[1], ray.org[2])
            + ray.tfar * Vector3(ray.dir[0], ray.dir[1], ray.dir[2]);
         const Vector4 pos4(pos[0], pos[1], pos[2], 1);

         Vector3 normal(ray.Ng[0], ray.Ng[1], ray.Ng[2]);
         normal.normalize();
         for (const auto &light: vd.lights) {
            Vector ldir = light.transformedPosition - pos;
            ldir.normalize();
            const float ldot = fabs(normal.dot(ldir));
            shaded += color.cwiseProduct(light.ambient + ldot*light.diffuse);
         }
         for (int i=0; i<4; ++i)
            if (shaded[i] > 255)
               shaded[i] = 255;

         float win2 = depthTransform2.dot(pos4);
         float win3 = depthTransform3.dot(pos4);
         zValue= (win2/win3+1.f)*0.5f;
      } else {

         shaded = rc.m_renderManager.m_defaultColor;
         zValue = 0.;
      }
   }

   depth[y*imgWidth+x] = zValue;
   unsigned char *rgba = this->rgba+(y*imgWidth+x)*4;
   for (int i=0; i<4; ++i) {
      rgba[i] = shaded[i];
   }
}



void RayCaster::render() {

   //vistle::StopWatch timer("render");

   const size_t numTimesteps = anim_geometry.size();
   if (m_renderManager.prepareFrame(numTimesteps)) {

      // switch time steps in embree scene
      if (m_timestep != m_renderManager.timestep()) {
         if (anim_geometry.size() > m_timestep) {
            for (auto &ro: anim_geometry[m_timestep])
               rtcDisable(m_scene, ro->instId);
         }
         m_timestep = m_renderManager.timestep();
         if (anim_geometry.size() > m_timestep) {
            for (auto &ro: anim_geometry[m_timestep])
               rtcEnable(m_scene, ro->instId);
         }
         rtcCommit(m_scene);
      }

      for (size_t i=0; i<m_renderManager.numViews(); ++i) {
         m_renderManager.setCurrentView(i);
         m_currentView = i;

         IceTDouble proj[16], mv[16];
         m_renderManager.getViewMat(i, mv);
         m_renderManager.getProjMat(i, proj);
         IceTFloat bg[4] = { 0., 0., 0., 0. };

         IceTImage img = icetDrawFrame(proj, mv, bg);

         m_renderManager.finishCurrentView(img);
      }
      m_currentView = -1;
   }
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
   const Vector ro(ro4[0], ro4[1], ro4[2]);

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
      std::cerr << "RTC error: " << rtcGetError() << std::endl;
   }
}


void RayCaster::removeObject(boost::shared_ptr<RenderObject> vro) {

   auto ro = boost::static_pointer_cast<RayRenderObject>(vro);

   rtcDeleteGeometry(m_scene, ro->instId);
   rtcCommit(m_scene);

   instances[ro->instId].reset();

   int t = ro->timestep;
   auto &objlist = t>=0 ? anim_geometry[t] : static_geometry;

   if (t == -1 || t == m_timestep) {
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
      if (anim_geometry.size() <= t)
         anim_geometry.resize(t+1);
      anim_geometry[t].push_back(ro);
   }

   ro->instId = rtcNewInstance(m_scene, ro->scene);
   if (instances.size() <= ro->instId)
      instances.resize(ro->instId+1);
   vassert(!instances[ro->instId]);
   instances[ro->instId] = ro;

   float identity[16];
   for (int i=0; i<16; ++i) {
      identity[i] = (i/4 == i%4) ? 1. : 0.;
   }
   rtcSetTransform(m_scene, ro->instId, RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, identity);
   if (t < 0 || t == m_timestep) {
      rtcEnable(m_scene, ro->instId);
      m_renderManager.setModified();
   } else {
      rtcDisable(m_scene, ro->instId);
   }
   rtcCommit(m_scene);

   m_renderManager.addObject(ro);

   return ro;
}

void  RayCaster::drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image) {

   RayCaster::the().renderRect(proj, mv, bg, viewport, image);
}


MODULE_MAIN(RayCaster)
