#undef NDEBUG
#include <GL/glew.h>

#include <IceT.h>
#include <IceTMPI.h>

#include <vector>

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

//#define USE_FBO

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
      : ovd(ovd), renderMgr(renderMgr), icetMutex(mutex), viewIdx(idx) {}

   void 	operator() (osg::RenderInfo &renderInfo) const {

       const auto &vd = renderMgr.viewData(viewIdx);
       const auto w = vd.width, h = vd.height;

       if (ovd.pboDepth[0] == 0) {
          glewInit();
          if (!glGenBuffers || !glBindBuffer || !glBufferStorage || !glFenceSync || !glWaitSync || !glMemoryBarrier) {
              std::cerr << "GL extensions missing" << std::endl;
              return;
          }
          const int flags = GL_MAP_READ_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT;

          ovd.mapDepth.resize(ovd.pboDepth.size());
          glGenBuffers(ovd.pboDepth.size(), ovd.pboDepth.data());
          for (size_t i=0; i<ovd.pboDepth.size(); ++i) {
             glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboDepth[i]);
             glBufferStorage(GL_PIXEL_PACK_BUFFER, w*h*sizeof(GLfloat), nullptr, flags);
             ovd.mapDepth[i] = static_cast<GLfloat *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, w*h*sizeof(GLfloat), GL_MAP_READ_BIT|GL_MAP_PERSISTENT_BIT));
          }

          ovd.mapColor.resize(ovd.pboColor.size());
          glGenBuffers(ovd.pboColor.size(), ovd.pboColor.data());
          for (size_t i=0; i<ovd.pboColor.size(); ++i) {
             glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboColor[i]);
             glBufferStorage(GL_PIXEL_PACK_BUFFER, w*h*4, nullptr, flags);
             ovd.mapColor[i] = static_cast<GLchar *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, w*h*4, GL_MAP_READ_BIT|GL_MAP_PERSISTENT_BIT));
          }
       }

       size_t pbo = ovd.readPbo;
       std::cerr << "reading to pbo idx " << pbo << std::endl;

       glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboDepth[pbo]);
       glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

       glBindBuffer(GL_PIXEL_PACK_BUFFER, ovd.pboColor[pbo]);
       glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

       glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

       glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
       GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
       ovd.outstanding.push_back(sync);
       ovd.compositePbo.push_back(pbo);
       ovd.readPbo = (pbo+1) % ovd.pboDepth.size();
   }

   OsgViewData &ovd;
   vistle::ParallelRemoteRenderManager &renderMgr;
   OpenThreads::Mutex *icetMutex;
   size_t viewIdx;
};


}

OsgViewData::OsgViewData(OSGRenderer &viewer, size_t viewIdx)
: viewer(viewer)
, viewIdx(viewIdx)
{
   readPbo = 0;
   pboDepth.resize(3);
   pboColor.resize(3);
   update();
}

void OsgViewData::createCamera() {

    const vistle::ParallelRemoteRenderManager::PerViewState &vd = viewer.m_renderManager.viewData(viewIdx);

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

   viewer.addSlave(camera);
   viewer.realize();
}


void OsgViewData::update() {

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
        }
#endif
    }
    if (create) {
        if (gc) {
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
       osg::ref_ptr<osg::DisplaySettings> ds = new osg::DisplaySettings;
       ds->setStereo(false);

       osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(ds.get());
       traits->readDISPLAY();
       traits->setUndefinedScreenDetailsToDefaultScreen();
       //traits->sharedContext = viewer.getCamera()->getGraphicsContext();
       traits->x = 0;
       traits->y = 0;
       traits->width = vd.width;
       traits->height = vd.height;
       traits->alpha = 8;
       traits->depth = 24;
       traits->doubleBuffer = false;
       traits->windowDecoration = true;
       traits->windowName = "OsgRenderer: slave " + boost::lexical_cast<std::string>(viewIdx);
       //traits->pbuffer =true;
       traits->vsync = false;
       gc = osg::GraphicsContext::createGraphicsContext(traits);
       gc->realize();
#if 0
       if (gc->isRealized()) {
          gc->makeCurrent();
          std::cerr << "enabling debug extension" << std::endl;
          auto dbg = new EnableGLDebugOperation;
          (*dbg)(gc);
          std::cerr << "enabling debug extension: done" << std::endl;
       } else {
          std::cerr << "not enabling debug extension: graphics context not realized" << std::endl;
       }
#endif
       camera->setGraphicsContext(gc);
       camera->setDisplaySettings(ds);
       camera->setRenderTargetImplementation(osg::Camera::PIXEL_BUFFER);
       GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
       camera->setDrawBuffer(buffer);
       camera->setReadBuffer(buffer);
#endif
       camera->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);

       camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
       camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

       camera->setRenderOrder(osg::Camera::PRE_RENDER);
       camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
       camera->setViewMatrix(osg::Matrix::identity());
       camera->setProjectionMatrix(osg::Matrix::identity());
       resize = true;
    }
    if (resize) {
       camera->setViewport(new osg::Viewport(0, 0, vd.width, vd.height));
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
       camera->setPostDrawCallback(new ReadOperation(*this, viewer.m_renderManager, viewer.icetMutex, viewIdx, viewer.rank()));
    }

    //osg::Matrix mv =  toOsg(vd.model) * toOsg(vd.view);
   camera->setViewMatrix(toOsg(vd.view));
   camera->setProjectionMatrix(toOsg(vd.proj));
}

void OsgViewData::composite() {

    if (outstanding.empty()) {
        std::cerr << "no outstanding frames to composite" << std::endl;
        return;
    }

    //glWaitSync(fin, 0, GL_TIMEOUT_IGNORED);
    GLenum ret = 0;
    while ((ret = glClientWaitSync(outstanding.front(), GL_SYNC_FLUSH_COMMANDS_BIT, 1000)) == GL_TIMEOUT_EXPIRED)
       ;
    outstanding.pop_front();
    size_t pbo = compositePbo.front();
    compositePbo.pop_front();

    if (ret == GL_WAIT_FAILED) {
       std::cerr << "GL sync wait failed" << std::endl;
       return;
    }

    const auto &vd = viewer.m_renderManager.viewData(viewIdx);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*viewer.icetMutex);
    viewer.m_renderManager.setCurrentView(viewIdx);
    //renderMgr.updateRect(viewIdx, const IceTInt *viewport, const IceTImage image);
    const osg::Vec4 clear = camera->getClearColor();
    IceTFloat bg[4] = { clear[0], clear[1], clear[2], clear[3] };
    IceTDouble proj[16], view[16];
    IceTInt viewport[4] = {0, 0, vd.width, vd.height};
    viewer.m_renderManager.getModelViewMat(viewIdx, view);
    viewer.m_renderManager.getProjMat(viewIdx, proj);
    IceTImage image = icetCompositeImage(mapColor[pbo], mapDepth[pbo], viewport, proj, view, bg);
    viewer.m_renderManager.finishCurrentView(image);
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

static void callbackIceT(const IceTDouble * proj, const IceTDouble * mv,
        const IceTFloat * bg, const IceTInt * viewport,
        IceTImage result) {

   IceTSizeType width = icetImageGetWidth(result);
   IceTSizeType height = icetImageGetHeight(result);

   //std::cerr << "icet draw: w=" << width << ",h=" << height << std::endl;


   //IceTEnum cf = icetImageGetColorFormat(result);
   //IceTEnum df = icetImageGetDepthFormat(result);

   IceTUByte *color = icetImageGetColorub(result);
   IceTFloat *depth = icetImageGetDepthf(result);

   if (color) {
      glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, color);
   }
   if (depth) {
      glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
   }
}

OSGRenderer::OSGRenderer(const std::string &shmname, const std::string &name, int moduleID)
   : Renderer("OSGRenderer", shmname, name, moduleID), osgViewer::Viewer()
   , m_renderManager(this, callbackIceT)
{
#if 0
   std::cerr << "waiting for debugger: pid=" << getpid() << std::endl;
   sleep(10);
#endif
   icetMutex = new OpenThreads::Mutex;

   m_debugLevel = addIntParameter("debug", "debug level", 1);
   m_visibleView = addIntParameter("visible_view", "no. of visible view (positive values will open window)", -1);
   setParameterRange(m_visibleView, (vistle::Integer)-1, (vistle::Integer)(m_renderManager.numViews())-1);

   osg::ref_ptr<osg::DisplaySettings> ds = new osg::DisplaySettings;
   ds->setStereo(false);

   osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(ds.get());
   traits->readDISPLAY();
   traits->setUndefinedScreenDetailsToDefaultScreen();
   traits->x = 0;
   traits->y = 0;
   traits->width=700;
   traits->height=700;
   traits->alpha = 8;
   traits->windowDecoration = true;
   traits->windowName = "OsgRenderer: master";
   traits->doubleBuffer = false;
   traits->sharedContext = 0;
   //traits->pbuffer = true;
   traits->vsync = false;
   osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
   if (!gc)
   {
       std::cerr << "Failed to create master graphics context" << std::endl;
       return;
   }

   getCamera()->setGraphicsContext(gc.get());
   getCamera()->setDisplaySettings(ds.get());
   getCamera()->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
   getCamera()->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);

   GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
   getCamera()->setDrawBuffer(buffer);
   getCamera()->setReadBuffer(buffer);
                                  
   setRealizeOperation(new EnableGLDebugOperation());
   realize();

   setThreadingModel(ThreadPerCamera);
   //setThreadingModel(SingleThreaded);

   getCamera()->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 0.0));
   getCamera()->setViewMatrix(osg::Matrix::identity());
   getCamera()->setProjectionMatrix(osg::Matrix::identity());
   scene = new osg::MatrixTransform();
   timesteps = new TimestepHandler;
   scene->addChild(timesteps->root());
   setSceneData(scene);
   rootState = scene->getOrCreateStateSet();

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


bool OSGRenderer::render() {

    const size_t numTimesteps = timesteps->numTimesteps();
    if (!m_renderManager.prepareFrame(numTimesteps)) {
       return false;
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
    }

    for (size_t i=0; i<m_viewData.size(); ++i) {
       m_viewData[i]->update();
       vassert(m_viewData[i]->outstanding.empty());
    }

    frame();

    for (size_t i=0; i<m_viewData.size(); ++i) {
       m_viewData[i]->composite();
    }

    return true;
}

bool OSGRenderer::parameterChanged(const vistle::Parameter *p) {

   m_renderManager.handleParam(p);
   return Renderer::parameterChanged(p);
}

boost::shared_ptr<vistle::RenderObject> OSGRenderer::addObject(int senderId, const std::string &senderPort,
            vistle::Object::const_ptr container,
            vistle::Object::const_ptr geometry,
            vistle::Object::const_ptr normals,
            vistle::Object::const_ptr colors,
            vistle::Object::const_ptr texture) {

   boost::shared_ptr<vistle::RenderObject> ro;
   VistleGeometryGenerator gen(geometry, normals, colors, texture);
   if (VistleGeometryGenerator::isSupported(geometry->getType()) || geometry->getType() == vistle::Object::PLACEHOLDER) {
      auto geode = gen(defaultState);

      if (geode) {
         ro.reset(new OsgRenderObject(senderId, senderPort, container, geometry, normals, colors, texture, geode));
         timesteps->addObject(geode, ro->timestep);
      }
   }

   m_renderManager.addObject(ro);

   return ro;
}

void OSGRenderer::removeObject(boost::shared_ptr<vistle::RenderObject> ro) {

   auto oro = boost::static_pointer_cast<OsgRenderObject>(ro);
   timesteps->removeObject(oro->node, oro->timestep);
   m_renderManager.removeObject(ro);
}

OsgRenderObject::OsgRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture,
         osg::ref_ptr<osg::Node> node)
: vistle::RenderObject(senderId, senderPort, container, geometry, normals, colors, texture)
, node(node)
{
}

MODULE_MAIN(OSGRenderer)
