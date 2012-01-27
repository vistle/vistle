#include <sstream>
#include <iomanip>
#include <values.h>

#include "object.h"

#include "Color.h"

MODULE_MAIN(Color)


Color::Color(int rank, int size, int moduleID)
   : Module("Color", rank, size, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");
}

Color::~Color() {

}

void Color::getMinMax(const vistle::Object * object,
                      float & min, float & max) {

   if (object) {
      switch(object->getType()) {

         case vistle::Object::SET: {

            const vistle::Set *set = static_cast<const vistle::Set*>(object);
            for (size_t index = 0; index < set->getNumElements(); index ++)
               getMinMax(set->getElement(index), min, max);

            break;
         }

         case vistle::Object::VECFLOAT: {

            const vistle::Vec<float> *data =
               static_cast<const vistle::Vec<float> *>(object);

            const float *x = &((*data->x)[0]);
            int numElements = data->getSize();
            for (int index = 0; index < numElements; index ++) {
               if (x[index] < min)
                  min = x[index];
               if (x[index] > max)
                  max = x[index];
            }
            break;
         }

         default:
            break;
      }
   }
}


bool Color::compute() {

   std::list<vistle::Object *> objects = getObjects("data_in");

   while (objects.size() > 0) {

      float min = FLT_MAX;
      float max = -FLT_MAX;

      getMinMax(objects.front(), min, max);

      vistle::Texture1D *tex = vistle::Texture1D::create(256 * 4, min, max);
      printf("Min %f Max %f\n", min, max);

      unsigned char *data = &(*tex->pixels)[0];
      for (int index = 0; index < 256; index ++) {
         data[index * 4] = index;
         data[index * 4 + 1] = 0;
         data[index * 4 + 2] = 0;
         data[index * 4 + 3] = 255;
      }

      addObject("data_out", tex);

      removeObject("data_in", objects.front());

      objects = getObjects("data_in");
   }

   return true;
}
