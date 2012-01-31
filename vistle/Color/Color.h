#ifndef COLOR_H
#define COLOR_H

#include "module.h"
#include "vector.h"

class ColorMap {

public:
   ColorMap(std::map<float, vistle::util::Vector> & pins, const size_t width);
   ~ColorMap();

   unsigned char *data;
   const size_t width;
};

class Color: public vistle::Module {

 public:
   Color(int rank, int size, int moduleID);
   ~Color();

 private:
   vistle::Object * addTexture(vistle::Object * object,
                               const float min, const float max,
                               const ColorMap & cmap);

   void getMinMax(const vistle::Object * object, float & min, float & max);

   virtual bool compute();
};

#endif
