#include <future>
#include <boost/algorithm/string/predicate.hpp>

// cover
#include <cover/coVRPluginSupport.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coCommandLine.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRPluginList.h>
#include <cover/coVRFileManager.h>

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

#include <VistlePluginUtil/VistleRenderObject.h>
#include "VistleGeometryGenerator.h"
#include "VistleInteractor.h"

using namespace opencover;

using namespace vistle;


namespace {
coVRPlugin *plugin = nullptr;
}

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

   std::shared_ptr<VistleRenderObject> coverRenderObject;
};

class OsgRenderer: public vistle::Renderer {

 public:
   OsgRenderer(const std::string &shmname,
         const std::string &name, int moduleId);
   ~OsgRenderer();

   bool render() override;
   std::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
      vistle::Object::const_ptr container,
      vistle::Object::const_ptr geometry,
      vistle::Object::const_ptr normals,
      vistle::Object::const_ptr colors,
      vistle::Object::const_ptr texture) override;
   void removeObject(std::shared_ptr<vistle::RenderObject> ro) override;

   osg::ref_ptr<osg::Group> vistleRoot;

   bool parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName) override;
   bool parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg) override;
   bool parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg) override;
   void prepareQuit() override;

   typedef std::map<VistleRenderObject *, std::string> FileAttachmentMap;
   FileAttachmentMap m_fileAttachmentMap;

   typedef std::map<int, VistleInteractor *> InteractorMap;
   InteractorMap m_interactorMap;

   struct DelayedObject {
      DelayedObject(std::shared_ptr<PluginRenderObject> ro, VistleGeometryGenerator generator)
         : ro(ro)
         , generator(generator)
         , node_future(std::async(std::launch::async, generator))
      {}
      std::shared_ptr<PluginRenderObject> ro;
      VistleGeometryGenerator generator;
      std::future<osg::MatrixTransform *> node_future;
   };
   std::deque<DelayedObject> m_delayedObjects;

 protected:
   struct Variant
   {
       std::string variant;
       std::string name;
       osg::ref_ptr<osg::Group> root;
       osg::ref_ptr<osg::Group> constant;
       osg::ref_ptr<osg::Sequence> animated;
       VariantRenderObject ro;

       Variant(const std::string &basename, osg::ref_ptr<osg::Group> parent, const std::string &variant=std::string())
       : variant(variant)
       , ro(variant)
       {
           std::stringstream s;
           s << basename;
           if (!variant.empty())
               s << "/" << variant;
           name = s.str();

           root = new osg::Group;
           root->setName(name);

           constant = new osg::Group;
           constant->setName(name + "/static");
           root->addChild(constant);

           animated = new osg::Sequence;
           animated->setName(name + "/animated");
           root->addChild(animated);

           parent->addChild(root);
       }
   };

   struct Creator {
      Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent)
      : id(id)
      , name(name)
      , baseVariant(name, parent)
      {
      }

      const Variant &getVariant(const std::string &variantName) const {

          if (variantName.empty() || variantName == "NULL")
              return baseVariant;

          auto it = variants.find(variantName);
          if (it == variants.end()) {
              it = variants.emplace(std::make_pair(variantName, Variant(name, baseVariant.constant, variantName))).first;
              coVRPluginList::instance()->addNode(it->second.root, &it->second.ro, plugin);
          }
          return it->second;
      }

      osg::ref_ptr<osg::Group> root(const std::string &variant = std::string()) const { return getVariant(variant).root; }
      osg::ref_ptr<osg::Group> constant(const std::string &variant = std::string()) const { return getVariant(variant).constant; }
      osg::ref_ptr<osg::Sequence> animated(const std::string &variant = std::string()) const { return getVariant(variant).animated; }

      int id;
      std::string name;
      Variant baseVariant;
      mutable std::map<std::string, Variant> variants;
   };
   typedef std::map<int, Creator> CreatorMap;
   CreatorMap creatorMap;

   Creator &getCreator(int id) {
      auto it = creatorMap.find(id);
      if (it == creatorMap.end()) {
         std::stringstream name;
         name << getModuleName(id) << "_" << id;
         it = creatorMap.insert(std::make_pair(id, Creator(id, name.str(), vistleRoot))).first;
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
   cover->getObjectsRoot()->addChild(vistleRoot);

   m_fastestObjectReceivePolicy = message::ObjectReceivePolicy::Distribute;
   setObjectReceivePolicy(m_fastestObjectReceivePolicy);

   initDone();
}

OsgRenderer::~OsgRenderer() {

   prepareQuit();
}

bool OsgRenderer::parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName) {

   std::string plugin = moduleName;
   if (boost::algorithm::ends_with(plugin, "Old"))
      plugin = plugin.substr(0, plugin.size()-3);
   if (plugin == "CutGeometry")
      plugin = "CuttingSurface";
   // std::cerr << "parameterAdded: sender=" <<  senderId << ", name=" << name << ", plugin=" << plugin << std::endl;

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

bool OsgRenderer::parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg) {

    InteractorMap::iterator it = m_interactorMap.find(senderId);
    if (it != m_interactorMap.end()) {
        auto inter = it->second;
        inter->removeParam(name, msg);
        if (!inter->hasParams()) {
            std::cerr << "removing interactor for " << senderId << " (" << getModuleName(senderId) << ")" << std::endl;
            delete inter;
            m_interactorMap.erase(it);
        }
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

void OsgRenderer::prepareQuit() {

   removeAllObjects();
   cover->getObjectsRoot()->removeChild(vistleRoot);
   vistleRoot.release();

   Renderer::prepareQuit();
}

void OsgRenderer::removeObject(std::shared_ptr<vistle::RenderObject> vro) {
   auto pro = std::static_pointer_cast<PluginRenderObject>(vro);
   auto ro = pro->coverRenderObject;
   std::string variant = pro->variant;

   if (!ro) {
      std::cerr << "removeObject: no COVER render object" << std::endl;
      return;
   }

   coVRPluginList::instance()->removeObject(ro->getName(), false);

   auto it = m_fileAttachmentMap.find(ro.get());
   if (it != m_fileAttachmentMap.end()) {
       coVRFileManager::instance()->unloadFile(it->second.c_str());
       m_fileAttachmentMap.erase(it);
   }

   if (!ro->isGeometry()) {
      std::cerr << "removeObject: Node is not geometry: " << ro->getName() << std::endl;
      return;
   }

   osg::ref_ptr<osg::Node> node = ro->node();
   if (node) {
      coVRPluginList::instance()->removeNode(node, false, node);
      Creator &cr= getCreator(pro->container->getCreator());
      if (pro->timestep == -1) {
         cr.constant(variant)->removeChild(node);
      } else {
         if (ssize_t(cr.animated(variant)->getNumChildren()) > pro->timestep) {
            osg::Group *gr = cr.animated(variant)->getChild(pro->timestep)->asGroup();
            if (gr) {
               gr->removeChild(node);
               if (gr->getNumChildren() == 0) {
                  bool removed = false;
                  for (size_t i=cr.animated(variant)->getNumChildren(); i>1; --i) {
                     size_t idx = i-1;
                     osg::Group *g = cr.animated(variant)->getChild(idx)->asGroup();
                     if (!g)
                        break;
                     if (g->getNumChildren() > 0)
                        break;
                     cr.animated(variant)->removeChildren(idx, 1);
                     removed = true;
                  }
                  if (removed) {
                     if (cr.animated(variant)->getNumChildren() > 0) {
                        coVRAnimationManager::instance()->addSequence(cr.animated(variant));
                     } else {
                        coVRAnimationManager::instance()->removeSequence(cr.animated(variant));
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

std::shared_ptr<vistle::RenderObject> OsgRenderer::addObject(int senderId, const std::string &senderPort,
      vistle::Object::const_ptr container,
      vistle::Object::const_ptr geometry,
      vistle::Object::const_ptr normals,
      vistle::Object::const_ptr colors,
      vistle::Object::const_ptr texture) {

   if (!container)
      return nullptr;
   if (!geometry)
      return nullptr;

   std::string plugin = container->getAttribute("_plugin");
   if (!plugin.empty())
      cover->addPlugin(plugin.c_str());
   plugin = geometry->getAttribute("_plugin");
   if (!plugin.empty())
      cover->addPlugin(plugin.c_str());
   if (normals) {
      plugin = normals->getAttribute("_plugin");
      if (!plugin.empty())
         cover->addPlugin(plugin.c_str());
   }
   if (colors) {
      plugin = colors->getAttribute("_plugin");
      if (!plugin.empty())
         cover->addPlugin(plugin.c_str());
   }
   if (texture) {
      plugin = texture->getAttribute("_plugin");
      if (!plugin.empty())
         cover->addPlugin(plugin.c_str());
   }

   int creatorId = container->getCreator();
   getCreator(creatorId);

   if (geometry->getType() != vistle::Object::PLACEHOLDER && !VistleGeometryGenerator::isSupported(geometry->getType()))
      return nullptr;

   std::shared_ptr<PluginRenderObject> pro(new PluginRenderObject(senderId, senderPort,
         container, geometry, normals, colors, texture));
   if (!pro->variant.empty()) {
      cover->addPlugin("Variant");
   }

   pro->coverRenderObject.reset(new VistleRenderObject(pro));
   m_delayedObjects.push_back(DelayedObject(pro, VistleGeometryGenerator(pro, geometry, colors, normals, texture)));

   coVRPluginList::instance()->addObject(pro->coverRenderObject.get(), nullptr, nullptr, nullptr, nullptr, nullptr,
                                         0, 0, 0,
                                         nullptr, nullptr, nullptr, nullptr,
                                         0, 0, nullptr, nullptr, nullptr,
                                         0.0);

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
      osg::MatrixTransform *transform = node_future.get();
      if (ro->coverRenderObject && transform) {
         int creatorId = ro->coverRenderObject->getCreator();
         Creator &creator = getCreator(creatorId);

         ro->coverRenderObject->setNode(transform);
         transform->setName(ro->coverRenderObject->getName());
         const std::string variant = ro->variant;
         osg::ref_ptr<osg::Group> parent = creator.constant(variant);
         const int t = ro->timestep;
         if (t >= 0) {
            while (size_t(t) >= creator.animated(variant)->getNumChildren()) {
               auto g = new osg::Group;
               std::stringstream name;
               name << "t" << t;
               g->setName(name.str());
               creator.animated(variant)->addChild(g);
            }
            parent = dynamic_cast<osg::Group *>(creator.animated(variant)->getChild(t));
            assert(parent);
            coVRAnimationManager::instance()->addSequence(creator.animated(variant));
         }
         const char *filename = ro->coverRenderObject->getAttribute("_model_file");
         if (filename) {
             osg::Node *filenode = coVRFileManager::instance()->loadFile(filename, NULL, transform, ro->coverRenderObject->getName());
             if (filenode) {
                 m_fileAttachmentMap.emplace(ro->coverRenderObject.get(), filename);
             }
         }
         parent->addChild(transform);
      } else if (!ro->coverRenderObject) {
         std::cerr << rank() << ": discarding delayed object - already deleted" << std::endl;
      } else if (!transform) {
         //std::cerr << rank() << ": discarding delayed object " << ro->coverRenderObject->getName() << ": no node created" << std::endl;
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
   bool destroy() override;
   void menuEvent(vrui::coMenuItem *item) override;
   void preFrame() override;
   void requestQuit(bool killSession) override;
   bool executeAll() override;

 private:
   OsgRenderer *m_module;
   vrui::coButtonMenuItem *executeButton;
};

using opencover::coCommandLine;

VistlePlugin::VistlePlugin()
: m_module(nullptr)
{
   assert(plugin == nullptr);
   plugin = this;

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
      m_module->prepareQuit();
      delete m_module;
      m_module = nullptr;
   }

   plugin = nullptr;
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

bool VistlePlugin::destroy() {

   if (m_module) {
      MPI_Barrier(MPI_COMM_WORLD);
      m_module->prepareQuit();
      delete m_module;
      m_module = nullptr;
   }

    return true;
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
   MPI_Barrier(MPI_COMM_WORLD);
   m_module->prepareQuit();
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
