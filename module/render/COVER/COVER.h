#ifndef RENDERER_COVER_H
#define RENDERER_COVER_H

#include <future>
#include <string>
#include <memory>

#include <osg/Group>
#include <osg/Sequence>
#include <osg/Texture1D>

#include <vistle/renderer/renderobject.h>
#include <vistle/renderer/renderer.h>
#include <vistle/module/resultcache.h>
#include <vistle/core/messages.h>
#include <VistlePluginUtil/VistleRenderObject.h>
#include "VistleGeometryGenerator.h"

#include "export.h"

class VistleInteractor;
class CoverConfigBridge;

namespace opencover {
class coVRPlugin;
namespace config {
class Access;
}
} // namespace opencover

namespace vistle {} // namespace vistle

class PluginRenderObject: public vistle::RenderObject {
public:
    PluginRenderObject(int senderId, const std::string &senderPort, vistle::Object::const_ptr container,
                       vistle::Object::const_ptr geometry, vistle::Object::const_ptr normals,
                       vistle::Object::const_ptr texture)
    : vistle::RenderObject(senderId, senderPort, container, geometry, normals, texture)
    {}

    ~PluginRenderObject() override;

    std::shared_ptr<VistleRenderObject> coverRenderObject;
};

class V_COVEREXPORT COVER: public vistle::Renderer {
    friend class CoverConfigBridge;

public:
    COVER(const std::string &name, int moduleId, mpi::communicator comm);
    ~COVER() override;
    static COVER *the();

    void eventLoop() override;

    void setPlugin(opencover::coVRPlugin *plugin);

    bool updateRequired() const;
    void clearUpdate();

    bool render() override;

    bool addColorMap(const std::string &species, vistle::Object::const_ptr cmap) override;
    bool removeColorMap(const std::string &species) override;

    std::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
                                                    vistle::Object::const_ptr container,
                                                    vistle::Object::const_ptr geometry,
                                                    vistle::Object::const_ptr normals,
                                                    vistle::Object::const_ptr texture) override;
    void removeObject(std::shared_ptr<vistle::RenderObject> ro) override;

    osg::ref_ptr<osg::Group> vistleRoot;

    bool handleMessage(const vistle::message::Message *message, const vistle::MessagePayload &payload) override;
    bool parameterAdded(const int senderId, const std::string &name, const vistle::message::AddParameter &msg,
                        const std::string &moduleName) override;
    bool parameterChanged(const int senderId, const std::string &name,
                          const vistle::message::SetParameter &msg) override;
    bool parameterRemoved(const int senderId, const std::string &name,
                          const vistle::message::RemoveParameter &msg) override;
    bool changeParameter(const vistle::Parameter *p) override;
    void prepareQuit() override;

    bool executeAll() const;

    typedef std::map<std::string, std::string> FileAttachmentMap;
    FileAttachmentMap m_fileAttachmentMap;

    typedef std::map<int, VistleInteractor *> InteractorMap;
    InteractorMap m_interactorMap;

    struct DelayedObject {
        DelayedObject(std::shared_ptr<PluginRenderObject> ro, VistleGeometryGenerator generator);
        std::shared_ptr<PluginRenderObject> ro;
        std::string name;
        VistleGeometryGenerator generator;
        std::shared_future<osg::Geode *> node_future;
        osg::ref_ptr<osg::MatrixTransform> transform;
    };
    std::deque<DelayedObject> m_delayedObjects;
    size_t m_status = 0;

protected:
    struct Variant {
        std::string variant;
        std::string name;
        osg::ref_ptr<osg::Group> root;
        osg::ref_ptr<osg::Group> constant;
        osg::ref_ptr<osg::Sequence> animated;
        VariantRenderObject ro;

        Variant(const std::string &basename, const std::string &variant = std::string());
    };

    struct Creator {
        Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent);
        const Variant &
        getVariant(const std::string &variantName,
                   vistle::RenderObject::InitialVariantVisibility vis = vistle::RenderObject::DontChange) const;
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
    static COVER *s_instance;

    void updateStatus();

    std::string setupEnvAndGetLibDir(const std::string &bindir);
    int runMain(int argc, char *argv[]);
    bool m_requireUpdate = true;

    typedef std::map<std::string, OsgColorMap> ColorMapMap;
    ColorMapMap m_colormaps;

    std::set<int> m_dataTypeWarnings; // set of unsupported data types for which a warning has already been printed

    std::unique_ptr<opencover::config::Access> m_config;
    std::unique_ptr<CoverConfigBridge> m_coverConfigBridge;
};

#endif
