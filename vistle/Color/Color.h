#ifndef COLOR_H
#define COLOR_H

#include "module.h"
#include "vector.h"

class Color: public vistle::Module {

 public:
   Color(int rank, int size, int moduleID);
   ~Color();

 private:
   void getMinMax(const vistle::Object * object, float & min, float & max);

   virtual bool compute();
};

#endif
