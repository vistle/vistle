#include <boost/algorithm/string/predicate.hpp>
#include <core/coords.h>
#include <core/coordswradius.h>
#include <core/assert.h>

#include "renderobject.h"

namespace vistle {

RenderObject::RenderObject(int senderId, const std::string &senderPort,
      Object::const_ptr container,
      Object::const_ptr geometry,
      Object::const_ptr colors,
      Object::const_ptr normals,
      Object::const_ptr texture)
: senderId(senderId)
, senderPort(senderPort)
, container(container)
, geometry(geometry)
, normals(Normals::as(normals))
, colors(colors)
, texture(Texture1D::as(texture))
, timestep(-1)
, hasSolidColor(false)
, solidColor(0., 0., 0., 0.)
{
   const Scalar smax = std::numeric_limits<Scalar>::max();
   bMin = Vector(smax, smax, smax);
   bMax = Vector(-smax, -smax, -smax);

   if (geometry && geometry->hasAttribute("_color")) {

      std::stringstream str(geometry->getAttribute("_color"));
      char ch;
      str >> ch;
      if (ch == '#') {
         str << std::hex;
         unsigned long c = 0xffffffffL;
         str >> c;
         if (c > 0x00ffffffL) {
            c = 0x00ffffffL;
         }
         float b = (c & 0xffL)/255.f;
         c >>= 8;
         float g = (c & 0xffL)/255.f;
         c >>= 8;
         float r = (c & 0xffL)/255.f;
         c >>= 8;

         hasSolidColor = true;
         solidColor = Vector4(r, g, b, 1.f);
      }
   }

   if (auto coords = CoordsWithRadius::as(geometry)) {
      for (Index i=0; i<coords->getNumCoords(); ++i) {
         for (int c=0; c<3; ++c) {
            if (coords->x(c)[i]-coords->r()[i] < bMin[c])
               bMin[c] = coords->x(c)[i]-coords->r()[i];
            if (coords->x(c)[i]+coords->r()[i] > bMax[c])
               bMax[c] = coords->x(c)[i]+coords->r()[i];
         }
      }
   } else if (auto coords = Coords::as(geometry)) {

      for (Index i=0; i<coords->getNumCoords(); ++i) {
         for (int c=0; c<3; ++c) {
            if (coords->x(c)[i] < bMin[c])
               bMin[c] = coords->x(c)[i];
            if (coords->x(c)[i] > bMax[c])
               bMax[c] = coords->x(c)[i];
         }
      }
   }

   if (geometry)
      timestep = geometry->getTimestep();
   if (timestep < 0 && colors) {
      timestep = colors->getTimestep();
   }
   if (timestep < 0 && normals) {
      timestep = normals->getTimestep();
   }
   if (timestep < 0 && texture) {
      timestep = texture->getTimestep();
   }

   variant = container->getAttribute("_variant");
   if (variant.empty())
       variant = geometry->getAttribute("_variant");

   if (boost::algorithm::ends_with(variant, "_on")) {
       variant = variant.substr(0, variant.length()-3);
       visibility = Visible;
   } else if (boost::algorithm::ends_with(variant, "_off")) {
       variant = variant.substr(0, variant.length()-4);
       visibility = Hidden;
   }
}

RenderObject::~RenderObject() {
}

}
