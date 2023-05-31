#include <IceT.h>
#include <IceTMPI.h>

#include <cover/coVRPluginSupport.h>
#include <cover/coVRConfig.h>
#include <cover/coVRMSController.h>
#include <cover/coVRRenderer.h>
#include <cover/VRViewer.h>
#include <cover/VRSceneGraph.h>

#include "CompositorIceT.h"

#include <osg/MatrixTransform>

#include <vistle/rhr/ReadBackCuda.h>
#include <sysdep/opengl.h>


using namespace opencover;

static CompositorIceT *plugin = NULL;
static const bool doublebuffer = false;

#define CERR std::cerr << "CompositorIceT(" << (plugin ? std::to_string(plugin->mpiRank()) : std::string("?")) << "): "

bool checkGL(const char *desc)
{
    GLenum errorNo = glGetError();
    if (errorNo != GL_NO_ERROR) {
        CERR << "GL error: " << gluErrorString(errorNo) << " " << desc << endl;
        return false;
    }
    return true;
}

CompositorIceT::CompositorIceT()
: coVRPlugin(COVER_PLUGIN_NAME)
, m_initialized(false)
, m_rank(-1)
, m_size(-1)
, m_useCuda(true)
, m_sparseReadback(true)
, m_writeDepth(true)
, m_icetComm(0)
{
    CERR << "ctor" << std::endl;

    assert(plugin == NULL);
    plugin = this;
    //m_useCuda = true;
}


// this is called if the plugin is removed at runtime
CompositorIceT::~CompositorIceT()
{
    if (m_initialized)
        icetDestroyMPICommunicator(m_icetComm);

    cover->getScene()->removeChild(m_drawer);

    for (auto &cd: m_compositeData) {
        cover->getScene()->removeChild(cd.camera);
        VRViewer::instance()->removeCamera(cd.camera);
    }

    VRSceneGraph::instance()->setObjects(true);

    plugin = NULL;
}

int CompositorIceT::mpiSize() const
{
    return m_size;
}

int CompositorIceT::mpiRank() const
{
    return m_rank;
}

void CompositorIceT::screenBounds(int *bounds, const CompositeData &cd, const ViewData &vd, const osg::Vec3 &center,
                                  float r)
{
    bounds[0] = cd.width;
    bounds[1] = cd.height;
    bounds[2] = 0;
    bounds[3] = 0;

    float left = cd.width, right = 0.;
    float top = 0., bottom = cd.height;
    osg::Matrix vpt;
    vpt(0, 0) = cd.width;
    vpt(1, 1) = cd.height;
    vpt(2, 2) = 2.;
    vpt(3, 0) = cd.width;
    vpt(3, 1) = cd.height;
    vpt(3, 3) = 2.;

    osg::Matrix T = vd.model * vd.view * vd.proj * vpt;
    osg::Vec4 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i] = osg::Vec4(center[0], center[1], center[2], 1.);
        if (i & 0x1) {
            corners[i][0] += r;
        } else {
            corners[i][0] -= r;
        }
        if (i & 0x2) {
            corners[i][1] += r;
        } else {
            corners[i][1] -= r;
        }
        if (i & 0x4) {
            corners[i][2] += r;
        } else {
            corners[i][2] -= r;
        }
    }

    for (int i = 0; i < 8; ++i) {
        osg::Vec4 v = corners[i] * T;
        if (v[2] + v[3] >= 0.) {
            float x = v[0] / v[3];
            float y = v[1] / v[3];
            if (x < left)
                left = x;
            if (x > right)
                right = x;
            if (y < bottom)
                bottom = y;
            if (y > top)
                top = y;
        } else {
            for (int j = 0; j < 8; ++j) {
                if (i == j)
                    continue;
                osg::Vec4 v2 = corners[j] * T;
                if (v2[2] + v2[3] < 0.)
                    continue;
                float t = (v2[2] + v2[3]) / (v2[2] - v[2] + v2[3] - v[3]);
                float invw = 1.0 / ((v[3] - v2[3]) * t + v2[3]);
                float x = ((v[0] - v2[0]) * t + v2[0]) * invw;
                float y = ((v[1] - v2[1]) * t + v2[1]) * invw;
                if (x < left)
                    left = x;
                if (x > right)
                    right = x;
                if (y < bottom)
                    bottom = y;
                if (y > top)
                    top = y;
            }
        }
    }
    if (left < 0.)
        left = 0.;
    if (right > cd.width)
        right = cd.width;
    if (top > cd.height)
        top = cd.height;
    if (bottom < 0.)
        bottom = 0.;
    bounds[0] = floor(left);
    bounds[1] = floor(bottom);
    bounds[2] = ceil(right);
    bounds[3] = ceil(top);

    if (bounds[0] > bounds[2])
        bounds[0] = bounds[2];
    if (bounds[1] > bounds[3])
        bounds[1] = bounds[3];

#if 0
    CERR << "BOUNDS: r= " << r << ", c=" << center[0] << " " << center[1] << " " << center[2]
         << " ==> [" << bounds[0] << "," << bounds[2] << "]x["  << bounds[1] << "," << bounds[3] << "]" << std::endl;
#endif
}

struct CompositeCallback: public osg::Camera::DrawCallback {
    CompositeCallback(CompositorIceT *plugin, int view): plugin(plugin), view(view) {}
    virtual void operator()(const osg::Camera &cam) const override
    {
        //CERR << "Camera::DrawCallback: view=" << view << std::endl;

        auto &cd = plugin->m_compositeData[view];
        auto &vd = plugin->m_viewData[view];
        int vp[4] = {0, 0, cd.width, cd.height};

        if (plugin->m_sparseReadback) {
            osg::Node *root = cover->getObjectsRoot();
            osg::BoundingSphere bs = root->getBound();
            plugin->screenBounds(vp, cd, vd, bs.center(), bs.radius());
            vp[2] -= vp[0];
            vp[3] -= vp[1];
        }
        for (int i = 0; i < 4; ++i) {
            cd.viewport[i] = vp[i];
        }

        int width = cd.width;
        if (!CompositorIceT::readpixels(cd.cudaColor, vp[0], vp[1], vp[2], width, vp[3], GL_RGBA, 4,
                                        &cd.rgba[4 * (vp[1] * width + vp[0])], cd.buffer, GL_UNSIGNED_BYTE)) {
            CERR << "read RGBA failed" << std::endl;
        }
        if (!CompositorIceT::readpixels(cd.cudaDepth, vp[0], vp[1], vp[2], width, vp[3], GL_DEPTH_COMPONENT, 4,
                                        reinterpret_cast<unsigned char *>(&cd.depth[vp[1] * width + vp[0]]), cd.buffer,
                                        GL_FLOAT)) {
            CERR << "read depth failed" << std::endl;
        }
    }

    CompositorIceT *plugin;
    int view;
};

class NoSwapCallback: public osg::GraphicsContext::SwapCallback {
public:
    void swapBuffersImplementation(osg::GraphicsContext *gc) override {}
};

osg::ref_ptr<osg::GraphicsContext> CompositorIceT::createGraphicsContext(int view, bool pbuffer,
                                                                         osg::ref_ptr<osg::GraphicsContext> sharedGc)
{
    auto &cd = m_compositeData[view];

    osg::ref_ptr<osg::DisplaySettings> displaySettings = new osg::DisplaySettings;
    displaySettings->setStereo(false);

    osg::ref_ptr<osg::GraphicsContext> gc;
    cd.camera->setRenderOrder(osg::Camera::PRE_RENDER);
    if (pbuffer) {
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(displaySettings);
        traits->readDISPLAY();
        traits->setScreenIdentifier(":0");
        traits->setUndefinedScreenDetailsToDefaultScreen();
        traits->sharedContext = sharedGc;
        traits->x = 0;
        traits->y = 0;
        traits->width = cd.width > 0 ? cd.width : 1;
        traits->height = cd.height > 0 ? cd.height : 1;
        traits->alpha = 8;
        traits->depth = 24;
        traits->doubleBuffer = pbuffer ? false : doublebuffer;
        traits->windowDecoration = true;
        traits->windowName = "OsgRenderer: view " + std::to_string(view);
        traits->pbuffer = true;
        traits->vsync = false;

        gc = osg::GraphicsContext::createGraphicsContext(traits);
        cd.camera->setGraphicsContext(gc);
#if 0
        cd.camera->setRenderTargetImplementation(osg::Camera::PIXEL_BUFFER);
        cd.camera->setRenderTargetImplementation(osg::Camera::SEPARATE_WINDOW);
        cd.camera->setImplicitBufferAttachmentMask(0, 0);
        cd.buffer = GL_BACK;
#if 0
        if (!traits->doubleBuffer) {
            gc->setSwapCallback(new NoSwapCallback);
            cd.buffer = GL_FRONT;
        }
#endif
#if 0
        cd.camera->setDrawBuffer(cd.buffer);
        cd.camera->setReadBuffer(cd.buffer);
#endif

    } else {
        gc = cd.camera->getGraphicsContext();
#endif

#if 1
        cd.camera->attach(osg::Camera::COLOR_BUFFER, GL_RGBA);
        cd.camera->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT32F);
#else
        cd.camera->setImplicitBufferAttachmentMask(
            osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT | osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
            osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT | osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT);
#endif
        cd.camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        cd.camera->setRenderTargetImplementation(osg::Camera::PIXEL_BUFFER);
    }

    cd.camera->setPostDrawCallback(new CompositeCallback(this, view));
    cd.camera->setClearColor(osg::Vec4(m_rank % 2, (m_rank / 2) % 2, 1, 1));
    cd.camera->setClearMask(osg::Camera::DEPTH_BUFFER | osg::Camera::COLOR_BUFFER);

    return gc;
}

bool CompositorIceT::init()
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized) {
        std::cerr << "CompositorIceT: No MPI support" << std::endl;
        return false;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_size);
    m_icetComm = icetCreateMPICommunicator(MPI_COMM_WORLD);
    m_initialized = true;

    const int numChannels = coVRConfig::instance()->numChannels();

    m_drawer = new MultiChannelDrawer();
    m_drawer->setMode(MultiChannelDrawer::AsIs);
    int localViews = m_drawer->numViews();
    cover->getScene()->addChild(m_drawer);

    if (coVRMSController::instance()->isMaster()) {
        int viewBase = 0;
        m_viewBase.push_back(viewBase);
        m_numViews.push_back(localViews);
        viewBase += localViews;
        coVRMSController::SlaveData sNumViews(sizeof(localViews));
        coVRMSController::instance()->readSlaves(&sNumViews);
        for (int i = 0; i < coVRMSController::instance()->getNumSlaves(); ++i) {
            int n = *static_cast<int *>(sNumViews.data[i]);
            m_viewBase.push_back(viewBase);
            viewBase += n;
            m_numViews.push_back(n);
        }
        assert(m_numViews.size() == size_t(coVRMSController::instance()->getNumSlaves() + 1));
        coVRMSController::instance()->sendSlaves(&m_numViews[0], sizeof(m_numViews[0]) * m_numViews.size());
        coVRMSController::instance()->sendSlaves(&m_viewBase[0], sizeof(m_viewBase[0]) * m_viewBase.size());
        CERR << "compositing a total of " << viewBase << " views" << std::endl;
    } else {
        coVRMSController::instance()->sendMaster(&localViews, sizeof(localViews));
        m_numViews.resize(coVRMSController::instance()->getNumSlaves() + 1);
        m_viewBase.resize(coVRMSController::instance()->getNumSlaves() + 1);
        coVRMSController::instance()->readMaster(&m_numViews[0], sizeof(m_numViews[0]) * m_numViews.size());
        coVRMSController::instance()->readMaster(&m_viewBase[0], sizeof(m_viewBase[0]) * m_viewBase.size());
    }

    int numViews = 0;
    for (auto nv: m_numViews)
        numViews += nv;
    m_viewData.resize(numViews);
    m_compositeData.resize(numViews);

    int view = 0;
    for (int i = 0; i < numChannels; ++i) {
        int idx = m_viewBase[m_rank] + view;
        m_compositeData[idx].localIdx = view;
        m_compositeData[idx].chan = i;
        ++view;
        if (coVRConfig::instance()->channels[i].stereoMode == osg::DisplaySettings::QUAD_BUFFER) {
            int idx = m_viewBase[m_rank] + view;
            m_compositeData[idx].localIdx = view;
            m_compositeData[idx].chan = i;
            ++view;
        }
    }

    for (int i = 0; i < coVRMSController::instance()->getNumSlaves() + 1; ++i) {
        for (int v = 0; v < m_numViews[i]; ++v) {
            int idx = m_viewBase[i] + v;
            CERR << "adding view " << idx << " for rank " << i << std::endl;
            auto &cd = m_compositeData[idx];
            cd.rank = i;
            cd.camera = new osg::Camera;
            auto &cam = *cd.camera;
            cam.setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            cam.setCullMask(~0 & ~(Isect::Collision | Isect::Intersection | Isect::NoMirror | Isect::Pick |
                                   Isect::Walk | Isect::Touch));
            cam.setRenderTargetImplementation(osg::Camera::PIXEL_BUFFER);
            //cam.setRenderTargetImplementation(osg::Camera::SEPARATE_WINDOW);
#if 0
           cam.setImplicitBufferAttachmentMask(osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
                                                   osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT);
#endif

            cam.setPostDrawCallback(new CompositeCallback(this, idx));
            cam.setClearColor(osg::Vec4(m_rank % 2, (m_rank / 2) % 2, 1, 1));
            cam.setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            checkResize(idx);

            cam.setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

            osgViewer::Renderer *renderer = new coVRRenderer(&cam, -1 /* channel */);
            renderer->getSceneView(0)->setSceneData(VRSceneGraph::instance()->getObjectsScene());
            renderer->getSceneView(1)->setSceneData(VRSceneGraph::instance()->getObjectsScene());
            cam.setRenderer(renderer);

            cam.setViewport(0, 0, cd.width, cd.height);
            VRViewer::instance()->addCamera(&cam);
            cover->getScene()->addChild(&cam);
        }
    }

    VRSceneGraph::instance()->setObjects(false);

    return true;
}

void CompositorIceT::getViewData(CompositorIceT::ViewData &view, int idx)
{
    view.model = m_drawer->modelMatrix(idx);
    view.view = m_drawer->viewMatrix(idx);
    view.proj = m_drawer->projectionMatrix(idx);
}


void CompositorIceT::preFrame()
{
    int view = m_viewBase[coVRMSController::instance()->getID()];
    for (int i = 0; i < m_drawer->numViews(); ++i) {
        getViewData(m_viewData[view + i], i);
    }

    int numViews = 0;
    for (auto nv: m_numViews)
        numViews += nv;

    for (int view = 0; view < numViews; ++view)
        checkResize(view);

    if (coVRMSController::instance()->isMaster()) {
        for (int i = 0; i < coVRMSController::instance()->getNumSlaves(); ++i) {
            int viewBase = m_viewBase[i + 1];
            int numViews = m_numViews[i + 1];
            coVRMSController::instance()->readSlave(i, &m_viewData[viewBase], sizeof(m_viewData[0]) * numViews);
        }
        coVRMSController::instance()->sendSlaves(&m_viewData[0], sizeof(m_viewData[0]) * m_viewData.size());
    } else {
        int viewBase = m_viewBase[coVRMSController::instance()->getID()];
        int numViews = m_numViews[coVRMSController::instance()->getID()];
        coVRMSController::instance()->sendMaster(&m_viewData[viewBase], sizeof(m_viewData[0]) * numViews);
        coVRMSController::instance()->readMaster(&m_viewData[0], sizeof(m_viewData[0]) * m_viewData.size());
    }

    for (int i = 0; i < numViews; ++i) {
        auto &cd = m_compositeData[i];
        auto &vd = m_viewData[i];
        cd.camera->setViewMatrix(vd.view);
        cd.camera->setProjectionMatrix(vd.proj);
    }
}

void CompositorIceT::checkResize(int view)
{
    assert(int(m_compositeData.size()) > view);

    CompositeData &cd = m_compositeData[view];
    int dim[2] = {0, 0}; // width, height;
    if (cd.chan >= 0) {
        // channel is local
        const auto &ch = coVRConfig::instance()->channels[cd.chan];
        const viewportStruct &vp = coVRConfig::instance()->viewports[ch.viewportNum];
        if (vp.window >= 0) {
            const auto &win = coVRConfig::instance()->windows[vp.window];
            dim[0] = (vp.viewportXMax - vp.viewportXMin) * win.sx;
            dim[1] = (vp.viewportYMax - vp.viewportYMin) * win.sy;
        }
        if (coVRMSController::instance()->isSlave()) {
            coVRMSController::instance()->sendMaster(dim, sizeof(dim));
        }
    } else {
        if (coVRMSController::instance()->isMaster()) {
            coVRMSController::instance()->readSlave(cd.rank - 1, dim, sizeof(dim));
        }
    }

    if (coVRMSController::instance()->isMaster()) {
        coVRMSController::instance()->sendSlaves(dim, sizeof(dim));
    } else {
        coVRMSController::instance()->readMaster(dim, sizeof(dim));
    }

    if (dim[0] != cd.width || dim[1] != cd.height) {
        CERR << "resizing view " << view << " from " << cd.width << "x" << cd.height << " to " << dim[0] << "x"
             << dim[1] << std::endl;
        cd.rgba.resize(dim[0] * dim[1] * 4);
        cd.depth.resize(dim[0] * dim[1]);
        if (cd.width != -1) {
            icetDestroyContext(cd.icetCtx);
        } else {
            if (m_useCuda) {
                cd.cudaColor = new vistle::ReadBackCuda();
                if (!cd.cudaColor->init()) {
                    delete cd.cudaColor;
                    cd.cudaColor = NULL;
                }

                cd.cudaDepth = new vistle::ReadBackCuda();
                if (!cd.cudaDepth->init()) {
                    delete cd.cudaDepth;
                    cd.cudaDepth = NULL;
                }
            }
        }
        cd.icetCtx = icetCreateContext(m_icetComm);
        icetSetContext(cd.icetCtx);
        cd.width = dim[0];
        cd.height = dim[1];

        icetResetTiles();
        icetAddTile(0, 0, cd.width, cd.height, cd.rank);
        if (m_writeDepth) {
            icetDisable(ICET_COMPOSITE_ONE_BUFFER);
        } else {
            icetEnable(ICET_COMPOSITE_ONE_BUFFER);
        }
        icetStrategy(ICET_STRATEGY_REDUCE);
        icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
        icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
        icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);

        if (cd.camera)
            cd.camera->setViewport(0, 0, cd.width, cd.height);

        if (cd.chan >= 0) {
            m_drawer->resizeView(cd.localIdx, cd.width, cd.height, GL_FLOAT);
        }
    }
}

void CompositorIceT::composite(int view)
{
    //CERR << "composite(" << view << ")" << std::endl << std::flush;

    auto &cd = m_compositeData[view];
    auto &vd = m_viewData[view];

    icetSetContext(cd.icetCtx);

    osg::Matrix osgmv = vd.model * vd.view;
    IceTDouble mv[16], proj[16];
    for (int i = 0; i < 16; ++i) {
        proj[i] = vd.proj(i / 4, i % 4);
        mv[i] = osgmv(i / 4, i % 4);
    }

    IceTFloat bg[4] = {0.0, 0.0, 0.0, 0.0};
    //CERR << "VP: " << cd.viewport[0] << " " << cd.viewport[1] << " " << cd.viewport[2] << " " << cd.viewport[3] << std::endl;
    cd.image = icetCompositeImage(cd.rgba.data(), cd.depth.data(), cd.viewport, proj, mv, bg);
    if (!icetImageIsNull(cd.image)) {
        IceTUByte *color = icetImageGetColorub(cd.image);
        memcpy(m_drawer->rgba(cd.localIdx), color, cd.width * cd.height * 4);
        if (m_writeDepth) {
            const IceTFloat *depth = icetImageGetDepthf(cd.image);
            memcpy(m_drawer->depth(cd.localIdx), depth, cd.width * cd.height * 4);
        }
    } else {
        CERR << "IceT img: NULL" << std::endl;
    }
}

void CompositorIceT::clusterSyncDraw()
{
    assert(m_compositeData.size() == m_viewData.size());

    for (size_t i = 0; i < m_compositeData.size(); ++i) {
        composite(i);
    }
    if (m_drawer)
        m_drawer->swapFrame();
}

//! OpenGL framebuffer read-back
bool CompositorIceT::readpixels(vistle::ReadBackCuda *cuda, GLint x, GLint y, GLint w, GLint pitch, GLint h,
                                GLenum format, int ps, GLubyte *bits, GLenum buf, GLenum type)
{
    checkGL("readpixels pre");

    if (cuda) {
        return cuda->readpixels(x, y, w, pitch, h, format, ps, bits, buf, type);
    }

    bool ok = true;

    GLint readbuf = GL_BACK;
    glGetIntegerv(GL_READ_BUFFER, &readbuf);
    if (!checkGL("readpixels get read buffer")) {
        ok = false;
    }

    //CERR << "setting read buffer to " << buf << std::endl;
    glReadBuffer(buf);
    if (!checkGL("readpixels set read buffer")) {
        ok = false;
    }
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    if (pitch % 8 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 8);
    else if (pitch % 4 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    else if (pitch % 2 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 2);
    else if (pitch % 1 == 0)
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glPixelStorei(GL_PACK_ROW_LENGTH, pitch);
    if (!checkGL("readpixels set pixel store")) {
        ok = false;
    }

    glReadPixels(x, y, w, h, format, type, bits);
    if (!checkGL("readpixels read")) {
        ok = false;
    }

    glPopClientAttrib();
    glReadBuffer(readbuf);
    if (!checkGL("readpixels restore read buffer")) {
        ok = false;
    }
    return ok;
}

void CompositorIceT::expandBoundingSphere(osg::BoundingSphere &bs)
{
    if (coVRMSController::instance()->isMaster()) {
        coVRMSController::SlaveData sd(sizeof(bs));
        coVRMSController::instance()->readSlaves(&sd);
        for (int i = 0; i < coVRMSController::instance()->getNumSlaves(); ++i) {
            osg::BoundingSphere *expand = static_cast<osg::BoundingSphere *>(sd.data[i]);
            bs.expandBy(*expand);
        }
        coVRMSController::instance()->sendSlaves(&bs, sizeof(bs));
    } else {
        coVRMSController::instance()->sendMaster(&bs, sizeof(bs));
        coVRMSController::instance()->readMaster(&bs, sizeof(bs));
    }
}

COVERPLUGIN(CompositorIceT)
