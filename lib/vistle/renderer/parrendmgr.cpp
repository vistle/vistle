#include "parrendmgr.h"
#include "renderobject.h"
#include "renderer.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>

#include <IceT.h>
#include <IceTMPI.h>

#ifdef MODULE_THREAD
std::recursive_mutex vistle::ParallelRemoteRenderManager::s_icetMutex;
#define LOCK_ICET(mtx) mtx.lock()
#define UNLOCK_ICET(mtx) mtx.unlock()
#define ICET_LOCKER std::unique_lock icet_lock(s_icetMutex)
#else
#define ICET_LOCKER
#define LOCK_ICET(mtx)
#define UNLOCK_ICET(mtx)
#endif

namespace mpi = boost::mpi;


namespace boost {
namespace serialization {

template<class Archive>
inline void serialize(Archive &ar, vistle::Renderer::Variant &t, const unsigned int file_version)
{
    ar &t.name;
    ar &t.objectCount;
    ar &t.visible;
}

} // namespace serialization
} // namespace boost


namespace vistle {

namespace {

void toIcet(IceTDouble *imat, const vistle::Matrix4 &vmat)
{
    for (int i = 0; i < 16; ++i) {
        imat[i] = vmat.data()[i];
    }
}

} // namespace

ParallelRemoteRenderManager::ParallelRemoteRenderManager(Renderer *module)
: m_module(module)
, m_displayRank(0)
, m_rhrControl(module, m_displayRank)
, m_delay(nullptr)
, m_delaySec(0.)
, m_defaultColor(Vector4(255, 255, 255, 255))
, m_updateBounds(1)
, m_updateVariants(1)
, m_updateScene(0)
, m_doRender(1)
, m_lightsUpdateCount(1000) // start with a value that is different from the one managed by RhrServer
, m_currentView(-1)
, m_frameComplete(true)
{
    m_module->setCurrentParameterGroup("Advanced", false);
    m_continuousRendering = m_module->addIntParameter("continuous_rendering", "render even though nothing has changed",
                                                      0, Parameter::Boolean);
    m_delay = m_module->addFloatParameter("delay", "artificial delay (s)", m_delaySec);
    m_module->setParameterRange(m_delay, 0., 3.);
    m_colorRank = m_module->addIntParameter("color_rank", "different colors on each rank", 0, Parameter::Boolean);
    m_module->setCurrentParameterGroup("");
}

ParallelRemoteRenderManager::~ParallelRemoteRenderManager()
{
    ICET_LOCKER;

    for (auto &icet: m_icet) {
        if (icet.ctxValid)
            icetDestroyContext(icet.ctx);
    }
}

void ParallelRemoteRenderManager::setLinearDepth(bool linear)
{
    m_rhrControl.setLinearDepth(linear);
}

bool ParallelRemoteRenderManager::linearDepth() const
{
    return m_rhrControl.linearDepth();
}

Port *ParallelRemoteRenderManager::outputPort() const
{
    return m_rhrControl.outputPort();
}

void ParallelRemoteRenderManager::connectionAdded(const Port *to)
{
    m_rhrControl.addClient(to);
}

void ParallelRemoteRenderManager::connectionRemoved(const Port *to)
{
    m_rhrControl.removeClient(to);
}

bool ParallelRemoteRenderManager::checkIceTError(const char *msg) const
{
    ICET_LOCKER;

    const char *err = "No error.";
    switch (icetGetError()) {
    case ICET_INVALID_VALUE:
        err = "An inappropriate value has been passed to a function.";
        break;
    case ICET_INVALID_OPERATION:
        err = "An inappropriate function has been called.";
        break;
    case ICET_OUT_OF_MEMORY:
        err = "IceT has ran out of memory for buffer space.";
        break;
    case ICET_BAD_CAST:
        err = "A function has been passed a value of the wrong type.";
        break;
    case ICET_INVALID_ENUM:
        err = "A function has been passed an invalid constant.";
        break;
    case ICET_SANITY_CHECK_FAIL:
        err = "An internal error (or warning) has occurred.";
        break;
    case ICET_NO_ERROR:
        return false;
        break;
    }

    std::cerr << "IceT error at " << msg << ": " << err << std::endl;
    return true;
}


void ParallelRemoteRenderManager::setModified()
{
    m_doRender = 1;
}

bool ParallelRemoteRenderManager::sceneChanged() const
{
    return m_updateScene;
}

bool ParallelRemoteRenderManager::isVariantVisible(const std::string &variant) const
{
    if (variant.empty())
        return true;
    const auto it = m_clientVariants.find(variant);
    if (it != m_clientVariants.end()) {
        return it->second;
    }
    auto it2 = m_localVariants.find(variant);
    if (it2 != m_localVariants.end()) {
        return it2->second.visible != RenderObject::Hidden;
    }

    std::cerr << "ParallelRemoteRenderManager: isVariantVisible(" << variant << "): unknown variant" << std::endl;
    return true;
}

void ParallelRemoteRenderManager::setLocalBounds(const Vector3 &min, const Vector3 &max)
{
    localBoundMin = min;
    localBoundMax = max;
    m_updateBounds = 1;
}

bool ParallelRemoteRenderManager::handleParam(const Parameter *p)
{
    setModified();

    if (p == m_colorRank) {
        if (m_colorRank->getValue()) {
            const int r = m_module->rank() % 3;
            const int g = (m_module->rank() / 3) % 3;
            const int b = (m_module->rank() / 9) % 3;
            m_defaultColor = Vector4(63 + r * 64, 63 + g * 64, 63 + b * 64, 255);
        } else {
            m_defaultColor = Vector4(255, 255, 255, 255);
        }
        return true;
    } else if (p == m_delay) {
        m_delaySec = m_delay->getValue();
    }

    return m_rhrControl.handleParam(p);
}

void ParallelRemoteRenderManager::updateVariants()
{
    auto rhr = m_rhrControl.server();
    if (rhr) {
        m_clientVariants = rhr->getVariants();
    }
    mpi::broadcast(m_module->comm(), m_clientVariants, rootRank());

    std::vector<Renderer::VariantMap> all;
    mpi::gather(m_module->comm(), m_module->variants(), all, rootRank());
    Renderer::VariantMap newState;
    if (m_module->rank() == 0) {
        for (const auto &varmap: all) {
            for (const auto &var: varmap) {
                auto it = newState.find(var.first);
                if (it == newState.end()) {
                    it = newState.emplace(var.first, var.second).first;
                } else {
                    it->second.objectCount += var.second.objectCount;
                    if (it->second.visible == RenderObject::DontChange) {
                        it->second.visible = var.second.visible;
                    }
                }
            }
        }
    }
    mpi::broadcast(m_module->comm(), newState, rootRank());
    std::vector<std::pair<std::string, vistle::RenderObject::InitialVariantVisibility>> addedVariants;
    std::vector<std::string> removedVariants;
    for (const auto &var: newState) {
        auto it = m_localVariants.find(var.first);
        if (it == m_localVariants.end()) {
            if (var.second.objectCount > 0) {
                addedVariants.emplace_back(var.first, var.second.visible);
            }
        } else {
            if (it->second.objectCount == 0 && var.second.objectCount > 0) {
                addedVariants.emplace_back(var.first, var.second.visible);
            }
        }
    }
    for (const auto &var: m_localVariants) {
        auto it = newState.find(var.first);
        if (it == newState.end()) {
            if (var.second.objectCount > 0)
                removedVariants.push_back(var.first);
        } else {
            if (var.second.objectCount > 0 && it->second.objectCount == 0)
                removedVariants.push_back(var.first);
        }
    }

    m_localVariants = std::move(newState);

    if (rhr) {
        rhr->updateVariants(addedVariants, removedVariants);
    }
}

bool ParallelRemoteRenderManager::prepareFrame(size_t numTimesteps)
{
    m_state.numTimesteps = numTimesteps;
    m_state.numTimesteps = mpi::all_reduce(m_module->comm(), m_state.numTimesteps, mpi::maximum<unsigned>());

    if (int(numTimesteps) != m_module->numTimesteps()) {
        std::cerr << "ParallelRemoteRenderManager: WARNING: #timesteps mismatch: have " << m_module->numTimesteps()
                  << ", expected " << numTimesteps << std::endl;
    }

    if (m_updateBounds) {
        Vector3 min, max;
        m_module->getBounds(min, max);
        setLocalBounds(min, max);
        //std::cerr << "local bounds min=" << min << ", max=" << max << std::endl;
    }

    m_updateBounds = mpi::all_reduce(m_module->comm(), m_updateBounds, mpi::maximum<int>());
    if (m_updateBounds) {
        mpi::all_reduce(m_module->comm(), localBoundMin.data(), 3, m_state.bMin.data(), mpi::minimum<Scalar>());
        mpi::all_reduce(m_module->comm(), localBoundMax.data(), 3, m_state.bMax.data(), mpi::maximum<Scalar>());
    }

    if (!m_rhrControl.hasConnection())
        m_rhrControl.tryConnect(0.0);

    m_updateScene = 0;
    auto rhr = m_rhrControl.server();
    if (rhr) {
        if (m_updateBounds) {
            //std::cerr << "updated bounds: min=" << m_state.bMin[0] << " " << m_state.bMin[1] << " " << m_state.bMin[2] << std::endl;
            Vector3 center = 0.5 * m_state.bMin + 0.5 * m_state.bMax;
            Scalar radius = (m_state.bMax - m_state.bMin).norm() * 0.5;
            for (int c = 0; c < 3; ++c) {
                if (m_state.bMin[c] > m_state.bMax[c]) {
                    radius = -1.;
                    break;
                }
            }
            rhr->setBoundingSphere(center, radius);
        }

        rhr->setNumTimesteps(m_state.numTimesteps);
#if 0
      if (rhr->timestep() >= int(numTimesteps)) {
          std::cerr << "ParallelRemoteRenderManager: WARNING: #timesteps mismatch: have "
                    << m_module->numTimesteps() << ", current is " << rhr->timestep() << std::endl;
      }
#endif

        rhr->preFrame();
        if (m_module->rank() == rootRank()) {
            if (m_state.timestep != rhr->timestep())
                m_doRender = 1;
        }
        m_state.timestep = rhr->timestep();

        if (rhr->lightsUpdateCount != m_lightsUpdateCount) {
            m_doRender = 1;
            m_lightsUpdateCount = rhr->lightsUpdateCount;
        }

        if (m_viewData.size() > rhr->numViews()) {
            std::cerr << "removing views " << rhr->numViews() << "-" << m_viewData.size() - 1 << std::endl;
            m_viewData.resize(rhr->numViews());
            m_doRender = 1;
        }
        for (size_t i = m_viewData.size(); i < rhr->numViews(); ++i) {
            std::cerr << "new view no. " << i << std::endl;
            m_doRender = 1;
            m_viewData.emplace_back();
        }
        assert(m_viewData.size() == rhr->numViews());

        for (size_t i = 0; i < rhr->numViews(); ++i) {
            PerViewState &vd = m_viewData[i];

            if (vd.width != rhr->width(i) || vd.height != rhr->height(i) || vd.proj != rhr->projMat(i) ||
                vd.view != rhr->viewMat(i) || vd.model != rhr->modelMat(i)) {
                m_doRender = 1;
            }

            vd.rhrParam = rhr->getViewParameters(i);

            vd.width = rhr->width(i);
            vd.height = rhr->height(i);
            vd.model = rhr->modelMat(i);
            vd.view = rhr->viewMat(i);
            vd.proj = rhr->projMat(i);

            vd.lights = rhr->lights;
        }

        if (rhr->updateCount() != m_updateCount) {
            m_updateScene = 1;
            m_updateCount = rhr->updateCount();
        }
    } else if (m_module->rank() == rootRank()) {
        if (!m_viewData.empty()) {
            m_viewData.clear();
            m_doRender = 1;
        }
    }
    m_updateBounds = 0;

    m_updateVariants = mpi::all_reduce(m_module->comm(), m_updateVariants, mpi::maximum<int>());
    if (m_updateVariants) {
        updateVariants();
        m_updateVariants = 0;
    }

    if (m_continuousRendering->getValue())
        m_doRender = 1;

    m_updateScene = mpi::all_reduce(m_module->comm(), m_updateScene, mpi::maximum<int>());
    if (m_updateScene) {
        updateVariants();
        m_doRender = 1;
    }
    bool doRender = mpi::all_reduce(m_module->comm(), m_doRender, mpi::maximum<int>());
    m_doRender = 0;

    if (doRender) {
        mpi::broadcast(m_module->comm(), m_state, rootRank());
        mpi::broadcast(m_module->comm(), m_viewData, rootRank());

        if (m_rgba.size() != m_viewData.size())
            m_rgba.resize(m_viewData.size());
        if (m_depth.size() != m_viewData.size())
            m_depth.resize(m_viewData.size());
        for (size_t i = 0; i < m_viewData.size(); ++i) {
            const PerViewState &vd = m_viewData[i];
            m_rgba[i].resize(vd.width * vd.height * 4);
            m_depth[i].resize(vd.width * vd.height);
            if (rhr && m_module->rank() != rootRank()) {
                rhr->resize(i, vd.width, vd.height);
            }
        }
    }

    return doRender;
}

int ParallelRemoteRenderManager::timestep() const
{
    return m_state.timestep;
}

size_t ParallelRemoteRenderManager::numViews() const
{
    return m_viewData.size();
}

void ParallelRemoteRenderManager::setCurrentView(size_t i)
{
    ICET_LOCKER;
    LOCK_ICET(s_icetMutex);

    checkIceTError("setCurrentView");

    assert(m_currentView == -1);
    auto rhr = m_rhrControl.server();

    int resetTiles = 0;
    while (m_icet.size() <= i) {
        resetTiles = 1;
        m_icet.emplace_back();
        auto &icet = m_icet.back();
        auto icetComm = icetCreateMPICommunicator((MPI_Comm)m_module->comm());
        icet.ctx = icetCreateContext(icetComm);
        icetDestroyMPICommunicator(icetComm);
        icet.ctxValid = true;
#ifndef NDEBUG
        // that's too much output
        //icetDiagnostics(ICET_DIAG_ALL_NODES | ICET_DIAG_DEBUG);
#endif
    }
    assert(i < m_icet.size());
    auto &icet = m_icet[i];
    //std::cerr << "setting IceT context: " << icet.ctx << " (valid: " << icet.ctxValid << ")" << std::endl;
    icetSetContext(icet.ctx);
    m_currentView = i;

    if (rhr) {
        if (icet.width != rhr->width(i) || icet.height != rhr->height(i))
            resetTiles = 1;
    }
    resetTiles = mpi::all_reduce(m_module->comm(), resetTiles, mpi::maximum<int>());

    if (resetTiles) {
        std::cerr << "resetting IceT tiles for view " << i << "..." << std::endl;
        icet.width = icet.height = 0;
        if (m_module->rank() == rootRank()) {
            icet.width = rhr->width(i);
            icet.height = rhr->height(i);
            std::cerr << "IceT dims on rank " << m_module->rank() << ": " << icet.width << "x" << icet.height
                      << std::endl;
        }

        DisplayTile localTile;
        localTile.x = 0;
        localTile.y = 0;
        localTile.width = icet.width;
        localTile.height = icet.height;

        std::vector<DisplayTile> icetTiles;
        mpi::all_gather(m_module->comm(), localTile, icetTiles);
        assert(icetTiles.size() == (unsigned)m_module->size());

        icetResetTiles();

        for (int i = 0; i < m_module->size(); ++i) {
            if (icetTiles[i].width > 0 && icetTiles[i].height > 0)
                icetAddTile(icetTiles[i].x, icetTiles[i].y, icetTiles[i].width, icetTiles[i].height, i);
        }

        icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
        icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
        icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
        //icetStrategy(ICET_STRATEGY_REDUCE);
        icetStrategy(ICET_STRATEGY_SEQUENTIAL);
        icetDisable(ICET_COMPOSITE_ONE_BUFFER); // include depth buffer in compositing result

        icetDrawCallback(nullptr);

        checkIceTError("after reset tiles");
    }

    icetBoundingBoxf(localBoundMin[0], localBoundMax[0], localBoundMin[1], localBoundMax[1], localBoundMin[2],
                     localBoundMax[2]);
    checkIceTError("exit setCurrentView");
}

void ParallelRemoteRenderManager::compositeCurrentView(const unsigned char *rgba, const float *depth, const int vp[4],
                                                       int timestep, bool lastView)
{
    ICET_LOCKER;

    if (!rgba || !depth) {
        finishCurrentView(nullptr, timestep, lastView);
        UNLOCK_ICET(s_icetMutex);
        return;
    }

    checkIceTError("compositeCurrentView");

    const auto &i = m_currentView;

    auto P = getProjMat(i);
    IceTDouble proj[16];
    toIcet(proj, P);
    auto MV = getModelViewMat(i);
    IceTDouble mv[16];
    toIcet(mv, MV);

    IceTInt viewport[4] = {vp[0], vp[1], vp[2], vp[3]};
    IceTFloat bg[4] = {0., 0., 0., 0.};
    IceTImage img = icetCompositeImage(rgba, depth, viewport, proj, mv, bg);
    finishCurrentView(&img, timestep, lastView);

    UNLOCK_ICET(s_icetMutex);
}

void ParallelRemoteRenderManager::finishCurrentView(const IceTImagePtr imgp, int timestep)
{
    const bool lastView = size_t(m_currentView) == m_viewData.size() - 1;
    finishCurrentView(imgp, timestep, lastView);
}

void ParallelRemoteRenderManager::finishCurrentView(const IceTImagePtr imgp, int timestep, bool lastView)
{
    ICET_LOCKER;

    checkIceTError("before finishCurrentView");

    assert(m_currentView >= 0);
    const size_t i = m_currentView;
    assert(i < numViews());

    //std::cerr << "finishCurrentView: " << i << ", time=" << timestep << std::endl;
    if (m_module->rank() == rootRank()) {
        if (auto rhr = m_rhrControl.server()) {
            assert(rhr);
            if (imgp && i < rhr->numViews()) {
                // otherwise client just disconnected
                const int bpp = 4;
                const int w = rhr->width(i);
                const int h = rhr->height(i);
                const IceTImage &img = *static_cast<const IceTImage *>(imgp);
                std::cerr << "w=" << w << ", h=" << h << ", img: w=" << icetImageGetWidth(img)
                          << ", h=" << icetImageGetHeight(img) << std::endl;

                const IceTUByte *color = nullptr;
                switch (icetImageGetColorFormat(img)) {
                case ICET_IMAGE_COLOR_RGBA_UBYTE:
                    color = icetImageGetColorcub(img);
                    break;
                case ICET_IMAGE_COLOR_RGBA_FLOAT:
                    std::cerr << "expected byte color, got float" << std::endl;
                    break;
                case ICET_IMAGE_COLOR_NONE:
                    std::cerr << "expected byte color, got no color" << std::endl;
                    break;
                }
                const IceTFloat *depth = nullptr;
                switch (icetImageGetDepthFormat(img)) {
                case ICET_IMAGE_DEPTH_FLOAT:
                    depth = icetImageGetDepthcf(img);
                    break;
                case ICET_IMAGE_DEPTH_NONE:
                    std::cerr << "expected float depth, got none" << std::endl;
                    break;
                }

                assert(std::max(0, w) == icetImageGetWidth(img));
                assert(std::max(0, h) == icetImageGetHeight(img));

                if (color && depth && rhr->rgba(i) && rhr->depth(i)) {
                    for (int y = 0; y < h; ++y) {
                        memcpy(rhr->rgba(i) + w * bpp * y, color + w * (h - 1 - y) * bpp, bpp * w);
                    }
                    for (int y = 0; y < h; ++y) {
                        memcpy(rhr->depth(i) + w * y, depth + w * (h - 1 - y), sizeof(float) * w);
                    }

                    m_viewData[i].rhrParam.timestep = timestep;
                    rhr->invalidate(i, 0, 0, rhr->width(i), rhr->height(i), m_viewData[i].rhrParam, lastView);
                }
            }
        }
    }
    m_currentView = -1;
    m_frameComplete = lastView;

    checkIceTError("after finishCurrentView");
}

bool ParallelRemoteRenderManager::finishFrame(int timestep)
{
    assert(m_currentView == -1);

    if (m_frameComplete)
        return false;

    if (m_delaySec > 0.) {
        usleep(int32_t(m_delaySec * 1e6));
    }

    if (m_module->rank() == rootRank()) {
        if (auto rhr = m_rhrControl.server()) {
            assert(rhr);
            m_viewData[0].rhrParam.timestep = timestep;
            rhr->invalidate(-1, 0, 0, 0, 0, m_viewData[0].rhrParam, true);
        }
    }
    m_frameComplete = true;
    return true;
}

vistle::Matrix4 ParallelRemoteRenderManager::getModelViewMat(size_t viewIdx) const
{
    const PerViewState &vd = m_viewData[viewIdx];
    Matrix4 mv = vd.view * vd.model;
    return mv;
}

vistle::Matrix4 ParallelRemoteRenderManager::getProjMat(size_t viewIdx) const
{
    const PerViewState &vd = m_viewData[viewIdx];
    return vd.proj;
}

const ParallelRemoteRenderManager::PerViewState &ParallelRemoteRenderManager::viewData(size_t viewIdx) const
{
    return m_viewData[viewIdx];
}

unsigned char *ParallelRemoteRenderManager::rgba(size_t viewIdx)
{
    return m_rgba[viewIdx].data();
}

float *ParallelRemoteRenderManager::depth(size_t viewIdx)
{
    return m_depth[viewIdx].data();
}

void ParallelRemoteRenderManager::updateRect(size_t viewIdx, const int *viewport)
{
    auto rhr = m_rhrControl.server();
    if (rhr && m_module->rank() != rootRank()) {
        // observe what slaves are rendering
        const int bpp = 4;
        const int w = rhr->width(m_currentView);
        const int h = rhr->height(m_currentView);

        const unsigned char *color = rgba(viewIdx);

        memset(rhr->rgba(m_currentView), 0, w * h * bpp);

        for (int y = viewport[1]; y < viewport[1] + viewport[3]; ++y) {
            memcpy(rhr->rgba(m_currentView) + (w * y + viewport[0]) * bpp,
                   color + (w * (h - 1 - y) + viewport[0]) * bpp, bpp * viewport[2]);
        }

        rhr->invalidate(m_currentView, 0, 0, rhr->width(m_currentView), rhr->height(m_currentView),
                        rhr->getViewParameters(m_currentView), true);
    }
}

void ParallelRemoteRenderManager::addObject(std::shared_ptr<RenderObject> ro)
{
    m_updateBounds = 1;
    if (!ro->variant.empty()) {
        m_updateVariants = 1;
    }
    auto rhr = m_rhrControl.server();
    if (rhr)
        rhr->updateModificationCount();
}

void ParallelRemoteRenderManager::removeObject(std::shared_ptr<RenderObject> ro)
{
    m_updateBounds = 1;
    if (!ro->variant.empty()) {
        m_updateVariants = 1;
    }
    auto rhr = m_rhrControl.server();
    if (rhr)
        rhr->updateModificationCount();
}

bool ParallelRemoteRenderManager::handleMessage(const message::Message *message, const MessagePayload &payload)
{
    auto rhr = m_rhrControl.server();
    if (rhr)
        return rhr->handleMessage(message, payload);

    return false;
}

} // namespace vistle
