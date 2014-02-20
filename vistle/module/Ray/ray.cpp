#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <module/renderer.h>
#include <core/geometry.h>
#include <core/polygons.h>
#include <core/triangles.h>
#include <core/vector.h>
#include <core/texture1d.h>

#include <util/stopwatch.h>

#include "framebuffer.h"
#include "vncserver.h"
#include "renderobject.h"

#include <core/assert.h>

//#define SERIAL
//#undef USE_TBB

#ifdef SERIAL
#ifdef USE_TBB
#undef USE_TBB
#endif
#endif

#ifndef SERIAL
#ifdef USE_TBB
#include <tbb/parallel_for.h>
#else
#include <future>
#endif
#endif

using namespace vistle;

class RayCaster: public vistle::Renderer {

 public:
   RayCaster(const std::string &shmname, int rank, int size, int moduleId);
   ~RayCaster();

   bool compute();
   void render();

   void updateBounds();

   void addInputObject(vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr texture);
   bool addInputObject(const std::string & portName,
         vistle::Object::const_ptr object);

   bool removeObject(RenderObject *ro);
   void removeAllCreatedBy(int creator);

   typedef std::map<unsigned int, RenderObject *> InstanceMap;
   InstanceMap instances;

   void shadeRay(const RTCRay &ray, int x, int y) const;

   struct Creator {
      Creator(int id, const std::string &basename)
      : id(id)
      , age(0)
      {
         std::stringstream s;
         s << basename << "_" << id;
         name = s.str();

      }
      int id;
      int age;
      std::string name;
   };
   typedef std::map<int, Creator> CreatorMap;
   CreatorMap creatorMap;

   std::vector<RenderObject *> static_geometry;
   std::vector<std::vector<RenderObject *>> anim_geometry;

   int width, height;
   int ntx, nty;
   int packetSize;
   boost::shared_ptr<VncServer> vnc;
   boost::shared_ptr<FrameBuffer> framebuffer;

   RTCScene m_scene;

   int m_timestep;
   Scalar zNear, zFar;
   float tNear, tFar;
   Matrix4 rayTransform;
   Matrix4 depthTransform;
   Vector3 lowerBottom, dx, dy;
   const int tilesize = 64;
};





MODULE_MAIN(RayCaster)

using namespace vistle;

RayCaster::RayCaster(const std::string &shmname, int rank, int size, int moduleId)
: Renderer("RayCaster", shmname, rank, size, moduleId)
, width(1024)
, height(768)
, packetSize(MaxPacketSize)
, vnc(new VncServer(width, height))
, framebuffer(new FrameBuffer(vnc->width(), vnc->height(), (FrameBuffer::uchar4 *)vnc->rgba(), vnc->depth()))
, m_timestep(0)
, zNear(1)
, zFar(10)
{
   rtcInit("verbose=1");
   m_scene = rtcNewScene(RTC_SCENE_DYNAMIC|sceneFlags, intersections);

   rtcCommit(m_scene);

   //MODULE_DEBUG(RayCaster);
}

RayCaster::~RayCaster() {

   rtcDeleteScene(m_scene);
   rtcExit();
}

bool RayCaster::compute() {

   return true;
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

void RayCaster::shadeRay(const RTCRay &ray, int x, int y) const {

   if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {
      //std::cerr << "intersection at " << x << "," << y << ": " << ray.tfar*zNear << std::endl;
      auto it = instances.find(ray.instID);
      const RenderObject *ro = it->second;
      assert(ro->geomId == ray.geomID);
      const float &u = ray.u;
      const float &v = ray.v;
      const float w = 1.f - u - v;
      FrameBuffer::uchar4 rgba = { 128, 128, 128, 255 };
      if (ro->indexBuffer) {
         Index v0 = ro->indexBuffer[ray.primID].v0;
         Index v1 = ro->indexBuffer[ray.primID].v1;
         Index v2 = ro->indexBuffer[ray.primID].v2;

         if (auto tex = Texture1D::as(ro->texture)) {
            float tc0 = tex->coords()[v0];
            float tc1 = tex->coords()[v1];
            float tc2 = tex->coords()[v2];
            float tc = u*tc0 + v*tc1 + w*tc2;
            assert(tc >= 0.f);
            assert(tc <= 1.f);
            unsigned idx = tc * tex->getWidth();
            if (idx >= tex->getWidth())
               idx = tex->getWidth()-1;
            memcpy(&rgba, &tex->pixels()[idx*4], sizeof(rgba));
         }
      }
      Vector3 org(ray.org[0], ray.org[1], ray.org[2]);
      Vector3 dir(ray.dir[0], ray.dir[1], ray.dir[2]);
      Vector3 pos = org+ray.tfar*dir;
      Vector4 pos4(pos[0], pos[1], pos[2], 1);
      Vector4 color(rgba.r, rgba.g, rgba.b, rgba.a);
      Vector3 normal(ray.Ng[0], ray.Ng[1], ray.Ng[2]);
      normal /= normal.norm();
      Vector4 lit(0, 0, 0, 0);
      for (const auto &light: vnc->lights) {
         lit += color.cwiseProduct(light.ambient);
#if 1
         Vector4 ldir4 = light.transformedPosition - pos4;
         Vector ldir(ldir4[0], ldir4[1], ldir4[2]);
         //ldir /= ldir4[3];
         ldir /= ldir.norm();
         float ldot = normal.dot(ldir);
         if (ldot < 0)
            ldot = -ldot;
         lit += ldot * color.cwiseProduct(light.diffuse);
#endif
      }
      for (int i=0; i<4; ++i)
         if (lit[i] > 255)
            lit[i] = 255;
      rgba.r = lit[0];
      rgba.g = lit[1];
      rgba.b = lit[2];
      rgba.a = lit[3];
      //Vector4 win = vnc->projMat() * rayTransform.inverse() * pos4;
      Vector4 win = depthTransform * pos4;
      float d = (win[2]/win[3]+1.f)*0.5f;
      framebuffer->rgba(x, y) = rgba;
      framebuffer->depth(x, y) = d;
   } else {
      framebuffer->rgba(x, y) = { 0, 0, 0, 0 };
      framebuffer->depth(x, y) = 1.;
   }
}

struct TileTask {
#ifdef USE_TBB
   TileTask(const RayCaster &rc)
#else
   TileTask(const RayCaster &rc, int tile)
#endif
   : rc(rc)
#ifndef USE_TBB
   , tile(tile)
#endif
   , tilesize(rc.tilesize)
   , width(rc.width)
   , height(rc.height)
   , rayTransform(rc.rayTransform)
   , depthTransform(rc.depthTransform)
   , lowerBottom(rc.lowerBottom)
   , dx(rc.dx)
   , dy(rc.dy)
   , tNear(rc.tNear)
   , tFar(rc.tFar)
   , packetSize(rc.packetSize)
   {

   }

#ifdef USE_TBB
   void operator()(int tile) const;
#else
   void operator()() const;
#endif
   const RayCaster &rc;
#ifndef USE_TBB
   const int tile;
#endif
   const int tilesize;
   const int width, height;
   const Matrix4 rayTransform;
   const Matrix4 depthTransform;
   const Vector3 lowerBottom, dx, dy;
   const float tNear, tFar;
   const int packetSize;
};

#ifdef USE_TBB
void TileTask::operator()(int tile) const
#else
void TileTask::operator()() const
#endif
{

   int RTCORE_ALIGN(32) validMask[MaxPacketSize];
   RTCRay4 ray4;
   RTCRay8 ray8;

   const int tx = tile%rc.ntx;
   const int ty = tile/rc.ntx;
   const int x0=tx*tilesize, x1=x0+tilesize;
   const int y0=ty*tilesize, y1=y0+tilesize;
   for (int y=y0; y<y1; ++y) {
      for (int x=x0; x<x1; ++x) {

         RTCRay ray;
         if (x>=width || y>=height) {

            validMask[x%packetSize] = RayDisabled;
            ray.mask = RayDisabled;
            ray.primID = RTC_INVALID_GEOMETRY_ID;
            ray.geomID = RTC_INVALID_GEOMETRY_ID;
            ray.instID = RTC_INVALID_GEOMETRY_ID;
         } else {

            validMask[x%packetSize] = RayEnabled;

            Vector4 ro4 = rayTransform * Vector4(0, 0, 0, 1);
            Vector rl3 = lowerBottom+x*dx+y*dy;
            Vector4 rl4 = rayTransform * Vector4(rl3[0], rl3[1], rl3[2], 1);
            rl4 /= rl4[3];

            Vector ro(ro4[0]/ro4[3], ro4[1]/ro4[3], ro4[2]/ro4[3]);
            Vector rl(rl4[0], rl4[1], rl4[2]);
            Vector rd = rl - ro;

            ray = makeRay(ro, rd, tNear, tFar);
         }

         if (packetSize==8) {
            setRay(ray8, x%packetSize, ray);
            if (x%packetSize == packetSize-1) {
               rtcIntersect8(validMask, rc.m_scene, ray8);
               for (int i=0; i<packetSize; ++i) {
                  ray = getRay(ray8, packetSize-1-i);
                  rc.shadeRay(ray, x-i, y);
               }
            }
         } else if (packetSize==4) {
            setRay(ray4, x%packetSize, ray);
            if (x%packetSize == packetSize-1) {
               rtcIntersect4(validMask, rc.m_scene, ray4);
               for (int i=0; i<packetSize; ++i) {
                  ray = getRay(ray4, packetSize-1-i);
                  rc.shadeRay(ray, x-i, y);
               }
            }
         } else {
            rtcIntersect(rc.m_scene, ray);
            rc.shadeRay(ray, x, y);
         }
      }
   }
}


void RayCaster::render() {

   if (m_timestep != vnc->timestep()) {
      if (anim_geometry.size() > m_timestep) {
         for (auto &ro: anim_geometry[m_timestep])
            rtcDisable(m_scene, ro->instId);
      }
   }

   m_timestep = vnc->timestep();

   if (anim_geometry.size() > m_timestep) {
      for (auto &ro: anim_geometry[m_timestep])
         rtcEnable(m_scene, ro->instId);
   }
   rtcCommit(m_scene);

   vnc->preFrame();
   width = vnc->width();
   height = vnc->height();
   framebuffer->resize(width, height, (FrameBuffer::uchar4 *)vnc->rgba(), vnc->depth());

   const int ts = tilesize;
   const int wt = ((width+ts-1)/ts)*ts;
   const int ht = ((height+ts-1)/ts)*ts;
   ntx = wt/ts;
   nty = ht/ts;

   const auto &screen = vnc->screen();
   {
      //StopWatch timer("RayCaster::render()");

      vistle::Matrix4 MVP = vnc->projMat() * vnc->viewMat();
      auto inv = MVP.inverse();
      const Vector4 origin4 = inv * Vector4(0, 0, 0, 1);
      const Vector3 origin(origin4[0]/origin4[3], origin4[1]/origin4[3], origin4[2]/origin4[3]);
      vistle::Matrix4 invProj = vnc->projMat().inverse();
      Vector4 lbn4 = invProj * Vector4(-1, 1, -1, 1);
      Vector lbn(lbn4[0], lbn4[1], lbn4[2]);
      lbn /= lbn4[3];
      Vector4 lbf4 = invProj * Vector4(-1, 1, 1, 1);
      Vector lbf(lbf4[0], lbf4[1], lbf4[2]);
      lbf /= lbf4[3];
      zNear = -lbn[2];
      zFar = -lbf[2];
      tNear = 1., tFar = zFar/zNear;
      vassert(zNear > 0.);
      vassert(zFar > zNear);
      vassert(tFar > 1.);

      //std::cerr << "near: " << zNear << ", far: " << zFar << std::endl;
      Vector4 rbn4 = invProj * Vector4(1, 1, -1, 1);
      Vector rbn(rbn4[0], rbn4[1], rbn4[2]);
      rbn /= rbn4[3];
      Vector4 ltn4 = invProj * Vector4(-1, -1, -1, 1);
      Vector ltn(ltn4[0], ltn4[1], ltn4[2]);
      ltn /= ltn4[3];

      lowerBottom = lbn;
      dx = (rbn-lbn)/width;
      dy = (ltn-lbn)/height;

      rayTransform = (vnc->viewMat() * vnc->transformMat() * vnc->scaleMat()).inverse();
      depthTransform = vnc->projMat() * vnc->viewMat() * vnc->transformMat() * vnc->scaleMat();
      Matrix4 lightTransform = vnc->viewMat().inverse();

      for (auto &light: vnc->lights) {
         light.transformedPosition = lightTransform * light.position;
      }

#ifdef SERIAL
      for (int t=0; t<ntx*nty; ++t) {
         TileTask tile(*this, t);
         tile();
      }
#else
#ifdef USE_TBB
      TileTask tiles(*this);
      tbb::parallel_for(0, ntx*nty, tiles);
#else
      std::vector<std::future<void>> tiles;

      for (int t=0; t<ntx*nty; ++t) {
         TileTask tile(*this, t);
         tiles.emplace_back(std::async(std::launch::async, tile));
      }

      for (auto &task: tiles) {
         task.get();
      }
#endif
#endif
   }

   vnc->invalidate(0, 0, vnc->width(), vnc->height());
   vnc->postFrame();

   int err = rtcGetError();
   if (err != 0) {
      std::cerr << "RTC error: " << rtcGetError() << std::endl;
   }
}

void RayCaster::removeAllCreatedBy(int creator) {

   std::vector<RenderObject *> toRemove;

   for (RenderObject *ro: static_geometry) {
      if (ro->container->getCreator() == creator)
         toRemove.push_back(ro);
   }
   for (size_t i=0; i<anim_geometry.size(); ++i) {
      for (RenderObject *ro: anim_geometry[i]) {
         if (ro->container->getCreator() == creator)
            toRemove.push_back(ro);
      }
   }
   while(!toRemove.empty()) {
      removeObject(toRemove.back());
      toRemove.pop_back();
   }
}

void RayCaster::updateBounds() {

   const Scalar smax = std::numeric_limits<Scalar>::max();
   Vector3 min(smax, smax, smax), max(-smax, -smax, -smax);

   for (RenderObject *ro: static_geometry) {
      for (int i=0; i<3; ++i) {
         if (ro->bMin[i] < min[i])
            min[i] = ro->bMin[i];
         if (ro->bMax[i] > max[i])
            max[i] = ro->bMax[i];
      }
   }
   for (int ts=0; ts<anim_geometry.size(); ++ts) {
      for (RenderObject *ro: anim_geometry[ts]) {
         for (int i=0; i<3; ++i) {
            if (ro->bMin[i] < min[i])
               min[i] = ro->bMin[i];
            if (ro->bMax[i] > max[i])
               max[i] = ro->bMax[i];
         }
      }
   }

#if 0
   std::cerr << "<<< BOUNDS <<<" << std::endl;
   std::cerr << "min: " << min << std::endl;
   std::cerr << "max: " << max << std::endl;
   std::cerr << ">>> BOUNDS >>>" << std::endl;
#endif

   Vector center = 0.5*(min+max);
   Scalar radius = (max-min).norm()*0.5;

   vnc->setBoundingSphere(center, radius);
}

bool RayCaster::removeObject(RenderObject *ro) {

   instances.erase(ro->instId);

   int t = ro->t;
   auto &objlist = t>=0 ? anim_geometry[t] : static_geometry;

   bool ret = false;
   auto it = std::find(objlist.begin(), objlist.end(), ro);
   if (it != objlist.end()) {
      std::swap(*it, objlist.back());
      objlist.pop_back();
      ret = true;
   }

   while (!anim_geometry.empty() && anim_geometry.back().empty())
      anim_geometry.pop_back();

   rtcDeleteGeometry(m_scene, ro->instId);
   rtcCommit(m_scene);

   vnc->setNumTimesteps(anim_geometry.size());
   updateBounds();

   delete ro;

   return ret;
}

void RayCaster::addInputObject(vistle::Object::const_ptr container,
                                 vistle::Object::const_ptr geometry,
                                 vistle::Object::const_ptr colors,
                                 vistle::Object::const_ptr normals,
                                 vistle::Object::const_ptr texture) {

   int creatorId = container->getCreator();
   CreatorMap::iterator it = creatorMap.find(creatorId);
   if (it != creatorMap.end()) {
      if (it->second.age < container->getExecutionCounter()) {
         std::cerr << "removing all created by " << creatorId << ", age " << container->getExecutionCounter() << ", was " << it->second.age << std::endl;
         removeAllCreatedBy(creatorId);
      } else if (it->second.age > container->getExecutionCounter()) {
         std::cerr << "received outdated object created by " << creatorId << ", age " << container->getExecutionCounter() << ", was " << it->second.age << std::endl;
         return;
      }
   } else {
      std::string name = getModuleName(container->getCreator());
      it = creatorMap.insert(std::make_pair(creatorId, Creator(container->getCreator(), name))).first;
   }
   Creator &creator = it->second;
   creator.age = container->getExecutionCounter();

   RenderObject *ro = new RenderObject(container, geometry, colors, normals, texture);

   int t = geometry->getTimestep();
   if (t < 0 && colors) {
      t = colors->getTimestep();
   }
   if (t < 0 && normals) {
      t = normals->getTimestep();
   }
   if (t < 0 && texture) {
      t = texture->getTimestep();
   }
   ro->t = t;

   if (t == -1) {
      static_geometry.push_back(ro);
   } else {
      if (anim_geometry.size() <= t)
         anim_geometry.resize(t+1);
      anim_geometry[t].push_back(ro);
   }

   ro->instId = rtcNewInstance(m_scene, ro->scene);
   float identity[16];
   for (int i=0; i<16; ++i) {
      identity[i] = (i/4 == i%4) ? 1. : 0.;
   }
   rtcSetTransform(m_scene, ro->instId, RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, identity);
   if (t < 0 || t == m_timestep) {
      rtcEnable(m_scene, ro->instId);
   } else {
      rtcDisable(m_scene, ro->instId);
   }
   rtcCommit(m_scene);

   instances[ro->instId] = ro;

   vnc->setNumTimesteps(anim_geometry.size());
   updateBounds();
}


bool RayCaster::addInputObject(const std::string & portName,
                                 vistle::Object::const_ptr object) {

#if 0
   std::cout << "++++++RayCaster addInputObject " << object->getType()
             << " creator " << object->getCreator()
             << " exec " << object->getExecutionCounter()
             << " block " << object->getBlock()
             << " timestep " << object->getTimestep() << std::endl;
#endif

   switch (object->getType()) {
      case vistle::Object::GEOMETRY: {

         vistle::Geometry::const_ptr geom = vistle::Geometry::as(object);

#if 0
         std::cerr << "   Geometry: [ "
            << (geom->geometry()?"G":".")
            << (geom->colors()?"C":".")
            << (geom->normals()?"N":".")
            << (geom->texture()?"T":".")
            << " ]" << std::endl;
#endif
         addInputObject(object, geom->geometry(), geom->colors(), geom->normals(),
                        geom->texture());

         break;
      }

      case vistle::Object::PLACEHOLDER:
      default: {
         if (object->getType() == vistle::Object::PLACEHOLDER
               || true /*VistleGeometryGenerator::isSupported(object->getType())*/)
            addInputObject(object, object, vistle::Object::ptr(), vistle::Object::ptr(), vistle::Object::ptr());

         break;
      }
   }

   return true;
}


