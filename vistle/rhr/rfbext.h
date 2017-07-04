/**\file
 * \brief RFB protocol extensions for remote hybrid rendering (RHR)
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

/** \mainpage Remote Hybrid Rendering (RHR)
 *
 * Remote Hybrid Rendering (RHR) can composite 2.5D imagery rendered remotely
 * with locally rendered data. This is useful to
 * - avoid transfer of large dynamic data sets,
 * - decouple interaction from the slow rendering of large data.
 *
 * This implementation is based on the RFB protocol, the protocol used by VNC.
 * The generic parts of the RFB protocol are handled by LibVNCServer/LibVNCClient [http://libvncserver.sourceforge.net].
 * The data structures for the RFB protocol extensions are defined in <RHR/rfbext.h>.
 *
 * Its implementation consists of two parts, a server-side (remote) plug-in (VncServer) and a
 * client-side (local) plug-in (VncClient) for COVER, the VR renderer of the
 * visualization system COVISE [http://www.hlrs.de/organization/av/vis/covise/]
 * and Vistle [http://www.hlrs.de/organization/av/vis/vistle/].
 * For sort-last parallel rendering, VncServer can be combined with
 * CompositorIceT.
 *
 * Regular VNC clients are capable of interfacing with VncServer, but
 * VncClient requires a matching VncServer plug-in.
 * It is also compatible with Vistle's ray caster render module.
 *
 * Depth images can be optionally compressed with SNAPPY [http://code.google.com/p/snappy/]
 * or, on the GPU before being read back, with a lossy algorithm similiar to Direct3D texture compression (cf. DepthQuantize).
 *
 * This work was funded by the EU within the project CRESTA [http://cresta-project.eu].
 */


#ifndef PROTOCOL_EXTENSIONS_H
#define PROTOCOL_EXTENSIONS_H

#include <cstring> // memset
#include "depthquant.h"
#include "export.h"

#include <core/message.h>

namespace vistle {

const size_t RhrMessageSize = 840;

//! RFB protocol extension message types for remote hybrid rendering (RHR)
enum {
   rfbMatrices, //!< send matrices from client to server
   rfbLights, //!< send lighting parameters from client to server
   rfbTile, //!< send image tile from server to client
   rfbBounds, //!< send scene bounds from server to client
   rfbAnimation, //!< current/total animation time steps
   rfbVariant, //!< control visibility of variants
};

//! basic RFB message header
struct V_RHREXPORT rfbMsg
{
   rfbMsg(uint8_t type)
   : type(type)
   {}

   uint8_t type; //!< type of RFB message
};

//! send matrices from client to server
struct V_RHREXPORT matricesMsg: public rfbMsg {
   matricesMsg()
   : rfbMsg(rfbMatrices)
   , last(0)
   , viewNum(-1)
   , width(0)
   , height(0)
   , requestNumber(0)
   , time(0.)
   {
      memset(model, '\0', sizeof(model));
      memset(view, '\0', sizeof(view));
      memset(proj, '\0', sizeof(proj));
   }

   uint8_t last; //!< 1 if this is the last view requested for this frame
   int16_t viewNum; //!< number of window/view that these matrices apply to
   uint16_t width, height; //!< dimensions of requested viewport
   uint32_t requestNumber; //!< number of render request
   double time; //!< time of request - for latency measurement
   double model[16]; //!< model matrix
   double view[16]; //!< view matrix
   double proj[16]; //!< projection matrix
};
static_assert(sizeof(matricesMsg) < RhrMessageSize, "RHR message too large");

//! send lighting parameters from client to server
struct V_RHREXPORT lightsMsg: public rfbMsg {
   lightsMsg()
   : rfbMsg(rfbLights)
   , viewNum(-1)
   {
      memset(pad, '\0', sizeof(pad));

      lights[0].diffuse[0] = lights[0].diffuse[1] = lights[0].diffuse[2] = lights[0].diffuse[3] = 1;
      lights[0].specular[0] = lights[0].specular[1] = lights[0].specular[2] = lights[0].specular[3] = 1;
   }

   //! maximum number of supported lights
   static const int NumLights = 4;

   //! store parameters for one OpenGL light source
   struct Light {
      // as in OpenGL
      uint8_t enabled; //!< whether the light source is enabled

      uint8_t pad[7]; //!< ensure alignment

      double position[4]; //!< position

      double ambient[4]; //!< contribution to ambient lighting (RGBA)
      double diffuse[4]; //!< contribution to diffuse lighting (RGBA)
      double specular[4]; //!< contribution to specular highlights (RGBA)

      double spot_direction[3]; //!< direction of spot light
      double spot_cutoff; //!< cut-off coefficient for spot light
      double spot_exponent; //!< cut-off exponenent for spot light

      double attenuation[3]; //! attunuation: 0: constant, 1: linear, 2: quadratic

      Light()
      {
         memset(this, '\0', sizeof(*this));

         position[2] = 1;

         ambient[3] = 1;

         spot_direction[2] = -1;
         spot_cutoff = 180;

         attenuation[0] = 1;
      }

      bool operator!=(const Light &o) {

          if (enabled != o.enabled)
              return true;

          for (int i=0; i<4; ++i) {
              if (position[i] != o.position[i])
                  return true;
              if (ambient[i] != o.ambient[i])
                  return true;
              if (diffuse[i] != o.diffuse[i])
                  return true;
              if (specular[i] != o.specular[i])
                  return true;
          }

          for (int i=0; i<3; ++i) {
              if (spot_direction[i] != o.spot_direction[i])
                  return true;
              if (attenuation[i] != o.attenuation[i])
                  return true;
          }

          if (spot_cutoff != o.spot_cutoff)
              return true;
          if (spot_exponent != o.spot_exponent)
              return true;

          return false;
      }
   };

   int16_t viewNum; //!< number of window that these lights apply to
   uint8_t pad[5]; //!< ensure alignment

   //! all light sources
   Light lights[NumLights];

   bool operator!=(const lightsMsg &o) {
       if (viewNum != o.viewNum)
           return true;
       for (int i=0; i<NumLights; ++i) {
           if (lights[i] != o.lights[i])
               return true;
       }
       return false;
   }
};
static_assert(sizeof(lightsMsg) < RhrMessageSize, "RHR message too large");

//! send scene bounding sphere from server to client
struct V_RHREXPORT boundsMsg: public rfbMsg {
   boundsMsg()
   : rfbMsg(rfbBounds)
   , sendreply(0)
   , radius(-1.)
   {
      memset(pad, '\0', sizeof(pad));
      memset(center, '\0', sizeof(center));
   }

   uint8_t sendreply; //!< request reply with scene bounding sphere
   uint8_t pad[6]; //!< ensure alignment
   double center[3]; //!< center of bounding sphere
   double radius; //!< radius of bounding sphere
};
static_assert(sizeof(boundsMsg) < RhrMessageSize, "RHR message too large");

enum rfbTileFlags {
   rfbTileNone = 0,
   rfbTileFirst = 1,
   rfbTileLast = 2,
   rfbTileRequest = 4,
};

enum rfbTileFormats {
   rfbDepth8Bit,
   rfbDepth16Bit,
   rfbDepth24Bit,
   rfbDepth32Bit,
   rfbDepthFloat,
   rfbColorRGBA
};

enum rfbTileCompressions {
   rfbTileRaw = 0,
   rfbTileSnappy = 1,
   rfbTileDepthQuantize = 2,
   rfbTileJpeg = 4,
   rfbTileDepthZfp = 8,
   rfbTileClear = 16,
};

//! send image tile from server to client
struct V_RHREXPORT tileMsg: public rfbMsg {
   tileMsg()
   : rfbMsg(rfbTile)
   , flags(rfbTileNone)
   , format(rfbColorRGBA)
   , compression(rfbTileRaw)
   , frameNumber(0)
   , requestNumber(0)
   , size(0)
   , x(0)
   , y(0)
   , viewNum(-1)
   , width(0)
   , height(0)
   , totalwidth(0)
   , totalheight(0)
   , timestep(-1)
   , unzippedsize(0)
   , requestTime(0.)
   {
      memset(model, '\0', sizeof(model));
      memset(view, '\0', sizeof(view));
      memset(proj, '\0', sizeof(proj));
   }

   uint8_t flags; //!< request depth buffer update
   uint8_t format; //!< depth format, \see rfbDepthFormats
   uint8_t compression; //!< compression, \see rfbDepthCompressions
   uint32_t frameNumber; //!< number of frame this tile belongs to
   uint32_t requestNumber; //!< number of request this tile is in response to, copied from matrices request
   uint32_t size; //!< size of payload, \see appSubMessage
   uint16_t x; //!< x coordinate of depth sub-image
   uint16_t y; //!< y coordinate of depth sub-image
   int16_t viewNum; //!< viewNum this tile belongs to
   uint16_t width; //!< width of depth sub-image
   uint16_t height; //!< height of depth sub-image
   uint16_t totalwidth; //!< total width of image
   uint16_t totalheight; //!< total height of image
   int32_t timestep; //! number of rendered timestep 
   int32_t unzippedsize; //! payload size before snappy compression
   double view[16]; //!< view matrix from request
   double proj[16]; //!< projection matrix from request
   double model[16]; //!< model matrix from request
   double requestTime; //!< time copied from matrices request
};
static_assert(sizeof(tileMsg) < RhrMessageSize, "RHR message too large");

//! animation time step on client or no. of animation steps on server changed
struct V_RHREXPORT animationMsg: public rfbMsg {
   animationMsg()
   : rfbMsg(rfbAnimation)
   , total(0)
   , current(0)
   {}

   uint32_t total; //!< total number of animation timesteps
   uint32_t current; //!< timestep currently displayed
};
static_assert(sizeof(animationMsg) < RhrMessageSize, "RHR message too large");

struct V_RHREXPORT variantMsg: public rfbMsg {
    variantMsg()
    : rfbMsg(rfbVariant)
    , configureVisibility(0)
    , visible(0)
    , remove(0)
    {
        memset(name, '\0', sizeof(name));
    }

    uint32_t configureVisibility;
    uint32_t visible;
    uint32_t remove;
    char name[200];
};
static_assert(sizeof(variantMsg) < RhrMessageSize, "RHR message too large");

//! header for remote hybrid rendering message
typedef rfbMsg RhrSubMessage;

namespace message {

class V_RHREXPORT RemoteRenderMessage: public MessageBase<RemoteRenderMessage, REMOTERENDERING> {
public:
    RemoteRenderMessage(const RhrSubMessage &rhr, size_t payloadSize=0);
    const RhrSubMessage &rhr() const;
private:
    std::array<char, RhrMessageSize> m_rhr;
};
static_assert(sizeof(RemoteRenderMessage) <= Message::MESSAGE_SIZE, "message too large");

} // namespace message

} // namespace vistle
#endif
