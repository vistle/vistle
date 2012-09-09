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

size_t write_uint64_int(const int fd, const uint64_t * data, const size_t num) {

   size_t r = 0;

   while (r < num * sizeof(uint64_t)) {
      size_t n = write(fd, ((char *) data) + r, num * sizeof(uint64_t) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(uint64_t))
      std::cout << "ERROR ReadCovise::write_uint64 wrote " << r
                << " bytes instead of " << num * sizeof(uint64_t) << std::endl;

   return r;
}

size_t write_uint64(const int fd, const uint64_t * data, const size_t num) {

    return write_uint64_int(fd, data, num);
}

size_t write_uint64(const int fd, const unsigned int * data, const size_t num) {

   uint64_t *d64  = new uint64_t[num];
   for (size_t index = 0; index < num; index ++)
      d64[index] = data[index];

   size_t r = write_uint64_int(fd, d64, num);

   delete[] d64;
   return r;
}

#ifdef __APPLE__
size_t write_uint64(const int fd, const unsigned long * data, const size_t num) {

   uint64_t *d64  = new uint64_t[num];
   for (size_t index = 0; index < num; index ++)
      d64[index] = data[index];

   size_t r = write_uint64_int(fd, d64, num);

   delete[] d64;
   return r;
}
#endif

size_t write_char(const int fd, char * data, const size_t num) {

   size_t r = 0;

   while (r < num) {
      size_t n = write(fd, ((char *) data) + r, num - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num)
      std::cout << "ERROR WriteVistle::write_char wrote " << r
                << " bytes instead of " << num << std::endl;

   return r;
}

size_t write_float(const int fd, float * data, const size_t num) {

   size_t r = 0;

   while (r < num * sizeof(float)) {
      size_t n = write(fd, ((char *) data) + r, num * sizeof(float) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(float))
      std::cout << "ERROR WriteVistle::write_float wrote " << r
                << " bytes instead of " << num * sizeof(float) << std::endl;

   return r;
}

WriteVistle::~WriteVistle() {

}

iteminfo * WriteVistle::createInfo(vistle::Object * object, size_t offset) {

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
         s->offset = offset;

         for (size_t index = 0; index < set->getNumElements(); index ++) {
            iteminfo * info = createInfo(set->getElement(index), offset);
            if (info) {
               infosize += info->infosize;
               itemsize += info->itemsize;
               offset += info->itemsize;
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
         info->itemsize = info->numElements * sizeof(uint64_t) +
            info->numCorners * sizeof(uint64_t) +
            info->numVertices * sizeof(float) * 3;
         info->offset = offset;
         return info;
      }

      case vistle::Object::VECFLOAT: {

         const vistle::Vec<float> *data =
            static_cast<const vistle::Vec<float> *>(object);
         datainfo *info = new datainfo;
         info->type = object->getType();
         info->numElements = data->getSize();
         info->block = data->getBlock();
         info->timestep = data->getTimestep();
         info->infosize = sizeof(datainfo);
         info->itemsize = info->numElements * sizeof(float);
         info->offset = offset;
         return info;
      }

      case vistle::Object::VEC3FLOAT: {

         const vistle::Vec3<float> *data =
            static_cast<const vistle::Vec3<float> *>(object);
         datainfo *info = new datainfo;
         info->type = object->getType();
         info->numElements = data->getSize();
         info->block = data->getBlock();
         info->timestep = data->getTimestep();
         info->infosize = sizeof(datainfo);
         info->itemsize = info->numElements * sizeof(float) * 3;
         info->offset = offset;
         return info;
      }

      case vistle::Object::UNSTRUCTUREDGRID: {

         const vistle::UnstructuredGrid *grid =
            static_cast<const vistle::UnstructuredGrid *>(object);
         usginfo *info = new usginfo;
         info->type = object->getType();
         info->numElements = grid->getNumElements();
         info->numCorners = grid->getNumCorners();
         info->numVertices = grid->getNumVertices();
         info->block = grid->getBlock();
         info->timestep = grid->getTimestep();
         info->infosize = sizeof(usginfo);
         info->itemsize =
            info->numElements * sizeof(char) +     // tl
            info->numElements * sizeof(uint64_t) + // el
            info->numCorners * sizeof(uint64_t) +  // cl
            info->numVertices * 3 * sizeof(float); // x, y, z
         info->offset = offset;
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
   uint64_t offset = 0;

   switch (object->getType()) {

      case vistle::Object::SET: {

         infosize += (sizeof(iteminfo) + sizeof(int32_t)); // numitems
         const vistle::Set *set = static_cast<const vistle::Set *>(object);
         setinfo *s = new setinfo;
         s->offset = offset;
         s->type = vistle::Object::SET;
         s->block = set->getBlock();
         s->timestep = set->getTimestep();

         for (size_t index = 0; index < set->getNumElements(); index ++) {
            iteminfo * info = createInfo(set->getElement(index), offset);
            if (info) {
               infosize += info->infosize;
               itemsize += info->itemsize;
               s->items.push_back(info);
               offset += info->itemsize;
            }
         }
         s->infosize = infosize;
         s->itemsize = itemsize;
         c.item = s;
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
   if (c.item)
      printItemInfo(c.item);
}

void WriteVistle::saveObject(const int fd, const vistle::Object * object) {

   switch(object->getType()) {

      case vistle::Object::SET: {

         const vistle::Set *set = static_cast<const vistle::Set *>(object);
         for (size_t index = 0; index < set->getNumElements(); index ++)
            saveObject(fd, set->getElement(index));
         break;
      }

      case vistle::Object::POLYGONS: {

         const vistle::Polygons *polygons =
            static_cast<const vistle::Polygons *>(object);

         std::cout << " write polygons "
                << polygons->getBlock() << " " << polygons->getTimestep() << " "
                << polygons->getNumElements() << " " << polygons->getNumCorners() << " "
                << polygons->getNumVertices() << std::endl;

         write_uint64(fd, &((*polygons->el)[0]), polygons->getNumElements());
         write_uint64(fd, &((*polygons->cl)[0]), polygons->getNumCorners());

         write_float(fd, &((*polygons->x)[0]), polygons->getNumVertices());
         write_float(fd, &((*polygons->y)[0]), polygons->getNumVertices());
         write_float(fd, &((*polygons->z)[0]), polygons->getNumVertices());
         break;
      }

      case vistle::Object::UNSTRUCTUREDGRID: {

         const vistle::UnstructuredGrid *usg =
            static_cast<const vistle::UnstructuredGrid *>(object);

         std::cout << " write usg "
                   << usg->getBlock() << " " << usg->getTimestep() << " "
                   << usg->getNumElements() << " " << usg->getNumCorners() << " "
                   << usg->getNumVertices() << std::endl;

         write_char(fd, &((*usg->tl)[0]), usg->getNumElements());
         write_uint64(fd, &((*usg->el)[0]), usg->getNumElements());
         write_uint64(fd, &((*usg->cl)[0]), usg->getNumCorners());

         write_float(fd, &((*usg->x)[0]), usg->getNumVertices());
         write_float(fd, &((*usg->y)[0]), usg->getNumVertices());
         write_float(fd, &((*usg->z)[0]), usg->getNumVertices());
         break;
      }

      case vistle::Object::VECFLOAT: {

         const vistle::Vec<float> *data =
            static_cast<const vistle::Vec<float> *>(object);

         std::cout << " write float vec "
                   << data->getBlock() << " " << data->getTimestep() << " "
                   << data->getSize() << std::endl;

         write_float(fd, &((*data->x)[0]), data->getSize());
         break;
      }

      case vistle::Object::VEC3FLOAT: {

         const vistle::Vec3<float> *data =
            static_cast<const vistle::Vec3<float> *>(object);

         std::cout << " write float vec "
                   << data->getBlock() << " " << data->getTimestep() << " "
                   << data->getSize() << std::endl;

         write_float(fd, &((*data->x)[0]), data->getSize());
         write_float(fd, &((*data->y)[0]), data->getSize());
         write_float(fd, &((*data->z)[0]), data->getSize());
         break;
      }

      default:
         printf("WriteVistle::saveObject unsupported object type %d\n",
                object->getType());
         break;
   }
}

void WriteVistle::saveItemInfo(const int fd, const iteminfo * info) {

   write_uint64(fd, &info->infosize, 1);
   write_uint64(fd, &info->itemsize, 1);
   write_uint64(fd, &info->offset, 1);
   write_uint64(fd, &info->type, 1);
   write_uint64(fd, &info->block, 1);
   write_uint64(fd, &info->timestep, 1);

   const setinfo *set = dynamic_cast<const setinfo *>(info);
   const polygoninfo *polygons = dynamic_cast<const polygoninfo *>(info);
   const usginfo *usg = dynamic_cast<const usginfo *>(info);
   const datainfo *data = dynamic_cast<const datainfo *>(info);

   if (set) {
      uint64_t numItems = set->items.size();
      write_uint64(fd, &numItems, 1);
      for (size_t index = 0; index < set->items.size(); index ++)
         saveItemInfo(fd, set->items[index]);
   }
   else if (polygons) {
      write_uint64(fd, &polygons->numElements, 1);
      write_uint64(fd, &polygons->numCorners, 1);
      write_uint64(fd, &polygons->numVertices, 1);
   }
   else if (usg) {
      write_uint64(fd, &usg->numElements, 1);
      write_uint64(fd, &usg->numCorners, 1);
      write_uint64(fd, &usg->numVertices, 1);
   }
   else if (data) {
      write_uint64(fd, &usg->numElements, 1);
   }
}

void WriteVistle::saveCatalogue(const int fd, const catalogue & c) {

   uint64_t numItems = c.item?1:0;
   write_uint64(fd, &c.infosize, 1);
   write_uint64(fd, &c.itemsize, 1);
   write_uint64(fd, &numItems, 1);

   if (c.item)
      saveItemInfo(fd, c.item);
}

void WriteVistle::save(const std::string & name, vistle::Object * object) {

   int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
   if (fd == -1) {
      std::cout << "ERROR WriteVistle::save could not open file [" << name
                << "]" << std::endl;
      return;
   }

   catalogue c;
   createCatalogue(object, c);
   printCatalogue(c);

   write_char(fd, (char *) "VISTLE", 6);
   header h('l', 1, 0, 0);
   write_char(fd, (char *) &h, sizeof(header));

   saveCatalogue(fd, c);
   saveObject(fd, object);

   close(fd);
   std::cout << "saved [" << name << "]" << std::endl;
}

bool WriteVistle::compute() {

   std::list<vistle::Object *> objects = getObjects("grid_in");

   if (objects.size() == 1) {
      save(getFileParameter("filename"), objects.front());
   }

   return true;
}
