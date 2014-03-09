//#undef NDEBUG

#include <IceT.h>
#include <IceTMPI.h>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <boost/mpi.hpp>

#include <module/renderer.h>
#include <core/geometry.h>
#include <core/texture1d.h>
#include <core/message.h>
#include <core/assert.h>

#include <util/stopwatch.h>

#include "vncserver.h"
#include "renderobject.h"


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

   RayCaster(const std::string &shmname, int rank, int size, int moduleId);
   ~RayCaster();

   static RayCaster &the() {

      return *s_instance;
   }

   bool compute();
   void render();

   bool parameterChanged(Parameter *p);

   IntParameter *m_continuousRendering;
   IntParameter *m_colorRank;
   IntParameter *m_vncOnSlaves;
   IntParameter *m_vncBasePort;
   IntParameter *m_shading;
   IntParameter *m_depthQuant;
   IntParameter *m_depthSnappy;
   IntParameter *m_depthPrec;

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

   std::vector<RenderObject *> instances;

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
   int packetSize;
   boost::shared_ptr<VncServer> vnc;

   RTCScene m_scene;

   int m_timestep;

   const int tilesize = 64;

   IceTContext m_icetCtx;
   IceTCommunicator m_icetComm;
   int m_displayRank;
   bool m_initIcet;
   Vector3 boundMin, boundMax;
   int m_updateBounds;
   int m_doRender;
   bool m_doShade;
   bool m_snappy;
   bool m_quant;
   Integer m_prec;

   int rootRank() const {

      return m_displayRank==-1 ? 0 : m_displayRank;
   }

   Vector4 m_defaultColor;

   std::vector<VncServer::Light> lights;

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
   std::vector<DisplayTile> icetTiles;

   struct SharedState {

      int timestep;
      int numTimesteps;
      int width, height;
      Vector3 bMin, bMax;
      Matrix4 view;
      Matrix4 proj;

      SharedState()
      : timestep(-1)
      , numTimesteps(-1)
      , width(0)
      , height(0)
      {

         view.Identity();
         proj.Identity();
      }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & timestep;
         ar & numTimesteps;
         ar & width;
         ar & height;
         ar & view;
         ar & proj;
      }
   };

   SharedState state;

   void composite();
   static void drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
   void renderRect(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image);
};

RayCaster *RayCaster::s_instance = nullptr;


RayCaster::RayCaster(const std::string &shmname, int rank, int size, int moduleId)
: Renderer("RayCaster", shmname, rank, size, moduleId)
, width(1024)
, height(768)
, packetSize(MaxPacketSize)
, m_timestep(0)
, m_displayRank(0)
, m_initIcet(true)
, m_updateBounds(1)
, m_doRender(1)
, m_doShade(true)
, m_snappy(true)
, m_quant(true)
, m_prec(24)
{
   vassert(s_instance == nullptr);
   s_instance = this;

   m_vncBasePort = addIntParameter("vnc_base_port", "listen port for VNC server", 31590);
   setParameterRange(m_vncBasePort, (Integer)1, (Integer)((1<<16)-1));
   m_vncOnSlaves = addIntParameter("vnc_on_slaves", "start VNC servers on slave ranks", 0, Parameter::Boolean);
   m_continuousRendering = addIntParameter("continuous_rendering", "render even though nothing has changed", 0, Parameter::Boolean);
   m_colorRank = addIntParameter("color_rank", "different colors on each rank", 0, Parameter::Boolean);
   m_shading = addIntParameter("shading", "shade and light objects", (Integer)m_doShade, Parameter::Boolean);
   m_depthSnappy = addIntParameter("depth_snappy", "compress depth with SNAPPY", (Integer)m_snappy, Parameter::Boolean);
   m_depthQuant = addIntParameter("depth_quant", "DXT-like depth quantization", (Integer)m_quant, Parameter::Boolean);
   m_depthPrec = addIntParameter("depth_prec", "quantized depth precision", (Integer)(m_prec==24), Parameter::Choice);
   std::vector<std::string> choices;
   choices.push_back("16 bit + 2 bit/pixel");
   choices.push_back("24 bit + 3 bit/pixel");
   setParameterChoices(m_depthPrec, choices);

   m_defaultColor = Vector4(127, 127, 127, 255);

   if (m_displayRank==-1 || rank==m_displayRank)
      vnc.reset(new VncServer(width, height, m_vncBasePort->getValue()));

   icetTiles.resize(size);

   rtcInit("verbose=1");
   m_scene = rtcNewScene(RTC_SCENE_DYNAMIC|sceneFlags, intersections);

   rtcCommit(m_scene);

   m_icetComm = icetCreateMPICommunicator(MPI_COMM_WORLD);
   m_icetCtx = icetCreateContext(m_icetComm);
}


RayCaster::~RayCaster() {

   vassert(s_instance == this);
   s_instance = nullptr;
   rtcDeleteScene(m_scene);
   rtcExit();
}


bool RayCaster::compute() {

   return true;
}

bool RayCaster::parameterChanged(Parameter *p) {

   if (p == m_colorRank) {

      if (m_colorRank->getValue()) {

         const int r = rank()%3;
         const int g = (rank()/3)%3;
         const int b = (rank()/9)%3;
         m_defaultColor = Vector4(63+r*64, 63+g*64, 63+b*64, 255);
      } else {

         m_defaultColor = Vector4(127, 127, 127, 255);
      }
   } else if (p == m_vncOnSlaves) {

      if (rank() != rootRank()) {

         if (m_vncOnSlaves->getValue()) {
            if (!vnc) {
               vnc.reset(new VncServer(state.width, state.height, m_vncBasePort->getValue()));
            }
         } else {
            vnc.reset();
         }
      }
   } else if (p == m_vncBasePort) {

      if (rank() == rootRank() || m_vncOnSlaves->getValue()) {
         if (!vnc || vnc->port() != m_vncBasePort->getValue())
            vnc.reset(new VncServer(state.width, state.height, m_vncBasePort->getValue()));
      }
   } else if (p == m_shading) {

      m_doShade = m_shading->getValue();
   } else if (p == m_depthPrec) {

       if (m_depthPrec->getValue() == 0)
           m_prec = 16;
       else
           m_prec = 24;
       if (vnc)
           vnc->setDepthPrecision(m_prec);
   } else if (p == m_depthSnappy) {

       m_snappy = m_depthSnappy->getValue();
       if (vnc)
           vnc->enableSnappy(m_snappy);
   } else if (p == m_depthQuant) {

       m_quant = m_depthQuant->getValue();
       if (vnc)
           vnc->enableQuantization(m_quant);
   }

   m_doRender = 1;

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
   TileTask(const RayCaster &rc, int tile=-1)
   : rc(rc)
   , tile(tile)
   , tilesize(rc.tilesize)
   , packetSize(rc.packetSize)
   , packetSizeX((packetSize+1)/2)
   , packetSizeY(packetSize/packetSizeX)
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
   const int tile;
   const int tilesize;
   int imgWidth, imgHeight;
   int xlim, ylim;
   int ntx;
   Vector4 depthTransform2, depthTransform3;
   Vector3 lowerBottom, dx, dy;
   Vector3 origin;
   float tNear, tFar;
   const int packetSize;
   const int packetSizeX, packetSizeY;
   int xoff, yoff;
   float *depth;
   unsigned char *rgba;
};


void TileTask::render(int tile) const {

   int RTCORE_ALIGN(32) validMask[MaxPacketSize];
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

               if (packetSize==8) {
                  setRay(ray8, idx, ray);
               } else if (packetSize==4) {
                  setRay(ray4, idx, ray);
               }

            ++idx;
            }
         }

         if (packetSize==8) {
            rtcIntersect8(validMask, rc.m_scene, ray8);
         } else if (packetSize==4) {
            rtcIntersect4(validMask, rc.m_scene, ray4);
         } else {
            rtcIntersect(rc.m_scene, ray);
         }

         idx = 0;
         for (int y=yy; y<yy+packetSizeY; ++y) {
            for (int x=xx; x<xx+packetSizeX; ++x) {

               if (validMask[idx] == RayEnabled) {
                  if (packetSize==8) {
                     ray = getRay(ray8, idx);
                  } else if (packetSize==4) {
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

         vassert(ray.instID < rc.instances.size());
         const RenderObject *ro = rc.instances[ray.instID];
         vassert(ro->geomId == ray.geomID);

         Vector4 color = rc.m_defaultColor;
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
            const float tc = w*tc0 + u*tc1 + v*tc2;
            vassert(tc >= 0.f);
            vassert(tc <= 1.f);
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
         for (const auto &light: rc.lights) {
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

         shaded = rc.m_defaultColor;
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

   state.numTimesteps = anim_geometry.size();
   state.numTimesteps = mpi::all_reduce(mpi::communicator(), state.numTimesteps, mpi::maximum<int>());

   m_updateBounds = mpi::all_reduce(mpi::communicator(), m_updateBounds, mpi::maximum<int>());
   if (m_updateBounds) {
      mpi::all_reduce(mpi::communicator(), boundMin.data(), 3, state.bMin.data(), mpi::minimum<Scalar>());
      mpi::all_reduce(mpi::communicator(), boundMax.data(), 3, state.bMax.data(), mpi::maximum<Scalar>());
   }

   if (vnc) {
      if (m_updateBounds) {
         Vector center = 0.5*(state.bMin+state.bMax);
         Scalar radius = (state.bMax-state.bMin).norm()*0.5;
         vnc->setBoundingSphere(center, radius);
      }

      vnc->setNumTimesteps(state.numTimesteps);

      vnc->preFrame();

      const Matrix4 view = vnc->viewMat() * vnc->transformMat() * vnc->scaleMat();

      if (rank() == rootRank()) {

         if (state.timestep != vnc->timestep()
             || state.width != vnc->width()
             || state.height != vnc->height()
             || state.proj != vnc->projMat()
             || state.view != view) {
            m_doRender = 1;
         }
      }

      state.timestep = vnc->timestep();

      state.width = vnc->width();
      state.height = vnc->height();

      state.view = view;
      state.proj = vnc->projMat();

      lights = vnc->lights;
      const Matrix4 lightTransform = vnc->viewMat().inverse();
      for (auto &light: lights) {
         Vector4 transformedPosition = lightTransform * light.position;
         transformedPosition /= transformedPosition[3];
         light.transformedPosition = Vector3(transformedPosition[0], transformedPosition[1], transformedPosition[2]);
      }
   }
   m_updateBounds = 0;

   m_doRender = mpi::all_reduce(mpi::communicator(), m_doRender, mpi::maximum<int>());

   if (m_continuousRendering->getValue())
      m_doRender = 1;

   if (m_doRender) {
      m_doRender = 0;

      mpi::broadcast(mpi::communicator(), state, rootRank());
      mpi::broadcast(mpi::communicator(), lights, rootRank());

      if (vnc && rank() != rootRank()) {
         vnc->resize(state.width, state.height);
      }

      if (m_timestep != state.timestep) {
         if (anim_geometry.size() > m_timestep) {
            for (auto &ro: anim_geometry[m_timestep])
               rtcDisable(m_scene, ro->instId);
         }
         m_timestep = state.timestep;
         if (anim_geometry.size() > m_timestep) {
            for (auto &ro: anim_geometry[m_timestep])
               rtcEnable(m_scene, ro->instId);
         }
         rtcCommit(m_scene);
      }

      int resetTiles = 0;
      if (m_initIcet || (vnc && (width != vnc->width() || height != vnc->height())))
         resetTiles = 1;
      resetTiles = mpi::all_reduce(mpi::communicator(), resetTiles, mpi::maximum<int>());

      if (resetTiles) {
         width = height = 0;
         if (vnc) {
            width =  vnc->width();
            height = vnc->height();
         }

         icetTiles[rank()].x = 0;
         icetTiles[rank()].y = 0;
         icetTiles[rank()].width = width;
         icetTiles[rank()].height = height;

         mpi::all_gather(mpi::communicator(), icetTiles[rank()], icetTiles);

         icetResetTiles();

         for (int i=0; i<size(); ++i) {
            if (i == m_displayRank || m_displayRank == -1) {
               if (icetTiles[i].width>0 && icetTiles[i].height>0)
                  icetAddTile(icetTiles[i].x, icetTiles[i].y, icetTiles[i].width, icetTiles[i].height, i);
            }
         }

         icetDisable(ICET_COMPOSITE_ONE_BUFFER); // include depth buffer in compositing result
         icetStrategy(ICET_STRATEGY_REDUCE);
         icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
         icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
         icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);

         icetDrawCallback(drawCallback);
      }

      icetBoundingBoxf(boundMin[0], boundMax[0],
            boundMin[1], boundMax[1],
            boundMin[2], boundMax[2]);

      Matrix4 modelview;
      IceTDouble proj[16], mv[16];
      for (int i=0; i<16; ++i) {
         proj[i] = state.proj.data()[i];
         mv[i] = state.view.data()[i];
      }
      IceTFloat bg[4] = { 0., 0., 0., 1. };

      IceTImage img = icetDrawFrame(proj, mv, bg);

      if (rank() == m_displayRank || m_displayRank==-1) {
         vassert(vnc);
         const int bpp = 4;
         const int w = vnc->width();
         const int h = vnc->height();
         vassert(w == icetImageGetWidth(img));
         vassert(h == icetImageGetHeight(img));

         const IceTUByte *color = icetImageGetColorcub(img);
         const IceTFloat *depth = icetImageGetDepthcf(img);

         for (int y=0; y<h; ++y) {
            memcpy(vnc->rgba()+w*bpp*y, color+w*(h-1-y)*bpp, bpp*w);
            memcpy(vnc->depth()+w*y, depth+w*(h-1-y), sizeof(float)*w);
         }

         vnc->invalidate(0, 0, vnc->width(), vnc->height());
      }
   }
   if (vnc)
      vnc->postFrame();
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
   const int ts = tilesize;
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

   TileTask renderTile(*this);
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

   if (vnc && rank() != m_displayRank && m_displayRank != -1) {
      const int bpp = 4;
      const int w = vnc->width();
      const int h = vnc->height();
      vassert(w == icetImageGetWidth(image));
      vassert(h == icetImageGetHeight(image));

      const IceTUByte *color = icetImageGetColorcub(image);

      memset(vnc->rgba(), 0, w*h*bpp);

      for (int y=viewport[1]; y<viewport[1]+viewport[3]; ++y) {
         memcpy(vnc->rgba()+(w*y+viewport[0])*bpp, color+(w*(h-1-y)+viewport[0])*bpp, bpp*viewport[2]);
      }

      vnc->invalidate(0, 0, vnc->width(), vnc->height());
   }

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

   boundMin = min;
   boundMax = max;

   m_updateBounds = 1;

#if 0
   std::cerr << "<<< BOUNDS <<<" << std::endl;
   std::cerr << "min: " << min << std::endl;
   std::cerr << "max: " << max << std::endl;
   std::cerr << ">>> BOUNDS >>>" << std::endl;
#endif
}


bool RayCaster::removeObject(RenderObject *ro) {

   instances[ro->instId] = nullptr;

   int t = ro->t;
   auto &objlist = t>=0 ? anim_geometry[t] : static_geometry;

   if (t == -1 || t == m_timestep)
      m_doRender = 1;

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
   if (instances.size() <= ro->instId)
      instances.resize(ro->instId+1);
   vassert(instances[ro->instId] == nullptr);
   instances[ro->instId] = ro;

   float identity[16];
   for (int i=0; i<16; ++i) {
      identity[i] = (i/4 == i%4) ? 1. : 0.;
   }
   rtcSetTransform(m_scene, ro->instId, RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, identity);
   if (t < 0 || t == m_timestep) {
      rtcEnable(m_scene, ro->instId);
      m_doRender = 1;
   } else {
      rtcDisable(m_scene, ro->instId);
   }
   rtcCommit(m_scene);

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

void  RayCaster::drawCallback(const IceTDouble *proj, const IceTDouble *mv, const IceTFloat *bg, const IceTInt *viewport, IceTImage image) {

   RayCaster::the().renderRect(proj, mv, bg, viewport, image);
}


MODULE_MAIN(RayCaster)
