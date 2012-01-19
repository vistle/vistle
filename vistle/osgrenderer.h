#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <osgViewer/Viewer>

#include "renderer.h"

class OSGRenderer: public vistle::Renderer, public osgViewer::Viewer {

 public:
   OSGRenderer(int rank, int size, int moduleID);
   ~OSGRenderer();

 private:
   void render();
};

#endif
