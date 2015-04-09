#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <osgViewer/Viewer>
#include <osg/Sequence>
#include <osg/MatrixTransform>

#include <renderer/renderer.h>
#include <renderer/vnccontroller.h>
#include <renderer/parrendmgr.h>

namespace osg {
   class Group;
   class Geode;
   class Material;
   class LightModel;
}

namespace vistle {
   class Object;
}


class OsgRenderObject: public vistle::RenderObject {

 public:
   OsgRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture,
         osg::ref_ptr<osg::Node> node);

      osg::ref_ptr<osg::Node> node;
};


class TimestepHandler: public osg::Referenced {

public:
   TimestepHandler();

   void addObject(osg::Node * geode, const int step);
   void removeObject(osg::Node * geode, const int step);

   osg::ref_ptr<osg::MatrixTransform> root() const;

   bool setTimestep(const int timestep);
   size_t numTimesteps() const;
   void getBounds(vistle::Vector3 &min, vistle::Vector3 &max) const;

 private:
   int timestep;

   osg::ref_ptr<osg::MatrixTransform> m_root;
   osg::ref_ptr<osg::Group> m_fixed;
   osg::ref_ptr<osg::Sequence> m_animated;
};


class OSGRenderer;


struct OsgViewData {

   OsgViewData(OSGRenderer &viewer, size_t viewIdx);
   void createCamera();
   void update();
   void composite();

   OSGRenderer &viewer;
   size_t viewIdx;
   osg::ref_ptr<osg::GraphicsContext> gc;
   osg::ref_ptr<osg::Camera> camera;
   std::vector<GLuint> pboDepth, pboColor;
   std::vector<GLchar*> mapColor;
   std::vector<GLfloat*> mapDepth;
   size_t readPbo;
   std::deque<GLsync> outstanding;
   std::deque<size_t> compositePbo;
};


class OSGRenderer: public vistle::Renderer, public osgViewer::Viewer {

   friend struct OsgViewData;

 public:
   OSGRenderer(const std::string &shmname, const std::string &name, int moduleID);
   ~OSGRenderer();

 private:
   boost::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture) override;
   void removeObject(boost::shared_ptr<vistle::RenderObject> ro) override;
   bool parameterChanged(const vistle::Parameter *p) override;

   bool render() override;

   vistle::IntParameter *m_debugLevel;
   vistle::IntParameter *m_visibleView;
   std::vector<boost::shared_ptr<OsgViewData>> m_viewData;

   vistle::ParallelRemoteRenderManager m_renderManager;

   osg::ref_ptr<osg::MatrixTransform> scene;

   osg::ref_ptr<osg::Material> material;
   osg::ref_ptr<osg::LightModel> lightModel;
   osg::ref_ptr<osg::StateSet> defaultState;
   osg::ref_ptr<osg::StateSet> rootState;

   osg::ref_ptr<TimestepHandler> timesteps;
   std::vector<osg::ref_ptr<osg::LightSource>> lights;
   OpenThreads::Mutex *icetMutex;
};

#endif
