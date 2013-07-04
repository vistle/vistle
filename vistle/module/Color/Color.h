#ifndef COLOR_H
#define COLOR_H

#include <module/module.h>
#include <core/vector.h>

class ColorMap {

public:
   ColorMap(std::map<vistle::Scalar, vistle::Vector> & pins, const size_t width);
   ~ColorMap();

   unsigned char *data;
   const size_t width;
};

class Color: public vistle::Module {

 public:
   Color(const std::string &shmname, int rank, int size, int moduleID);
   ~Color();

 private:
   vistle::Object::ptr addTexture(vistle::Object::const_ptr object,
                               const vistle::Scalar min, const vistle::Scalar max,
                               const ColorMap & cmap);

   void getMinMax(vistle::Object::const_ptr object, vistle::Scalar & min, vistle::Scalar & max);

   virtual bool compute();
};

#endif
