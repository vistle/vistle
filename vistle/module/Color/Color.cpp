#include <sstream>
#include <iomanip>
#include <cfloat>
#include <limits>
#include <core/vector.h>
#include <core/object.h>
#include <core/vec.h>
#include <core/texture1d.h>

#include "Color.h"

#define lerp(a, b, t) ( a + t * (b - a) )

MODULE_MAIN(Color)

using namespace vistle;

ColorMap::ColorMap(std::map<vistle::Scalar, vistle::Vector> & pins,
                   const size_t w): width(w) {

   data = new unsigned char[width * 4];

   std::map<vistle::Scalar, vistle::Vector>::iterator current = pins.begin();
   std::map<vistle::Scalar, vistle::Vector>::iterator next = ++pins.begin();

   for (size_t index = 0; index < width; index ++) {

      if (((vistle::Scalar) index) / width > next->first) {
         if (next != pins.end()) {
            current ++;
            next ++;
         }
      }

      if (next == pins.end()) {
         data[index * 4] = current->second[0];
         data[index * 4 + 1] = current->second[1];
         data[index * 4 + 2] = current->second[2];
         data[index * 4 + 3] = 1.0;
      } else {

         vistle::Scalar a = current->first;
         vistle::Scalar b = next->first;

         vistle::Scalar t = ((index / (vistle::Scalar) width) - a) / (b - a);

         data[index * 4] =
            (lerp(current->second[0], next->second[0], t) * 255.0);
         data[index * 4 + 1] =
            (lerp(current->second[1], next->second[1], t) * 255.0);
         data[index * 4 + 2] =
            (lerp(current->second[2], next->second[2], t) * 255.0);
         data[index * 4 + 3] = 1.0;
      }
   }
}

ColorMap::~ColorMap() {

   delete[] data;
}

Color::Color(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Color", shmname, name, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");

   addFloatParameter("min", "minimum value of range to map", 0.0);
   addFloatParameter("max", "maximum value of range to map", 0.0);
}

Color::~Color() {

}

void Color::getMinMax(vistle::Object::const_ptr object,
                      vistle::Scalar & min, vistle::Scalar & max) {

   
   if (Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(object)) {
      const vistle::Scalar *x = &scal->x()[0];
      size_t numElements = scal->getSize();
#pragma omp parallel
      {
         Scalar tmin = std::numeric_limits<Scalar>::max();
         Scalar tmax = -std::numeric_limits<Scalar>::max();
#pragma omp for
         for (size_t index = 0; index < numElements; index ++) {
            if (x[index] < tmin)
               tmin = x[index];
            if (x[index] > tmax)
               tmax = x[index];
         }
#pragma omp critical
         {
            if (tmin < min)
               min = tmin;
            if (tmax > max)
               max = tmax;
         }
      }
   }

   if (Vec<Scalar,3>::const_ptr vec = Vec<Scalar,3>::as(object)) {
      const vistle::Scalar *x = vec->x().data();
      const vistle::Scalar *y = vec->y().data();
      const vistle::Scalar *z = vec->z().data();
      size_t numElements = vec->getSize();
#pragma omp parallel
      {
         Scalar tmin = std::numeric_limits<Scalar>::max();
         Scalar tmax = -std::numeric_limits<Scalar>::max();
#pragma omp for
         for (size_t index = 0; index < numElements; index ++) {
            Scalar v = Vector(x[index], y[index], z[index]).norm();
            if (v < tmin)
               tmin = v;
            if (v > tmax)
               tmax = v;
         }
#pragma omp critical
         {
            if (tmin < min)
               min = tmin;
            if (tmax > max)
               max = tmax;
         }
      }
   }
}

vistle::Object::ptr Color::addTexture(vistle::Object::const_ptr object,
      const vistle::Scalar min, const vistle::Scalar max,
      const ColorMap & cmap) {

   const Scalar range = max - min;

   if (Vec<Scalar>::const_ptr f = Vec<Scalar>::as(object)) {

      const size_t numElem = f->getSize();
      const vistle::Scalar *x = &f->x()[0];

      vistle::Texture1D::ptr tex(new vistle::Texture1D(cmap.width, min, max));
      tex->setMeta(object->meta());

      unsigned char *pix = &tex->pixels()[0];
#pragma omp parallel for
      for (size_t index = 0; index < cmap.width * 4; index ++)
         pix[index] = cmap.data[index];

      tex->coords().resize(numElem);
      auto tc = tex->coords().data();
#pragma omp parallel for
      for (size_t index = 0; index < numElem; index ++)
         tc[index] = (x[index] - min) / range;

      return tex;
   }

   if (Vec<Scalar,3>::const_ptr f = Vec<Scalar,3>::as(object)) {

      const size_t numElem = f->getSize();
      const vistle::Scalar *x = f->x().data();
      const vistle::Scalar *y = f->y().data();
      const vistle::Scalar *z = f->z().data();

      vistle::Texture1D::ptr tex(new vistle::Texture1D(cmap.width, min, max));
      tex->setMeta(object->meta());

      unsigned char *pix = &tex->pixels()[0];
#pragma omp parallel for
      for (size_t index = 0; index < cmap.width * 4; index ++)
         pix[index] = cmap.data[index];

      tex->coords().resize(numElem);
      auto tc = tex->coords().data();
#pragma omp parallel for
      for (size_t index = 0; index < numElem; index ++) {
         Scalar v = Vector(x[index], y[index], z[index]).norm();
         tc[index] = (v - min) / range;
      }

      return tex;
   }

   std::cerr << "Color: cannot handle input" << std::endl;

   return vistle::Object::ptr(new Empty(Object::Initialized));
}

bool Color::compute() {

   std::map<vistle::Scalar, vistle::Vector> pins;
   pins.insert(std::make_pair(0.0, vistle::Vector(0.0, 0.0, 1.0)));
   pins.insert(std::make_pair(0.5, vistle::Vector(1.0, 0.0, 0.0)));
   pins.insert(std::make_pair(1.0, vistle::Vector(1.0, 1.0, 0.0)));

   ColorMap cmap(pins, 32);

   while (hasObject("data_in")) {

      Object::const_ptr obj = takeFirstObject("data_in");
      Scalar min = std::numeric_limits<Scalar>::max();
      Scalar max = -std::numeric_limits<Scalar>::max();

      if (getFloatParameter("min") == getFloatParameter("max"))
         getMinMax(obj, min, max);
      else {
         min = getFloatParameter("min");
         max = getFloatParameter("max");
      }

      //std::cerr << "Color: [" << min << "--" << max << "]" << std::endl;

      vistle::Object::ptr out(addTexture(obj, min, max, cmap));

         addObject("data_out", out);

   }

   return true;
}
