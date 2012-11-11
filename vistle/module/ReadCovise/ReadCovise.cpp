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
#include "lines.h"
#include "unstr.h"

#include <covReadFiles.h>

#include "ReadCovise.h"

MODULE_MAIN(ReadCovise)

using namespace vistle;

ReadCovise::ReadCovise(const std::string &shmname, int rank, int size, int moduleID)
   : Module("ReadCovise", shmname, rank, size, moduleID) {

   createOutputPort("grid_out");
   addFileParameter("filename", "");
}

ReadCovise::~ReadCovise() {

}

off_t tell(const int fd) {

   return lseek(abs(fd), 0, SEEK_CUR);
}

off_t seek(const int fd, off_t off) {

   return lseek(abs(fd), off, SEEK_SET);
}

void ReadCovise::applyAttributes(Object::ptr obj, const Element &elem, int index) {

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
            std::cerr << "ReadCovise: multiple TIMESTEP attributes in object hierarchy" << std::endl;
         }
         obj->setTimestep(index);
      } else if (obj->getBlock() == -1) {
         obj->setBlock(index);
      }
   }
}

AttributeList ReadCovise::readAttributes(const int fd) {

   std::vector<std::pair<std::string, std::string> > attributes;
   int num, size;
   covReadNumAttributes(fd, &num, &size);
   assert(num >= 0);
   if (num > 0) {
      assert(size >= 0);
   }

   if (num > 0 && size > 0) {
      std::vector<char *> key(num), value(num);
      std::vector<char> buf(size);
      key[0] = &buf[0];
      covReadAttributes(fd, &key[0], &value[0], num, size);

      for (int i=0; i<num; ++i) {
         attributes.push_back(std::pair<std::string, std::string>(key[i], value[i]));
      }
   }

   return attributes;
}

bool ReadCovise::readSETELE(const int fd, Element *parent) {

   int num=-1;
   covReadSetBegin(fd, &num);
   assert(num >= 0);

   for (int index = 0; index < num; index ++) {

      Element *elem = new Element();
      elem->parent = parent;
      elem->index = index;
      readSkeleton(fd, elem);
      parent->subelems.push_back(elem);
   }

   return true;
}

Object::ptr ReadCovise::readUNSGRD(const int fd, const bool skeleton) {

   int numElements=-1;
   int numCorners=-1;
   int numVertices=-1;

   covReadSizeUNSGRD(fd, &numElements, &numCorners, &numVertices);
   assert(numElements>=0);
   assert(numCorners>=0);
   assert(numVertices>=0);

   if (skeleton) {

      covSkipUNSGRD(fd, numElements, numCorners, numVertices);
   } else {

      UnstructuredGrid::ptr usg(new UnstructuredGrid(numElements, numCorners, numVertices));

      int *_tl = new int[numElements];
      int *_el = new int[numElements];
      int *_cl = new int[numCorners];

      char *tl = &usg->tl()[0];
      size_t *el = &usg->el()[0];
      size_t *cl = &usg->cl()[0];

      std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

      covReadUNSGRD(fd, numElements, numCorners, numVertices,
            _el, _cl, _tl,
            &_x[0], &_y[0], &_z[0]);

      for (int index = 0; index < numElements; index ++) {
         el[index] = _el[index];
         tl[index] = (UnstructuredGrid::Type) _tl[index];
      }

      for (int index = 0; index < numCorners; index ++)
         cl[index] = _cl[index];

      for (int index = 0; index < numVertices; ++index) {
         usg->x()[index] = _x[index];
         usg->y()[index] = _y[index];
         usg->z()[index] = _z[index];
      }

      delete[] _el;
      delete[] _tl;
      delete[] _cl;

      return usg;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readUSTSDT(const int fd, const bool skeleton) {

   int numElements=-1;
   covReadSizeUSTSDT(fd, &numElements);
   assert(numElements>=0);

   if (skeleton) {

      covSkipUSTSDT(fd, numElements);
   } else {

      Vec<Scalar>::ptr array(new Vec<Scalar>(numElements));
      std::vector<float> _x(numElements);
      covReadUSTSDT(fd, numElements, &_x[0]);
      for (int i=0; i<numElements; ++i)
         array->x()[i] = _x[i];

      return array;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readUSTVDT(const int fd, const bool skeleton) {

   int numElements=-1;
   covReadSizeUSTVDT(fd, &numElements);
   assert(numElements>=0);

   if (skeleton) {

      covSkipUSTVDT(fd, numElements);
   } else {

      Vec3<Scalar>::ptr array(new Vec3<Scalar>(numElements));
      std::vector<float> _x(numElements), _y(numElements), _z(numElements);
      covReadUSTVDT(fd, numElements, &_x[0], &_y[0], &_z[0]);
      for (int i=0; i<numElements; ++i) {
         array->x()[i] = _x[i];
         array->y()[i] = _y[i];
         array->z()[i] = _z[i];
      }

      return array;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readLINES(const int fd, const bool skeleton) {

   int numElements=-1, numCorners=-1, numVertices=-1;
   covReadSizeLINES(fd, &numElements, &numCorners, &numVertices);
   assert(numElements>=0);
   assert(numCorners>=0);
   assert(numVertices>=0);

   if (skeleton) {

      covSkipLINES(fd, numElements, numCorners, numVertices);
   } else {

      Lines::ptr lines(new Lines(numElements, numCorners, numVertices));

      std::vector<int> el(numElements);
      std::vector<int> cl(numCorners);
      std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

      covReadLINES(fd, numElements, &el[0], numCorners, &cl[0], numVertices, &_x[0], &_y[0], &_z[0]);

      for (int index = 0; index < numElements; index ++)
         lines->el()[index] = el[index];

      for (int index = 0; index < numCorners; index ++)
         lines->cl()[index] = cl[index];

      for (int index = 0; index < numVertices; index ++) {
         lines->x()[index] = _x[index];
         lines->y()[index] = _y[index];
         lines->z()[index] = _z[index];
      }

      return lines;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readPOLYGN(const int fd, const bool skeleton) {

   int numElements=-1, numCorners=-1, numVertices=-1;
   covReadSizePOLYGN(fd, &numElements, &numCorners, &numVertices);
   assert(numElements>=0);
   assert(numCorners>=0);
   assert(numVertices>=0);

   if (skeleton) {

      covSkipPOLYGN(fd, numElements, numCorners, numVertices);
   } else {

      Polygons::ptr polygons(new Polygons(numElements, numCorners, numVertices));

      std::vector<int> el(numElements);
      std::vector<int> cl(numCorners);
      std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

      covReadPOLYGN(fd, numElements, &el[0], numCorners, &cl[0], numVertices, &_x[0], &_y[0], &_z[0]);

      for (int index = 0; index < numElements; index ++)
         polygons->el()[index] = el[index];

      for (int index = 0; index < numCorners; index ++)
         polygons->cl()[index] = cl[index];

      for (int index = 0; index < numVertices; index ++) {
         polygons->x()[index] = _x[index];
         polygons->y()[index] = _y[index];
         polygons->z()[index] = _z[index];
      }

      return polygons;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readGEOTEX(const int fd, const bool skeleton, Element *elem) {

   assert(elem);

   const size_t ncomp = 4;
   int contains[ncomp] = { 0, 0, 0, 0 };
   covReadGeometryBegin(fd, &contains[0], &contains[1], &contains[2], &contains[3]);

   if (skeleton) {

      for (size_t i=0; i<ncomp; ++i) {
         Element *e = new Element();
         e->offset = tell(fd);
         if (contains[i])
            readSkeleton(fd, e);
         elem->subelems.push_back(e);
      }
   } else {
      assert(elem->subelems.size() == ncomp);

      Geometry::ptr container(new Geometry);

      if (contains[0]) {
         container->setGeometry(readObject(fd, *elem->subelems[0]));
      }

      if (contains[1]) {
         container->setColors(readObject(fd, *elem->subelems[1]));
      }

      if (contains[2]) {
         container->setNormals(readObject(fd, *elem->subelems[2]));
      }

      if (contains[3]) {
         container->setTexture(readObject(fd, *elem->subelems[3]));
      }

      return container;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readObjectIntern(const int fd, const bool skeleton, Element *elem) {

   Object::ptr object;

   if (skeleton) {
      elem->offset = tell(fd);
   } else {
      seek(fd, elem->offset);
   }

   char buf[7];
   if(covReadDescription(fd, buf) == -1) {
      std::cerr << "ReadCovise: not a COVISE file?" << std::endl;
      return object;
   }
   buf[6] = '\0';
   std::string type(buf);
   //std::cerr << "ReadCovise::readObject " << type << std::endl;

   bool handled = false;
#define HANDLE(t) \
   if (!handled && type == "" #t) { \
      handled = true; \
      object = read##t(fd, skeleton); \
   }
   HANDLE(UNSGRD);
   HANDLE(USTSDT);
   HANDLE(USTVDT);
   HANDLE(POLYGN);
   HANDLE(LINES);
#undef HANDLE

   if (!handled) {
      if (type == "GEOTEX") {
         object = readGEOTEX(fd, skeleton, elem);
      } else {
         if (type == "SETELE") {
            if (skeleton) {
               readSETELE(fd, elem);
            }
         } else {
            std::cerr << "ReadCovise: object type [" << buf << "] unsupported"
               << std::endl;
            exit(1);
         }
      }
   }

   if (skeleton) {
      elem->attribs = readAttributes(fd);
   } else {
      if (object)
         applyAttributes(object, *elem);
   }

   return object;
}

Object::ptr ReadCovise::readObject(const int fd, const Element &elem) {

   return readObjectIntern(fd, false, const_cast<Element *>(&elem));
}

bool ReadCovise::readSkeleton(const int fd, Element *elem) {

   readObjectIntern(fd, true, elem);
   return true;
}

bool ReadCovise::readRecursive(const int fd, const Element &elem) {

   if (Object::ptr obj = readObject(fd, elem)) {
      // obj is regular
      // do not recurse as subelems are abused for Geometry components
      addObject("grid_out", obj);
   } else {
      // obj corresponds to a Set, recurse
      for (size_t i=0; i<elem.subelems.size(); ++i) {
         readRecursive(fd, *elem.subelems[i]);
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

bool ReadCovise::load(const std::string & name) {

   int fd = covOpenInFile(name.c_str());
   if (!fd) {
      std::cerr << "ReadCovise: failed to open " << name << std::endl;
      return false;
   }

   Element elem;
   readSkeleton(fd, &elem);

   readRecursive(fd, elem);
   deleteRecursive(elem);

   covCloseInFile(fd);

   return true;
}

bool ReadCovise::compute() {

   return load(getFileParameter("filename"));
}
