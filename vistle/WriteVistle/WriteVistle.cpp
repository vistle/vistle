#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <inttypes.h>

#include <sstream>
#include <iomanip>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "object.h"

#include "WriteVistle.h"

MODULE_MAIN(WriteVistle)

WriteVistle::WriteVistle(int rank, int size, int moduleID)
   : Module("WriteVistle", rank, size, moduleID) {

   createInputPort("grid_in");
   addFileParameter("filename", "");
}

void swap_int(unsigned int * data, const unsigned int num) {

   for (unsigned int i = 0; i < num; i++) {
      *data = (((*data) & 0xff000000) >> 24)
         | (((*data) & 0x00ff0000) >>  8)
         | (((*data) & 0x0000ff00) <<  8)
         | (((*data) & 0x000000ff) << 24);
      data++;
   }
}

void swap_float(float * d, const unsigned int num) {

   unsigned int *data = (unsigned int *) d;
   for (unsigned int i = 0; i < num; i++) {
      *data = (((*data) & 0xff000000) >> 24)
         | (((*data) & 0x00ff0000) >>  8)
         | (((*data) & 0x0000ff00) <<  8)
         | (((*data) & 0x000000ff) << 24);
      data++;
   }
}

size_t read_type(const int fd, char * data) {

   size_t r = 0, num = 6;
   while (r != num) {
      size_t n = read(fd, data + r, sizeof(char) * (num - r));
      if (n <= 0)
         break;
      r += n;
   }

   if (r != num) {
      std::cout << "ERROR ReadCovise::read_type read " << r
                << " elements instead of " << num << std::endl;
   }
   return r;
}


size_t read_int(const int fd, unsigned int * data, const size_t num,
                const bool byte_swap) {

   size_t r = 0;

   while (r < num * sizeof(int)) {
      size_t n = read(fd, ((char *) data) + r, num * sizeof(int) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(int))
      std::cout << "ERROR ReadCovise::read_int read " << r
                << " bytes instead of " << num * sizeof(int) << std::endl;

   if (byte_swap)
      swap_int(data, r / sizeof(int));

   return r;
}

size_t read_char(const int fd, char * data, const size_t num) {

   size_t r = 0;

   while (r < num) {
      size_t n = read(fd, ((char *) data) + r, num - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num)
      std::cout << "ERROR ReadCovise::read_int read " << r
                << " bytes instead of " << num << std::endl;

   return r;
}

size_t read_float(const int fd, float * data,
                  const size_t num, const bool byte_swap) {

   size_t r = 0;

   while (r < num * sizeof(float)) {
      size_t n = read(fd, ((char *) data) + r, num * sizeof(float) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(float))
      std::cout << "ERROR ReadCovise::read_float read " << r
                << " bytes instead of " << num * sizeof(float) << std::endl;

   if (byte_swap)
      swap_float(data, r / sizeof(float));

   return r;
}

WriteVistle::~WriteVistle() {

}

iteminfo * WriteVistle::createInfo(vistle::Object * object) {

   uint64_t infosize = 0;
   uint64_t itemsize = 0;

   switch (object->getType()) {

      case vistle::Object::SET: {

         infosize += (sizeof(iteminfo) + sizeof(int32_t)); // numitems
         const vistle::Set *set = static_cast<const vistle::Set *>(object);
         setinfo *s = new setinfo;
         s->type = vistle::Object::SET;
         s->block = set->getBlock();
         s->timestep = set->getTimestep();

         for (size_t index = 0; index < set->getNumElements(); index ++) {
            iteminfo * info = createInfo(set->getElement(index));
            if (info) {
               infosize += info->infosize;
               itemsize += info->itemsize;
               s->items.push_back(info);
            }
         }
         s->infosize = infosize;
         s->itemsize = itemsize;
         return s;
         break;
      }

      case vistle::Object::POLYGONS: {

         const vistle::Polygons *polygons =
            static_cast<const vistle::Polygons *>(object);
         polygoninfo *info = new polygoninfo;
         info->type = vistle::Object::POLYGONS;
         info->numElements = polygons->getNumElements();
         info->numCorners = polygons->getNumCorners();
         info->numVertices = polygons->getNumVertices();
         info->block = polygons->getBlock();
         info->timestep = polygons->getTimestep();
         info->infosize = sizeof(polygoninfo);
         info->itemsize = info->numElements * sizeof(size_t) +
            info->numCorners * sizeof(size_t) +
            info->numVertices * sizeof(float) * 3;
         return info;
      }

      default:
         break;
   }

   return NULL;
}

void WriteVistle::createCatalogue(const vistle::Object * object,
                                  catalogue & c) {

   uint64_t infosize = 0;
   uint64_t itemsize = 0;

   switch (object->getType()) {

      case vistle::Object::SET: {

         infosize += (sizeof(iteminfo) + sizeof(int32_t)); // numitems
         const vistle::Set *set = static_cast<const vistle::Set *>(object);
         setinfo *s = new setinfo;
         s->type = vistle::Object::SET;
         s->block = set->getBlock();
         s->timestep = set->getTimestep();

         for (size_t index = 0; index < set->getNumElements(); index ++) {
            iteminfo * info = createInfo(set->getElement(index));
            if (info) {
               infosize += info->infosize;
               itemsize += info->itemsize;
               s->items.push_back(info);
            }
         }
         s->infosize = infosize;
         s->itemsize = itemsize;
         c.items.push_back(s);
         break;
      }

      default:
         break;
   }
   c.infosize = infosize + 2 * sizeof(int64_t); // info + item size;
   c.itemsize = itemsize;
}

void printItemInfo(const iteminfo * info, const int depth = 0) {

   const setinfo * set = dynamic_cast<const setinfo *>(info);
   const polygoninfo * poly = dynamic_cast<const polygoninfo *>(info);

   if (set) {

      for (int s = 0; s < depth * 4; s ++)
         printf(" ");
      printf("set: info %" PRIu64 ", item %" PRIu64 "\n",
             set->infosize, set->itemsize);

      for (size_t index = 0; index < set->items.size(); index ++) {
         printItemInfo(set->items[index], depth + 1);
      }
   }
   else if (poly) {

      for (int s = 0; s < depth * 4; s ++)
         printf(" ");
      printf("poly %" PRIu64 " %" PRIu64 " %" PRIu64 ": info %" PRIu64 ", item %" PRIu64 "\n",
             poly->numElements, poly->numCorners, poly->numVertices,
             poly->infosize, poly->itemsize);
   }
}

void printCatalogue(const catalogue & c) {

   printf("catalogue info: %" PRIu64 ", item %" PRIu64 "\n",
          c.infosize, c.itemsize);
   for (size_t index = 0; index < c.items.size(); index ++)
      printItemInfo(c.items[index]);
}

void WriteVistle::save(const std::string & name, vistle::Object * object) {

   //bool byteswap = true;
   /*
   int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
   if (fd == -1) {
      std::cout << "ERROR WriteVistle::save could not open file [" << name
                << "]" << std::endl;
      return;
   }
   */
   catalogue c;
   createCatalogue(object, c);
   printCatalogue(c);
}

bool WriteVistle::compute() {

   std::list<vistle::Object *> objects = getObjects("grid_in");

   if (objects.size() == 1) {
      save(getFileParameter("filename"), objects.front());
   }

   return true;
}
