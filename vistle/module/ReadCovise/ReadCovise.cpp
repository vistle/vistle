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
#include "geometry.h"
#include "vec.h"
#include "polygons.h"
#include "unstr.h"

#include "ReadCovise.h"

MODULE_MAIN(ReadCovise)

using namespace vistle;

ReadCovise::ReadCovise(const std::string &shmname, int rank, int size, int moduleID)
   : Module("ReadCovise", shmname, rank, size, moduleID) {

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

off_t tell(const int fd) {

   return lseek(fd, 0, SEEK_CUR);
}

off_t seek(const int fd, off_t off) {

   return lseek(fd, off, SEEK_SET);
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
      std::cerr << "ERROR ReadCovise::read_type read " << r
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
      std::cerr << "ERROR ReadCovise::read_int read " << r
                << " bytes instead of " << num * sizeof(int) << std::endl;

   if (byte_swap)
      swap_int(data, r / sizeof(int));

   return r;
}

size_t seek_int(const int fd, const size_t num) {

   const size_t s = num * sizeof(uint32_t);

   lseek(fd, s, SEEK_CUR);

   return s;
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
      std::cerr << "ERROR ReadCovise::read_int read " << r
                << " bytes instead of " << num << std::endl;

   return r;
}

size_t seek_char(const int fd, const size_t num) {

   lseek(fd, num, SEEK_CUR);
   return num;
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
      std::cerr << "ERROR ReadCovise::read_float read " << r
                << " bytes instead of " << num * sizeof(float) << std::endl;

   if (byte_swap)
      swap_float(data, r / sizeof(float));

   return r;
}

size_t read_float(const int fd, double *data,
      const size_t num, const bool byte_swap) {

   std::vector<float> fdata(num);

   size_t r = read_float(fd, &fdata[0], num, byte_swap);
   for (size_t i=0; i<num; ++i)
      data[i] = fdata[i];

   return r;
}

size_t seek_float(const int fd, const size_t num) {

   const size_t s = num*sizeof(float);
   lseek(fd, s, SEEK_CUR);
   return s;
}

ReadCovise::~ReadCovise() {

}

void ReadCovise::applyAttributes(vistle::Object::ptr obj, const Element &elem, int index) {

   if (elem.parent) {
      applyAttributes(obj, *elem.parent, elem.index);
   }

   bool isTimestep = false;
   for (size_t i=0; i<elem.attribs.size(); ++i) {
      const std::pair<std::string, std::string> &att = elem.attribs[i];
      if (att.first == "TIMESTEP")
         isTimestep = true;
      else
         obj->addAttribute(att.first, att.second);
   }

   if (index != -1) {
      if (isTimestep) {
         if (obj->getTimestep() != -1) {
            std::cerr << "multiple TIMESTEP attributes in object hierarchy" << std::endl;
         }
         obj->setTimestep(index);
      } else if (obj->getBlock() == -1) {
         obj->setBlock(index);
      }
   }
}

AttributeList ReadCovise::readAttributes(const int fd, const bool byteswap) {

   unsigned int size, num;
   read_int(fd, &size, 1, byteswap);
   size -= sizeof(unsigned int);
   read_int(fd, &num, 1, byteswap);
   /*
   std::cerr << "ReadCovise::readAttributes: " << num << " attributes\n"
             << std::endl;
   */
   std::vector<char> attrib(size+1);
   attrib[size] = '\0';
   read_char(fd, &attrib[0], size);

   std::vector<std::pair<std::string, std::string> > attributes;
   const char *loc = &attrib[0];
   for (size_t index = 0; index < num; index ++) {

      const char *value = strchr(loc, '\0');
      if (!value || value >= &attrib[size])
         break;
      ++value;

      attributes.push_back(std::pair<std::string, std::string>(std::string(loc), std::string(value)));

      loc = strchr(value, '\0');
      if (!loc || loc >= &attrib[size])
         break;
      ++loc;
   }

   return attributes;
}

bool ReadCovise::readSETELE(const int fd, const bool byteswap, Element *parent) {

   unsigned int num;
   read_int(fd, &num, 1, byteswap);

   for (unsigned int index = 0; index < num; index ++) {

      Element *elem = new Element();
      elem->parent = parent;
      elem->index = index;
      readSkeleton(fd, byteswap, elem);
      parent->subelems.push_back(elem);
   }

   return true;
}

vistle::Object::ptr ReadCovise::readUNSGRD(const int fd, const bool byteswap, const bool skeleton) {

   unsigned int numElements;
   unsigned int numCorners;
   unsigned int numVertices;

   read_int(fd, &numElements, 1, byteswap);
   read_int(fd, &numCorners, 1, byteswap);
   read_int(fd, &numVertices, 1, byteswap);

   if (skeleton) {

      seek_int(fd, numElements);
      seek_int(fd, numElements);
      seek_int(fd, numCorners);
      seek_float(fd, numVertices);
      seek_float(fd, numVertices);
      seek_float(fd, numVertices);

   } else {

      vistle::UnstructuredGrid::ptr usg(new vistle::UnstructuredGrid(numElements, numCorners, numVertices));

      unsigned int *_tl = new unsigned int[numElements];
      unsigned int *_el = new unsigned int[numElements];
      unsigned int *_cl = new unsigned int[numCorners];

      char *tl = &usg->tl()[0];
      size_t *el = &usg->el()[0];
      size_t *cl = &usg->cl()[0];

      read_int(fd, _tl, numElements, byteswap);
      read_int(fd, _el, numElements, byteswap);
      read_int(fd, _cl, numCorners, byteswap);

      read_float(fd, &usg->x()[0], numVertices, byteswap);
      read_float(fd, &usg->y()[0], numVertices, byteswap);
      read_float(fd, &usg->z()[0], numVertices, byteswap);

      for (unsigned int index = 0; index < numElements; index ++) {
         el[index] = _el[index];
         tl[index] = (vistle::UnstructuredGrid::Type) _tl[index];
      }

      for (unsigned int index = 0; index < numCorners; index ++)
         cl[index] = _cl[index];

      delete[] _el;
      delete[] _tl;
      delete[] _cl;

      return usg;
   }

   return Object::ptr();
}

vistle::Object::ptr ReadCovise::readUSTSDT(const int fd, const bool byteswap, const bool skeleton) {

   unsigned int numElements;
   read_int(fd, &numElements, 1, byteswap);

   if (skeleton) {

      seek_float(fd, numElements);
   } else {

      vistle::Vec<vistle::Scalar>::ptr array(new vistle::Vec<vistle::Scalar>(numElements));
      read_float(fd, &array->x()[0], numElements, byteswap);
      return array;
   }

   return Object::ptr();
}

vistle::Object::ptr ReadCovise::readUSTVDT(const int fd, const bool byteswap, const bool skeleton) {

   unsigned int numElements;
   read_int(fd, &numElements, 1, byteswap);

   if (skeleton) {

      seek_float(fd, numElements);
      seek_float(fd, numElements);
      seek_float(fd, numElements);
   } else {

      vistle::Vec3<vistle::Scalar>::ptr array(new vistle::Vec3<vistle::Scalar>(numElements));
      read_float(fd, &array->x()[0], numElements, byteswap);
      read_float(fd, &array->y()[0], numElements, byteswap);
      read_float(fd, &array->z()[0], numElements, byteswap);

      return array;
   }

   return Object::ptr();
}

vistle::Object::ptr ReadCovise::readPOLYGN(const int fd, const bool byteswap, const bool skeleton) {

   unsigned int numElements, numCorners, numVertices;
   read_int(fd, &numElements, 1, byteswap);
   read_int(fd, &numCorners, 1, byteswap);
   read_int(fd, &numVertices, 1, byteswap);

   if (skeleton) {

      seek_int(fd, numElements);
      seek_int(fd, numCorners);
      seek_float(fd, numVertices);
      seek_float(fd, numVertices);
      seek_float(fd, numVertices);
   } else {

      vistle::Polygons::ptr polygons(new vistle::Polygons(numElements, numCorners, numVertices));

      unsigned int *el = new unsigned int[numElements];
      unsigned int *cl = new unsigned int[numCorners];

      read_int(fd, el, numElements, byteswap);
      read_int(fd, cl, numCorners, byteswap);

      for (unsigned int index = 0; index < numElements; index ++)
         polygons->el()[index] = el[index];

      for (unsigned int index = 0; index < numCorners; index ++)
         polygons->cl()[index] = cl[index];

      read_float(fd, &polygons->x()[0], numVertices, byteswap);
      read_float(fd, &polygons->y()[0], numVertices, byteswap);
      read_float(fd, &polygons->z()[0], numVertices, byteswap);

      return polygons;
   }

   return Object::ptr();
}

vistle::Object::ptr ReadCovise::readGEOTEX(const int fd, const bool byteswap, const bool skeleton, Element *elem) {

   assert(elem);

   const size_t ncomp = 4;
   unsigned int contains[ncomp] = { 0, 0, 0, 0 };
   unsigned int ignore[ncomp];

   read_int(fd, contains, ncomp, byteswap);
   read_int(fd, ignore, ncomp, byteswap);

   if (skeleton) {

      for (size_t i=0; i<ncomp; ++i) {
         Element *e = new Element();
         e->offset = tell(fd);
         if (contains[i])
            readSkeleton(fd, byteswap, e);
         elem->subelems.push_back(e);
      }
   } else {
      assert(elem->subelems.size() == ncomp);

      vistle::Geometry::ptr container(new vistle::Geometry);

      if (contains[0]) {
         container->setGeometry(readObject(fd, byteswap, *elem->subelems[0]));
      }

      if (contains[1]) {
         container->setColors(readObject(fd, byteswap, *elem->subelems[1]));
      }

      if (contains[2]) {
         container->setNormals(readObject(fd, byteswap, *elem->subelems[2]));
      }

      if (contains[3]) {
         container->setTexture(readObject(fd, byteswap, *elem->subelems[3]));
      }

      return container;
   }

   return Object::ptr();
}

vistle::Object::ptr ReadCovise::readObject(const int fd, const bool byteswap, const Element &elem) {

   seek(fd, elem.offset);

   char buf[7];
   buf[6] = 0;

   vistle::Object::ptr object;

   if (read_type(fd, buf) == 6) {

      std::cerr << "ReadCovise::readObject " << buf << std::endl;

      if (!strncmp(buf, "UNSGRD", 6))
         object = readUNSGRD(fd, byteswap, false);

      else if (!strncmp(buf, "USTSDT", 6))
         object = readUSTSDT(fd, byteswap, false);

      else if (!strncmp(buf, "USTVDT", 6))
         object = readUSTVDT(fd, byteswap, false);

      else if (!strncmp(buf, "POLYGN", 6))
         object = readPOLYGN(fd, byteswap, false);

      else if (!strncmp(buf, "GEOTEX", 6))
         object = readGEOTEX(fd, byteswap, false, const_cast<Element *>(&elem));

      else {
         if (strncmp(buf, "SETELE", 6)) {
            std::cerr << "ReadCovise: object type [" << buf << "] unsupported"
               << std::endl;
            exit(1);
         }
      }
   }

   if (object)
      applyAttributes(object, elem);

   return object;
}

bool ReadCovise::readRecursive(const int fd, const bool byteswap, const Element &elem) {

   if (Object::ptr obj = readObject(fd, byteswap, elem)) {
      // obj is regular
      // do not recurse as subelems are abused for Geometry components
      addObject("grid_out", obj);
   } else {
      // obj corresponds to a Set, recurse
      for (size_t i=0; i<elem.subelems.size(); ++i) {
         readRecursive(fd, byteswap, *elem.subelems[i]);
      }
   }
   return true;
}

void ReadCovise::deleteRecursive(Element &elem) {

   for (size_t i=0; i<elem.subelems.size(); ++i) {
      deleteRecursive(*elem.subelems[i]);
      delete elem.subelems[i];
   }
}

bool ReadCovise::readSkeleton(const int fd, const bool byteswap, Element *elem) {

   vistle::Object::ptr object;
   elem->offset = tell(fd);

   char buf[7];
   buf[6] = 0;
   if (read_type(fd, buf) == 6) {

      std::cerr << "ReadCovise::readSkeleton " << buf << std::endl;

      if (!strncmp(buf, "SETELE", 6))
         readSETELE(fd, byteswap, elem);

      else if (!strncmp(buf, "UNSGRD", 6))
         readUNSGRD(fd, byteswap, true);

      else if (!strncmp(buf, "USTSDT", 6))
         readUSTSDT(fd, byteswap, true);

      else if (!strncmp(buf, "USTVDT", 6))
         readUSTVDT(fd, byteswap, true);

      else if (!strncmp(buf, "POLYGN", 6))
         readPOLYGN(fd, byteswap, true);

      else if (!strncmp(buf, "GEOTEX", 6))
         readGEOTEX(fd, byteswap, true, elem);

      else {
         std::cerr << "ReadCovise: object type [" << buf << "] unsupported"
                    << std::endl;
         exit(1);
      }

      elem->attribs = readAttributes(fd, byteswap);
   }

   return true;
}

bool ReadCovise::load(const std::string & name) {

   bool byteswap = true;

   int fd = open(name.c_str(), O_RDONLY);
   if (fd == -1) {
      std::cerr << "ERROR ReadCovise::load could not open file [" << name
                << "]" << std::endl;
      return vistle::Object::const_ptr();
   }

   char buf[7];
   buf[6] = 0;

   int r = read_type(fd, buf);
   if (r != 6)
      return vistle::Object::const_ptr();

   if (!strncmp(buf, "COV_BE", 6))
      byteswap = false;
   else if (!strncmp(buf, "COV_LE", 6))
      byteswap = true;
   else
      lseek(fd, 0, SEEK_SET);

   Element elem;
   readSkeleton(fd, byteswap, &elem);

   readRecursive(fd, byteswap, elem);
   deleteRecursive(elem);

   close(fd);

   return true;
}

bool ReadCovise::compute() {

   return load(getFileParameter("filename"));
}
