#include <future>
#include <boost/algorithm/string/predicate.hpp>

// cover
#include <cover/coVRPluginSupport.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coCommandLine.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRPluginList.h>

// vistle
#include <renderer/renderer.h>
#include <util/exception.h>
#include <core/message.h>
#include <core/object.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/polygons.h>
#include <core/texture1d.h>


#include <osg/Group>
#include <osg/Node>
#include <osg/Sequence>

#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>

#include "VistleRenderObject.h"
#include "VistleGeometryGenerator.h"
#include "VistleInteractor.h"

using namespace opencover;

using namespace vistle;

class PluginRenderObject: public vistle::RenderObject {

 public:
   PluginRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture)
      : vistle::RenderObject(senderId, senderPort, container, geometry, normals, colors, texture)
      {
      }

   ~PluginRenderObject() {
   }

   boost::shared_ptr<VistleRenderObject> coverRenderObject;
};

class OsgRenderer: public vistle::Renderer {

 public:
   OsgRenderer(const std::string &shmname,
         const std::string &name, int moduleId);
   ~OsgRenderer();

   bool render() override;
   boost::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
      vistle::Object::const_ptr container,
      vistle::Object::const_ptr geometry,
      vistle::Object::const_ptr normals,
      vistle::Object::const_ptr colors,
      vistle::Object::const_ptr texture) override;
   void removeObject(boost::shared_ptr<vistle::RenderObject> ro) override;

   osg::ref_ptr<osg::Group> vistleRoot;

   bool parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName) override;
   bool parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg) override;

   typedef std::map<int, VistleInteractor *> InteractorMap;
   InteractorMap m_interactorMap;

   struct DelayedObject {
      DelayedObject(boost::shared_ptr<PluginRenderObject> ro, VistleGeometryGenerator generator)
         : ro(ro)
         , generator(generator)
         , node_future(std::async(std::launch::async, generator))
      {}
      boost::shared_ptr<PluginRenderObject> ro;
      VistleGeometryGenerator generator;
      std::future<osg::Node *> node_future;
   };
   std::deque<DelayedObject> m_delayedObjects;

 protected:
   struct Creator {
      Creator(int id, const std::string &basename, osg::ref_ptr<osg::Group> parent)
      : id(id)
      {
         std::stringstream s;
         s << basename << "_" << id;
         name = s.str();

         root = new osg::Group;
         root->setName(name);

         constant = new osg::Group;
         constant->setName(name + "_static");
         root->addChild(constant);

         animated = new osg::Sequence;
         animated->setName(name + "_animated");
         root->addChild(animated);

         VRSceneGraph::instance()->addNode(root, parent, NULL);
      }
      int id;
      std::string name;
      osg::ref_ptr<osg::Group> root;
      osg::ref_ptr<osg::Group> constant;
      osg::ref_ptr<osg::Sequence> animated;
   };
   typedef std::map<int, Creator> CreatorMap;
   CreatorMap creatorMap;

   Creator &getCreator(int id) {
      auto it = creatorMap.find(id);
      if (it == creatorMap.end()) {
         std::string name = getModuleName(id);
         it = creatorMap.insert(std::make_pair(id, Creator(id, name, vistleRoot))).first;
      }
      return it->second;
   }
};

OsgRenderer::OsgRenderer(const std::string &shmname,
      const std::string &name, int moduleId)
: vistle::Renderer("COVER", shmname, name, moduleId)
{
   vistle::registerTypes();
   //createInputPort("data_in"); - already done by Renderer base class

   vistleRoot = new osg::Group;
   vistleRoot->setName("VistlePlugin");
   VRSceneGraph::instance()->addNode(vistleRoot, (osg::Group*)NULL, NULL);

   m_fastestObjectReceivePolicy = message::ObjectReceivePolicy::Distribute;
   setObjectReceivePolicy(m_fastestObjectReceivePolicy);

   initDone();
}

OsgRenderer::~OsgRenderer() {

   VRSceneGraph::instance()->deleteNode("VistlePlugin", true);
}

bool OsgRenderer::parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName) {

   std::string plugin = moduleName;
   if (boost::algorithm::ends_with(name, "Old"))
      plugin = plugin.substr(0, plugin.size()-3);
   if (plugin == "CutGeometry")
      plugin = "CuttingSurface";
    std::cerr << "parameterAdded: sender=" <<  senderId << ", name=" << name << ", plugin=" << plugin << std::endl;

   if (plugin == "CuttingSurface"
         || plugin == "Tracer"
         || plugin == "IsoSurface") {
      cover->addPlugin(plugin.c_str());

#if 0
      auto creator = creatorMap.find(senderId);
      if (creator == creatorMap.end()) {
         creatorMap.insert(std::make_pair(senderId, Creator(senderId, moduleName, vistleRoot)));
      }
#endif

      InteractorMap::iterator it = m_interactorMap.find(senderId);
      if (it == m_interactorMap.end()) {
         m_interactorMap[senderId] = new VistleInteractor(this, moduleName, senderId);
         it = m_interactorMap.find(senderId);
         std::cerr << "created interactor for " << moduleName << ":" << senderId << std::endl;
      }
      VistleInteractor *inter = it->second;
      inter->addParam(name, msg);
   }
   return true;
}

bool OsgRenderer::parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg) {

   InteractorMap::iterator it = m_interactorMap.find(senderId);
   if (it == m_interactorMap.end()) {
      return false;
   }

   VistleInteractor *inter = it->second;
   inter->applyParam(name, msg);
   coVRPluginList::instance()->newInteractor(inter->getObject(), inter);

   return true;
}

void OsgRenderer::removeObject(boost::shared_ptr<vistle::RenderObject> vro) {
   auto pro = boost::static_pointer_cast<PluginRenderObject>(vro);
   auto ro = pro->coverRenderObject;

   if (!ro) {
      std::cerr << "removeObject: no COVER render object" << std::endl;
      return;
   }

   if (!ro->isGeometry()) {
      std::cerr << "removeObject: Node is not geometry: " << ro->getName() << std::endl;
      return;
   }

   osg::ref_ptr<osg::Node> node = ro->node();
   if (node) {
      VRSceneGraph::instance()->deleteNode(ro->getName(), false);
      Creator &cr= getCreator(pro->container->getCreator());
      if (pro->timestep == -1) {
         cr.constant->removeChild(node);
      } else {
         if (cr.animated->getNumChildren() > pro->timestep) {
            osg::Group *gr = cr.animated->getChild(pro->timestep)->asGroup();
            if (gr) {
               gr->removeChild(node);
               if (gr->getNumChildren() == 0) {
                  bool removed = false;
                  for (size_t i=cr.animated->getNumChildren()-1; i>0; --i) {
                     osg::Group *g = cr.animated->getChild(i)->asGroup();
                     if (!g)
                        break;
                     if (g->getNumChildren() > 0)
                        break;
                     cr.animated->removeChildren(i, 1);
                     removed = true;
                  }
                  if (removed) {
                     if (cr.animated->getNumChildren() > 0) {
                        coVRAnimationManager::instance()->addSequence(cr.animated);
                     } else {
                        coVRAnimationManager::instance()->removeSequence(cr.animated);
                     }
                  }
               }
            }
         }
      }
      while (node->getNumParents() > 0) {
         osg::Group *parent = node->getParent(0);
         parent->removeChild(node);
      }
   }
   pro->coverRenderObject.reset();
}

boost::shared_ptr<vistle::RenderObject> OsgRenderer::addObject(int senderId, const std::string &senderPort,
      vistle::Object::const_ptr container,
      vistle::Object::const_ptr geometry,
      vistle::Object::const_ptr normals,
      vistle::Object::const_ptr colors,
      vistle::Object::const_ptr texture) {

   if (!container)
      return nullptr;
   if (!geometry)
      return nullptr;

   int creatorId = container->getCreator();
   getCreator(creatorId);

   if (geometry->getType() != vistle::Object::PLACEHOLDER && !VistleGeometryGenerator::isSupported(geometry->getType()))
      return nullptr;

   boost::shared_ptr<PluginRenderObject> pro(new PluginRenderObject(senderId, senderPort,
         container, geometry, normals, colors, texture));

   pro->coverRenderObject.reset(new VistleRenderObject(pro));
   m_delayedObjects.push_back(DelayedObject(pro, VistleGeometryGenerator(geometry, colors, normals, texture)));

   return pro;
}

bool OsgRenderer::render() {

   if (m_delayedObjects.empty())
      return false;

   int numReady = 0;
   for (size_t i=0; i<m_delayedObjects.size(); ++i) {
      auto &node_future = m_delayedObjects[i].node_future;
      if (!node_future.valid()) {
         std::cerr << "OsgRenderer::render(): future not valid" << std::endl;
         break;
      }
      auto status = node_future.wait_for(std::chrono::seconds(0));
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ <= 6)
      if (!status)
         break;
#else
      if (status != std::future_status::ready)
         break;
#endif
      ++numReady;
   }

   int numAdd = 0;
   MPI_Allreduce(&numReady, &numAdd, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

   //std::cerr << "adding " << numAdd << " delayed objects, " << m_delayedObjects.size() << " waiting" << std::endl;
   for (int i=0; i<numAdd; ++i) {
      auto &node_future = m_delayedObjects.front().node_future;
      auto &ro = m_delayedObjects.front().ro;
      osg::Node *node = node_future.get();
      if (ro->coverRenderObject && node) {
         int creatorId = ro->coverRenderObject->getCreator();
         Creator &creator = getCreator(creatorId);

         ro->coverRenderObject->setNode(node);
         node->setName(ro->coverRenderObject->getName());
         osg::ref_ptr<osg::Group> parent = creator.constant;
         const int t = ro->timestep;
         if (t >= 0) {
            while (size_t(t) >= creator.animated->getNumChildren()) {
               creator.animated->addChild(new osg::Group);
            }
            parent = dynamic_cast<osg::Group *>(creator.animated->getChild(t));
            assert(parent);
            coVRAnimationManager::instance()->addSequence(creator.animated);
         }
         VRSceneGraph::instance()->addNode(node, parent, ro->coverRenderObject.get());
      } else if (!ro->coverRenderObject) {
         std::cerr << rank() << ": discarding delayed object - already deleted" << std::endl;
      } else if (!node) {
         std::cerr << rank() << ": discarding delayed object " << ro->coverRenderObject->getName() << ": no node created" << std::endl;
      }
      m_delayedObjects.pop_front();
   }

   return true;
}

class VistlePlugin: public opencover::coVRPlugin, public vrui::coMenuListener {

 public:
   VistlePlugin();
   ~VistlePlugin();
   bool init() override;
   void menuEvent(vrui::coMenuItem *item) override;
   void preFrame() override;
   void requestQuit(bool killSession) override;
   bool executeAll();

 private:
   OsgRenderer *m_module;
   vrui::coButtonMenuItem *executeButton;
};

using opencover::coCommandLine;

VistlePlugin::VistlePlugin()
: m_module(nullptr)
{
   int initialized = 0;
   MPI_Initialized(&initialized);
   if (!initialized) {
      std::cerr << "VistlePlugin: no MPI support - started from within Vistle?" << std::endl;
      //throw(vistle::exception("no MPI support"));
      return;
   }

   if (coCommandLine::argc() < 3) {
      for (int i=0; i<coCommandLine::argc(); ++i) {
         std::cout << i << ": " << coCommandLine::argv(i) << std::endl;
      }
      throw(vistle::exception("at least 2 command line arguments required"));
   }

   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   const std::string &shmname = coCommandLine::argv(1);
   const std::string &name = coCommandLine::argv(2);
   int moduleID = atoi(coCommandLine::argv(3));

   m_module = new OsgRenderer(shmname, name, moduleID);
}

VistlePlugin::~VistlePlugin() {

   if (m_module) {
      MPI_Barrier(MPI_COMM_WORLD);
      delete m_module;
   }
}

bool VistlePlugin::init() {

   VRMenu *covise = VRPinboard::instance()->namedMenu("COVISE");
   if (!covise)
   {
      VRPinboard::instance()->addMenu("COVISE", VRPinboard::instance()->mainMenu->getCoMenu());
      covise = VRPinboard::instance()->namedMenu("COVISE");
      cover->addSubmenuButton("COVISE...", NULL, "COVISE", false, NULL, -1, this);
   }
   vrui::coMenu *coviseMenu = covise->getCoMenu();
   executeButton = new vrui::coButtonMenuItem("Execute");
   executeButton->setMenuListener(this);
   coviseMenu->add(executeButton);

   return m_module;
}

void VistlePlugin::menuEvent(vrui::coMenuItem *item) {

   if (item == executeButton) {
      executeAll();
   }
}

void VistlePlugin::preFrame() {

   MPI_Barrier(MPI_COMM_WORLD);
   if (m_module && !m_module->dispatch()) {
      std::cerr << "Vistle requested COVER to quit" << std::endl;
      OpenCOVER::instance()->quitCallback(NULL,NULL);
   }
}

void VistlePlugin::requestQuit(bool killSession)
{
   delete m_module;
   m_module = nullptr;
}

bool VistlePlugin::executeAll() {

   message::Execute exec; // execute all sources in data flow graph
   exec.setDestId(message::Id::MasterHub);
   m_module->sendMessage(exec);
   return true;
}

COVERPLUGIN(VistlePlugin);
