#include <future>
#include <boost/algorithm/string/predicate.hpp>

// cover
#include <cover/coVRPluginSupport.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coVRPluginList.h>
#include <cover/coVRFileManager.h>

// vistle
#include <core/messages.h>
#include <core/object.h>

#include <osg/Node>
#include <osg/Group>
#include <osg/Sequence>
#include <osg/MatrixTransform>

#include <VistlePluginUtil/VistleRenderObject.h>
#include "VistleGeometryGenerator.h"
#include "VistleInteractor.h"

#include "OsgRenderer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>

#include <util/findself.h>
#include <util/sysdep.h>

#if defined(WIN32)
    const char libcover[] = "mpicover.dll";
#elif defined(__APPLE__)
    const char libcover[] = "libmpicover.dylib";
#else
    const char libcover[] = "libmpicover.so";
#endif


using namespace opencover;
using namespace vistle;

OsgRenderer *OsgRenderer::s_instance = nullptr;

OsgRenderer::Variant::Variant(const std::string &basename, osg::ref_ptr<osg::Group> parent, const std::string &variant)
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

OsgRenderer::Creator::Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent)
    : id(id)
    , name(name)
    , baseVariant(name, parent)
{
}

const OsgRenderer::Variant &OsgRenderer::Creator::getVariant(const std::string &variantName) const {

    if (variantName.empty() || variantName == "NULL")
        return baseVariant;

    auto it = variants.find(variantName);
    if (it == variants.end()) {
        it = variants.emplace(std::make_pair(variantName, Variant(name, baseVariant.constant, variantName))).first;
        coVRPluginList::instance()->addNode(it->second.root, &it->second.ro, OsgRenderer::the()->m_plugin);
    }
    return it->second;
}

osg::ref_ptr<osg::Group> OsgRenderer::Creator::root(const std::string &variant) const { return getVariant(variant).root; }
osg::ref_ptr<osg::Group> OsgRenderer::Creator::constant(const std::string &variant) const { return getVariant(variant).constant; }
osg::ref_ptr<osg::Sequence> OsgRenderer::Creator::animated(const std::string &variant) const { return getVariant(variant).animated; }

OsgRenderer::Creator &OsgRenderer::getCreator(int id) {
    auto it = creatorMap.find(id);
    if (it == creatorMap.end()) {
        std::stringstream name;
        name << getModuleName(id) << "_" << id;
        it = creatorMap.insert(std::make_pair(id, Creator(id, name.str(), vistleRoot))).first;
    }
    return it->second;
}


OsgRenderer::OsgRenderer(const std::string &name, int moduleId, mpi::communicator comm)
: vistle::Renderer("VR renderer for immersive environments", name, moduleId, comm)
{
   assert(!s_instance);
   s_instance = this;

   //createInputPort("data_in"); - already done by Renderer base class

   vistleRoot = new osg::Group;
   vistleRoot->setName("VistlePlugin");

   m_fastestObjectReceivePolicy = message::ObjectReceivePolicy::Distribute;
   setObjectReceivePolicy(m_fastestObjectReceivePolicy);
}

OsgRenderer::~OsgRenderer() {

    prepareQuit();
}

OsgRenderer *OsgRenderer::the() {

    return s_instance;
}

void OsgRenderer::setPlugin(coVRPlugin *plugin) {

    m_plugin = plugin;

   cover->getObjectsRoot()->addChild(vistleRoot);
   initDone();
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
   if (cover)
       cover->getObjectsRoot()->removeChild(vistleRoot);
   vistleRoot.release();

   Renderer::prepareQuit();
}

bool OsgRenderer::executeAll() const {

   message::Execute exec; // execute all sources in data flow graph
   exec.setDestId(message::Id::MasterHub);
   sendMessage(exec);
   return true;
}

osg::ref_ptr<osg::Group> OsgRenderer::getParent(VistleRenderObject *ro) {
    int creatorId = ro->getCreator();
    Creator &creator = getCreator(creatorId);

    std::string variant = ro->renderObject()->variant;
    osg::ref_ptr<osg::Group> parent = creator.constant(variant);
    const int t = ro->renderObject()->timestep;
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
    }
    return parent;
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
         std::cerr << "variant '" << variant << "' for creator " << cr.name << ":" << cr.id << " has " << cr.animated(variant)->getNumChildren() << " vs. " << pro->timestep << " being removed" << std::endl;
         if (ssize_t(cr.animated(variant)->getNumChildren()) > pro->timestep) {
            osg::Group *gr = cr.animated(variant)->getChild(pro->timestep)->asGroup();
            if (gr) {
               gr->removeChild(node);
               if (gr->getNumChildren() == 0) {
                  std::cerr << "empty timestep " << pro->timestep << " of variant '" << variant << "' for creator " << cr.name << ":" << cr.id << std::endl;
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
                      std::cerr << "empty timestep: removed some timesteps" << std::endl;
                     if (cr.animated(variant)->getNumChildren() > 0) {
                        coVRAnimationManager::instance()->addSequence(cr.animated(variant));
                     } else {
                        coVRAnimationManager::instance()->removeSequence(cr.animated(variant));
                     }
                  }
               } else {
                   std::cerr << "still " << gr->getNumChildren() << " other children left in timestep " << pro->timestep << " of variant '" << variant << "' for creator " << cr.name << ":" << cr.id << std::endl;
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
      vistle::Object::const_ptr texture) {

   if (!container)
      return nullptr;

   std::string plugin = container->getAttribute("_plugin");
   if (!plugin.empty())
      cover->addPlugin(plugin.c_str());

   if (geometry) {
       plugin = geometry->getAttribute("_plugin");
       if (!plugin.empty())
           cover->addPlugin(plugin.c_str());
   }
   if (normals) {
      plugin = normals->getAttribute("_plugin");
      if (!plugin.empty())
         cover->addPlugin(plugin.c_str());
   }
   if (texture) {
      plugin = texture->getAttribute("_plugin");
      if (!plugin.empty())
         cover->addPlugin(plugin.c_str());
   }

   if (!geometry)
       return nullptr;

   if (geometry->getType() != vistle::Object::PLACEHOLDER && !VistleGeometryGenerator::isSupported(geometry->getType()))
      return nullptr;

   std::shared_ptr<PluginRenderObject> pro(new PluginRenderObject(senderId, senderPort,
         container, geometry, normals, texture));

   const std::string variant = pro->variant;
   if (!variant.empty()) {
      cover->addPlugin("Variant");
   }
   pro->coverRenderObject.reset(new VistleRenderObject(pro));
   auto cro = pro->coverRenderObject;
   m_delayedObjects.push_back(DelayedObject(pro, VistleGeometryGenerator(pro, geometry, normals, texture)));
   osg::ref_ptr<osg::Group> parent = getParent(pro->coverRenderObject.get());

   coVRPluginList::instance()->addObject(cro.get(), parent, cro->getGeometry(), cro->getNormals(), cro->getColors(), cro->getTexture());
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

   int numAdd = boost::mpi::all_reduce(comm(), numReady, boost::mpi::minimum<int>());

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
         const int t = ro->timestep;
         if (t >= 0) {
            coVRAnimationManager::instance()->addSequence(creator.animated(variant));
         }
         const char *filename = ro->coverRenderObject->getAttribute("_model_file");
         if (filename) {
             osg::Node *filenode = nullptr;
             if (!ro->coverRenderObject->isPlaceHolder()) {
                 filenode = coVRFileManager::instance()->loadFile(filename, NULL, transform, ro->coverRenderObject->getName());
             }
             if (!filenode) {
                 filenode = new osg::Node();
                 filenode->setName(filename);
                 transform->addChild(filenode);
             }
             m_fileAttachmentMap.emplace(ro->coverRenderObject.get(), filename);
         }
         osg::ref_ptr<osg::Group> parent = getParent(ro->coverRenderObject.get());
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


std::string OsgRenderer::setupEnvAndGetLibDir(const std::string &bindir) {
    int rank = comm().rank();

    std::map<std::string, std::string> env;
    std::map<std::string, bool> envToSet;
    if (rank == 0) {
        std::vector<std::string> envvars;
        // system
        envvars.push_back("PATH");
        envvars.push_back("LD_LIBRARY_PATH");
        envvars.push_back("DYLD_LIBRARY_PATH");
        envvars.push_back("DYLD_FRAMEWORK_PATH");
        envvars.push_back("LANG");
        // covise config
        envvars.push_back("COCONFIG");
        envvars.push_back("COCONFIG_LOCAL");
        envvars.push_back("COCONFIG_DEBUG");
        envvars.push_back("COCONFIG_DIR");
        envvars.push_back("COCONFIG_SCHEMA");
        envvars.push_back("COVISE_CONFIG");
        // cover
        envvars.push_back("COVER_PLUGINS");
        envvars.push_back("COVER_TABLETPC");
        envvars.push_back("COVISE_SG_DEBUG");
        //envvars.push_back("COVISE_HOST");
        envvars.push_back("COVISEDIR");
        envvars.push_back("COVISE_PATH");
        envvars.push_back("ARCHSUFFIX");
        // OpenSceneGraph
        envvars.push_back("OSGFILEPATH");
        envvars.push_back("OSG_FILE_PATH");
        envvars.push_back("OSG_NOTIFY_LEVEL");
        envvars.push_back("OSG_LIBRARY_PATH");
        envvars.push_back("OSG_LD_LIBRARY_PATH");
        for (auto v : envvars) {

            const char *val = getenv(v.c_str());
            if (val)
                env[v] = val;
        }

        std::string covisedir = env["COVISEDIR"];
        std::string archsuffix = env["ARCHSUFFIX"];

        if (!covisedir.empty()) {
            std::string print_covise_env = covisedir + "/bin/print_covise_env";
#ifdef _WIN32
            print_covise_env += ".bat";
#endif
            if (FILE *fp = popen(print_covise_env.c_str(), "r")) {
                std::vector<char> buf(10000);
                while (fgets(buf.data(), buf.size(), fp)) {
                    auto sep = std::find(buf.begin(), buf.end(), '=');
                    if (sep != buf.end()) {
                        std::string name = std::string(buf.begin(), sep);
                        ++sep;
                        auto end = std::find(sep, buf.end(), '\n');
                        std::string val = std::string(sep, end);
                        //std::cerr << name << "=" << val << std::endl;
                        env[name] = val;
                    }
                    //ld_library_path = buf.data();
                    //std::cerr << "read ld_lib: " << ld_library_path << std::endl;
                }
                pclose(fp);
            }
        }
    }

    std::string vistleplugin = "Vistle";
    env["VISTLE_PLUGIN"] = vistleplugin;

    std::string ldpath, dyldpath, dyldfwpath, covisepath;

    int numvars = env.size();
    MPI_Bcast(&numvars, 1, MPI_INT, 0, (MPI_Comm)comm());
    auto it = env.begin();
    for (int i = 0; i < numvars; ++i) {
        std::string name;
        std::string value;
        if (rank == 0) {
            name = it->first;
            value = it->second;
        }

        auto sync_string = [this, rank](std::string &s) {
            std::vector<char> buf;
            int len = -1;
            if (rank == 0)
                len = s.length() + 1;
            MPI_Bcast(&len, 1, MPI_INT, 0, (MPI_Comm)comm());
            buf.resize(len);
            if (rank == 0)
                strcpy(buf.data(), s.c_str());
            MPI_Bcast(buf.data(), buf.size(), MPI_BYTE, 0, (MPI_Comm)comm());
            s = buf.data();
        };
        sync_string(name);
        sync_string(value);

#if 0
        if (name == "COVISE_PATH") {
            // adapt in order to find VistlePlugin
            covisepath = bindir + "/../..";
            if (!value.empty()) {
                covisepath += ":";
                covisepath += value;
            }
            value = covisepath;
        }
#endif

        setenv(name.c_str(), value.c_str(), 1 /* overwrite */);

        if (rank == 0)
            ++it;
        else
            env[name] = value;

        //std::cerr << name << " -> " << value << std::endl;
    }

#if 0
    std::string abslib = bindir + "/../../lib/" + libcover;
#else
    std::string coviselibdir = env["COVISEDIR"] + "/" + env["ARCHSUFFIX"] + "/lib/";
    //std::string abslib = coviselibdir + libcover;
#endif

    return coviselibdir;
}

int OsgRenderer::runMain(int argc, char *argv[]) {

    std::string bindir = vistle::getbindir(argc, argv);
    std::string abslib = setupEnvAndGetLibDir(bindir) + libcover;

    int ret = 0;
    const char mainname[] = "mpi_main";
    typedef int(*mpi_main_t)(MPI_Comm comm, int, char *[]);
    mpi_main_t mpi_main = NULL;
#ifdef WIN32
    void *handle = LoadLibraryA(abslib.c_str());
#else
    void *handle = dlopen(abslib.c_str(), RTLD_LAZY);
#endif

    if (!handle) {
#ifdef WIN32
        std::cerr << "failed to dlopen " << abslib << std::endl;
#else
        std::cerr << "failed to dlopen " << abslib << ": " << dlerror() << std::endl;
#endif
        ret = 1;
        goto finish;
    }

#ifdef WIN32
    mpi_main = (main_t)GetProcAddress((HINSTANCE)handle, mainname);;
#else
    mpi_main = (mpi_main_t)dlsym(handle, mainname);
#endif
    if (!mpi_main) {
        std::cerr << "could not find " << mainname << " in " << libcover << std::endl;
        ret = 1;
        goto finish;
    }

    ret = mpi_main((MPI_Comm)comm(), argc, argv);

finish:
    if (handle)
    {

#ifdef _WIN32
        FreeLibrary((HINSTANCE)handle);
#else
        dlclose(handle);
#endif
    }

    return ret;
}

void OsgRenderer::eventLoop() {

    int argc=0;
    char *argv[] = {nullptr};
    runMain(argc, argv);
}
