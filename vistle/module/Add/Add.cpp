#include <sstream>
#include <iomanip>

#include "object.h"

#include "Add.h"

MODULE_MAIN(Add)

Add::Add(int rank, int size, int moduleID)
   : Module("Add", rank, size, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");
}

Add::~Add() {

}

bool Add::compute() {

   std::list<vistle::Object *> objects = getObjects("data_in");
   std::cout << "Add: " << objects.size() << " objects" << std::endl;

   std::list<vistle::Object *>::iterator oit;
   for (oit = objects.begin(); oit != objects.end(); oit ++) {
      vistle::Object *object = *oit;
      switch (object->getType()) {

         case vistle::Object::VECFLOAT: {
            vistle::Vec<float> *in = static_cast<vistle::Vec<float> *>(object);
            size_t size = in->getSize();

            vistle::Vec<float> *out = vistle::Vec<float>::create(size);
            out->setSize(size);

            for (unsigned int index = 0; index < size; index ++)
               (*out->x)[index] = (*in->x)[index] + rank + 1;

            addObject("data_out", out);
            break;
         }

         case vistle::Object::VEC3INT: {

            vistle::Vec3<int> *in = static_cast<vistle::Vec3<int> *>(object);
            size_t size = in->getSize();

            vistle::Vec3<int> *out = vistle::Vec3<int>::create(size);
            out->setSize(size);

            for (unsigned int index = 0; index < size; index ++) {
               (*out->x)[index] = (*in->x)[index] + 1 + rank;
               (*out->y)[index] = (*in->y)[index] + 1 + rank;
               (*out->z)[index] = (*in->z)[index] + 1 + rank;
            }
            addObject("data_out", out);
         }
         default:
            break;
      }
      removeObject("data_in", object);
   }

   return true;
}
