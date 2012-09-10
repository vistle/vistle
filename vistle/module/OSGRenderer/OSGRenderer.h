#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <map>

#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>

#include "renderer.h"

namespace osg {
   class Group;
   class Geode;
   class Material;
   class LightModel;
}

namespace vistle {
   class Object;
}

class TimestepHandler: public osgGA::GUIEventHandler {

public:
   TimestepHandler();

   void addObject(osg::Geode * geode, const int step);
   bool handle(const osgGA::GUIEventAdapter & ea,
               osgGA::GUIActionAdapter & aa,
               osg::Object *obj,
               osg::NodeVisitor *nv);

 private:
   bool setTimestepState(const int timestep, const int state);
   int firstTimestep();
   int lastTimestep();

   std::map<int, std::vector<osg::Geode *> *> timesteps;

   int timestep;
   double lastEvent;
};

class OSGRenderer: public vistle::Renderer, public osgViewer::Viewer {

 public:
   OSGRenderer(int rank, int size, int moduleID);
   ~OSGRenderer();

 private:
   bool compute();
   void addInputObject(const vistle::Object * geometry,
                       const vistle::Object * colors,
                       const vistle::Object * normals,
                       const vistle::Object * texture);

   bool addInputObject(const std::string & portName,
                       const vistle::Object * object);

   void render();
   void distributeAndHandleEvents();
   void distributeModelviewMatrix();
   void distributeProjectionMatrix();

   osg::Group *scene;
   std::map<std::string, osg::ref_ptr<osg::Geode> > nodes;

   osg::ref_ptr<osg::Material> material;
   osg::ref_ptr<osg::LightModel> lightModel;

   TimestepHandler timesteps;
};

#endif
