#ifndef VISTLE_OSGRENDERER_OSGRENDERER_H
#define VISTLE_OSGRENDERER_OSGRENDERER_H

#include <osgViewer/Viewer>
#include <osg/Sequence>
#include <osg/MatrixTransform>

#include <vistle/renderer/renderer.h>
#include <vistle/renderer/parrendmgr.h>

namespace osg {
class Group;
class Geode;
class Material;
class LightModel;
} // namespace osg

namespace vistle {
class Object;
}


class OsgRenderObject: public vistle::RenderObject {
public:
    OsgRenderObject(int senderId, const std::string &senderPort, vistle::Object::const_ptr container,
                    vistle::Object::const_ptr geometry, vistle::Object::const_ptr normals,
                    vistle::Object::const_ptr texture, osg::ref_ptr<osg::Node> node);

    osg::ref_ptr<osg::Node> node;
};


class TimestepHandler: public osg::Referenced {
public:
    TimestepHandler();

    void addObject(osg::Node *geode, const int step);
    void removeObject(osg::Node *geode, const int step);

    osg::ref_ptr<osg::MatrixTransform> root() const;

    bool setTimestep(const int timestep);
    size_t numTimesteps() const;
    void getBounds(vistle::Vector3 &min, vistle::Vector3 &max) const;

private:
    int timestep;

    osg::ref_ptr<osg::MatrixTransform> m_root;
    osg::ref_ptr<osg::Group> m_fixed;
    osg::ref_ptr<osg::Sequence> m_animated;
};


class OSGRenderer;


struct OsgViewData {
    OsgViewData(OSGRenderer &viewer, size_t viewIdx);
    ~OsgViewData();
    void createCamera();
    bool update(bool frameQueued);
    bool composite(size_t maxQueuedFrames, int timestep, bool wait = true);

    OSGRenderer &viewer;
    size_t viewIdx;
    osg::ref_ptr<osg::GraphicsContext> gc;
    osg::ref_ptr<osg::Camera> camera;
    osg::ref_ptr<osg::Camera::DrawCallback> readOperation;
    std::vector<GLuint> pboDepth, pboColor;
    std::vector<unsigned char *> mapColor;
    std::vector<GLfloat *> mapDepth;

    OpenThreads::Mutex mutex;
    size_t readPbo;
    std::deque<GLsync> outstanding;
    std::deque<size_t> compositePbo;
    int width, height;
};


class OSGRenderer: public vistle::Renderer, public osgViewer::Viewer {
    friend struct OsgViewData;

public:
    OSGRenderer(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool handleMessage(const vistle::message::Message *message, const vistle::MessagePayload &payload) override;

    std::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
                                                    vistle::Object::const_ptr container,
                                                    vistle::Object::const_ptr geometry,
                                                    vistle::Object::const_ptr normals,
                                                    vistle::Object::const_ptr texture) override;
    void removeObject(std::shared_ptr<vistle::RenderObject> ro) override;
    bool changeParameter(const vistle::Parameter *p) override;

    bool render() override;
    bool composite(size_t maxQueued);
    void flush();

    vistle::IntParameter *m_debugLevel;
    vistle::IntParameter *m_visibleView;
    vistle::IntParameter *m_threading;
    vistle::IntParameter *m_async;
    std::vector<std::shared_ptr<OsgViewData>> m_viewData;

    vistle::ParallelRemoteRenderManager m_renderManager;

    osg::ref_ptr<osg::DisplaySettings> displaySettings;
    osg::ref_ptr<osg::MatrixTransform> scene;

    osg::ref_ptr<osg::Material> material;
    osg::ref_ptr<osg::LightModel> lightModel;
    osg::ref_ptr<osg::StateSet> defaultState;
    osg::ref_ptr<osg::StateSet> rootState;

    osg::ref_ptr<TimestepHandler> timesteps;
    std::vector<osg::ref_ptr<osg::LightSource>> lights;
    OpenThreads::Mutex *icetMutex;
    size_t m_numViewsToComposite, m_numFramesToComposite;
    std::deque<int> m_previousTimesteps;
    int m_asyncFrames;
    ThreadingModel m_requestedThreadingModel;
    void prepareQuit() override;
    void connectionAdded(const vistle::Port *from, const vistle::Port *to) override;
    void connectionRemoved(const vistle::Port *from, const vistle::Port *to) override;
};

#endif
