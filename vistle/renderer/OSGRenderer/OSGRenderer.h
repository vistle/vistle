#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>
#include <osg/Sequence>

#include <renderer/renderer.h>

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

class TimestepHandler: public osgGA::GUIEventHandler {

public:
   TimestepHandler();

   void addObject(osg::Node * geode, const int step);
   void removeObject(osg::Node * geode, const int step);
   bool handle(const osgGA::GUIEventAdapter & ea,
               osgGA::GUIActionAdapter & aa,
               osg::Object *obj,
               osg::NodeVisitor *nv);
   void getUsage(osg::ApplicationUsage &usage) const;

   osg::ref_ptr<osg::Group> root() const;

 private:
   bool setTimestep(const int timestep);
   int firstTimestep();
   int lastTimestep();

   int timestep;

   osg::ref_ptr<osg::Group> m_root;
   osg::ref_ptr<osg::Group> m_fixed;
   osg::ref_ptr<osg::Sequence> m_animated;
};

class OSGRenderer: public vistle::Renderer, public osgViewer::Viewer {

 public:
   OSGRenderer(const std::string &shmname, const std::string &name, int moduleID);
   ~OSGRenderer();
   void scheduleResize(int x, int y, int w, int h);

 private:
   boost::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture) override;
   void removeObject(boost::shared_ptr<vistle::RenderObject> ro) override;

   void render();
   void distributeAndHandleEvents();
   void distributeModelviewMatrix();
   void distributeProjectionMatrix();
   void resize(int x, int y, int w, int h);

   osg::ref_ptr<osg::Group> scene;

   osg::ref_ptr<osg::Material> material;
   osg::ref_ptr<osg::LightModel> lightModel;
   osg::ref_ptr<osg::StateSet> defaultState;

   osg::ref_ptr<TimestepHandler> timesteps;

   bool m_resize;
   int m_x;
   int m_y;
   int m_width;
   int m_height;
};

#endif
