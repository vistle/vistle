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

#include "ReadVistle.h"

MODULE_MAIN(ReadVistle)

ReadVistle::ReadVistle(int rank, int size, int moduleID)
   : Module("ReadVistle", rank, size, moduleID) {

   createOutputPort("grid_out");
   addFileParameter("filename", "");
}

size_t read_uint64(const int fd, const uint64_t * data, const size_t num) {

   size_t r = 0;

   while (r < num * sizeof(uint64_t)) {
      size_t n = read(fd, ((char *) data) + r, num * sizeof(uint64_t) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(uint64_t))
      std::cout << "ERROR ReadCovise::read_uint64 wrote " << r
                << " bytes instead of " << num * sizeof(uint64_t) << std::endl;

   return r;
}

size_t read_uint64(const int fd, unsigned int * data, const size_t num) {

   uint64_t *d64  = new uint64_t[num];

   size_t r = 0;

   while (r < num * sizeof(uint64_t)) {
      size_t n = read(fd, ((char *) d64) + r, num * sizeof(uint64_t) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(uint64_t))
      std::cout << "ERROR ReadCovise::read_uint64 wrote " << r
                << " bytes instead of " << num * sizeof(uint64_t) << std::endl;

   for (size_t index = 0; index < num; index ++)
      data[index] = (unsigned int) d64[index];

   delete[] d64;
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
      std::cout << "ERROR ReadVistle::read_char wrote " << r
                << " bytes instead of " << num << std::endl;

   return r;
}

size_t read_float(const int fd, float * data, const size_t num) {

   size_t r = 0;

   while (r < num * sizeof(float)) {
      size_t n = read(fd, ((char *) data) + r, num * sizeof(float) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(float))
      std::cout << "ERROR ReadVistle::read_float wrote " << r
                << " bytes instead of " << num * sizeof(float) << std::endl;

   return r;
}

ReadVistle::~ReadVistle() {

}


vistle::Object * ReadVistle::readObject(const int fd, const iteminfo * info,
                                        uint64_t start) {

   const setinfo *seti = dynamic_cast<const setinfo *>(info);
   const polygoninfo *polyi = dynamic_cast<const polygoninfo *>(info);

   if (seti) {

      vistle::Set *set = vistle::Set::create();
      for (size_t index = 0; index < seti->items.size(); index ++) {

         vistle::Object *object = readObject(fd, seti->items[index], start);
         set->elements->push_back(object);
      }
      return set;

   } else if (polyi) {

      if ((polyi->block % size) == rank) {
         lseek(fd, start + info->offset, SEEK_SET);
         vistle::Polygons *polygons =
            vistle::Polygons::create(polyi->numElements,
                                     polyi->numCorners,
                                     polyi->numVertices,
                                     polyi->block,
                                     polyi->timestep);

         read_uint64(fd, &((*polygons->el)[0]), polyi->numElements);
         read_uint64(fd, &((*polygons->cl)[0]), polyi->numCorners);

         read_float(fd, &((*polygons->x)[0]), polyi->numVertices);
         read_float(fd, &((*polygons->y)[0]), polyi->numVertices);
         read_float(fd, &((*polygons->z)[0]), polyi->numVertices);

         addObject("grid_out", polygons);
         return polygons;
      }
   }

   return NULL;
}

iteminfo * ReadVistle::readItemInfo(const int fd) {

   uint64_t infosize, itemsize, offset, type, block, timestep;
   read_uint64(fd, &infosize, 1);
   read_uint64(fd, &itemsize, 1);
   read_uint64(fd, &offset, 1);
   read_uint64(fd, &type, 1);
   read_uint64(fd, &block, 1);
   read_uint64(fd, &timestep, 1);

   switch (type) {

      case vistle::Object::SET: {

         setinfo *info = new setinfo;
         info->infosize = infosize;
         info->itemsize = itemsize;
         info->offset = offset;
         info->type = type;
         info->block = block;
         info->timestep = timestep;

         uint64_t numItems;
         read_uint64(fd, &numItems, 1);
         std::cout << "   set [" << numItems << "] items -> "
                   << info->offset << std::endl;
         for (size_t index = 0; index < numItems; index ++) {
            iteminfo * element = readItemInfo(fd);
            if (element)
               info->items.push_back(element);
         }
         return info;
      }

      case vistle::Object::POLYGONS: {

         polygoninfo *info = new polygoninfo;
         info->infosize = infosize;
         info->itemsize = itemsize;
         info->offset = offset;
         info->type = type;
         info->block = block;
         info->timestep = timestep;
         read_uint64(fd, &info->numElements, 1);
         read_uint64(fd, &info->numCorners, 1);
         read_uint64(fd, &info->numVertices, 1);
         std::cout << "   polygons [" << info->numElements << ", "
                   << info->numCorners << ", " << info->numVertices << "] -> "
                   << info->offset << std::endl;
         return info;
      }

      default: {
         // unknown entry
         lseek(fd, infosize - sizeof(iteminfo), SEEK_CUR);
         break;
      }
   }

   return NULL;
}

catalogue * ReadVistle::readCatalogue(const int fd) {

   catalogue *cat = new catalogue;
   uint64_t numItems;
   read_uint64(fd, &cat->infosize, 1);
   read_uint64(fd, &cat->itemsize, 1);
   read_uint64(fd, &numItems, 1);

   if (numItems == 1) {
      iteminfo *info = readItemInfo(fd);
      cat->item = info;
   }
   return cat;
}

vistle::Object * ReadVistle::load(const std::string & name) {

   int fd = open(name.c_str(), O_RDONLY);
   if (fd == -1) {
      std::cout << "ERROR ReadVistle::load could not open file [" << name
                << "]" << std::endl;
      return NULL;
   }

   header h;
   char magic[7];
   magic[6] = 0;
   read_char(fd, (char *) magic, 6);
   std::cout << "magic [" << magic << "]" << std::endl;
   read_char(fd, (char *) &h, sizeof(header));
   std::cout << "  endian " << h.endian << ", version "
             << (int) h.v_major << "." << (int) h.v_minor << "."
             << (int) h.v_patch << std::endl;

   catalogue *cat = readCatalogue(fd);

   uint64_t start = lseek(fd, 0, SEEK_CUR);

   vistle::Object *object = readObject(fd, cat->item, start);
   delete cat;

   return object;
}

bool ReadVistle::compute() {

   load(getFileParameter("filename"));
   return true;
}
