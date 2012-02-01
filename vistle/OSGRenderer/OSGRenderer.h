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
   TimestepHandler() {
      timestep = timesteps.begin();
   }

   void addObject(osg::Geode * geode, const int step) {

      std::vector<osg::Geode *> *vector = NULL;
      std::map<int, std::vector<osg::Geode *> *>::iterator i =
         timesteps.find(step);
      if (i != timesteps.end())
         vector = i->second;
      else {
         vector = new std::vector<osg::Geode *>;
         timesteps[step] = vector;
         timestep = timesteps.begin();
      }

      vector->push_back(geode);
   }

   void hideTimestep() {
      std::vector<osg::Geode *>::iterator i;
      for (i = timestep->second->begin(); i != timestep->second->end(); i ++)
         (*i)->setNodeMask(0);
   }

   void showTimestep() {
      printf("time: %d\n", timestep->first);
      std::vector<osg::Geode *>::iterator i;
      for (i = timestep->second->begin(); i != timestep->second->end(); i ++)
         (*i)->setNodeMask(-1);
   }

   bool handle(const osgGA::GUIEventAdapter & ea,
               osgGA::GUIActionAdapter & aa) {

      switch (ea.getScrollingMotion()) {

         case osgGA::GUIEventAdapter::SCROLL_UP:
            printf("UP\n");
            hideTimestep();
            if (++timestep == timesteps.end())
               timestep = timesteps.begin();
            showTimestep();

            ea.setHandled(true);
            return true;
            break;

         case osgGA::GUIEventAdapter::SCROLL_DOWN:
            printf("DOWN\n");
            hideTimestep();
            if (timestep-- == timesteps.begin())
               timestep = --timesteps.end();
            showTimestep();

            ea.setHandled(true);
            return true;
            break;

         default:
            break;
      }

      /*
      if (step != 0 && step != -1)
         geode->setNodeMask(0);
      */
      return false;
   }

   int getNumTimesteps();

 private:
   std::map<int, std::vector<osg::Geode *> *> timesteps;
   std::map<int, std::vector<osg::Geode *> *>::iterator timestep;
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

   osg::Group *scene;
   std::map<std::string, osg::ref_ptr<osg::Geode> > nodes;

   osg::ref_ptr<osg::Material> material;
   osg::ref_ptr<osg::LightModel> lightModel;

   TimestepHandler *timesteps;
};

#endif
