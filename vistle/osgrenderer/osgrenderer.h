#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <map>

#include <osgViewer/Viewer>

#include "renderer.h"

namespace osg {
   class Group;
   class Geode;
}

namespace vistle {
   class Object;
}

class OSGRenderer: public vistle::Renderer, public osgViewer::Viewer {

 public:
   OSGRenderer(int rank, int size, int moduleID);
   ~OSGRenderer();

 private:
   bool compute();
   bool addInputObject(const std::string & portName,
                       const vistle::Object * object);

   void render();

   osg::Group *scene;
   std::map<std::string, osg::ref_ptr<osg::Geode> > nodes;
};

#endif
