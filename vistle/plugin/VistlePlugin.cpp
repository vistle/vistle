// cover
#include <kernel/coVRPluginSupport.h>
#include <kernel/coVRPluginSupport.h>
#include <kernel/VRSceneGraph.h>
#include <kernel/coVRAnimationManager.h>
#include <kernel/coCommandLine.h>
#include <kernel/OpenCOVER.h>

// vistle
#include <core/renderer.h>
#include <core/exception.h>

#include <osg/Group>
#include <osg/Node>
#include <osg/Sequence>

#include <core/object.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/polygons.h>
#include <core/texture1d.h>
#include <core/geometry.h>

#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>

#include "VistleRenderObject.h"
#include "VistleGeometryGenerator.h"

using namespace opencover;

using namespace vistle;

class OsgRenderer: public vistle::Renderer {
   public:
   OsgRenderer(const std::string &shmname,
         int rank, int size, int moduleId);
   ~OsgRenderer();

   bool compute();
   void render();

   void addInputObject(vistle::Object::const_ptr geometry,
                                 vistle::Object::const_ptr colors,
                                 vistle::Object::const_ptr normals,
                                 vistle::Object::const_ptr texture);
   bool addInputObject(const std::string & portName,
                                 vistle::Object::const_ptr object);

   osg::ref_ptr<osg::Group> staticGeo;
   osg::ref_ptr<osg::Sequence> animation;

   std::map<std::string, VistleRenderObject *> objects;
   bool removeObject(VistleRenderObject *ro);
};

OsgRenderer::OsgRenderer(const std::string &shmname,
      int rank, int size, int moduleId)
: vistle::Renderer("COVER", shmname, rank, size, moduleId)
{
   m_mpiFinalize = false; // cover will call that for us

   createInputPort("data_in");

   staticGeo = new osg::Group;
   staticGeo->setName("vistle_static_geometry");
   animation = new osg::Sequence;
   animation->setName("vistle_animated_geometry");
   VRSceneGraph::instance()->addNode(staticGeo, (osg::Group*)NULL, NULL);
   VRSceneGraph::instance()->addNode(animation, (osg::Group*)NULL, NULL);
}

OsgRenderer::~OsgRenderer() {

   std::map<std::string, VistleRenderObject *>::iterator next;
   for (std::map<std::string, VistleRenderObject *>::iterator it = objects.begin(), next=it;
         it != objects.end();
         it = next) {
      next = it;
      ++next;

      VistleRenderObject *ro = it->second;
      removeObject(ro);
   }

   VRSceneGraph::instance()->deleteNode("vistle_static_geometry", true);
   VRSceneGraph::instance()->deleteNode("vistle_animated_geometry", true);
}

bool OsgRenderer::removeObject(VistleRenderObject *ro) {
   if (!ro->isGeometry()) {
      std::cerr << "Node is not geometry: " << ro->getName() << std::endl;
      return false;
   }

   std::string name = ro->getGeometry()->getName();
   std::map<std::string, VistleRenderObject *>::iterator it = objects.find(name);
   if (it == objects.end()) {
      std::cerr << "Did not find node for " << name << std::endl;
      return false;
   }

   osg::Node *node = ro->node();
   if (node) {
      VRSceneGraph::instance()->deleteNode(node->getName().c_str(), false);
      if (node->getNumParents() > 0) {
         osg::Group *parent = node->getParent(0);
         parent->removeChild(node);
      }
   }
   delete ro;
   objects.erase(it);

   return true;
}

bool OsgRenderer::compute() {

   return true;
}

void OsgRenderer::addInputObject(vistle::Object::const_ptr geometry,
                                 vistle::Object::const_ptr colors,
                                 vistle::Object::const_ptr normals,
                                 vistle::Object::const_ptr texture) {

   if (objects.find(geometry->getName()) != objects.end()) {
      std::cerr << "Object added twice: " << geometry->getName() << std::endl;
      return;
   }

   VistleGeometryGenerator generator(geometry, colors, normals, texture);
   osg::Node *node = generator();

   if (!node)
      return;

   VistleRenderObject *container = new VistleRenderObject();
   objects[geometry->getName()] = container;
   node->setName(geometry->getName());
   container->setNode(node);
   container->setGeometry(geometry);
   container->setColors(colors);
   container->setNormals(normals);
   container->setTexture(texture);

   osg::Group *parent = staticGeo;
   int t = geometry->getTimestep();
   if (t >= 0) {
      while (t >= animation->getNumChildren()) {
         animation->addChild(new osg::Group);
      }
      parent = dynamic_cast<osg::Group *>(animation->getChild(t));
      assert(parent);
      coVRAnimationManager::instance()->addSequence(animation);
   }
   VRSceneGraph::instance()->addNode(container->node(), parent, container);
}



bool OsgRenderer::addInputObject(const std::string & portName,
                                 vistle::Object::const_ptr object) {

   std::cout << "++++++OSGRenderer addInputObject " << object->getType()
             << " block " << object->getBlock()
             << " timestep " << object->getTimestep() << std::endl;

   switch (object->getType()) {

      case vistle::Object::TRIANGLES:
      case vistle::Object::POLYGONS:
      case vistle::Object::LINES: {

         addInputObject(object, vistle::Object::ptr(), vistle::Object::ptr(), vistle::Object::ptr());
         break;
      }

      case vistle::Object::GEOMETRY: {

         vistle::Geometry::const_ptr geom = vistle::Geometry::as(object);

         std::cerr << "   Geometry: [ "
            << (geom->geometry()?"G":".")
            << (geom->colors()?"C":".")
            << (geom->normals()?"N":".")
            << (geom->texture()?"T":".")
            << " ]" << std::endl;
         addInputObject(geom->geometry(), geom->colors(), geom->normals(),
                        geom->texture());

         break;
      }

      default:
         break;
   }

   return true;
}

void OsgRenderer::render() {

}

class VistlePlugin: public opencover::coVRPlugin {
   public:
   VistlePlugin();
   ~VistlePlugin();
   bool init();
   void preFrame();
   private:
   OsgRenderer *mod;
};

using opencover::coCommandLine;

VistlePlugin::VistlePlugin()
{
   if (coCommandLine::argc() < 3) {
      throw(vistle::exception("at least 2 command line arguments required"));
   }

   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   const std::string &shmname = coCommandLine::argv(1);
   int moduleID = atoi(coCommandLine::argv(2));

   mod = new OsgRenderer(shmname, rank, size, moduleID);
}

VistlePlugin::~VistlePlugin() {

   MPI_Barrier(MPI_COMM_WORLD);
   delete mod;
}

bool VistlePlugin::init() {

   return mod;
}

void VistlePlugin::preFrame() {

   if (!mod->dispatch()) {
      std::cerr << "Vistle requested COVER to quit" << std::endl;
      OpenCOVER::instance()->quitCallback(NULL,NULL);
   }
}

COVERPLUGIN(VistlePlugin);
