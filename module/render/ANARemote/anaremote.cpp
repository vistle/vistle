
#include <vistle/renderer/renderer.h>
#include <vistle/core/message.h>
#include <cassert>

#include <vistle/util/stopwatch.h>

#include <vistle/renderer/parrendmgr.h>

// anari
#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>

#include "anarirenderobject.h"
#include "projection.h"

#define CERR std::cerr << "ANARemote: "

namespace mpi = boost::mpi;

using namespace vistle;

const float Epsilon = 1e-9f;

struct CompareMat4 {
    bool operator()(const AnariRenderObject::mat4 &a, const AnariRenderObject::mat4 &b) const
    {
        return memcmp(&a, &b, sizeof(AnariRenderObject::mat4)) < 0;
    }
};

class Anari: public vistle::Renderer {
public:
    struct RGBA {
        unsigned char r, g, b, a;
    };

    Anari(const std::string &name, int moduleId, mpi::communicator comm);
    ~Anari() override;
    void prepareQuit() override;

    bool render() override;

    bool handleMessage(const vistle::message::Message *message, const vistle::MessagePayload &payload) override;

    bool changeParameter(const Parameter *p) override;
    void connectionAdded(const Port *from, const Port *to) override;
    void connectionRemoved(const Port *from, const Port *to) override;

    ParallelRemoteRenderManager m_renderManager;

    // parameters
    FloatParameter *m_pointSizeParam = nullptr;
    StringParameter *m_libraryParam = nullptr;
    StringParameter *m_rendererParam = nullptr;
    IntParameter *m_debugWrapper = nullptr;
    IntParameter *m_linearDepthParam = nullptr;
    bool m_linearDepth = false;

    // colormaps
    bool addColorMap(const vistle::message::Colormap &cm, std::vector<vistle::RGBA> &rgba) override;
    bool removeColorMap(const std::string &species, int sourceModule) override;

    // object lifetime management
    std::shared_ptr<RenderObject> addObject(int sender, const std::string &senderPort,
                                            vistle::Object::const_ptr container, vistle::Object::const_ptr geometry,
                                            vistle::Object::const_ptr normals,
                                            vistle::Object::const_ptr texture) override;

    void removeObject(std::shared_ptr<RenderObject> ro) override;

    std::vector<std::shared_ptr<AnariRenderObject>> static_geometry;
    std::vector<std::vector<std::shared_ptr<AnariRenderObject>>> anim_geometry;
    std::map<vistle::ColorMapKey, AnariColorMap> m_colormaps;

    int m_timestep = -1;
    bool m_modified = false;

    static void anariStatusFunc(const void *userData, ANARIDevice device, ANARIObject source, ANARIDataType sourceType,
                                ANARIStatusSeverity severity, ANARIStatusCode code, const char *message);
    void statusFunc(ANARIDevice device, ANARIObject source, ANARIDataType sourceType, ANARIStatusSeverity severity,
                    ANARIStatusCode code, const char *message) const;

    anari::Library m_wrapperLibrary = nullptr; // debug wrapper
    anari::Device m_wrapperDevice = nullptr;
    anari::Library m_library = nullptr;
    anari::Device m_nestedDevice = nullptr;
    anari::Device m_device = nullptr; // device to send commands to: m_wrapperDevice for debug, m_nestedDevice otherwise
    anari::Renderer m_renderer = nullptr;
    anari::World m_world = nullptr;
    std::vector<anari::Frame> m_frames;
    std::vector<anari::Camera> m_cameras;

    void unloadAnari();
    std::vector<std::string> getRendererTypes(anari::Library lib, anari::Device dev);

    anari::Device recreate(anari::Device dev);
    void releaseDevice(anari::Device dev);
    anari::Renderer createRenderer(anari::Device dev, const std::string &type);
};

void Anari::anariStatusFunc(const void *userData, ANARIDevice device, ANARIObject source, ANARIDataType sourceType,
                            ANARIStatusSeverity severity, ANARIStatusCode code, const char *message)
{
    auto *anari = static_cast<const Anari *>(userData);
    anari->statusFunc(device, source, sourceType, severity, code, message);
}

void Anari::statusFunc(ANARIDevice device, ANARIObject source, ANARIDataType sourceType, ANARIStatusSeverity severity,
                       ANARIStatusCode code, const char *message) const
{
    (void)device;
    (void)source;
    (void)sourceType;
    (void)code;
    if (severity == ANARI_SEVERITY_FATAL_ERROR) {
        sendError("FATAL: %s", message);
    } else if (severity == ANARI_SEVERITY_ERROR) {
        sendError("%s", message);
    } else if (severity == ANARI_SEVERITY_WARNING) {
        sendWarning("%s", message);
    } else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING) {
        sendWarning("Performance: %s", message);
    } else if (severity == ANARI_SEVERITY_INFO) {
        sendInfo("%s", message);
    } else if (severity == ANARI_SEVERITY_DEBUG) {
        fprintf(stderr, "[DEBUG] %s\n", message);
    }
}


Anari::Anari(const std::string &name, int moduleId, mpi::communicator comm)
: Renderer(name, moduleId, comm), m_renderManager(this), m_timestep(-1)
{
    setCurrentParameterGroup("Advanced", false);
    m_renderManager.setLinearDepth(m_linearDepth);
    setCurrentParameterGroup("");

    m_pointSizeParam = addFloatParameter("point_size", "size of points", AnariRenderObject::pointSize);
    setParameterRange(m_pointSizeParam, (Float)0, (Float)1e6);

    m_libraryParam = addStringParameter("library", "ANARI renderer library", "helide", Parameter::Choice);
    setParameterChoices(m_libraryParam, {"sink", "environment", "helide", "ospray", "visrtx", "visgl", "visionaray"});

    m_rendererParam = addStringParameter("renderer", "ANARI renderer type", "default", Parameter::Choice);
    setParameterChoices(m_rendererParam, {"default"});

    m_debugWrapper = addIntParameter("debug_wrapper", "use ANARI debug wrapper", false, Parameter::Boolean);
    m_linearDepthParam = addIntParameter("linear_depth", "use viewer distance instead of GL depth buffer",
                                         m_linearDepth, Parameter::Boolean);
}


Anari::~Anari()
{
    unloadAnari();
    anari::unloadLibrary(m_wrapperLibrary);
    m_wrapperLibrary = nullptr;
}

void Anari::prepareQuit()
{
    removeAllObjects();
    unloadAnari();
    anari::unloadLibrary(m_wrapperLibrary);
    m_wrapperLibrary = nullptr;

    Renderer::prepareQuit();
}

void Anari::connectionAdded(const Port *from, const Port *to)
{
    Renderer::connectionAdded(from, to);
    if (from == m_renderManager.outputPort()) {
        m_renderManager.connectionAdded(to);
    }
}

void Anari::connectionRemoved(const Port *from, const Port *to)
{
    if (from == m_renderManager.outputPort()) {
        m_renderManager.connectionRemoved(to);
    }
    Renderer::connectionRemoved(from, to);
}

bool Anari::addColorMap(const vistle::message::Colormap &cm, std::vector<vistle::RGBA> &rgba)
{
    std::string species = cm.species();
    ColorMapKey key(species, cm.source());
    bool addedOrChanged = m_colormaps.find(key) != m_colormaps.end();
    auto &cmap = m_colormaps[key];
    cmap.species = species;
    cmap.min = cm.min();
    cmap.max = cm.max();
    cmap.blendWithMaterial = cm.blendWithMaterial();
    cmap.rgba.resize(rgba.size());
    std::copy(rgba.begin(), rgba.end(), cmap.rgba.begin());
    addedOrChanged = true;
    if (addedOrChanged) {
        CERR << "add or change colormap, species=" << species << ", have tex=" << (cmap.rgba.empty() ? "no" : "yes")
             << std::endl;
        cmap.create(m_device);
        m_renderManager.setModified();
    }
    return true;
}

bool Anari::removeColorMap(const std::string &species, int sourceModule)
{
    CERR << "removing colormap " << sourceModule << ":" << species << std::endl;
    ColorMapKey key(species, sourceModule);
    auto it = m_colormaps.find(key);
    if (it == m_colormaps.end())
        return false;

    m_colormaps.erase(it);
    m_renderManager.setModified();
    return true;
}

std::vector<std::string> Anari::getRendererTypes(anari::Library lib, anari::Device dev)
{
    std::vector<std::string> result;
    const char *const *rendererSubtypes = nullptr;
    if (dev) {
        anariGetProperty(dev, dev, "subtypes.renderer", ANARI_STRING_LIST, &rendererSubtypes, sizeof(rendererSubtypes),
                         ANARI_WAIT);
        if (rendererSubtypes != nullptr) {
            while (const char *rendererType = *rendererSubtypes++) {
                result.push_back(rendererType);
            }
        }
    }

    if (lib && dev && result.empty()) {
        // If the device does not support the "subtypes.renderer" property,
        // try to obtain the renderer types from the library directly
        const char **deviceSubtypes = anariGetDeviceSubtypes(lib);
        if (deviceSubtypes != nullptr) {
            while (const char *dstype = *deviceSubtypes++) {
                (void)dstype;
                const char **rendererTypes = anariGetObjectSubtypes(dev, ANARI_RENDERER);
                while (rendererTypes && *rendererTypes) {
                    const char *rendererType = *rendererTypes++;
                    result.push_back(rendererType);
                }
            }
        }
    }

    if (result.empty())
        result.push_back("default");

    return result;
}

anari::Renderer Anari::createRenderer(anari::Device dev, const std::string &type)
{
    auto rend = anari::newObject<anari::Renderer>(dev, type.c_str());
    if (!rend) {
        sendError("failed to create renderer %s, trying default", type.c_str());
        rend = anari::newObject<anari::Renderer>(dev, "default");
    }
    if (!rend) {
        sendError("failed to create default renderer");
        return nullptr;
    }

    float bgcolor[] = {0.f, 0.f, 0.f, 0.f};
    anari::setParameter(dev, rend, "background", bgcolor);

    anari::setParameter(dev, rend, "ambientRadiance", 1.f); // FIXME
    //anariSetParameter(dev, rend, "pixelSamples", ANARI_INT32, &spp);

    anari::commitParameters(dev, rend);

    for (auto &frame: m_frames) {
        //anari::release(m_device, frame);
        anari::setParameter(dev, frame, "renderer", rend);
        anari::commitParameters(dev, frame);
    }

    return rend;
}

void Anari::releaseDevice(anari::Device dev)
{
    if (dev) {
        for (auto &frame: m_frames) {
            anari::release(dev, frame);
        }
        m_frames.clear();

        for (auto &camera: m_cameras) {
            anari::release(dev, camera);
        }
        m_cameras.clear();

        anari::release(dev, m_renderer);
        m_renderer = nullptr;

        anari::release(dev, m_world);
        m_world = nullptr;

#if 0
        anari::release(dev, m_device);
        m_device = nullptr;
#endif
    }
}

void Anari::unloadAnari()
{
    releaseDevice(m_device);

    if (m_wrapperDevice) {
        anari::unsetParameter(m_wrapperDevice, m_wrapperDevice, "wrappedDevice");
        anari::commitParameters(m_wrapperDevice, m_wrapperDevice);
        anari::release(m_wrapperDevice, m_wrapperDevice);
        m_wrapperDevice = nullptr;
    }
    if (m_nestedDevice) {
        anari::release(m_nestedDevice, m_nestedDevice);
        m_nestedDevice = nullptr;
    }
    m_device = nullptr;
    anari::unloadLibrary(m_library);
    m_library = nullptr;
    anari::unloadLibrary(m_wrapperLibrary);
    m_wrapperLibrary = nullptr;
}

anari::Device Anari::recreate(anari::Device dev)
{
    anari::Device old = m_device;
    CERR << "recreating objects for " << dev << " (was " << old << ")" << std::endl;

    if (m_device == dev)
        return old;

    if (m_device) {
        releaseDevice(m_device);
    }

    m_device = dev;

    if (m_device) {
        assert(m_world == nullptr);
        m_world = anari::newObject<anari::World>(m_device);

        m_renderer = createRenderer(m_device, m_rendererParam->getValue());

        for (auto &cmap: m_colormaps) {
            cmap.second.create(m_device);
        }

        for (auto &ro: static_geometry) {
            ro->create(m_device);
        }
        for (auto &ts: anim_geometry) {
            for (auto &ro: ts) {
                ro->create(m_device);
            }
        }
    }

    m_modified = true;

    return old;
}


bool Anari::changeParameter(const Parameter *p)
{
    m_renderManager.handleParam(p);

    if (p == m_pointSizeParam) {
        AnariRenderObject::pointSize = m_pointSizeParam->getValue();
    } else if (p == m_libraryParam) {
        if (m_device) {
            releaseDevice(m_device);
        }

        auto libname = m_libraryParam->getValue();
        auto lib = anari::loadLibrary(libname.c_str(), anariStatusFunc, this);
        if (lib) {
            anari::Extensions extensions = anari::extension::getDeviceExtensionStruct(lib, "default");
            if (!extensions.ANARI_KHR_GEOMETRY_TRIANGLE)
                sendWarning("device does not support ANARI_KHR_GEOMETRY_TRIANGLE");
            if (!extensions.ANARI_KHR_CAMERA_PERSPECTIVE)
                sendWarning("device does not support ANARI_KHR_CAMERA_PERSPECTIVE");
            if (!extensions.ANARI_KHR_MATERIAL_MATTE)
                sendWarning("device doesn't support ANARI_KHR_MATERIAL_MATTE");

            auto nested = anari::newDevice(lib, "default");
            auto dev = nested;
            if (m_wrapperDevice) {
                anari::setParameter(m_wrapperDevice, m_wrapperDevice, "wrappedDevice", nested);
                anari::commitParameters(m_wrapperDevice, m_wrapperDevice);
                dev = m_wrapperDevice;
            }
            auto old = recreate(dev);
            if (m_nestedDevice && old == m_nestedDevice) {
                anari::release(m_nestedDevice, m_nestedDevice);
                m_nestedDevice = nullptr;
            }
            if (dev) {
                m_nestedDevice = nested;
                assert(m_world);

                auto rt = getRendererTypes(lib, nested);
                setParameterChoices(m_rendererParam, rt);

                m_renderer = createRenderer(dev, m_rendererParam->getValue());
            } else {
                sendError("failed to create default device");
                unloadAnari();
            }

        } else {
            sendError("failed to load library %s", libname.c_str());
        }
        anari::unloadLibrary(m_library);
        m_library = lib;
    } else if (p == m_debugWrapper) {
        if (m_debugWrapper->getValue()) {
            if (!m_wrapperLibrary) {
                m_wrapperLibrary = anari::loadLibrary("debug", anariStatusFunc, this);
            }
            if (m_wrapperLibrary) {
                if (!m_wrapperDevice) {
                    m_wrapperDevice = anari::newDevice(m_wrapperLibrary, "debug");
                    anari::setParameter(m_wrapperDevice, m_wrapperDevice, "wrappedDevice", m_nestedDevice);
                    anari::commitParameters(m_wrapperDevice, m_wrapperDevice);
                }
                releaseDevice(m_device);
                recreate(m_wrapperDevice); // do not release m_nestedDevice, as it is wrapped and still in use
                m_device = m_wrapperDevice;
            }
        } else {
            if (m_wrapperDevice) {
                releaseDevice(m_wrapperDevice);
                anari::unsetParameter(m_wrapperDevice, m_wrapperDevice, "wrappedDevice");
                anari::commitParameters(m_wrapperDevice, m_wrapperDevice);
                recreate(m_nestedDevice);
                anari::release(m_wrapperDevice, m_wrapperDevice);
                m_wrapperDevice = nullptr;
            }
        }
    } else if (p == m_rendererParam) {
        if (m_device) {
            anari::release(m_device, m_renderer);
        }
        m_renderer = nullptr;
        if (m_library && m_device) {
            auto rt = m_rendererParam->getValue();
            m_renderer = createRenderer(m_device, rt);
            if (!m_renderer) {
                sendError("failed to create renderer %s", rt.c_str());
            }
        }
    } else if (p == m_linearDepthParam) {
        m_linearDepth = m_linearDepthParam->getValue() != 0;
        m_renderManager.setLinearDepth(m_linearDepth);
    }

    return Renderer::changeParameter(p);
}


bool Anari::render()
{
    // ensure that previous frame is completed
    bool immed_resched = m_renderManager.finishFrame(m_timestep);

    //vistle::StopWatch timer("render");

    const size_t numTimesteps = anim_geometry.size();
    if (!m_renderManager.prepareFrame(numTimesteps)) {
        return immed_resched;
    }

    if (!m_device || !m_world) {
        CERR << "missing device or missing world" << std::endl;
        return immed_resched;
    }

    // switch time steps in embree scene
    if (m_renderManager.timestep() != m_timestep || m_renderManager.sceneChanged() || m_modified) {
        m_timestep = m_renderManager.timestep();
        m_modified = false;
        std::map<AnariRenderObject::mat4, std::vector<anari::Surface>, CompareMat4> toTransform;
        std::vector<anari::Surface> untransformed;
        for (auto &ro: static_geometry) {
            if (!ro->surface)
                continue;
            if (!m_renderManager.isVariantVisible(ro->variant))
                continue;
            if (ro->identityTransform)
                untransformed.push_back(ro->surface);
            else
                toTransform[ro->transform].push_back(ro->surface);
        }
        if (m_timestep >= 0 && anim_geometry.size() > unsigned(m_timestep)) {
            for (auto &ro: anim_geometry[m_timestep]) {
                if (!ro->surface)
                    continue;
                if (!m_renderManager.isVariantVisible(ro->variant))
                    continue;
                if (ro->identityTransform)
                    untransformed.push_back(ro->surface);
                else
                    toTransform[ro->transform].push_back(ro->surface);
            }
        }
        if (untransformed.empty()) {
            anari::unsetParameter(m_device, m_world, "surface");
        } else {
            anari::setAndReleaseParameter(m_device, m_world, "surface",
                                          anari::newArray1D(m_device, untransformed.data(), untransformed.size()));
        }
        std::vector<anari::Instance> instances;
        for (const auto &ts: toTransform) {
            auto group = anari::newObject<anari::Group>(m_device);
            anari::setAndReleaseParameter(m_device, group, "surface",
                                          anari::newArray1D(m_device, ts.second.data(), ts.second.size()));
            anari::commitParameters(m_device, group);
            instances.emplace_back();
            auto &instance = instances.back();
            instance = anari::newObject<anari::Instance>(m_device, "transform");
            CERR << "#surfaces: " << ts.second.size() << ", transform=";
            for (int i = 0; i < 16; ++i) {
                std::cerr << " " << ts.first[i / 4][i % 4];
            }
            std::cerr << std::endl;
            anari::setParameter(m_device, instance, "transform", ts.first);
            anari::setParameter(m_device, instance, "group", group);
            anari::commitParameters(m_device, instance);
        }
        anari::setAndReleaseParameter(m_device, m_world, "instance",
                                      anari::newArray1D(m_device, instances.data(), instances.size()));
        CERR << "world updated for time=" << m_timestep << " with " << untransformed.size() << " surfaces and "
             << instances.size() << " instances" << std::endl;
        anari::commitParameters(m_device, m_world);

        anari::std_types::box3 bounds;
        anari::getProperty(m_device, m_world, "bounds", bounds, ANARI_WAIT);
        CERR << "scene bounds: " << bounds[0][0] << bounds[0][1] << bounds[0][2] << bounds[1][0] << bounds[1][1]
             << bounds[1][2] << std::endl;
    }

    while (m_cameras.size() > m_renderManager.numViews()) {
        anari::release(m_device, m_cameras.back());
        m_cameras.pop_back();
        anari::release(m_device, m_frames.back());
        m_frames.pop_back();
    }

    while (m_cameras.size() < m_renderManager.numViews()) {
        m_frames.push_back(anari::newObject<anari::Frame>(m_device));
        auto &frame = m_frames.back();
        assert(m_world);
        anari::setParameter(m_device, frame, "world", m_world);

        //ANARIDataType fbFormat = ANARI_UFIXED8_RGBA_SRGB;
        ANARIDataType fbFormat = ANARI_UFIXED8_VEC4;
        ANARIDataType dbFormat = ANARI_FLOAT32;
        anariSetParameter(m_device, frame, "channel.color", ANARI_DATA_TYPE, &fbFormat);
        anariSetParameter(m_device, frame, "channel.depth", ANARI_DATA_TYPE, &dbFormat);
        anariSetParameter(m_device, frame, "renderer", ANARI_RENDERER, &m_renderer);

        m_cameras.push_back(anari::newObject<anari::Camera>(m_device, "perspective"));
        auto &camera = m_cameras.back();
        anari::commitParameters(m_device, camera);
        anari::setParameter(m_device, frame, "camera", camera);
        anari::commitParameters(m_device, frame);
    }

    // submit frames to ANARI
    for (size_t i = 0; i < m_renderManager.numViews(); ++i) {
        auto &vd = m_renderManager.viewData(i);
        //CERR << "rendering view " << i << ", proj=" << vd.proj << std::endl;
        const vistle::Matrix4 lightTransform = vd.model.inverse();
        for (auto &light: vd.lights) {
            light.transformedPosition = lightTransform * light.position;
            if (fabs(light.transformedPosition[3]) > Epsilon) {
                light.isDirectional = false;
                light.transformedPosition /= light.transformedPosition[3];
            } else {
                light.isDirectional = true;
            }
#if 0
          if (light.enabled)
             CERR << "light pos " << light.position.transpose() << " -> " << light.transformedPosition.transpose() << std::endl;
#endif
        }
        auto mv = m_renderManager.getModelViewMat(i);
        auto proj = m_renderManager.getProjMat(i);

        glm::vec3 eye, dir, up;
        float fovy, aspect;
        glm::box2 imgRegion;
        offaxisStereoCameraFromTransform(proj.inverse(), mv.inverse(), eye, dir, up, fovy, aspect, imgRegion);

        auto &camera = m_cameras[i];
        anariSetParameter(m_device, camera, "fovy", ANARI_FLOAT32, &fovy);
        anariSetParameter(m_device, camera, "aspect", ANARI_FLOAT32, &aspect);
        anariSetParameter(m_device, camera, "position", ANARI_FLOAT32_VEC3, &eye[0]);
        anariSetParameter(m_device, camera, "direction", ANARI_FLOAT32_VEC3, &dir[0]);
        anariSetParameter(m_device, camera, "up", ANARI_FLOAT32_VEC3, &up[0]);
        anariSetParameter(m_device, camera, "imageRegion", ANARI_FLOAT32_BOX2, &imgRegion.min);
        anariCommitParameters(m_device, camera);

        auto &frame = m_frames[i];
        uint32_t imgSize[] = {uint32_t(vd.width), uint32_t(vd.height)};
        anari::setParameter(m_device, frame, "size", imgSize);
        anari::commitParameters(m_device, frame);

        anari::render(m_device, frame);
    }

    // wait for frame completion and composite
    for (size_t i = 0; i < m_renderManager.numViews(); ++i) {
        m_renderManager.setCurrentView(i);
        auto &vd = m_renderManager.viewData(i);
        int viewport[4] = {0, 0, vd.width, vd.height};

        anari::wait(m_device, m_frames[i]);
        const auto mappedCol = anari::map<uint32_t>(m_device, m_frames[i], "channel.color");
        const auto mappedDepth = anari::map<float>(m_device, m_frames[i], "channel.depth");
        if (m_renderManager.linearDepth()) {
            m_renderManager.compositeCurrentView(reinterpret_cast<const unsigned char *>(mappedCol.data),
                                                 mappedDepth.data, viewport, m_timestep, false);
        } else {
            float *depth = m_renderManager.depth(i);
            auto mv = m_renderManager.getModelViewMat(i);
            auto proj = m_renderManager.getProjMat(i);
            glm::vec3 eye, dir, up;
            float fovy, aspect;
            glm::box2 imgRegion;
            offaxisStereoCameraFromTransform(proj.inverse(), mv.inverse(), eye, dir, up, fovy, aspect, imgRegion);
            transformDepthFromWorldToGL(mappedDepth.data, depth, eye, dir, up, fovy, aspect, imgRegion, mv, proj,
                                        mappedDepth.width, mappedDepth.height);
            m_renderManager.compositeCurrentView(reinterpret_cast<const unsigned char *>(mappedCol.data), depth,
                                                 viewport, m_timestep, false);
        }
        anari::unmap(m_device, m_frames[i], "channel.depth");
        anari::unmap(m_device, m_frames[i], "channel.color");
    }

    return true;
}


void Anari::removeObject(std::shared_ptr<RenderObject> vro)
{
    auto ro = std::static_pointer_cast<AnariRenderObject>(vro);

    const int t = ro->timestep;
    auto &objlist = t >= 0 ? anim_geometry[t] : static_geometry;

    auto it = std::find(objlist.begin(), objlist.end(), ro);
    if (it != objlist.end()) {
        std::swap(*it, objlist.back());
        objlist.pop_back();
    }

    while (!anim_geometry.empty() && anim_geometry.back().empty())
        anim_geometry.pop_back();

    CERR << "removeObject(" << ro->senderId << "/" << ro->variant << ", t=" << t << "@" << m_timestep << "/"
         << numTimesteps() << ")" << std::endl;

    m_renderManager.removeObject(vro);
}


std::shared_ptr<RenderObject> Anari::addObject(int sender, const std::string &senderPort,
                                               vistle::Object::const_ptr container, vistle::Object::const_ptr geometry,
                                               vistle::Object::const_ptr normals, vistle::Object::const_ptr texture)
{
    auto ro = std::make_shared<AnariRenderObject>(m_device, sender, senderPort, container, geometry, normals, texture);
    std::string species = container->getAttribute(attribute::Species);
    if (!species.empty()) {
        CERR << "add object: species=" << species << ", " << *container << std::endl;
        // make sure that colormap exists on current device
        //addColorMap(species, nullptr);
        ro->colorMap = &m_colormaps[species];
    }
    ro->create(m_device);

    const int t = ro->timestep;
    if (t == -1) {
        static_geometry.push_back(ro);
    } else {
        if (anim_geometry.size() <= size_t(t))
            anim_geometry.resize(t + 1);
        anim_geometry[t].push_back(ro);
    }

    m_renderManager.addObject(ro);

    return ro;
}

bool Anari::handleMessage(const vistle::message::Message *message, const vistle::MessagePayload &payload)
{
    if (m_renderManager.handleMessage(message, payload)) {
        return true;
    }

    return Renderer::handleMessage(message, payload);
}

MODULE_MAIN(Anari)
