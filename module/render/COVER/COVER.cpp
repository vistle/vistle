#include <future>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/mpi.hpp>
#include <boost/serialization/map.hpp>
// cover
#include <net/message.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coVRCommunication.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coVRPluginList.h>
#include <cover/coVRFileManager.h>
#include <PluginUtil/StaticSequence.h>
#include <mpiwrapper/mpicover.h>
#include <PluginUtil/StaticSequence.h>
#include <PluginUtil/PluginMessageTypes.h>
// vistle
#include <vistle/core/messages.h>
#include <vistle/core/object.h>
#include <vistle/core/placeholder.h>
#include <vistle/core/statetracker.h>
#include <vistle/util/threadname.h>

#include <osg/Node>
#include <osg/Group>
#include <osg/Sequence>
#include <osg/MatrixTransform>

#include <VistlePluginUtil/VistleRenderObject.h>
#include <VistlePluginUtil/VistleInteractor.h>
#include <VistlePluginUtil/VistleMessage.h>
#include "VistleGeometryGenerator.h"
#include "CoverConfigBridge.h"

#include "COVER.h"

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

#include <vistle/util/findself.h>
#include <vistle/util/sysdep.h>
#include <vistle/util/directory.h>

#include <OpenConfig/config.h>
#include <vistle/config/config.h>

#include <vistle/manager/run_on_main_thread.h>

#if defined(WIN32)
const char libcover[] = "mpicover.dll";
#elif defined(__APPLE__)
const char libcover[] = "libmpicover.dylib";
#else
const char libcover[] = "libmpicover.so";
#endif


using namespace opencover;
using namespace vistle;

COVER *COVER::s_instance = nullptr;


COVER::DelayedObject::DelayedObject(std::shared_ptr<PluginRenderObject> pro, VistleGeometryGenerator generator)
: pro(pro)
, name(pro->container ? pro->container->getName() : "(no container)")
, generator(generator)
, node_future(std::async(std::launch::async, [this]() {
    setThreadName("COVER:Geom:" + name);
    auto result = this->generator();
    return result;
}))
{}

COVER::Variant::Variant(const std::string &basename, const std::string &variant): variant(variant), ro(variant)
{
    std::stringstream s;
    s << basename;
    if (!variant.empty())
        s << "/" << variant;
    name = s.str();

    if (variant.empty()) {
        root = new osg::ClipNode;
    } else {
        root = new osg::Group;
    }
    root->setName(name);

    constant = new osg::Group;
    constant->setName(name + "/static");
    root->addChild(constant);

    animated = new StaticSequence;
    animated->setName(name + "/animated");
    root->addChild(animated);
}

COVER::Creator::Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent)
: id(id), name(name), baseVariant(name)
{
    parent->addChild(baseVariant.root);
    coVRPluginList::instance()->addNode(baseVariant.root, nullptr, COVER::the()->m_plugin);
}

bool COVER::Creator::empty() const
{
    if (!variants.empty())
        return false;

    if (baseVariant.animated->getNumChildren() > 0)
        return false;
    if (baseVariant.constant->getNumChildren() > 0)
        return false;

    return true;
}

const COVER::Variant &COVER::Creator::getVariant(const std::string &variantName,
                                                 vistle::RenderObject::InitialVariantVisibility vis) const
{
    if (variantName.empty() || variantName == "NULL")
        return baseVariant;

    auto it = variants.find(variantName);
    if (it == variants.end()) {
        it = variants.emplace(std::make_pair(variantName, Variant(name, variantName))).first;
        if (vis != vistle::RenderObject::DontChange)
            it->second.ro.setInitialVisibility(vis);
        baseVariant.constant->addChild(it->second.root);
        coVRPluginList::instance()->addNode(it->second.root, &it->second.ro, COVER::the()->m_plugin);
    }
    return it->second;
}

bool COVER::Creator::removeVariant(const std::string &variantName)
{
    osg::ref_ptr<osg::Group> root;

    if (variantName.empty() || variantName == "NULL") {
        root = baseVariant.root;
    } else {
        auto it = variants.find(variantName);
        if (it != variants.end()) {
            root = it->second.root;
            variants.erase(it);
        }
    }

    if (!root)
        return false;

    coVRPluginList::instance()->removeNode(root, true, root);
    while (root->getNumParents() > 0) {
        root->getParent(0)->removeChild(root);
    }

    return true;
}

osg::ref_ptr<osg::Group> COVER::Creator::root(const std::string &variant) const
{
    return getVariant(variant).root;
}
osg::ref_ptr<osg::Group> COVER::Creator::constant(const std::string &variant) const
{
    return getVariant(variant).constant;
}
osg::ref_ptr<osg::Sequence> COVER::Creator::animated(const std::string &variant) const
{
    return getVariant(variant).animated;
}

COVER::Creator &COVER::getCreator(int id)
{
    auto it = creatorMap.find(id);
    if (it == creatorMap.end()) {
        std::stringstream name;
        name << getModuleName(id) << "_" << id;
        if (id < message::Id::ModuleBase)
            std::cerr << "COVER::getCreator: invalid id " << id << std::endl;
        it = creatorMap.insert(std::make_pair(id, Creator(id, name.str(), vistleRoot))).first;
    }
    return it->second;
}

bool COVER::removeCreator(int id)
{
    auto it = creatorMap.find(id);
    if (it == creatorMap.end())
        return false;

    it->second.removeVariant();
    creatorMap.erase(it);

    return true;
}


COVER::COVER(const std::string &name, int moduleId, mpi::communicator comm): vistle::Renderer(name, moduleId, comm)
{
    assert(!s_instance);
    s_instance = this;

    int argc = 1;
    char *argv[] = {strdup("COVER"), nullptr};
    vistle::Directory dir(argc, argv);
    if (manager_in_cover_plugin()) {
        // use existing configuration manager
        m_config.reset(new opencover::config::Access());
    } else {
        m_config.reset(
            new opencover::config::Access(configAccess()->hostname(), configAccess()->cluster(), comm.rank()));
        if (auto covisedir = getenv("COVISEDIR")) {
            m_config->setPrefix(covisedir);
        }
    }
    m_coverConfigBridge.reset(new CoverConfigBridge(this));
    m_config->setWorkspaceBridge(m_coverConfigBridge.get());

    //createInputPort("data_in"); - already done by Renderer base class

    vistleRoot = new osg::ClipNode;
    vistleRoot->setName("Root");

    m_fastestObjectReceivePolicy = message::ObjectReceivePolicy::Distribute;
    setObjectReceivePolicy(m_fastestObjectReceivePolicy);

    setIntParameter("render_mode", AllShmLeaders);

    m_optimizeIndices =
        addIntParameter("optimize_indices", "optimize geometry indices for better GPU vertex cache utilization",
                        m_options.optimizeIndices, Parameter::Boolean);
    m_indexedGeometry = addIntParameter("indexed_geometry", "build indexed geometry, if useful",
                                        m_options.indexedGeometry, Parameter::Boolean);
    m_numPrimitives =
        addIntParameter("num_primitives", "number of primitives to process before splitting into multiple geodes",
                        m_options.numPrimitives);
    setParameterMinimum<Integer>(m_numPrimitives, 1);

    m_maySleep = false;
}

COVER::~COVER()
{
    prepareQuit();
    s_instance = nullptr;
}

COVER *COVER::the()
{
    return s_instance;
}

void COVER::setPlugin(coVRPlugin *plugin)
{
    if (plugin) {
        cover->getObjectsRoot()->addChild(vistleRoot);
        coVRPluginList::instance()->addNode(vistleRoot, nullptr, plugin);
        m_colormaps[""] = OsgColorMap(false); // fake colormap for objects without mapped data for using shaders
        initDone();
    } else if (m_plugin) {
        prepareQuit();
    }

    m_plugin = plugin;
}

bool COVER::updateRequired() const
{
    return m_requireUpdate;
}

void COVER::clearUpdate()
{
    m_requireUpdate = false;
}

bool COVER::parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg,
                           const std::string &moduleName)
{
    std::string plugin = moduleName;
    if (boost::algorithm::ends_with(plugin, "Old"))
        plugin = plugin.substr(0, plugin.size() - 3);
    if (boost::algorithm::ends_with(plugin, "Vtkm"))
        plugin = plugin.substr(0, plugin.size() - 4);
    if (plugin == "CutGeometry" || plugin == "Clip")
        plugin = "CuttingSurface";
    if (plugin == "DisCOVERay" || plugin == "OsgRenderer" || plugin == "ANARemote")
        plugin = "RhrClient";
    if (plugin == "Color" || plugin == "ColorRandom")
        plugin = "ColorBars";
    //std::cerr << "parameterAdded: sender=" << senderId << ", name=" << name << ", plugin=" << plugin << std::endl;

    InteractorMap::iterator it = m_interactorMap.find(senderId);
    if (it == m_interactorMap.end()) {
        if (vistle::message::Id::isModule(senderId))
            coVRPluginList::instance()->addPlugin(plugin.c_str(), coVRPluginList::Vis);

        m_interactorMap[senderId] = new VistleInteractor(this, moduleName, senderId);
        m_interactorMap[senderId]->setPluginName(plugin);
        auto dn = this->state().getModuleDisplayName(senderId);
        m_interactorMap[senderId]->setDisplayName(dn);
        it = m_interactorMap.find(senderId);
        std::cerr << "created interactor for " << moduleName << ":" << senderId << std::endl;
        auto inter = it->second;
        inter->incRefCount();
        coVRPluginList::instance()->newInteractor(inter->getObject(), inter);
    }
    VistleInteractor *inter = it->second;
    inter->addParam(name, msg);

    return true;
}

bool COVER::parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg)
{
    InteractorMap::iterator it = m_interactorMap.find(senderId);
    if (it != m_interactorMap.end()) {
        auto inter = it->second;
        inter->removeParam(name, msg);
        if (!inter->hasParams()) {
            std::cerr << "removing interactor for " << senderId << " (" << getModuleName(senderId)
                      << "), refcount=" << inter->refCount() << std::endl;
            coVRPluginList::instance()->removeObject(inter->getObjName(), false);
            inter->decRefCount();
            m_interactorMap.erase(it);
        }
    }
    return true;
}

bool COVER::parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg)
{
    InteractorMap::iterator it = m_interactorMap.find(senderId);
    if (it == m_interactorMap.end()) {
        return false;
    }

    VistleInteractor *inter = it->second;
    inter->applyParam(name, msg);
    coVRPluginList::instance()->removeObject(inter->getObjName(), true);
    coVRPluginList::instance()->newInteractor(inter->getObject(), inter);

    return true;
}

bool COVER::changeParameter(const Parameter *p)
{
    if (m_coverConfigBridge) {
        m_coverConfigBridge->changeParameter(p);
    }

    if (p == m_optimizeIndices) {
        m_options.optimizeIndices = m_optimizeIndices->getValue();
    } else if (p == m_numPrimitives) {
        m_options.numPrimitives = m_numPrimitives->getValue();
    } else if (p == m_indexedGeometry) {
        m_options.indexedGeometry = m_indexedGeometry->getValue();
    }

    return Renderer::changeParameter(p);
}

void COVER::prepareQuit()
{
    std::cerr << "COVER::prepareQuit()" << std::endl;
    removeAllObjects();
    if (vistleRoot) {
        if (m_plugin) {
            coVRPluginList::instance()->removeNode(vistleRoot, true, vistleRoot);
            if (cover)
                cover->getObjectsRoot()->removeChild(vistleRoot);
        }
        vistleRoot.release();
    }
    if (m_config)
        m_config->setWorkspaceBridge(nullptr);
    m_coverConfigBridge.reset();

    if (manager_in_cover_plugin()) {
        mark_cover_plugin_done();
    } else {
        Renderer::prepareQuit();
    }
}

bool COVER::executeAll() const
{
    message::Execute exec; // execute all sources in dataflow graph
    exec.setDestId(message::Id::MasterHub);
    sendMessage(exec);
    return true;
}

osg::ref_ptr<osg::Group> COVER::getParent(VistleRenderObject *ro)
{
    int creatorId = ro->getCreator();
    Creator &creator = getCreator(creatorId);

    std::string variant = ro->renderObject()->variant;
    osg::ref_ptr<osg::Group> parent = creator.constant(variant);
    const int t = ro->renderObject()->timestep;
    if (t >= 0) {
        while (size_t(t) >= creator.animated(variant)->getNumChildren()) {
            auto g = new osg::Group;
            std::stringstream name;
            name << creator.name;
            if (!variant.empty())
                name << "/" << variant;
            name << "/t" << t;
            g->setName(name.str());
            creator.animated(variant)->addChild(g);
        }
        parent = dynamic_cast<osg::Group *>(creator.animated(variant)->getChild(t));
        assert(parent);
    }
    return parent;
}

void COVER::removeObject(std::shared_ptr<vistle::RenderObject> vro)
{
    auto pro = std::static_pointer_cast<PluginRenderObject>(vro);
    auto ro = pro->coverRenderObject;
    std::string variant = pro->variant;

    m_requireUpdate = true;

    if (!ro) {
        std::cerr << "removeObject: no COVER render object" << std::endl;
        return;
    }

    auto it = m_fileAttachmentMap.find(ro->getName());
    if (it != m_fileAttachmentMap.end()) {
        coVRFileManager::instance()->unloadFile(it->second.c_str());
        m_fileAttachmentMap.erase(it);
    }

    coVRPluginList::instance()->removeObject(ro->getName(), false);

    if (!ro->isGeometry()) {
        std::cerr << "removeObject: Node is not geometry: " << ro->getName() << std::endl;
        return;
    }

    osg::ref_ptr<osg::Node> node = ro->node();
    if (node) {
        coVRPluginList::instance()->removeNode(node, false, node);
        Creator &cr = getCreator(pro->container->getCreator());
        if (pro->timestep == -1) {
            cr.constant(variant)->removeChild(node);
        } else {
            //std::cerr << "variant '" << variant << "' for creator " << cr.name << ":" << cr.id << " has " << cr.animated(variant)->getNumChildren() << " vs. " << pro->timestep << " being removed" << std::endl;
            if (ssize_t(cr.animated(variant)->getNumChildren()) > pro->timestep) {
                osg::Group *gr = cr.animated(variant)->getChild(pro->timestep)->asGroup();
                if (gr) {
                    gr->removeChild(node);
                    if (gr->getNumChildren() == 0) {
                        //std::cerr << "empty timestep " << pro->timestep << " of variant '" << variant << "' for creator " << cr.name << ":" << cr.id << std::endl;
                        bool removed = false;
                        for (size_t i = cr.animated(variant)->getNumChildren(); i > 0; --i) {
                            size_t idx = i - 1;
                            osg::Group *g = cr.animated(variant)->getChild(idx)->asGroup();
                            if (!g)
                                break;
                            if (g->getNumChildren() > 0)
                                break;
                            cr.animated(variant)->removeChildren(idx, 1);
                            removed = true;
                        }
                        if (removed) {
                            //std::cerr << "empty timestep: removed some timesteps" << std::endl;
                            if (cr.animated(variant)->getNumChildren() > 0) {
                                coVRAnimationManager::instance()->addSequence(cr.animated(variant));
                            } else {
                                coVRAnimationManager::instance()->removeSequence(cr.animated(variant));
                            }
                        }
                    } else {
                        //std::cerr << "still " << gr->getNumChildren() << " other children left in timestep " << pro->timestep << " of variant '" << variant << "' for creator " << cr.name << ":" << cr.id << std::endl;
                    }
                }
            }
        }
        while (node->getNumParents() > 0) {
            osg::Group *parent = node->getParent(0);
            parent->removeChild(node);
        }

        if (cr.constant(variant)->getNumChildren() == 0 && cr.animated(variant)->getNumChildren() == 0) {
            if (!variant.empty() && variant != "NULL")
                cr.removeVariant(variant);
            if (cr.empty())
                removeCreator(cr.id);
        }
    }
    pro->coverRenderObject.reset();
}

osg::ref_ptr<osg::MatrixTransform> makeTransform(const vistle::Object::const_ptr &obj)
{
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
    std::string nodename = obj->getName();
    transform->setName(nodename + ".transform");
    osg::Matrix osgMat;
    vistle::Matrix4 vistleMat = obj->getTransform();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            osgMat(i, j) = vistleMat.col(i)[j];
        }
    }
    transform->setMatrix(osgMat);
    return transform;
}

std::shared_ptr<vistle::RenderObject> COVER::addObject(int senderId, const std::string &senderPort,
                                                       vistle::Object::const_ptr container,
                                                       vistle::Object::const_ptr geometry,
                                                       vistle::Object::const_ptr normals,
                                                       vistle::Object::const_ptr texture)
{
    if (!container) {
        std::cerr << "COVER::addObject: no container" << std::endl;
        return nullptr;
    }

    m_requireUpdate = true;

    for (const auto &obj: {container, geometry, normals, texture}) {
        if (!obj)
            continue;
        std::string plugin = obj->getAttribute(attribute::Plugin);
        if (!plugin.empty())
            cover->addPlugin(plugin.c_str());
    }

    if (!geometry) {
        std::cerr << "COVER::addObject: container " << container->getName() << ": no geometry" << std::endl;
        return nullptr;
    }

    auto objType = geometry->getType();
    if (objType == vistle::Object::PLACEHOLDER) {
        auto ph = vistle::PlaceHolder::as(geometry);
        assert(ph);
        objType = ph->originalType();
    }
    if (objType == vistle::Object::UNIFORMGRID) {
        cover->addPlugin("Volume");
    } else if (!VistleGeometryGenerator::isSupported(objType)) {
        std::stringstream str;
        if (objType != vistle::Object::EMPTY) {
            str << "Unsupported input data: " << Object::toString(objType);
        }
        std::cerr << "COVER::addObject: " << str.str() << std::endl;
        if (m_dataTypeWarnings.find(objType) == m_dataTypeWarnings.end()) {
            m_dataTypeWarnings.insert(objType);
            cover->notify(Notify::Warning) << str.str();
        }
        return nullptr;
    }

    std::shared_ptr<PluginRenderObject> pro(
        new PluginRenderObject(senderId, senderPort, container, geometry, normals, texture));

    const std::string variant = pro->variant;
    if (!variant.empty()) {
        cover->addPlugin("Variant");
    }
    pro->coverRenderObject.reset(new VistleRenderObject(pro));
    auto cro = pro->coverRenderObject;
    int creatorId = cro->getCreator();
    Creator &creator = getCreator(creatorId);
    if (pro->visibility != vistle::RenderObject::DontChange)
        creator.getVariant(variant, pro->visibility);
    osg::ref_ptr<osg::MatrixTransform> transform;
    if (geometry) {
        transform = makeTransform(geometry);

        const char *filename = pro->coverRenderObject->getAttribute(attribute::ModelFile);
        if (filename) {
            osg::Node *filenode = nullptr;
            if (!pro->coverRenderObject->isPlaceHolder()) {
                filenode =
                    coVRFileManager::instance()->loadFile(filename, NULL, transform, pro->coverRenderObject->getName());
            }
            if (!filenode) {
                filenode = new osg::Node();
                filenode->setName(filename);
                transform->addChild(filenode);
            }
            m_fileAttachmentMap.emplace(pro->coverRenderObject->getName(), filename);
        }
    }

    if (VistleGeometryGenerator::isSupported(objType)) {
        auto vgr = VistleGeometryGenerator(pro, geometry, normals, texture);
        auto cache = getOrCreateGeometryCache<GeometryCache>(senderId, senderPort);
        vgr.setGeometryCache(*cache);
        auto species = vgr.species();
        if (!species.empty()) {
            VistleGeometryGenerator::lock();
            m_colormaps[species];
            VistleGeometryGenerator::unlock();
        }
        vgr.setColorMaps(&m_colormaps);
        vgr.setOptions(m_options);
        m_delayedObjects.emplace_back(pro, vgr);
        m_delayedObjects.back().transform = transform;
        //updateStatus();
    }
    const int t = pro->timestep;
    if (t >= 0) {
        coVRAnimationManager::instance()->addSequence(creator.animated(variant));
    }

    osg::ref_ptr<osg::Group> parent = getParent(cro.get());
    coVRPluginList::instance()->addObject(cro.get(), parent, cro->getGeometry(), cro->getNormals(), cro->getColors(),
                                          cro->getTexture());
    return pro;
}

bool COVER::render()
{
    updateStatus();

    if (m_delayedObjects.empty())
        return false;

    int numReady = 0;
    for (size_t i = 0; i < m_delayedObjects.size(); ++i) {
        auto &node_future = m_delayedObjects[i].node_future;
        if (!node_future.valid()) {
            std::cerr << "COVER::render(): future not valid" << std::endl;
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

    if (numAdd > 0)
        m_requireUpdate = true;

    //std::cerr << "adding " << numAdd << " delayed objects, " << m_delayedObjects.size() << " waiting" << std::endl;
    for (int i = 0; i < numAdd; ++i) {
        auto &node_future = m_delayedObjects.front().node_future;
        auto &pro = m_delayedObjects.front().pro;
        osg::Geode *geode = node_future.get();
        if (pro->coverRenderObject && geode) {
            int creatorId = pro->coverRenderObject->getCreator();
            Creator &creator = getCreator(creatorId);

            auto tr = m_delayedObjects.front().transform;
            geode->setNodeMask(~(opencover::Isect::Update | opencover::Isect::Intersection));
            geode->setName(pro->coverRenderObject->getName());
            tr->addChild(geode);
            pro->coverRenderObject->setNode(tr);
            const std::string variant = pro->variant;
            const int t = pro->timestep;
            if (t >= 0) {
                coVRAnimationManager::instance()->addSequence(creator.animated(variant));
            }
            osg::ref_ptr<osg::Group> parent = getParent(pro->coverRenderObject.get());
            parent->addChild(tr);
        } else if (!pro->coverRenderObject) {
            std::cerr << rank() << ": discarding delayed object " << m_delayedObjects.front().name
                      << " - already deleted" << std::endl;
        } else if (!geode) {
            //std::cerr << rank() << ": discarding delayed object " << pro->coverRenderObject->getName() << ": no node created" << std::endl;
        }
        m_delayedObjects.pop_front();
    }

    if (numAdd > 0) {
        //updateStatus();
    }

    return true;
}

bool COVER::addColorMap(const std::string &species, Object::const_ptr colormap)
{
    if (auto texture = Texture1D::as(colormap)) {
        VistleGeometryGenerator::lock();
        auto &cmap = m_colormaps[species];
        cmap.setName(species);
        cmap.setRange(texture->getMin(), texture->getMax());

        cmap.image->setPixelFormat(GL_RGBA);
        cmap.image->setImage(texture->getWidth(), 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                             const_cast<unsigned char *>(&texture->pixels()[0]), osg::Image::NO_DELETE);
        cmap.image->dirty();

        cmap.setBlendWithMaterial(texture->hasAttribute(attribute::BlendWithMaterial));
        VistleGeometryGenerator::unlock();
    }

    std::string plugin = colormap->getAttribute(attribute::Plugin);
    if (!plugin.empty())
        cover->addPlugin(plugin.c_str());

    int modId = colormap->getCreator();
    auto it = m_interactorMap.find(modId);
    if (it == m_interactorMap.end()) {
        std::cerr << rank() << ": no module with id " << modId << " found for colormap for species " << species
                  << std::endl;
        return true;
    }

    auto inter = it->second;
    if (!inter) {
        std::cerr << rank() << ": no interactor for id " << modId << " found for colormap for species " << species
                  << std::endl;
        return true;
    }
    auto *ro = inter->getObject();
    if (!ro) {
        std::cerr << rank() << ": no renderobject for id " << modId << " found for colormap for species " << species
                  << std::endl;
        return true;
    }

    auto att = colormap->getAttribute(attribute::ColorMap);
    if (att.empty()) {
        ro->removeAttribute("COLORMAP");
    } else {
        ro->addAttribute("COLORMAP", att);
        std::cerr << rank() << ": adding COLORMAP attribute for " << species << std::endl;
    }

    coVRPluginList::instance()->removeObject(inter->getObjName(), true);
    coVRPluginList::instance()->newInteractor(inter->getObject(), inter);

    return true;
}

bool COVER::removeColorMap(const std::string &species)
{
    auto it = m_colormaps.find(species);
    if (it == m_colormaps.end())
        return false;

    VistleGeometryGenerator::lock();
    auto &cmap = it->second;
    cmap.setRange(0.f, 1.f);

    cmap.image->setPixelFormat(GL_RGBA);
    unsigned char red_green[] = {1, 0, 0, 1, 0, 1, 0, 1};
    cmap.image->setImage(2, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, red_green, osg::Image::NO_DELETE);
    cmap.image->dirty();
    m_colormaps.erase(species);
    VistleGeometryGenerator::unlock();
    return true;
}

void COVER::updateStatus()
{
    if (comm().rank() != 0)
        return;

    if (m_delayedObjects.empty()) {
        if (m_status != 0)
            clearStatus();
    } else {
        if (m_status != m_delayedObjects.size()) {
            std::stringstream str;
            str << m_delayedObjects.size() << " objects to add";
            setStatus(str.str());
        }
    }
    m_status = m_delayedObjects.size();
}


std::map<std::string, std::string> COVER::setupEnv(const std::string &bindir)
{
    int rank = comm().rank();

    std::map<std::string, std::string> env;
    std::map<std::string, bool> envToSet;
    if (rank == 0) {
        constexpr std::array<const char *, 41> envvars = {
            // system
            "PATH", "LD_LIBRARY_PATH", "LD_PRELOAD", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH",
            "DYLD_FALLBACK_LIBRARY_PATH", "DYLD_FALLBACK_FRAMEWORK_PATH", "LANG", "LC_CTYPE", "LC_NUMERIC", "LC_TIME",
            "LC_COLLATE", "LC_MONETARY", "LC_MESSAGES", "LC_PAPER", "LC_NAME", "LC_ADDRESS", "LC_TELEPHONE",
            "LC_MEASUREMENT", "LC_IDENTIFICATION", "LC_ALL", "CONFIG_DEBUG", "COCONFIG", "COCONFIG_LOCAL",
            // covconfig
            "COCONFIG_DEBUG",
            // covise config
            "COCONFIG_DIR", "COCONFIG_SCHEMA", "COVISE_CONFIG",
            // cover
            "COVER_PLUGINS", "COVER_TABLETPC",
            // "COVISE_HOST",
            "COVISE_SG_DEBUG", "COVISEDIR", "COVISE_PATH", "ARCHSUFFIX",
            // OpenSceneGraph
            "OSGFILEPATH", "OSG_FILE_PATH", "OSG_NOTIFY_LEVEL", "OSG_LIBRARY_PATH", "OSG_LD_LIBRARY_PATH",
            // Qt
            "QT_AUTO_SCREEN_SCALE_FACTOR", "QT_SCREEN_SCALE_FACTORS"};

        for (const auto &v: envvars) {
            const char *val = getenv(v);
            if (val)
                env[v] = val;
        }

        std::string covisedir = env["COVISEDIR"];
        std::string archsuffix = env["ARCHSUFFIX"];

        std::string print_covise_env = "print_covise_env";
#ifdef _WIN32
        print_covise_env += ".bat";
#endif
        std::vector<std::string> cmds;
        if (covisedir.empty()) {
            cmds.push_back(print_covise_env);
        }
        if (archsuffix.empty()) {
            cmds.push_back(covisedir + "/bin/" + print_covise_env);
        }

        for (auto &cmd: cmds) {
            if (FILE *fp = popen(cmd.c_str(), "r")) {
                std::vector<char> buf(100000);
                while (fgets(buf.data(), buf.size(), fp)) {
                    auto sep = std::find(buf.begin(), buf.end(), '=');
                    if (sep != buf.end()) {
                        std::string name = std::string(buf.begin(), sep);
                        ++sep;
                        auto end = std::find(sep, buf.end(), '\n');
                        std::string val = std::string(sep, end);
                        //std::cerr << name << "=" << val << std::endl;
                        env[name] = val;
                        if (name == "COVISEDIR") {
                            covisedir = val;
                        } else if (name == "ARCHSUFFIX") {
                            archsuffix = val;
                        }
                    }
                    //ld_library_path = buf.data();
                    //std::cerr << "read ld_lib: " << ld_library_path << std::endl;
                }
                pclose(fp);
                if (!covisedir.empty() && !archsuffix.empty()) {
                    break;
                }
            }
        }
    }

    std::string vistleplugin = "Vistle";
    env["VISTLE_PLUGIN"] = vistleplugin;

    mpi::broadcast(comm(), env, 0);
    for (const auto &v: env)
        setenv(v.first.c_str(), v.second.c_str(), 1 /* overwrite */);

    return env;
}

std::vector<std::string> COVER::getLibPath(const std::map<std::string, std::string> &env)
{
    const auto &cd = env.find("COVISEDIR")->second;
    const auto &as = env.find("ARCHSUFFIX")->second;
    const std::string lib{"/lib"};
    std::vector<std::string> libpath{cd + "/" + as + lib, cd + lib};
    return libpath;
}

int COVER::runMain(int argc, char *argv[])
{
    std::string bindir = vistle::getbindir(argc, argv);
    auto env = setupEnv(bindir);
    auto libpath = getLibPath(env);
    void *handle = nullptr;
    mpi_main_t *mpi_main = NULL;

    for (const auto &libdir: libpath) {
        std::string abslib = libdir + "/" + libcover;
        const char mainname[] = "mpi_main";
#ifdef _WIN32
        handle = LoadLibraryA(abslib.c_str());
        if (!handle) {
            std::cerr << "failed to dlopen " << abslib << std::endl;
            continue;
        }
        mpi_main = (mpi_main_t *)GetProcAddress((HINSTANCE)handle, mainname);
#else
        handle = dlopen(abslib.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "failed to dlopen " << abslib << ": " << dlerror() << std::endl;
            continue;
        }
        mpi_main = (mpi_main_t *)dlsym(handle, mainname);
#endif
        if (mpi_main) {
            break;
        }

        std::cerr << "could not find " << mainname << " in " << libcover << std::endl;
    }

    int ret = 1;
    if (mpi_main) {
        mpi::communicator c(comm(), mpi::comm_duplicate);
        if (commShmGroup().size() == 1
#ifndef SHMBARRIER
            || true
#endif
        ) {
            ret = mpi_main((MPI_Comm)c, shmLeader(rank()), nullptr, argc, argv);
#ifdef SHMBARRIER
        } else {
            const std::string barrierName = name() + std::to_string(id());
            pthread_barrier_t *barrier = nullptr;
            if (rank() == shmLeader())
                barrier = Shm::the().newBarrier(barrierName, commShmGroup().size());
            commShmGroup().barrier();
            if (rank() != shmLeader())
                barrier = Shm::the().newBarrier(barrierName, commShmGroup().size());
            ret = mpi_main((MPI_Comm)c, shmLeader(rank()), barrier, argc, argv);
            Shm::the().deleteBarrier(barrierName);
#endif
        }
    }

    if (handle) {
#ifdef _WIN32
        FreeLibrary((HINSTANCE)handle);
#else
        dlclose(handle);
#endif
    }

    m_plugin = nullptr;

    return ret;
}

void COVER::eventLoop()
{
    if (manager_in_cover_plugin()) {
        std::cerr << "waiting for Vistle plugin to unload" << std::endl;
        wait_for_cover_plugin();
        std::cerr << "Vistle plugin unloaded" << std::endl;
    } else {
#if defined(MODULE_THREAD) && defined(COVER_ON_MAINTHREAD)
        std::function<void()> f = [this]() {
            std::cerr << "running COVER on main thread" << std::endl;
            int argc = 1;
            char *argv[] = {strdup("COVER"), nullptr};
            runMain(argc, argv);
        };
        run_on_main_thread(f);
        std::cerr << "COVER on main thread terminated" << std::endl;
#else
        int argc = 1;
        char *argv[] = {strdup("COVER"), nullptr};
        runMain(argc, argv);
#endif
    }
}

bool COVER::handleMessage(const message::Message *message, const MessagePayload &payload)
{
    switch (message->type()) {
    case vistle::message::REMOTERENDERING: {
        VistleMessage wrap(*message, payload);
        coVRPluginList::instance()->message(0, opencover::PluginMessageTypes::VistleMessageIn, sizeof(wrap), &wrap);
        return true;
    }
    case vistle::message::COVER: {
        auto &cmsg = message->as<const message::Cover>();
        covise::DataHandle dh(const_cast<char *>(payload ? payload->data() : nullptr), payload ? payload->size() : 0,
                              false /* do not delete */);
        covise::Message msg(cmsg.subType(), dh);
        msg.sender = cmsg.sender();
        msg.send_type = cmsg.senderType();
        coVRCommunication::instance()->handleVRB(msg);
        return true;
    }
    case vistle::message::SETNAME: {
        auto ret = Renderer::handleMessage(message, payload);
        auto &setname = message->as<const message::SetName>();
        InteractorMap::iterator it = m_interactorMap.find(setname.module());
        if (it != m_interactorMap.end()) {
            auto inter = it->second;
            inter->setDisplayName(setname.name());
            coVRPluginList::instance()->newInteractor(inter->getObject(), inter);
        }
        return ret;
    }
    default:
        break;
    }

    return Renderer::handleMessage(message, payload);
}

PluginRenderObject::~PluginRenderObject() = default;
