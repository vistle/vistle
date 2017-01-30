#undef NDEBUG
#include <GL/glew.h>

#include <IceT.h>
#include <IceTMPI.h>

#include <vector>
#include <boost/lexical_cast.hpp>

#include <osg/DisplaySettings>
#include <osg/Group>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/GraphicsContext>
#include <osg/io_utils>

#include <VistleGeometryGenerator.h>
#include "OSGRenderer.h"
#include "EnableGLDebugOperation.h"
#include <osgViewer/Renderer>
#include <osg/TextureRectangle>
#include <osg/FrameBufferObject>

#ifdef USE_X11
#include <X11/Xlib.h>
#endif

//#define USE_FBO
const int MaxAsyncFrames = 2;
static_assert(MaxAsyncFrames > 0, "MaxAsyncFrames needs to be positive");

DEFINE_ENUM_WITH_STRING_CONVERSIONS(OsgThreadingModel,
   (Single_Threaded)
   (Cull_Draw_Thread_Per_Context)
   (Thread_Per_Context)
   (Draw_Thread_Per_Context)
   (Cull_Thread_Per_Camera_Draw_Thread_Per_Context)
   (Thread_Per_Camera)
   (Automatic_Selection)
)

namespace {

osg::Matrix toOsg(const vistle::Matrix4 &mat) {

    osg::Matrix ret;
    for (int i=0; i<16; ++i) {
        ret.ptr()[i] = mat.data()[i];
    }
    return ret;
}

osg::Vec3 toOsg(const vistle::Vector3 &vec) {
    return osg::Vec3(vec[0], vec[1], vec[2]);
}

osg::Vec4 toOsg(const vistle::Vector4 &vec) {
    return osg::Vec4(vec[0], vec[1], vec[2], vec[3]);
}

class ReadOperation: public osg::Camera::DrawCallback {

public:
   ReadOperation(OsgViewData &ovd, vistle::ParallelRemoteRenderManager &renderMgr, OpenThreads::Mutex *mutex, size_t idx, const int r)
      : ovd(ovd), renderMgr(renderMgr), icetMutex(mutex), viewIdx(idx), init(true), release(false) {}

   void 	operator() (osg::RenderInfo &renderInfo) const {

       const auto &vd = renderMgr.viewData(viewIdx);
       const auto w = vd.width, h = vd.height;

       vassert(ovd.pboDepth.size() == ovd.pboColor.size());

       if (release || init) {
          glewInit();
          if (!glGenBuffers || !glBindBuffer || !glBufferStorage || !glFenceSync || !glClientWaitSync || !glMemoryBarrier) {
              std::cerr << "GL extensions missing" << std::endl;
              return;
          }
          release = false;

          if (!ovd.pboDepth.empty())
             glDeleteBuffers(ovd.pboDepth.size(), ovd.pboDepth.data());
          if (!ovd.pboColor.empty())
             glDeleteBuffers(ovd.pboColor.size(), ovd.pboColor.data());
          ovd.pboDepth.clear();
          ovd.pboColor.clear();
          ovd.mapDepth.clear();
          ovd.mapColor.clear();
       }

       if (init) {
          const int mapflags = GL_MAP_READ_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT;
          const int createflags = mapflags/*|GL_MAP_COHERENT_BIT*/;

          ovd.pboDepth.resize(MaxAsyncFrames);
          ovd.pboColor.resize(MaxAsyncFrames);
          ovd.mapDepth.resize(ovd.pboDepth.size());
          glGenBuffers(ovd.pboDepth.size(), ovd.pboDepth.data());
          for (size_t i=0; i<ovd.pboDepth.size(); ++i) {
             glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboDepth[i]);
             glBufferStorage(GL_PIXEL_PACK_BUFFER, w*h*sizeof(GLfloat), nullptr, createflags);
             ovd.mapDepth[i] = static_cast<GLfloat *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, w*h*sizeof(GLfloat), mapflags));
          }

          ovd.mapColor.resize(ovd.pboColor.size());
          glGenBuffers(ovd.pboColor.size(), ovd.pboColor.data());
          for (size_t i=0; i<ovd.pboColor.size(); ++i) {
             glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboColor[i]);
             glBufferStorage(GL_PIXEL_PACK_BUFFER, w*h*4, nullptr, createflags);
             ovd.mapColor[i] = static_cast<GLchar *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, w*h*4, mapflags));
          }
       }

       const size_t pbo = ovd.readPbo;

       glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboColor[pbo]);
       glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

       glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboDepth[pbo]);
       glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

       glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

       glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT);
       GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

       OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ovd.mutex);
       ovd.compositePbo.push_back(pbo);
       ovd.outstanding.push_back(sync);
       ovd.readPbo = (pbo+1) % ovd.pboDepth.size();
   }

   OsgViewData &ovd;
   vistle::ParallelRemoteRenderManager &renderMgr;
   OpenThreads::Mutex *icetMutex;
   size_t viewIdx;
   mutable bool init, release;
};

class NoSwapCallback: public osg::GraphicsContext::SwapCallback {

public:
    void swapBuffersImplementation(osg::GraphicsContext *gc) override {
    }
};


}

OsgViewData::OsgViewData(OSGRenderer &viewer, size_t viewIdx)
: viewer(viewer)
, viewIdx(viewIdx)
, readPbo(0)
, width(0)
, height(0)
{
   update(false);
}

OsgViewData::~OsgViewData() {

    if (camera) {
        int idx = viewer.findSlaveIndexForCamera(camera);
        if (idx >= 0) {
           viewer.removeSlave(idx);
        }
        camera = nullptr;
    }
    viewer.realize();
}

void OsgViewData::createCamera() {

    const vistle::ParallelRemoteRenderManager::PerViewState &vd = viewer.m_renderManager.viewData(viewIdx);

    ReadOperation *readOp = nullptr;
    if (viewIdx >= 0) {
       if (camera) {
          int idx = viewer.findSlaveIndexForCamera(camera);
          if (idx >= 0) {
             viewer.removeSlave(idx);
          }
          camera = nullptr;
       }

       camera = new osg::Camera;
       camera->setImplicitBufferAttachmentMask(0, 0);
       camera->setViewport(0, 0, vd.width, vd.height);

       viewer.addSlave(camera, osg::Matrix::identity(), osg::Matrix::identity(), false);
       camera->addChild(viewer.scene);
       viewer.realize();
       readOp = new ReadOperation(*this, viewer.m_renderManager, viewer.icetMutex, viewIdx, viewer.rank());
    } else {
        if (!camera.valid()) {
           camera = viewer.getCamera();
           readOp = new ReadOperation(*this, viewer.m_renderManager, viewer.icetMutex, viewIdx, viewer.rank());
        }
        camera->setViewport(0, 0, vd.width, vd.height);
    }

    if(readOp) {
        readOperation = readOp;
        camera->setPostDrawCallback(readOp);
    }
}


bool OsgViewData::update(bool frameQueued) {

    const vistle::ParallelRemoteRenderManager::PerViewState &vd = viewer.m_renderManager.viewData(viewIdx);

    bool create = false, resize = false;
    if (!camera) {
        std::cerr << "creating graphics context: no camera" << std::endl;
        create = true;
    } else if (!camera->getGraphicsContext()) {
        std::cerr << "creating graphics context: no context" << std::endl;
        create = true;
    } else if(!camera->getGraphicsContext()->getTraits()) {
        std::cerr << "creating graphics context: no traits" << std::endl;
        create = true;
    } else {
#ifdef USE_FBO
       auto &buffers = camera->getBufferAttachmentMap();
       const auto &col = buffers[osg::Camera::COLOR_BUFFER];
       const auto &dep = buffers[osg::Camera::DEPTH_BUFFER];
       if (col.width() != vd.width || dep.width() != vd.width
               || col.height() != vd.height || dep.height() != vd.height) {
           std::cerr << "resizing FBO" << std::endl;
           resize = true;
       }
#else
        auto traits = camera->getGraphicsContext()->getTraits();
        if (traits->width != vd.width || traits->height != vd.height) {
            std::cerr << "recreating PBuffer for resizing" << std::endl;
            create = true;
            resize = true;
            if (frameQueued)
               std::cerr << "cannot create, because frames queued" << std::endl;
        }
#endif
    }
    if (create && !frameQueued) {
        if (gc && viewIdx >= 0) {
            gc = nullptr;
        }
#ifdef USE_FBO
       if (!camera)
            createCamera();
       std::cerr << "creating FBO of size " << vd.width << "x" << vd.height << std::endl;
       camera->setGraphicsContext(viewer.getCamera()->getGraphicsContext());
       camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
       camera->setViewport(new osg::Viewport(0, 0, vd.width, vd.height));
       //camera->attach(osg::Camera::COLOR_BUFFER, GL_RGBA32F);
       camera->attach(osg::Camera::COLOR_BUFFER, GL_RGBA);
       camera->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT32F);
       GLenum buffer = GL_COLOR_ATTACHMENT0;
#else
       std::cerr << "creating PBuffer of size " << vd.width << "x" << vd.height << std::endl;
       createCamera();

       osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(viewer.displaySettings);
       traits->readDISPLAY();
       traits->setScreenIdentifier(":0");
       traits->setUndefinedScreenDetailsToDefaultScreen();
       traits->sharedContext = viewIdx>=0 ? viewer.getCamera()->getGraphicsContext() : nullptr;
       traits->x = 0;
       traits->y = 0;
       traits->width = vd.width;
       traits->height = vd.height;
       traits->alpha = 8;
       traits->depth = 24;
       traits->doubleBuffer = false;
       traits->windowDecoration = true;
       traits->windowName = "OsgRenderer: view " + boost::lexical_cast<std::string>(viewIdx);
       traits->pbuffer =true;
       traits->vsync = false;
       if (viewIdx >= 0) {
          gc = osg::GraphicsContext::createGraphicsContext(traits);
          if (!traits->doubleBuffer)
             gc->setSwapCallback(new NoSwapCallback);
          gc->realize();
          camera->setGraphicsContext(gc);
          camera->setDisplaySettings(viewer.displaySettings);
          camera->setRenderTargetImplementation(osg::Camera::PIXEL_BUFFER);
          GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
          camera->setDrawBuffer(buffer);
          camera->setReadBuffer(buffer);
          camera->setRenderOrder(osg::Camera::PRE_RENDER);
       } else {
           gc = camera->getGraphicsContext();
       }
#endif
       camera->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);

       camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
       camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

       camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
       camera->setViewMatrix(osg::Matrix::identity());
       camera->setProjectionMatrix(osg::Matrix::identity());
       resize = true;
    }
    if (resize && !frameQueued) {
        if (gc)
           gc->resized(0, 0, vd.width, vd.height);
       camera->setViewport(new osg::Viewport(0, 0, vd.width, vd.height));
       width = vd.width;
       height = vd.height;
#ifdef USE_FBO
#if 0
       auto &buffers = camera->getBufferAttachmentMap();
       buffers.clear();
#ifdef USE_FBO
       //auto &buffers = camera->getBufferAttachmentMap();
       //const auto &col = buffers[osg::Camera::COLOR_BUFFER];
       //const auto &dep = buffers[osg::Camera::DEPTH_BUFFER];
#endif
#endif
       auto rend = dynamic_cast<osgViewer::Renderer *>(camera->getRenderer());
       if (rend) {
          for (size_t i=0; i<2; ++i) {
             auto sv = rend->getSceneView(i);
             if (sv) {
                 sv->setViewport(camera->getViewport());
                 auto rs = sv->getRenderStage();
                 if (rs) {
#if 0
                     auto fbo = rs->getFrameBufferObject();
                     if (fbo) {
                         auto &col = fbo->getAttachment(osg::Camera::COLOR_BUFFER);
                         auto &dep = fbo->getAttachment(osg::Camera::DEPTH_BUFFER);
                         for (auto &att: std::vector<osg::FrameBufferAttachment>{col, dep}) {
                         //for (auto &att: {col, dep}) {
                             if (auto rb = att.getRenderBuffer()) {
                                rb->setWidth(vd.width);
                                rb->setHeight(vd.height);
                             }
                         }
                     }
#endif
                     rs->setCameraRequiresSetUp(true);
                     rs->setFrameBufferObject(nullptr);
                 }
             }
          }
       }
#endif
#if 0
       std::cerr << "realizing viewer" << std::endl;
       viewer.realize();
       std::cerr << "realizing viewer: done" << std::endl;
#endif
       ReadOperation *readOp = dynamic_cast<ReadOperation *>(readOperation.get());
       if (readOp) {
          readOp->release = true;
          readOp->init = true;
       }
    }

    if (camera) {
       camera->setViewMatrix(toOsg(vd.view));
       camera->setProjectionMatrix(toOsg(vd.proj));
    }

   if (frameQueued)
      return create || resize;

   return false;
}

bool OsgViewData::composite(size_t maxQueuedFrames, int timestep, bool wait) {

    size_t queued = 0;
    GLsync sync = 0;
    {
       OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mutex);
       queued = outstanding.size();
       if (queued==0 || queued+1 < maxQueuedFrames) {
          //std::cerr << "no outstanding frames to composite" << std::endl;
          return false;
       }
       sync = outstanding.front();
    }

    GLenum ret = 0;
    do {
        ret = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, wait ? 100000 : 0);
        if (!wait && ret == GL_TIMEOUT_EXPIRED)
            return false;
    } while (ret == GL_TIMEOUT_EXPIRED);

    glDeleteSync(sync);
    size_t pbo = 0;
    {
       OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mutex);
       pbo = compositePbo.front();
       outstanding.pop_front();
       compositePbo.pop_front();
       queued = outstanding.size();
    }

    if (ret == GL_WAIT_FAILED) {
       std::cerr << "GL sync wait failed" << std::endl;
       return true;
    }

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*viewer.icetMutex);
    viewer.m_renderManager.setCurrentView(viewIdx);
    //renderMgr.updateRect(viewIdx, const IceTInt *viewport, const IceTImage image);
    const osg::Vec4 clear = camera->getClearColor();
    IceTFloat bg[4] = { clear[0], clear[1], clear[2], clear[3] };
    IceTDouble proj[16], view[16];
    IceTInt viewport[4] = {0, 0, width, height};
    viewer.m_renderManager.getModelViewMat(viewIdx, view);
    viewer.m_renderManager.getProjMat(viewIdx, proj);
    IceTImage image = icetCompositeImage(mapColor[pbo], mapDepth[pbo], viewport, proj, view, bg);
    viewer.m_renderManager.finishCurrentView(image, timestep, false);

    return true;
}

TimestepHandler::TimestepHandler()
  : timestep(0) {

     m_root = new osg::MatrixTransform;
     m_fixed = new osg::Group;
     m_animated = new osg::Sequence;

     m_root->addChild(m_fixed);
     m_root->addChild(m_animated);
}

void TimestepHandler::addObject(osg::Node * geode, const int step) {

   if (step < 0) {
      m_fixed->addChild(geode);
   } else {
      for (int i=m_animated->getNumChildren(); i<=step; ++i) {
         m_animated->addChild(new osg::Group);
      }
      osg::Group *gr = m_animated->getChild(step)->asGroup();
      if (!gr)
         return;
      gr->addChild(geode);
   }
}

void TimestepHandler::removeObject(osg::Node *geode, const int step) {

   if (step < 0) {
      m_fixed->removeChild(geode);
   } else {
      if (unsigned(step) < m_animated->getNumChildren()) {
         osg::Group *gr = m_animated->getChild(step)->asGroup();
         gr->removeChild(geode);

         if (gr->getNumChildren() == 0) {
            int last = m_animated->getNumChildren()-1;
            for (; last>0; --last) {
               osg::Group *gr = m_animated->getChild(last)->asGroup();
               if (gr->getNumChildren() > 0)
                  break;
            }
            m_animated->removeChildren(last+1, m_animated->getNumChildren()-last-1);
         }
      }
   }
}

osg::ref_ptr<osg::MatrixTransform> TimestepHandler::root() const {

   return m_root;
}

bool TimestepHandler::setTimestep(const int timestep) {

   m_animated->setValue(timestep);
   return true;
}

size_t TimestepHandler::numTimesteps() const {

   return m_animated->getNumChildren();
}

OSGRenderer::OSGRenderer(const std::string &shmname, const std::string &name, int moduleID)
   : Renderer("OSGRenderer", shmname, name, moduleID), osgViewer::Viewer()
   , m_renderManager(this, nullptr)
   , m_numViewsToComposite(0)
   , m_numFramesToComposite(0)
   , m_asyncFrames(0)
{
#ifdef USE_X11
    XInitThreads();
#endif
#if 0
   std::cerr << "waiting for debugger: pid=" << getpid() << std::endl;
   sleep(10);
#endif
   icetMutex = new OpenThreads::Mutex;

   m_debugLevel = addIntParameter("debug", "debug level", 1);
   m_visibleView = addIntParameter("visible_view", "no. of visible view (positive values will open window)", -1);
   setParameterRange(m_visibleView, (vistle::Integer)-1, (vistle::Integer)(m_renderManager.numViews())-1);
#if 0
   m_requestedThreadingModel = CullThreadPerCameraDrawThreadPerContext;
   m_threading = addIntParameter("threading_model", "OpenSceneGraph threading model", Cull_Thread_Per_Camera_Draw_Thread_Per_Context, vistle::Parameter::Choice);
#else
   m_requestedThreadingModel = SingleThreaded;
   m_threading = addIntParameter("threading_model", "OpenSceneGraph threading model", Single_Threaded, vistle::Parameter::Choice);
#endif
   V_ENUM_SET_CHOICES(m_threading, OsgThreadingModel);
   m_async = addIntParameter("asynchronicity", "number of outstanding frames to tolerate", m_asyncFrames);
   setParameterRange(m_async, (vistle::Integer)0, (vistle::Integer)MaxAsyncFrames);

   //setRealizeOperation(new EnableGLDebugOperation());

   displaySettings = new osg::DisplaySettings;
   displaySettings->setStereo(false);

   osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(displaySettings);
   traits->readDISPLAY();
   traits->setScreenIdentifier(":0");
   traits->setUndefinedScreenDetailsToDefaultScreen();
   traits->x = 0;
   traits->y = 0;
   traits->width=1;
   traits->height=1;
   traits->alpha = 8;
   traits->windowDecoration = true;
   traits->windowName = "OsgRenderer: master";
   traits->doubleBuffer = false;
   traits->sharedContext = 0;
   traits->pbuffer = true;
   traits->vsync = false;
   osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
   if (!gc)
   {
       std::cerr << "Failed to create master graphics context" << std::endl;
       return;
   }
   if (!traits->doubleBuffer)
      gc->setSwapCallback(new NoSwapCallback);

   getCamera()->setGraphicsContext(gc.get());
   getCamera()->setDisplaySettings(displaySettings);
   getCamera()->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
   getCamera()->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);

   GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
   getCamera()->setDrawBuffer(buffer);
   getCamera()->setReadBuffer(buffer);
                                  
   realize();

   getCamera()->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 0.0));
   getCamera()->setViewMatrix(osg::Matrix::identity());
   getCamera()->setProjectionMatrix(osg::Matrix::identity());
   scene = new osg::MatrixTransform();
   timesteps = new TimestepHandler;
   scene->addChild(timesteps->root());
   rootState = scene->getOrCreateStateSet();
   //setSceneData(scene);

   material = new osg::Material();
   material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
   material->setColorMode(osg::Material::OFF);
   material->setAmbient(osg::Material::FRONT_AND_BACK,
                        osg::Vec4(0.2f, 0.2f, 0.2f, 1.0));
   material->setDiffuse(osg::Material::FRONT_AND_BACK,
                        osg::Vec4(1.0f, 1.0f, 1.0f, 1.0));
   material->setSpecular(osg::Material::FRONT_AND_BACK,
                         osg::Vec4(0.4f, 0.4f, 0.4f, 1.0));
   material->setEmission(osg::Material::FRONT_AND_BACK,
                         osg::Vec4(0.0f, 0.0f, 0.0f, 1.0));
   material->setShininess(osg::Material::FRONT_AND_BACK, 16.0f);
   material->setAlpha(osg::Material::FRONT_AND_BACK, 1.0f);

   lightModel = new osg::LightModel;
   lightModel->setTwoSided(true);
   lightModel->setLocalViewer(true);
   lightModel->setColorControl(osg::LightModel::SEPARATE_SPECULAR_COLOR);

   defaultState = new osg::StateSet;
   defaultState->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
   defaultState->setAttributeAndModes(lightModel.get(), osg::StateAttribute::ON);
   defaultState->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
   defaultState->setMode(GL_LIGHTING, osg::StateAttribute::ON);
   defaultState->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

   scene->setStateSet(rootState);
}

OSGRenderer::~OSGRenderer() {

}

void OSGRenderer::flush() {

    std::cerr << "flushing outstanding frames..." << std::endl;
    for (size_t f=m_asyncFrames; f>0; --f) {
       composite(f-1);
    }
    vassert(m_numFramesToComposite == 0);
}


bool OSGRenderer::composite(size_t maxQueued) {

   vassert(maxQueued >= 0);
   vassert(maxQueued <= MaxAsyncFrames);
   vassert(m_viewData.size()*m_numFramesToComposite == m_numViewsToComposite);

   //std::cerr << "composite(maxQueued=" << maxQueued <<"), to composite: frames=" << m_numFramesToComposite << ", views=" << m_numViewsToComposite << std::endl;

   if (m_viewData.size() == 0)
      return false;

   if (m_numViewsToComposite == 0)
       return false;

   if (m_numFramesToComposite == 0)
       return false;

   for (size_t i=0; i<m_viewData.size(); ++i) {
      const bool progress = m_viewData[i]->composite(maxQueued, m_previousTimesteps.front());
      vassert(progress);
      --m_numViewsToComposite;
   }

   vassert(m_numViewsToComposite%m_viewData.size() == 0);
   --m_numFramesToComposite;
   m_renderManager.finishFrame(m_previousTimesteps.front());
   m_previousTimesteps.pop_front();

   return true;
}

bool OSGRenderer::render() {

    bool again = false;
    if (m_asyncFrames > 0)
       again = composite(m_asyncFrames-1);

    const size_t numTimesteps = timesteps->numTimesteps();
    if (!m_renderManager.prepareFrame(numTimesteps)) {
       return again;
    }

    // statistics
    static int framecounter = 0;
    static double laststattime = 0.;
    ++framecounter;
    double time = getFrameStamp()->getReferenceTime();
    if (time - laststattime > 3.) {
       if (!rank() && m_debugLevel->getValue() >= 2)
          std::cerr << "FPS: " << framecounter/(time-laststattime) << std::endl;
       framecounter = 0;
       laststattime = time;
    }

    int t = m_renderManager.timestep();
    timesteps->setTimestep(t);

    if (m_renderManager.numViews() != m_viewData.size()) {
       setParameterRange(m_visibleView, (vistle::Integer)-1, (vistle::Integer)(m_renderManager.numViews())-1);

       flush();

       if (m_viewData.size() > m_renderManager.numViews()) {
          m_viewData.resize(m_renderManager.numViews());
       } else {
          while (m_renderManager.numViews() > m_viewData.size()) {
             m_viewData.emplace_back(new OsgViewData(*this, m_viewData.size()));
          }
       }
    }

    if (!m_viewData.empty()) {
       timesteps->root()->setMatrix(toOsg(m_renderManager.viewData(0).model));
       getCamera()->setViewMatrix(toOsg(m_renderManager.viewData(0).view));
       getCamera()->setProjectionMatrix(toOsg(m_renderManager.viewData(0).proj));

       auto &l = m_renderManager.viewData(0).lights;
       if (lights.size() != l.size()) {
          std::cerr<< "changing num lights from " << lights.size() << " to " << l.size() << std::endl;
          for (size_t i=l.size(); i<lights.size(); ++i) {
             scene->removeChild(lights[i]);
          }
          for (size_t i=lights.size(); i<l.size(); ++i) {
             lights.push_back(new osg::LightSource);
             scene->addChild(lights.back());
          }
       }
       vassert(l.size() == lights.size());
       for (size_t i=0; i<l.size(); ++i) {
          osg::Light *light = lights[i]->getLight();
          if (!light)
             light = new osg::Light;
          light->setLightNum(i);
          light->setPosition(toOsg(l[i].position));
          light->setDirection(toOsg(l[i].direction));
          light->setAmbient(toOsg(l[i].ambient));
          light->setDiffuse(toOsg(l[i].diffuse));
          light->setSpecular(toOsg(l[i].specular));
          light->setSpotCutoff(l[i].spotCutoff);
          light->setSpotExponent(l[i].spotExponent);
          lights[i]->setLight(light);

          osg::StateAttribute::Values val = l[i].enabled ? osg::StateAttribute::ON : osg::StateAttribute::OFF;
          rootState->setAttributeAndModes(light, val);
          lights[i]->setLocalStateSetModes(val);
          lights[i]->setStateSetModes(*rootState, val);

       }

       for (size_t i=0; i<m_viewData.size(); ++i) {
          if (m_viewData[i]->update(m_numFramesToComposite > 0)) {

              flush();
              m_viewData[i]->update(m_numFramesToComposite > 0);
          }
       }

       frame();
       ++m_numFramesToComposite;
       m_numViewsToComposite += m_viewData.size();
       m_previousTimesteps.push_back(t);

       if (m_asyncFrames == 0)
          composite(0);

       if (m_requestedThreadingModel != getThreadingModel()) {
          setThreadingModel(m_requestedThreadingModel);
       }
       //usleep(500000);
    }

    return true;
}

bool OSGRenderer::changeParameter(const vistle::Parameter *p) {

   m_renderManager.handleParam(p);

   if (p == m_threading) {
       switch(m_threading->getValue()) {
       case Single_Threaded:
           m_requestedThreadingModel = SingleThreaded;
           break;
       case Cull_Draw_Thread_Per_Context:
           m_requestedThreadingModel = CullDrawThreadPerContext;
           break;
       case Thread_Per_Context:
           m_requestedThreadingModel = ThreadPerContext;
           break;
       case Draw_Thread_Per_Context:
           m_requestedThreadingModel = DrawThreadPerContext;
           break;
       case Cull_Thread_Per_Camera_Draw_Thread_Per_Context:
           m_requestedThreadingModel = CullThreadPerCameraDrawThreadPerContext;
           break;
       case Thread_Per_Camera:
           m_requestedThreadingModel = ThreadPerCamera;
           break;
       case Automatic_Selection:
           m_requestedThreadingModel = AutomaticSelection;
           break;
       }
   } else if (p == m_async) {
       m_asyncFrames = m_async->getValue();
   }

   return Renderer::changeParameter(p);
}

std::shared_ptr<vistle::RenderObject> OSGRenderer::addObject(int senderId, const std::string &senderPort,
            vistle::Object::const_ptr container,
            vistle::Object::const_ptr geometry,
            vistle::Object::const_ptr normals,
            vistle::Object::const_ptr texture) {

   std::shared_ptr<vistle::RenderObject> ro;
   VistleGeometryGenerator gen(ro, geometry, normals, texture);
   if (VistleGeometryGenerator::isSupported(geometry->getType()) || geometry->getType() == vistle::Object::PLACEHOLDER) {
      auto geode = gen(defaultState);

      if (geode) {
         ro.reset(new OsgRenderObject(senderId, senderPort, container, geometry, normals, texture, geode));
         timesteps->addObject(geode, ro->timestep);
      }
   }

   m_renderManager.addObject(ro);

   return ro;
}

void OSGRenderer::removeObject(std::shared_ptr<vistle::RenderObject> ro) {

   auto oro = std::static_pointer_cast<OsgRenderObject>(ro);
   timesteps->removeObject(oro->node, oro->timestep);
   m_renderManager.removeObject(ro);
}

OsgRenderObject::OsgRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr texture,
         osg::ref_ptr<osg::Node> node)
: vistle::RenderObject(senderId, senderPort, container, geometry, normals, texture)
, node(node)
{
}

MODULE_MAIN(OSGRenderer)
