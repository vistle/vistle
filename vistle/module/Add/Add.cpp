#include <sstream>
#include <iomanip>

#include <vec.h>

#include "Add.h"

MODULE_MAIN(Add)

using namespace vistle;

Add::Add(const std::string &shmname, int rank, int size, int moduleID)
   : Module("Add", shmname, rank, size, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");
}

Add::~Add() {

}

bool Add::compute() {

   ObjectList objects = getObjects("data_in");
   std::cout << "Add: " << objects.size() << " objects" << std::endl;

   ObjectList::iterator oit;
   for (oit = objects.begin(); oit != objects.end(); oit ++) {
      Object::const_ptr object(*oit);
      switch (object->getType()) {

         case Object::VECFLOAT: {
            Vec<Scalar>::const_ptr in = boost::static_pointer_cast<const Vec<Scalar> >(object);
            size_t size = in->getSize();

            Vec<Scalar>::ptr out(new Vec<Scalar>(size));

            for (unsigned int index = 0; index < size; index ++)
               out->x()[index] = in->x()[index] + rank + 1;

            out->copyAttributes(object);
            addObject("data_out", out);
            break;
         }

         case Object::VEC3INT: {

            Vec<int,3>::const_ptr in = boost::static_pointer_cast<const Vec<int,3> >(object);
            size_t size = in->getSize();

            Vec<int,3>::ptr out(new Vec<int,3>(size));

            for (unsigned int index = 0; index < size; index ++) {
               out->x()[index] = in->x()[index] + rank + 1;
               out->y()[index] = in->y()[index] + rank + 1;
               out->z()[index] = in->z()[index] + rank + 1;
            }
            out->copyAttributes(in);
            addObject("data_out", out);
         }
         default:
            break;
      }
      removeObject("data_in", object);
   }

   return true;
}
