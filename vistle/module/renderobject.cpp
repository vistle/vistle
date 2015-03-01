#include <core/coords.h>
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
, colors(colors)
, normals(Normals::as(normals))
, texture(Texture1D::as(texture))
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
         float b = (c & 0xffL);
         c >>= 8;
         float g = (c & 0xffL);
         c >>= 8;
         float r = (c & 0xffL);
         c >>= 8;

         hasSolidColor = true;
         solidColor = Vector4(r, g, b, 255.f);
      }
   }

   if (auto coords = Coords::as(geometry)) {

      for (Index i=0; i<coords->getNumCoords(); ++i) {
         for (int c=0; c<3; ++c) {
            if (coords->x(c)[i] < bMin[c])
               bMin[c] = coords->x(c)[i];
            if (coords->x(c)[i] > bMax[c])
               bMax[c] = coords->x(c)[i];
         }
      }
   }
}

RenderObject::~RenderObject() {
}

}
