#include <cstdio>
#include <cstdlib>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <sstream>
#include <iomanip>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <object.h>
#include <set.h>
#include <polygons.h>
#include <unstr.h>
#include <vec.h>
#include <vec3.h>

#include "ReadVistle.h"

using namespace vistle;

MODULE_MAIN(ReadVistle)

ReadVistle::ReadVistle(const std::string &shmname, int rank, int size, int moduleID)
   : Module("ReadVistle", shmname, rank, size, moduleID) {

   createOutputPort("grid_out");
   addFileParameter("filename", "");
}

template<typename T>
size_t tread(const int fd, const T* data, const size_t num) {

   size_t r = 0;

   while (r < num * sizeof(T)) {
      size_t n = ::read(fd, ((char *) data) + r, num * sizeof(T) - r);
      if (n <= 0)
         break;
      r += n;
   }

   if (r < num * sizeof(T))
      std::cout << "ERROR ReadCovise::tread<T> wrote " << r
                << " bytes instead of " << num * sizeof(T) << std::endl;

   return r;
}

size_t read_uint64(const int fd, const uint64_t * data, const size_t num) {
    
    return tread<uint64_t>(fd, data, num);
}

size_t read_uint64(const int fd, unsigned int * data, const size_t num) {

   std::vector<uint64_t> d64(num);

   size_t r = tread<uint64_t>(fd, &d64[0], num);

   for (size_t index = 0; index < num; index ++)
      data[index] = (unsigned int) d64[index];

   return r;
}

size_t read_uint64(const int fd, unsigned long * data, const size_t num) {

   std::vector<uint64_t> d64(num);

   size_t r = tread<uint64_t>(fd, &d64[0], num);

   for (size_t index = 0; index < num; index ++)
      data[index] = (unsigned long) d64[index];

   return r;
}

size_t read_char(const int fd, char * data, const size_t num) {

   return tread<char>(fd, data, num);
}

size_t read_float(const int fd, float * data, const size_t num) {

   return tread<float>(fd, data, num);
}

size_t read_float(const int fd, double *data, const size_t num) {

   std::vector<float> fdata(num);

   size_t r = read_float(fd, &fdata[0], num);

   for (size_t index = 0; index < num; index ++)
      data[index] = fdata[index];

   return r;
}

ReadVistle::~ReadVistle() {

}


vistle::Object::ptr ReadVistle::readObject(const int fd, const vistle::Object::Info * info,
                                        uint64_t start, const std::vector<std::string> &setHierarchy, int count) {

   const Set::Info *seti = dynamic_cast<const Set::Info *>(info);
   const Polygons::Info *polyi = dynamic_cast<const Polygons::Info *>(info);
   const UnstructuredGrid::Info *usgi = dynamic_cast<const UnstructuredGrid::Info *>(info);
   const Vec<vistle::Scalar>::Info *datai = dynamic_cast<const Vec<vistle::Scalar>::Info *>(info);

   if (seti) {
      std::vector<std::string> sets(setHierarchy);
      std::stringstream setname;
      if (!sets.empty())
         setname << sets.back() << "_";
      setname << count;
      sets.push_back(setname.str());

      vistle::Object::ptr object;
      for (size_t index = 0; index < seti->items.size(); index ++) {

         object = readObject(fd, seti->items[index], start, sets, index);
         object->setAttributeList("set", sets);
      }
      return object;

   } else if (polyi) {

      if ((polyi->block % size) == rank) {
         lseek(fd, start + info->offset, SEEK_SET);
         vistle::Polygons::ptr polygons(new vistle::Polygons(polyi->numElements,
                                     polyi->numCorners,
                                     polyi->numVertices,
                                     polyi->block,
                                     polyi->timestep));

         read_uint64(fd, &polygons->el()[0], polyi->numElements);
         read_uint64(fd, &polygons->cl()[0], polyi->numCorners);

         read_float(fd, &polygons->x()[0], polyi->numVertices);
         read_float(fd, &polygons->y()[0], polyi->numVertices);
         read_float(fd, &polygons->z()[0], polyi->numVertices);

         if (first_object) {
            first_object = false;
            polygons->addAttribute("mark_begin");
         }

         addObject("grid_out", polygons);
         return polygons;
      }
   } else if (usgi) {

      if ((usgi->block % size) == rank) {
         lseek(fd, start + info->offset, SEEK_SET);
         vistle::UnstructuredGrid::ptr usg(new vistle::UnstructuredGrid(usgi->numElements, usgi->numCorners,
                                             usgi->numVertices, usgi->block,
                                             usgi->timestep));

         read_char(fd, &usg->tl()[0], usgi->numElements);
         read_uint64(fd, &usg->el()[0], usgi->numElements);
         read_uint64(fd, &usg->cl()[0], usgi->numCorners);

         read_float(fd, &usg->x()[0], usgi->numVertices);
         read_float(fd, &usg->y()[0], usgi->numVertices);
         read_float(fd, &usg->z()[0], usgi->numVertices);

         if (first_object) {
            first_object = false;
            usg->addAttribute("mark_begin");
         }

         addObject("grid_out", usg);
         return usg;
      }
   } else if (datai) {

      if ((datai->block % size) == rank) {
         lseek(fd, start + info->offset, SEEK_SET);
         switch (datai->type) {

            case vistle::Object::VECFLOAT: {
               vistle::Vec<vistle::Scalar>::ptr data(new vistle::Vec<vistle::Scalar>(datai->numElements));

               read_float(fd, &data->x()[0], datai->numElements);
               if (first_object) {
                  first_object = false;
                  data->addAttribute("mark_begin");
               }
               addObject("grid_out", data);
               return data;
            }

            case vistle::Object::VEC3FLOAT: {
               vistle::Vec3<vistle::Scalar>::ptr data(new vistle::Vec3<vistle::Scalar>(datai->numElements));

               read_float(fd, &data->x()[0], datai->numElements);
               read_float(fd, &data->y()[0], datai->numElements);
               read_float(fd, &data->z()[0], datai->numElements);
               if (first_object) {
                  first_object = false;
                  data->addAttribute("mark_begin");
               }
               addObject("grid_out", data);
               return data;
            }
         }
      }
   }
   return vistle::Object::ptr();
}

vistle::Object::Info * ReadVistle::readItemInfo(const int fd) {

   uint64_t infosize, itemsize, offset, type, block, timestep;
   read_uint64(fd, &infosize, 1);
   read_uint64(fd, &itemsize, 1);
   read_uint64(fd, &offset, 1);
   read_uint64(fd, &type, 1);
   read_uint64(fd, &block, 1);
   read_uint64(fd, &timestep, 1);

   switch (type) {

      case vistle::Object::SET: {

         Set::Info *info = new Set::Info;
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
            vistle::Object::Info * element = readItemInfo(fd);
            if (element)
               info->items.push_back(element);
         }
         return info;
      }

      case vistle::Object::POLYGONS: {

         Polygons::Info *info = new Polygons::Info;
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

      case vistle::Object::UNSTRUCTUREDGRID: {

         UnstructuredGrid::Info *info = new UnstructuredGrid::Info;
         info->infosize = infosize;
         info->itemsize = itemsize;
         info->offset = offset;
         info->type = type;
         info->block = block;
         info->timestep = timestep;
         read_uint64(fd, &info->numElements, 1);
         read_uint64(fd, &info->numCorners, 1);
         read_uint64(fd, &info->numVertices, 1);
         std::cout << "   usg [" << info->numElements << ", "
                   << info->numCorners << ", " << info->numVertices << "] -> "
                   << info->offset << std::endl;
         return info;
      }

      case vistle::Object::VECFLOAT: {

         Vec<vistle::Scalar>::Info *info = new Vec<vistle::Scalar>::Info;
         info->infosize = infosize;
         info->itemsize = itemsize;
         info->offset = offset;
         info->type = type;
         info->block = block;
         info->timestep = timestep;
         read_uint64(fd, &info->numElements, 1);
         std::cout << "   float vec [" << info->numElements << ", " << std::endl;
         return info;
      }

      case vistle::Object::VEC3FLOAT: {

         Vec3<vistle::Scalar>::Info *info = new Vec3<vistle::Scalar>::Info;
         info->infosize = infosize;
         info->itemsize = itemsize;
         info->offset = offset;
         info->type = type;
         info->block = block;
         info->timestep = timestep;
         read_uint64(fd, &info->numElements, 1);
         std::cout << "   float 3 vec [" << info->numElements << ", " << std::endl;
         return info;
      }

      default: {
         // unknown entry
         lseek(fd, infosize - sizeof(vistle::Object::Info), SEEK_CUR);
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
      vistle::Object::Info *info = readItemInfo(fd);
      cat->item = info;
   }
   return cat;
}

vistle::Object::ptr ReadVistle::load(const std::string & name) {

   int fd = open(name.c_str(), O_RDONLY);
   if (fd == -1) {
      std::cout << "ERROR ReadVistle::load could not open file [" << name
                << "]" << std::endl;
      return vistle::Object::ptr();
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

   boost::scoped_ptr<catalogue> cat(readCatalogue(fd));

   uint64_t start = lseek(fd, 0, SEEK_CUR);

   first_object = true;
   vistle::Object::ptr object = readObject(fd, cat->item, start, std::vector<std::string>(), 0);
   object->addAttribute("mark_end");

   return object;
}

bool ReadVistle::compute() {

   vistle::Object::ptr object = load(getFileParameter("filename"));
   (void) object;
   /*
   if (object)
      addObject("grid_out", object);
   */
   return true;
}
