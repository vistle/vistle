/**\file
 * \brief RhrClient plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

#include <config/CoviseConfig.h>

#include <net/tokenbuffer.h>

#include <cover/coVRConfig.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRMSController.h>
#include <cover/coVRPluginSupport.h>
#include <cover/VRViewer.h>
#include <cover/coVRLighting.h>
#include <cover/coVRAnimationManager.h>
#include <cover/RenderObject.h>
#include <cover/coVRStatsDisplay.h>

#include <cover/ui/Button.h>
#include <cover/ui/Menu.h>
#include <cover/ui/SelectionList.h>
#include <cover/ui/Menu.h>

#include <OpenVRUI/osg/mathUtils.h>

#include <PluginUtil/PluginMessageTypes.h>

#include <osg/io_utils>

#include <memory>

#include <core/tcpmessage.h>
#include <util/hostname.h>
#include <rhr/rfbext.h>

#include <VistlePluginUtil/VistleRenderObject.h>
#include <VistlePluginUtil/VistleInteractor.h>

#include "RemoteConnection.h"
#include "RhrClient.h"

//#define CONNDEBUG

#define CERR std::cerr << "RhrClient: "

using namespace opencover;
using namespace vistle;
using message::RemoteRenderMessage;

typedef std::lock_guard<std::recursive_mutex> lock_guard;

static const unsigned short PortRange[] = { 31000, 32000 };
static const std::string config("COVER.Plugin.RhrClient");

void RhrClient::fillMatricesMessage(matricesMsg &msg, int channel, int viewNum, bool second) {

   msg.type = rfbMatrices;
   msg.viewNum = viewNum;
   if (channel >= 0)
       msg.viewNum += m_channelBase;
   msg.time = cover->frameTime();
   const osg::Matrix &t = cover->getXformMat();
   const osg::Matrix &s = cover->getObjectsScale()->getMatrix();
   osg::Matrix model = s * t;
   if (m_noModelUpdate)
       model = m_oldModelMatrix;
   else
       m_oldModelMatrix = model;

   osg::Matrix view, proj;
   if (channel >= 0) {
       const channelStruct &chan = coVRConfig::instance()->channels[channel];
       if (chan.PBONum >= 0) {
           const PBOStruct &pbo = coVRConfig::instance()->PBOs[chan.PBONum];
           msg.width = pbo.PBOsx;
           msg.height = pbo.PBOsy;
       } else {
           if (chan.viewportNum < 0)
               return;
           const viewportStruct &vp = coVRConfig::instance()->viewports[chan.viewportNum];
           if (vp.window < 0)
               return;
           const windowStruct &win = coVRConfig::instance()->windows[vp.window];
           msg.width = (vp.viewportXMax - vp.viewportXMin) * win.sx;
           msg.height = (vp.viewportYMax - vp.viewportYMin) * win.sy;
       }

       bool left = chan.stereoMode == osg::DisplaySettings::LEFT_EYE;
       if (second)
           left = true;
       view = left ? chan.leftView : chan.rightView;
       proj = left ? chan.leftProj : chan.rightProj;
       msg.eye = left ? 1 : 2;

#if 0
       CERR << "retrieving matrices for channel: " << channel << ", view: " << viewNum << ", " << msg.width << "x" << msg.height << ", second: " << second << ", left: " << left << std::endl;
       std::cerr << "  view mat: " << view << std::endl;
       std::cerr << "  proj mat: " << proj << std::endl;
#endif

   } else {
       msg.width = 2048;
       msg.height = 2048;
       msg.eye = 0;

       view.makeIdentity();
       proj.makeIdentity();

       float dx = 3000;
       float dz = dx;

       osg::Vec3 xyz(0, 0.5*dx, 0);
       osg::Vec3 hpr(0,0,0);
       if (m_geoMode == RemoteConnection::Tiled) {
           xyz[1] = 0.0;
           switch (viewNum) {
           case 0: { // front-left
               msg.width = msg.height = 256;
               xyz[0] = -0.25*dx;
               break;
           }
           case 1: { // front-right
               xyz[0] = +0.25*dx;
               break;
           }
           default:
               return;
           }

       } else {
           switch (viewNum) {
           case 0: { // front
               break;
           }
           case 1: { // top
               hpr[1] = 90;
               hpr[2] = 180;
               break;
           }
           case 2: { // left
               hpr[0] = 90;
               break;
           }
           case 3: { // bottom
               hpr[1] = -90;
               break;
           }
           case 4: { // right
               hpr[0] = -90;
               break;
           }
           case 5: { // back
               hpr[0] = 180;
               break;
           }
           default:
               return;
           }
       }

       osg::Matrix euler;
       MAKE_EULER_MAT_VEC(euler, hpr);
       xyz = xyz * euler;
       euler.invert(euler);
       CERR << "viewNum=" << viewNum << ", xyz=" << xyz << std::endl;

       // transform the screen to fit the xz-plane
       osg::Matrix trans;
       trans.makeTranslate(-xyz);

       osg::Matrix mat;
       mat.mult(trans, euler);
       mat.mult(euler, mat);
       osg::Matrix offsetMat = mat;

       osg::Matrix viewMat = VRViewer::instance()->getViewerMat();
       osg::Vec3 middleEye(0, 0, 0);
       middleEye = viewMat.preMult(middleEye);
       middleEye = mat.preMult(middleEye);

       // compute right frustum
       // dist from eye to screen for left & right channel
       float mc_dist = -middleEye[1];

       // compute left frustum
       coVRConfig *coco = coVRConfig::instance();
       double n_over_d = coco->orthographic() ? 1.0 : coco->nearClip() / mc_dist;
       float mc_right = n_over_d * (dx / 2.0 - middleEye[0]);
       float mc_left = -n_over_d * (dx / 2.0 + middleEye[0]);
       float mc_top = n_over_d * (dz / 2.0 - middleEye[2]);
       float mc_bottom = -n_over_d * (dz / 2.0 + middleEye[2]);

       if (coco->orthographic())
       {
           proj.makeOrtho(mc_left, mc_right, mc_bottom, mc_top, coco->nearClip(), coco->farClip());
       }
       else
       {
           proj.makeFrustum(mc_left, mc_right, mc_bottom, mc_top, coco->nearClip(), coco->farClip());
       }

       // set view
       // take the normal to the plane as orientation this is (0,1,0)
       view = offsetMat * osg::Matrix::lookAt(osg::Vec3(middleEye[0], middleEye[1], middleEye[2]), osg::Vec3(middleEye[0], middleEye[1] + 1, middleEye[2]), osg::Vec3(0, 0, 1));
   }

   for (int i=0; i<16; ++i) {
       //TODO: swap to big-endian
       msg.model[i] = model.ptr()[i];
       msg.view[i] = view.ptr()[i];
       msg.proj[i] = proj.ptr()[i];
   }

   //CERR << "M: " << model.getTrans() << ", V: " << view.getTrans() << ", P: " << proj.getTrans() << std::endl;
}

std::vector<matricesMsg> RhrClient::gatherAllMatrices() {

    std::vector<matricesMsg> matrices;
    if (m_geoMode == RemoteConnection::Screen) {
        matrices.resize(m_numLocalViews);
        const int numChannels = coVRConfig::instance()->numChannels();
        int view=0;
        for (int i=0; i<numChannels; ++i) {

            fillMatricesMessage(matrices[view], i, view, false);
            ++view;
            if (coVRConfig::instance()->channels[i].stereoMode==osg::DisplaySettings::QUAD_BUFFER) {
                fillMatricesMessage(matrices[view], i, view, true);
                ++view;
            }
        }
        //CERR << "updated matrices for " << view << " views" << std::endl;
        assert(view == m_numLocalViews);

        if (coVRMSController::instance()->isMaster()) {
            coVRMSController::SlaveData sNumScreens(sizeof(m_numLocalViews));
            coVRMSController::instance()->readSlaves(&sNumScreens);
            for (int i=0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
                int n = *static_cast<int *>(sNumScreens.data[i]);
                for (int s=0; s<n; ++s) {
                    matricesMsg msg;
                    coVRMSController::instance()->readSlave(i, &msg, sizeof(msg));
                    matrices.push_back(msg);
                }
            }
        } else {
            coVRMSController::instance()->sendMaster(&m_numLocalViews, sizeof(m_numLocalViews));
            for (int s=0; s<m_numLocalViews; ++s) {
                coVRMSController::instance()->sendMaster(&matrices[s], sizeof(matrices[s]));
            }
        }
    } else {
        int nv = RemoteConnection::numViewsForMode(m_geoMode);
        for (int v=0; v<nv; ++v) {
            matrices.emplace_back();
            fillMatricesMessage(matrices[v], -1 /* cubemap */, v, false);
        }
    }

    return matrices;
}

bool RhrClient::swapFrames() {

    bool swapped = false;
    for (auto &r: m_remotes) {
        auto ct = r.second->currentTimestep();
        if (ct == m_visibleTimestep || ct == -1) {
            if (r.second->checkSwapFrame()) {
                r.second->swapFrame();
                swapped = true;
            } else {
                //CERR << "swap reject: CHECK failed" << std::endl;
            }
        } else {
            //CERR << "swap reject: visible=" << m_visibleTimestep << ", current=" << ct << std::endl;
        }
    }

    return swapped;
}

bool RhrClient::checkAdvanceFrame()
{
    if (m_requestedTimestep < 0)
        return false;

    bool readyForAdvance = true;

    //CERR << "checkAdvanceFrame: req=" << m_requestedTimestep << std::endl;
    int commonTimestep = -1;
    for (auto &r: m_remotes) {
        int t = r.second->currentTimestep();
        if (t == -1) {
            continue;
        }
        if (!r.second->checkSwapFrame()) {
            //CERR << "checkAdvanceFrame: common t=" << commonTimestep << ", not ready" << std::endl;
            readyForAdvance = false;
            break;
        }
        if (commonTimestep == -1) {
            commonTimestep = t;
        } else if (commonTimestep != t) {
            //CERR << "checkAdvanceFrame: no common t=" << commonTimestep << " != " << t << std::endl;
            readyForAdvance = false;
            break;
        }
    }

    if (readyForAdvance && commonTimestep != m_requestedTimestep) {
        //CERR << "checkAdvanceFrame: common t=" << commonTimestep << " not equal to req=" << m_requestedTimestep << std::endl;
        readyForAdvance = false;
    }

    if (coVRMSController::instance()->isMaster()) {
        coVRMSController::SlaveData sd(sizeof(readyForAdvance));
        coVRMSController::instance()->readSlaves(&sd);
        for (int i=0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
            bool *p = static_cast<bool *>(sd.data[i]);
            if (!*p) {
                readyForAdvance = false;
                break;
            }
        }
    } else {
        coVRMSController::instance()->sendMaster(&readyForAdvance, sizeof(readyForAdvance));
    }
    readyForAdvance = coVRMSController::instance()->syncBool(readyForAdvance);

    return readyForAdvance;
}

//! send lighting parameters to server
lightsMsg RhrClient::buildLightsMessage() {

   //CERR << "sending lights" << std::endl;
   lightsMsg msg;
   msg.type = rfbLights;

   for (size_t i=0; i<lightsMsg::NumLights; ++i) {

      const osg::Light *light = NULL;
      const auto &lightList = coVRLighting::instance()->lightList;
#if 0
      if (i == 0 && coVRLighting::instance()->headlight) {
         light = coVRLighting::instance()->headlight->getLight();
      } else if (i == 1 && coVRLighting::instance()->spotlight) {
         light = coVRLighting::instance()->spotlight->getLight();
      } else if (i == 2 && coVRLighting::instance()->light1) {
         light = coVRLighting::instance()->light1->getLight();
      } else if (i == 3 && coVRLighting::instance()->light2) {
         light = coVRLighting::instance()->light2->getLight();
      }
#else
      if (i < lightList.size())
          light = lightList[i].source->getLight();
#endif

#define COPY_VEC(d, dst, src) \
      for (int k=0; k<d; ++k) { \
         (dst)[k] = (src)[k]; \
      }

      if (light) {

         msg.lights[i].enabled = lightList[i].on;
         if (i == 0) {
             // headlight is always enabled for menu sub-graph
             msg.lights[i].enabled = coVRLighting::instance()->headlightState;
         }
         COPY_VEC(4, msg.lights[i].position, light->getPosition());
         COPY_VEC(4, msg.lights[i].ambient, light->getAmbient());
         COPY_VEC(4, msg.lights[i].diffuse, light->getDiffuse());
         COPY_VEC(4, msg.lights[i].specular, light->getSpecular());
         COPY_VEC(3, msg.lights[i].spot_direction, light->getDirection());
         msg.lights[i].spot_exponent = light->getSpotExponent();
         msg.lights[i].spot_cutoff = light->getSpotCutoff();
         msg.lights[i].attenuation[0] = light->getConstantAttenuation();
         msg.lights[i].attenuation[1] = light->getLinearAttenuation();
         msg.lights[i].attenuation[2] = light->getQuadraticAttenuation();
      }
      else
      {
          msg.lights[i].enabled = false;
      }

#undef COPY_VEC
   }

   return msg;
}

//! called when plugin is loaded
RhrClient::RhrClient()
: ui::Owner("RhrClient", cover->ui)
, m_requestedTimestep(-1)
, m_visibleTimestep(-1)
, m_numRemoteTimesteps(-1)
{
   //fprintf(stderr, "new RhrClient plugin\n");
}

//! called after plug-in is loaded and scenegraph is initialized
bool RhrClient::init()
{
   m_mode = MultiChannelDrawer::ReprojectMesh;
   m_noModelUpdate = false;
   m_oldModelMatrix = osg::Matrix::identity();

#if 0
   std::cout << "COVER waiting for debugger: pid=" << get_process_handle() << std::endl;
   sleep(10);
   std::cout << "COVER continuing..." << std::endl;
#endif

   m_benchmark = covise::coCoviseConfig::isOn("benchmark", config, true);
   m_lastStat = cover->currentTime();
   m_localFrames = 0;

   m_matrixNum = 0;

   m_channelBase = 0;

   const int numChannels = coVRConfig::instance()->numChannels();
   int numViews = numChannels;
   for (int i=0; i<numChannels; ++i) {
       if (coVRConfig::instance()->channels[i].stereoMode == osg::DisplaySettings::QUAD_BUFFER) {
          ++numViews;
       }
   }
   m_numLocalViews = numViews;
   if (coVRMSController::instance()->isMaster()) {
      coVRMSController::SlaveData sd(sizeof(numViews));
      coVRMSController::instance()->readSlaves(&sd);
      int channelBase = numViews;
      for (int i=0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
         int *p = static_cast<int *>(sd.data[i]);
         int n = *p;
         m_numChannels.push_back(n);
         *p = channelBase;
         channelBase += n;
      }
      coVRMSController::instance()->sendSlaves(sd);
      m_numClusterViews = channelBase;
   } else {
      coVRMSController::instance()->sendMaster(&numViews, sizeof(numViews));
      coVRMSController::instance()->readMaster(&m_channelBase, sizeof(m_channelBase));
   }
   coVRMSController::instance()->syncData(&m_numClusterViews, sizeof(m_numClusterViews));

   CERR << "#channels: " << numChannels << ", channel base: " << m_channelBase << std::endl;
   CERR << "#views: " << numViews << ", #cluster views: " << m_numClusterViews << std::endl;

   m_menu = new ui::Menu("RHR", this);
   m_menu->setText("Hybrid rendering");

   auto reprojMode = new ui::SelectionList(m_menu, "ReprojectionMode");
   reprojMode->setText("Reprojection");
   reprojMode->append("Disable");
   reprojMode->append("Points");
   reprojMode->append("Points (adaptive)");
   reprojMode->append("Points (adaptive with neighbors)");
   reprojMode->append("Mesh");
   reprojMode->append("Mesh with holes");
   reprojMode->setCallback([this](int choice){
         if (choice >= MultiChannelDrawer::AsIs && choice <= MultiChannelDrawer::ReprojectMeshWithHoles) {
             m_mode = MultiChannelDrawer::Mode(choice);
         }
         for (auto &r: m_remotes)
             r.second->drawer()->setMode(m_mode);
   });

   auto geoMode = new ui::SelectionList(m_menu, "GeometryMode");
   geoMode->setText("Proxy geometry");
   geoMode->append("Screen");
   geoMode->append("Tiled: 2x1");
   geoMode->append("Cubemap");
   geoMode->append("Cubemap (no back)");
   geoMode->setCallback([this](int choice){
       if (choice >= RemoteConnection::Screen && choice <= RemoteConnection::CubeMapFront) {
             m_configuredGeoMode = GeometryMode(choice);
         }
         setGeometryMode(m_configuredGeoMode);
   });
   geoMode->select(m_configuredGeoMode);
   reprojMode->select(m_mode);

   auto allViews = new ui::Button(m_menu, "AllViews");
   allViews->setText("Render all views");
   allViews->setState(m_configuredAllViews);
   allViews->setCallback([this](bool state){
       m_configuredAllViews = state;
       setRenderAllViews(state);
   });
   setRenderAllViews(m_configuredAllViews);
   setGeometryMode(m_configuredGeoMode);
   for (auto &r: m_remotes) {
       r.second->drawer()->setMode(m_mode);
   }

   m_matrixUpdate = new ui::Button(m_menu, "MatrixUpdate");
   m_matrixUpdate->setText("Update model matrix");
   m_matrixUpdate->setState(!m_noModelUpdate);
   m_matrixUpdate->setCallback([this](bool state){
         m_noModelUpdate = !state;
   });

   return true;
}

void RhrClient::addObject(const opencover::RenderObject *baseObj, osg::Group *parent, const opencover::RenderObject *geometry, const opencover::RenderObject *normals, const opencover::RenderObject *colors, const opencover::RenderObject *texture)
{

    if (!baseObj)
        return;
    auto attr = baseObj->getAttribute("_rhr_config");
    if (!attr)
        return;
    //CERR << "connection config string=" << attr << std::endl;

    std::stringstream config(attr);
    std::string method, address, portString;
    config >> method >> address >> portString;
    unsigned short port = 0;
    int moduleId = 0;
    if (portString[0] == ':') {
        std::stringstream(portString.substr(1)) >> moduleId;
    } else {
        std::stringstream(portString) >> port;
    }
    CERR << "connection config: method=" << method << ", address=" << address << ", port=" << port << std::endl;

    if (method == "connect") {
        if (address.empty()) {
            cover->notify(Notify::Error) << "RhrClient: no connection attempt: invalid dest address: " << address << std::endl;
        } else if (port == 0) {
            cover->notify(Notify::Error) << "RhrClient: no connection attempt: invalid dest port: " << port << std::endl;
        } else {
            connectClient(baseObj->getName(), address, port);
        }
    } else if (method == "listen") {
        if (moduleId > 0) {
            if (auto remote = startListen(baseObj->getName(), PortRange[0], PortRange[1])) {
                remote->m_moduleId = moduleId;
            }
        } else if (port > 0) {
            startListen(baseObj->getName(), port);
        } else {
            cover->notify(Notify::Error) << "RhrClient: no attempt to start server: invalid port: " << port << std::endl;
        }
    }
}

void RhrClient::removeObject(const char *objName, bool replaceFlag) {
    if (!objName)
        return;
    const std::string name(objName);
    auto it = m_remotes.find(name);
    if (it == m_remotes.end())
        return;
    removeRemoteConnection(it);
}

void RhrClient::newInteractor(const opencover::RenderObject *container, coInteractor *it) {

    auto vit = dynamic_cast<VistleInteractor *>(it);
    if (!vit)
        return;

    int id = vit->getModuleInstance();
    auto iter = m_interactors.find(id);
    if (iter != m_interactors.end()) {
        iter->second->decRefCount();
        m_interactors.erase(iter);
    }

    m_interactors[id] = vit;
    vit->incRefCount();
}

//! this is called if the plugin is removed at runtime
RhrClient::~RhrClient()
{
   for (auto &r: m_remotes) {
       cover->getObjectsRoot()->removeChild(r.second->scene());
   }

   coVRAnimationManager::instance()->removeTimestepProvider(this);
   if (m_requestedTimestep != -1)
       commitTimestep(m_requestedTimestep);

   while (!m_remotes.empty()) {
       removeRemoteConnection(m_remotes.begin());
   }
}

void RhrClient::setServerParameters(int module, const std::string &host, unsigned short port) const {
    for (auto &r: m_remotes) {
        auto it = m_interactors.find(r.second->m_moduleId);
        if (it == m_interactors.end())
            continue;

        auto inter = it->second;
        inter->setScalarParam("_rhr_auto_remote_port", int(port));
        inter->setStringParam("_rhr_auto_remote_host", host.c_str());
    }
}

bool RhrClient::syncRemotes() {

   for (auto r: m_remotes) {
       r.second->update();
   }

   int nt = -1;
   for (auto &r: m_remotes) {
       nt = std::max(nt, r.second->numTimesteps());
   }

   if (nt != m_numRemoteTimesteps) {
       m_numRemoteTimesteps = nt;
       return true;
   }

   return false;
}

//! this is called before every frame, used for polling for RFB messages
bool RhrClient::update()
{
   m_io.poll();

   //CERR << "drawer: #views=" << m_drawer->numViews() << std::endl;

   int numConnected = 0;
   for (auto it = m_remotes.begin(); it != m_remotes.end(); ) {
       bool running = coVRMSController::instance()->syncBool(it->second->isRunning());
       //CERR << "check " << it->first << ", running=" << running << std::endl;
       if (!running) {
           //CERR << "RhrClient: removing because not running" << std::endl;
           it = removeRemoteConnection(it);
           continue;
       }
       //CERR << "check " << it->first << ", connected=" << it->second->isConnected() << std::endl;
       if (it->second->isConnected())
           ++numConnected;
       ++it;
   }
   //CERR << "RhrClient: #connected=" << numConnected << std::endl;
   coVRMSController::instance()->syncData(&numConnected, sizeof(numConnected));

   if (auto sd = VRViewer::instance()->statsDisplay)
   {
       sd->enableRhrStats(numConnected > 0);
   }

   bool needUpdate = coVRMSController::instance()->syncBool(m_clientsChanged);
   m_clientsChanged = false;
   if (numConnected == 0)
      return needUpdate;

   bool render = false;

   lightsMsg lm = buildLightsMessage();
   auto matrices = gatherAllMatrices();

   if (coVRMSController::instance()->isMaster()) {
       for (auto &r: m_remotes) {
           r.second->setLights(lm);
           r.second->setMatrices(matrices);
       }
   }

   if (syncRemotes()) {
       coVRAnimationManager::instance()->setNumTimesteps(m_numRemoteTimesteps, this);
   }

   if (swapFrames())
       render = true;
   if (checkAdvanceFrame()) {
       render = true;
       commitTimestep(m_requestedTimestep);
       m_requestedTimestep = -1;
   }

   return render;
}

void RhrClient::preFrame() {
   ++m_localFrames;
   double diff = cover->frameTime() - m_lastStat;
   if (m_benchmark && diff > 3.) {
       m_lastStat = cover->frameTime();
       for (auto &r: m_remotes)
           r.second->printStats();
   }

   if (cover->isHighQuality()) {
       setRenderAllViews(false);
       setGeometryMode(RemoteConnection::Screen);
       for (auto &r: m_remotes) {
           r.second->drawer()->setMode(MultiChannelDrawer::AsIs);
       }
   } else {
       setRenderAllViews(m_configuredAllViews);
       setGeometryMode(m_configuredGeoMode);
       for (auto &r: m_remotes) {
           r.second->drawer()->setMode(m_mode);
           r.second->drawer()->reproject();
       }
   }

   for (auto r: m_remotes) {
       r.second->preFrame();
   }
}

//! called when scene bounding sphere is required
void RhrClient::expandBoundingSphere(osg::BoundingSphere &bs) {

    std::set<std::shared_ptr<RemoteConnection>> remotes;
    for (auto &r: m_remotes) {
        auto &remote = r.second;
        bool dead = !remote->isRunning() || !remote->isConnected();
        bool add = !dead && remote->requestBounds();
        if (add) {
            remotes.emplace(remote);
            //CERR << "expandBoundingSphere: checking with " << remote->name() << std::endl;
        }
    }
    //CERR << "expandBoundingSphere: checking with " << remotes.size() << " remotes" << std::endl;

    auto start = cover->currentTime();

    while (!remotes.empty()) {
        for (auto it = remotes.begin(); it != remotes.end(); ) {
            auto &remote = *it;
            bool dead = !remote->isRunning() || !remote->isConnected();
            if (dead) {
                //CERR << "dead, ignoring remote " << remote->name() << std::endl;
                it = remotes.erase(it);
                continue;
            }
            bool updated = remote->boundsUpdated();
            if (updated) {
                bs.expandBy(remote->getBounds());
            }
            if (updated) {
                //CERR << "already updated, not querying remote again: " << remote->name() << std::endl;
                it = remotes.erase(it);
                continue;
            }
            usleep(1000);
            ++it;
        }

        auto dt = cover->currentTime() - start;
        if (dt > 1.) {
            //CERR << "re-using previous bounding sphere after waiting for " << dt << " seconds" << std::endl;
            for (auto &r: remotes)
                bs.expandBy(r->getBounds());
            break;
        }
    }
}

void RhrClient::setTimestep(int t) {

   //CERR << "setTimestep(" << t << ")" << std::endl;
    if (m_requestedTimestep == t) {
        m_requestedTimestep = -1;
    }
    if (m_requestedTimestep >= 0) {
        CERR << "setTimestep(" << t << "), but requested was " << m_requestedTimestep << std::endl;
        m_requestedTimestep = -1;
    }
   m_visibleTimestep = t;

   for (auto &r: m_remotes)
       r.second->setVisibleTimestep(m_visibleTimestep);

   swapFrames();
}

void RhrClient::requestTimestep(int t) {

    syncRemotes();

    //CERR << "m_requestedTimestep: " << m_requestedTimestep << " -> " << t << std::endl;
    if (t < 0) {
        commitTimestep(t);
        return;
    }

    m_requestedTimestep = t;
    if (checkAdvanceFrame()) {
        commitTimestep(m_requestedTimestep);
        m_requestedTimestep = -1;
        return;
    }

    for (auto &r: m_remotes) {
        r.second->requestTimestep(t);
    }
}

void RhrClient::message(int toWhom, int type, int len, const void *msg) {

    switch (type) {
    case PluginMessageTypes::VariantHide:
    case PluginMessageTypes::VariantShow: {
        covise::TokenBuffer tb((char *)msg, len);
        std::string variantName;
        tb >> variantName;
        bool visible = type==PluginMessageTypes::VariantShow;
        m_coverVariants[variantName] = visible;
        if (coVRMSController::instance()->isMaster()) {
            variantMsg msg;
            msg.visible = visible;
            strncpy(msg.name, variantName.c_str(), sizeof(msg.name)-1);
            msg.name[sizeof(msg.name)-1] = '\0';
            for (auto r: m_remotes) {
                if (r.second->isConnected())
                    r.second->send(msg);
            }
        }

        for (auto &r: m_remotes) {
            r.second->setVariantVisibility(variantName, visible);
        }
        break;
    }
    }
}

std::shared_ptr<RemoteConnection> RhrClient::connectClient(const std::string &connectionName, const std::string &address, unsigned short port) {

   m_remotes.erase(connectionName);

   cover->notify(Notify::Info) << "starting new RemoteConnection to " << address << ":" << port << std::endl;
   std::shared_ptr<RemoteConnection> r(new RemoteConnection(this, address, port, coVRMSController::instance()->isMaster()));

   addRemoteConnection(connectionName, r);

   return r;
}

std::shared_ptr<RemoteConnection> RhrClient::startListen(const std::string &connectionName, unsigned short port, unsigned short portLast) {

   m_remotes.erase(connectionName);

   std::shared_ptr<RemoteConnection> r;
   if (portLast > 0) {
       cover->notify(Notify::Info) << "new RemoteConnection " << connectionName << " in port range: " << port << "-" << portLast << std::endl;
       r.reset(new RemoteConnection(this, port, portLast, coVRMSController::instance()->isMaster()));
   } else {
       cover->notify(Notify::Info) << "listening for new RemoteConnection " << connectionName << " on port: " << port << std::endl;
       r.reset(new RemoteConnection(this, port, coVRMSController::instance()->isMaster()));
   }

   addRemoteConnection(connectionName, r);

   return r;
}

void RhrClient::addRemoteConnection(const std::string &name, std::shared_ptr<RemoteConnection> remote) {
   CERR << "addRemoteConnection: have " << m_remotes.size() << " connections, adding client " << name << std::endl;

   m_remotes[name] = remote;
   remote->setName(name);
   m_clientsChanged = true;

   remote->setNumLocalViews(m_numLocalViews);
   remote->setNumClusterViews(m_numClusterViews);
   remote->setNumChannels(m_numChannels);
   remote->setFirstView(m_channelBase);
   remote->setVisibleTimestep(m_visibleTimestep);
   if (m_requestedTimestep != -1) {
       remote->requestTimestep(m_requestedTimestep);
   }
   lightsMsg lm = buildLightsMessage();
   remote->setLights(lm);

   auto matrices = gatherAllMatrices();
   remote->setMatrices(matrices);

   for (auto &var: m_coverVariants) {
       remote->setVariantVisibility(var.first, var.second);
   }
   cover->getObjectsRoot()->addChild(remote->scene());

   bool running = coVRMSController::instance()->syncBool(remote->isRunning());
   if (!running) {
       if (remote->m_moduleId) {
           setServerParameters(remote->m_moduleId, "", 0);
       }
       //removeRemoteConnection(remote);
       //remote.reset();
   }

   if (remote->isListening()) {
       if (remote->m_setServerParameters) {
           remote->m_setServerParameters = false;
           setServerParameters(remote->m_moduleId, vistle::hostname(), remote->m_port);
       }
   }
}

//! clean up when connection to server is lost
RhrClient::RemotesMap::iterator RhrClient::removeRemoteConnection(RhrClient::RemotesMap::iterator it) {

   CERR << "removeRemoteConnection: have " << m_remotes.size() << " connections, removing client " << it->first << std::endl;

   auto remote = it->second;
   cover->getObjectsRoot()->removeChild(remote->scene());
   m_clientsChanged = true;

   return m_remotes.erase(it);
}

void RhrClient::setGeometryMode(GeometryMode mode) {

    for (auto &r: m_remotes)
        r.second->setGeometryMode(mode);

    if (m_geoMode == mode)
        return;

    m_geoMode = mode;
}

void RhrClient::setRenderAllViews(bool state) {

    for (auto &r: m_remotes)
        r.second->setRenderAllViews(state);

    m_renderAllViews = state;

    if (m_geoMode != RemoteConnection::Screen)
        return;
}

COVERPLUGIN(RhrClient)
