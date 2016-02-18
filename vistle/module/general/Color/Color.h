#ifndef COLOR_H
#define COLOR_H

#include <module/module.h>
#include <core/vector.h>
#include <core/texture1d.h>

class ColorMap {

public:
   ColorMap(std::map<vistle::Scalar, vistle::Vector> & pins, const size_t width);
   ~ColorMap();

   unsigned char *data;
   const size_t width;
};

class Color: public vistle::Module {

 public:
   Color(const std::string &shmname, const std::string &name, int moduleID);
   ~Color();

 private:
   vistle::Texture1D::ptr addTexture(vistle::DataBase::const_ptr object,
                               const vistle::Scalar min, const vistle::Scalar max,
                               const ColorMap & cmap);

   void getMinMax(vistle::DataBase::const_ptr object, vistle::Scalar & min, vistle::Scalar & max);

   virtual bool compute();

   typedef std::map<vistle::Scalar, vistle::Vector> TF;
   std::map<int, TF> transferFunctions;
};

#endif
