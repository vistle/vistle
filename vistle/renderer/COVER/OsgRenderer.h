#ifndef OSG_RENDERER_H
#define OSG_RENDERER_H

#include <future>
#include <string>

#include <osg/Group>
#include <osg/Sequence>

#include <renderer/renderobject.h>
#include <renderer/renderer.h>
#include <core/messages.h>
#include <VistlePluginUtil/VistleRenderObject.h>
#include "VistleInteractor.h"
#include "VistleGeometryGenerator.h"

#include "export.h"

#define OsgRenderer COVER

namespace opencover {
class coVRPlugin;
};

class PluginRenderObject: public vistle::RenderObject {

 public:
   PluginRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr texture)
      : vistle::RenderObject(senderId, senderPort, container, geometry, normals, texture)
      {
      }

   ~PluginRenderObject() override;

   std::shared_ptr<VistleRenderObject> coverRenderObject;
};

class V_COVEREXPORT OsgRenderer: public vistle::Renderer {

 public:
   OsgRenderer(const std::string &name, int moduleId, mpi::communicator comm);
   ~OsgRenderer() override;
   static OsgRenderer *the();

   void eventLoop() override;

   void setPlugin(opencover::coVRPlugin *plugin);

   bool updateRequired() const;
   void clearUpdate();

   bool render() override;
   std::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
      vistle::Object::const_ptr container,
      vistle::Object::const_ptr geometry,
      vistle::Object::const_ptr normals,
      vistle::Object::const_ptr texture) override;
   void removeObject(std::shared_ptr<vistle::RenderObject> ro) override;

   osg::ref_ptr<osg::Group> vistleRoot;

   bool parameterAdded(const int senderId, const std::string &name, const vistle::message::AddParameter &msg, const std::string &moduleName) override;
   bool parameterChanged(const int senderId, const std::string &name, const vistle::message::SetParameter &msg) override;
   bool parameterRemoved(const int senderId, const std::string &name, const vistle::message::RemoveParameter &msg) override;
   void prepareQuit() override;

   bool executeAll() const;

   typedef std::map<VistleRenderObject *, std::string> FileAttachmentMap;
   FileAttachmentMap m_fileAttachmentMap;

   typedef std::map<int, VistleInteractor *> InteractorMap;
   InteractorMap m_interactorMap;

   struct DelayedObject {
      DelayedObject(std::shared_ptr<PluginRenderObject> ro, VistleGeometryGenerator generator)
         : ro(ro)
         , name(ro->container ? ro->container->getName() : "(no container)")
         , generator(generator)
         , node_future(std::async(std::launch::async, generator))
      {}
      std::shared_ptr<PluginRenderObject> ro;
      std::string name;
      VistleGeometryGenerator generator;
      std::future<osg::MatrixTransform *> node_future;
   };
   std::deque<DelayedObject> m_delayedObjects;

 protected:
   struct Variant {
       std::string variant;
       std::string name;
       osg::ref_ptr<osg::Group> root;
       osg::ref_ptr<osg::Group> constant;
       osg::ref_ptr<osg::Sequence> animated;
       VariantRenderObject ro;

       Variant(const std::string &basename, const std::string &variant=std::string());
   };

   struct Creator {
       Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent);
       const Variant &getVariant(const std::string &variantName) const;
       osg::ref_ptr<osg::Group> root(const std::string &variant = std::string()) const;
       osg::ref_ptr<osg::Group> constant(const std::string &variant = std::string()) const;
       osg::ref_ptr<osg::Sequence> animated(const std::string &variant = std::string()) const;
       bool removeVariant(const std::string &variant = std::string());
       bool empty() const;

       int id;
       std::string name;
       Variant baseVariant;
       mutable std::map<std::string, Variant> variants;
   };

   typedef std::map<int, Creator> CreatorMap;
   CreatorMap creatorMap;

   Creator &getCreator(int id);
   bool removeCreator(int id);
   osg::ref_ptr<osg::Group> getParent(VistleRenderObject *ro);

   opencover::coVRPlugin *m_plugin = nullptr;
   static OsgRenderer *s_instance;

   std::string setupEnvAndGetLibDir(const std::string &bindir);
   int runMain(int argc, char *argv[]);
   bool m_requireUpdate = true;
};

#endif
