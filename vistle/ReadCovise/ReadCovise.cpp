#include <sstream>
#include <iomanip>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include <google/profiler.h>

#include "object.h"

#include "ReadCovise.h"

MODULE_MAIN(ReadCovise)

ReadCovise::ReadCovise(int rank, int size, int moduleID)
   : Module("ReadCovise", rank, size, moduleID) {

   createOutputPort("grid_out");
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

ReadCovise::~ReadCovise() {

}

void ReadCovise::setTimesteps(vistle::Object * object, const int timestep) {

   if (object->getType() == vistle::Object::SET) {

      vistle::Set *set = static_cast<vistle::Set *>(object);
      for (size_t index = 0; index < set->getNumElements(); index ++)
         setTimesteps(set->getElement(index), timestep);
   }
   object->setTimestep(timestep);
}

bool ReadCovise::readAttributes(const int fd, const bool byteswap) {

   unsigned int size, num;
   read_int(fd, &size, 1, byteswap);
   size -= sizeof(unsigned int);
   read_int(fd, &num, 1, byteswap);
   /*
   std::cout << "ReadCovise::readAttributes: " << num << " attributes\n"
             << std::endl;
   */
   char *attrib = new char[size + 1];
   attrib[size] = 0;
   read_char(fd, attrib, size);
   // lseek(fd, size, SEEK_CUR);

   std::map<std::string, std::string> attributeMap;
   char *loc = attrib;
   for (size_t index = 0; index < num; index ++) {

      char *value = strchr(loc, 0);
      if (value && value < attrib + size) {
         attributeMap[std::string(loc)] = std::string(value + 1);
         loc = strchr(value + 1, 0) + 1;
      }
   }

   delete[] attrib;

   if (attributeMap.find("TIMESTEP") != attributeMap.end())
      return true;
   return false;
}

vistle::Object * ReadCovise::readSETELE(const int fd, const bool byteswap) {

   vistle::Set *set = vistle::Set::create();

   unsigned int num;
   read_int(fd, &num, 1, byteswap);

   for (unsigned int index = 0; index < num; index ++) {

      vistle::Object *object = readObject(fd, byteswap);
      if (object)
         set->elements->push_back(object);
   }

   bool timesteps = readAttributes(fd, byteswap);

   for (unsigned int index = 0; index < num; index ++) {
      if (timesteps)
         setTimesteps(set->getElement(index), index);
      else
         set->getElement(index)->setBlock(index);
   }

   return set;
}

vistle::Object *  ReadCovise::readUNSGRD(const int fd, const bool byteswap) {

   vistle::UnstructuredGrid *usg = NULL;

   unsigned int numElements;
   unsigned int numCorners;
   unsigned int numVertices;

   read_int(fd, &numElements, 1, byteswap);
   read_int(fd, &numCorners, 1, byteswap);
   read_int(fd, &numVertices, 1, byteswap);

   usg = vistle::UnstructuredGrid::create(numElements, numCorners, numVertices);

   unsigned int *_tl = new unsigned int[numElements];
   unsigned int *_el = new unsigned int[numElements];
   unsigned int *_cl = new unsigned int[numCorners];

   char *tl = &((*usg->tl)[0]);
   size_t *el = &((*usg->el)[0]);
   size_t *cl = &((*usg->cl)[0]);

   read_int(fd, _tl, numElements, byteswap);
   read_int(fd, _el, numElements, byteswap);
   read_int(fd, _cl, numCorners, byteswap);

   read_float(fd, &((*usg->x)[0]), numVertices, byteswap);
   read_float(fd, &((*usg->y)[0]), numVertices, byteswap);
   read_float(fd, &((*usg->z)[0]), numVertices, byteswap);

   for (unsigned int index = 0; index < numElements; index ++) {
      el[index] = _el[index];
      tl[index] = (vistle::UnstructuredGrid::Type) _tl[index];
   }

   for (unsigned int index = 0; index < numCorners; index ++)
      cl[index] = _cl[index];

   delete[] _el;
   delete[] _tl;
   delete[] _cl;

   readAttributes(fd, byteswap);

   return usg;
}

vistle::Object * ReadCovise::readUSTSDT(const int fd, const bool byteswap) {

   vistle::Vec<float> *array = NULL;
   unsigned int numElements;

   read_int(fd, &numElements, 1, byteswap);

   array = vistle::Vec<float>::create(numElements);
   read_float(fd, &((*array->x)[0]), numElements, byteswap);

   readAttributes(fd, byteswap);
   return array;
}

vistle::Object * ReadCovise::readUSTVDT(const int fd, const bool byteswap) {

   vistle::Vec3<float> *array = NULL;
   unsigned int numElements;

   read_int(fd, &numElements, 1, byteswap);

   array = vistle::Vec3<float>::create(numElements);
   read_float(fd, &((*array->x)[0]), numElements, byteswap);
   read_float(fd, &((*array->y)[0]), numElements, byteswap);
   read_float(fd, &((*array->z)[0]), numElements, byteswap);

   readAttributes(fd, byteswap);
   return array;
}

vistle::Object * ReadCovise::readPOLYGN(const int fd, const bool byteswap) {

   vistle::Polygons * polygons = NULL;
   unsigned int numElements, numCorners, numVertices;

   read_int(fd, &numElements, 1, byteswap);
   read_int(fd, &numCorners, 1, byteswap);
   read_int(fd, &numVertices, 1, byteswap);

   polygons = vistle::Polygons::create(numElements, numCorners, numVertices);

   unsigned int *el = new unsigned int[numElements];
   unsigned int *cl = new unsigned int[numCorners];

   read_int(fd, el, numElements, byteswap);
   read_int(fd, cl, numCorners, byteswap);

   for (unsigned int index = 0; index < numElements; index ++)
      (*polygons->el)[index] = el[index];

   for (unsigned int index = 0; index < numCorners; index ++)
      (*polygons->cl)[index] = cl[index];

   read_float(fd, &((*polygons->x)[0]), numVertices, byteswap);
   read_float(fd, &((*polygons->y)[0]), numVertices, byteswap);
   read_float(fd, &((*polygons->z)[0]), numVertices, byteswap);

   readAttributes(fd, byteswap);

   return polygons;
}

vistle::Object * ReadCovise::readGEOTEX(const int fd, const bool byteswap) {

   vistle::Geometry *container = vistle::Geometry::create();
   unsigned int contains[4] = { 0, 0, 0, 0 };
   unsigned int ignore[4];

   read_int(fd, contains, 4, byteswap);
   read_int(fd, ignore, 4, byteswap);

   if (contains[0])
      container->geometry = readObject(fd, byteswap);

   if (contains[1])
      container->colors = readObject(fd, byteswap);

   if (contains[2])
      container->normals = readObject(fd, byteswap);

   if (contains[3])
      container->texture = readObject(fd, byteswap);

   readAttributes(fd, byteswap);

   return container;
}

vistle::Object * ReadCovise::readObject(const int fd, const bool byteswap) {

   char buf[7];
   buf[6] = 0;

   vistle::Object *object = NULL;

   if (read_type(fd, buf) == 6) {

      std::cout << "ReadCovise::readObject " << buf << std::endl;

      if (!strncmp(buf, "SETELE", 6))
         object = readSETELE(fd, byteswap);

      else if (!strncmp(buf, "UNSGRD", 6))
         object = readUNSGRD(fd, byteswap);

      else if (!strncmp(buf, "USTSDT", 6))
         object = readUSTSDT(fd, byteswap);

      else if (!strncmp(buf, "USTVDT", 6))
         object = readUSTVDT(fd, byteswap);

      else if (!strncmp(buf, "POLYGN", 6))
         object = readPOLYGN(fd, byteswap);

      else if (!strncmp(buf, "GEOTEX", 6))
         object = readGEOTEX(fd, byteswap);

      else {
         std:: cout << "ReadCovise: object type [" << buf << "] unsupported"
                    << std::endl;
         exit(1);
      }
   }
   return object;
}

vistle::Object * ReadCovise::load(const std::string & name) {

   vistle::Object *object = NULL;

   bool byteswap = true;

   int fd = open(name.c_str(), O_RDONLY);
   if (fd == -1) {
      std::cout << "ERROR ReadCovise::load could not open file [" << name
                << "]" << std::endl;
      return NULL;
   }

   char buf[7];
   buf[6] = 0;

   int r = read_type(fd, buf);
   if (r != 6)
      return NULL;

   if (!strncmp(buf, "COV_BE", 6))
      byteswap = false;
   else if (!strncmp(buf, "COV_LE", 6))
      byteswap = true;
   else
      lseek(fd, 0, SEEK_SET);

   object = readObject(fd, byteswap);
   close(fd);

   return object;
}

bool ReadCovise::compute() {

   vistle::Object *object = load(getFileParameter("filename"));

   if (object)
      addObject("grid_out", object);

   return true;
}
