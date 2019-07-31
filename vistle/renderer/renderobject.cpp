#include <boost/algorithm/string/predicate.hpp>
#include <core/coords.h>
#include <core/coordswradius.h>
#include <core/vec.h>
#include <core/assert.h>

#include "renderobject.h"

namespace vistle {

RenderObject::RenderObject(int senderId, const std::string &senderPort,
      Object::const_ptr container,
      Object::const_ptr geometry,
      Object::const_ptr normals,
      Object::const_ptr mapdata)
: senderId(senderId)
, senderPort(senderPort)
, container(container)
, geometry(geometry)
, normals(Normals::as(normals))
, texture(Texture1D::as(mapdata))
, scalars(texture ? nullptr : Vec<Scalar>::as(mapdata))
, timestep(-1)
, hasSolidColor(false)
, solidColor(0., 0., 0., 0.)
{
   std::string color;
   if (geometry && geometry->hasAttribute("_color")) {
       color = geometry->getAttribute("_color");
   } else if (container && container->hasAttribute("_color")) {
       color = container->getAttribute("_color");
   }

   if (!color.empty()) {
      std::stringstream str(color);
      char ch;
      str >> ch;
      if (ch == '#') {
         str << std::hex;
         unsigned long c = 0xffffffffL;
         str >> c;
         float a = 1.f;
         if (color.length() > 7) {
            a = (c & 0xffL)/255.f;
            c >>= 8;
         }
         float b = (c & 0xffL)/255.f;
         c >>= 8;
         float g = (c & 0xffL)/255.f;
         c >>= 8;
         float r = (c & 0xffL)/255.f;
         c >>= 8;

         hasSolidColor = true;
         solidColor = Vector4(r, g, b, a);
      }
   }
   if (geometry)
      timestep = geometry->getTimestep();
   if (timestep < 0 && normals) {
      timestep = normals->getTimestep();
   }
   if (timestep < 0 && texture) {
      timestep = texture->getTimestep();
   }
   if (timestep < 0 && scalars) {
      timestep = scalars->getTimestep();
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

void RenderObject::updateBounds() {

    if (bComputed)
        return;
    computeBounds();
}

void RenderObject::computeBounds() {

   const Scalar smax = std::numeric_limits<Scalar>::max();
   bMin = Vector(smax, smax, smax);
   bMax = Vector(-smax, -smax, -smax);

   Matrix4 T = geometry->getTransform();
   bool identity = T.isIdentity();
   if (auto coords = CoordsWithRadius::as(geometry)) {
      Vector3 rMin(smax, smax, smax), rMax(-smax, -smax, -smax);
      for (int i=0; i<8; ++i) {
          Vector3 v(0,0,0);
          if (i%2) {
              v += Vector(1,0,0);
          } else {
              v -= Vector(1,0,0);
          }
          if ((i/2)%2) {
              v += Vector(0,1,0);
          } else {
              v -= Vector(0,1,0);
          }
          if ((i/4)%2) {
              v += Vector(0,0,1);
          } else {
              v -= Vector(0,0,1);
          }
          if (!identity)
              v = transformPoint(T, v);
          for (int c=0; c<3; ++c) {
              rMin[c] = std::min(rMin[c], v[c]);
              rMax[c] = std::max(rMax[c], v[c]);
          }
      }
      const auto nc = coords->getNumCoords();
      for (Index i=0; i<nc; ++i) {
         Scalar r = coords->r()[i];
         Vector3 p(coords->x(0)[i], coords->x(1)[i], coords->x(2)[i]);
         if (!identity)
             p = transformPoint(T, p);
         for (int c=0; c<3; ++c) {
             bMin[c] = std::min(bMin[c], p[c]+r*rMin[c]);
             bMax[c] = std::max(bMax[c], p[c]+r*rMax[c]);
         }
      }
   } else if (auto coords = Coords::as(geometry)) {

      const auto nc = coords->getNumCoords();
      const Scalar *x=coords->x(0), *y=coords->x(1), *z=coords->x(2);
      for (Index i=0; i<nc; ++i) {
         Vector3 p(x[i], y[i], z[i]);
         if (!identity)
             p = transformPoint(T, p);
         for (int c=0; c<3; ++c) {
            if (p[c] < bMin[c])
               bMin[c] = p[c];
            if (p[c] > bMax[c])
               bMax[c] = p[c];
         }
      }
   }

   bValid = true;
   for (int c=0; c<3; ++c) {
       if (bMin[c] > bMax[c])
           bValid = false;
   }

   bComputed = true;
}

bool RenderObject::boundsValid() const {
    return bValid;
}

}
