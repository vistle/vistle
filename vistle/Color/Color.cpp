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
      switch (object->getType()) {

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

vistle::Object * Color::addTexture(vistle::Object * object,
                                   const float min, const float max) {

   if (object) {
      switch (object->getType()) {

         case vistle::Object::SET: {

            vistle::Set *set = static_cast<vistle::Set*>(object);
            vistle::Set *out = vistle::Set::create();

            for (size_t index = 0; index < set->getNumElements(); index ++)
               out->elements->push_back(addTexture(set->getElement(index),
                                                   min, max));
            return out;
            break;
         }

         case vistle::Object::VECFLOAT: {

            vistle::Vec<float> * f = static_cast<vistle::Vec<float> *>(object);
            const size_t numElem = f->getSize();
            float *x = &((*f->x)[0]);

            vistle::Texture1D *tex = vistle::Texture1D::create(256, min, max);

            unsigned char *pix = &(*tex->pixels)[0];
            for (int index = 0; index < 256; index ++) {
               pix[index * 4] = index;
               pix[index * 4 + 1] = 0;
               pix[index * 4 + 2] = 0;
               pix[index * 4 + 3] = 255;
            }

            for (int index = 0; index < numElem; index ++)
               tex->coords->push_back((x[index] - min) / (max - min));

            return tex;
            break;
         }

         default:
            break;
      }
   }
   return NULL;
}

bool Color::compute() {

   std::list<vistle::Object *> objects = getObjects("data_in");

   while (objects.size() > 0) {

      float min = FLT_MAX;
      float max = -FLT_MAX;

      getMinMax(objects.front(), min, max);

      vistle::Object *out = addTexture(objects.front(), min, max);
      if (out)
         addObject("data_out", out);

      removeObject("data_in", objects.front());

      objects = getObjects("data_in");
   }

   return true;
}
