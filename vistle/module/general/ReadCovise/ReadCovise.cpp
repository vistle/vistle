#include <sstream>
#include <iomanip>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include <google/profiler.h>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/unstr.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>

#ifdef _WIN32
#include <io.h>
#endif

#include <file/covReadFiles.h>

#include "ReadCovise.h"

MODULE_MAIN(ReadCovise)

using namespace vistle;

ReadCovise::ReadCovise(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("read COVISE data", name, moduleID, comm)
{

   createOutputPort("grid_out");
   addStringParameter("filename", "name of COVISE file", "");
   addIntParameter("first_timestep", "first timestep to read", 0);
   addIntParameter("skip_timesteps", "number of timesteps to skip", 0);
}

ReadCovise::~ReadCovise() {

}

static off_t mytell(const int fd) {

   return lseek(abs(fd), 0, SEEK_CUR);
}

static off_t seek(const int fd, off_t off) {

   return lseek(abs(fd), off, SEEK_SET);
}

int findBlockNum(const Element &elem) {
   bool isTimestep = false;
   for (size_t i=0; i<elem.attribs.size(); ++i) {
      const std::pair<std::string, std::string> &att = elem.attribs[i];
      if (att.first == "TIMESTEP")
         isTimestep = true;
   }
   int block = elem.index;
   if (elem.parent && (isTimestep || block<0)) {
       block = findBlockNum(*elem.parent);
   }
   return block;
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
         int outputTimestep = (index) / (m_skipTimesteps+1);
         obj->setTimestep(outputTimestep);
#if 0
         if (elem.parent)
            obj->setNumTimesteps(elem.parent->subelems.size());
#endif
      } else if (obj->getBlock() == -1) {
         obj->setBlock(index);
#if 0
         if (elem.parent)
            obj->setNumBlocks(elem.parent->subelems.size());
#endif
      }

      if (!isTimestep) {
         std::string set = obj->getAttribute("_part_of");
         if (!set.empty())
            set = set + "_";
         std::stringstream idx;
         idx << index;
         set = set + idx.str();
         obj->addAttribute("_part_of", set);
      }
   }
}

void parseAttributes(Element *elem) {
   for (size_t i=0; i<elem->attribs.size(); ++i) {
      const std::pair<std::string, std::string> &att = elem->attribs[i];
      if (att.first == "TIMESTEP") {
          elem->is_timeset = true;
          break;
      }
   }
}


AttributeList ReadCovise::readAttributes(const int fd) {

   std::vector<std::pair<std::string, std::string> > attributes;
   int num, size;
   covReadNumAttributes(fd, &num, &size);
   vassert(num >= 0);
   if (num > 0) {
      vassert(size >= 0);
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
   vassert(num >= 0);

   for (int index = 0; index < num; index ++) {

      Element *elem = new Element();
      elem->parent = parent;
      elem->index = index;
      readSkeleton(fd, elem);
      parent->subelems.push_back(elem);
   }

   return true;
}

Object::ptr ReadCovise::readUNIGRD(const int fd, const bool skeleton) {

   int dim[3];
   float min[3], max[3];

   covReadUNIGRD(fd, &dim[0], &dim[1], &dim[2],
           &min[0], &max[0], &min[1], &max[1], &min[2], &max[2]);
   if (!skeleton) {

      UniformGrid::ptr uni(new UniformGrid(dim[0], dim[1], dim[2]));
      for (int i=0; i<3; ++i) {
          uni->min()[i] = min[i];
          uni->max()[i] = max[i];
      }

      return uni;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readRCTGRD(const int fd, const bool skeleton) {

   int dim[3];

   covReadSizeRCTGRD(fd, &dim[0], &dim[1], &dim[2]);

   if (skeleton) {
      covSkipRCTGRD(fd, dim[0], dim[1], dim[2]);
   } else {
      std::vector<float> coord[3];
      for (int i=0; i<3; ++i)
          coord[i].resize(dim[i]);
      covReadRCTGRD(fd, dim[0], dim[1], dim[2], coord[0].data(), coord[1].data(), coord[2].data());

      RectilinearGrid::ptr rect(new RectilinearGrid(dim[0], dim[1], dim[2]));
      for (int i=0; i<3; ++i) {
          for (int k=0; k<dim[i]+1; ++k) {
              rect->coords(i)[k] = coord[i][k];
          }
      }

      return rect;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readSTRGRD(const int fd, const bool skeleton) {

   int dim[3];
   covReadSizeSTRGRD(fd, &dim[0], &dim[1], &dim[2]);

   if (skeleton) {
      covSkipSTRGRD(fd, dim[0], dim[1], dim[2]);
   } else {
       size_t numVertices = dim[0]*dim[1]*dim[2];
       std::vector<float> v_x(numVertices), v_y(numVertices), v_z(numVertices);
       float *_x = v_x.data(), *_y = v_y.data(), *_z = v_z.data();
       covReadSTRGRD(fd, dim[0], dim[1], dim[2], _x, _y, _z);
       StructuredGrid::ptr str(new StructuredGrid(dim[0], dim[1], dim[2]));
       Scalar *x=str->x().data(), *y=str->y().data(), *z=str->z().data();
       for (Index i=0; i<numVertices; ++i) {
           x[i] = _x[i];
           y[i] = _y[i];
           z[i] = _z[i];
       }
       return str;
   }
   return Object::ptr();
}

Object::ptr ReadCovise::readUNSGRD(const int fd, const bool skeleton) {

   int numElements=-1;
   int numCorners=-1;
   int numVertices=-1;

   covReadSizeUNSGRD(fd, &numElements, &numCorners, &numVertices);
   vassert(numElements>=0);
   vassert(numCorners>=0);
   vassert(numVertices>=0);

   if (skeleton) {

      covSkipUNSGRD(fd, numElements, numCorners, numVertices);
   } else {

      UnstructuredGrid::ptr usg(new UnstructuredGrid(numElements, numCorners, numVertices));

      std::vector<int> v_tl(numElements);
      std::vector<int> v_el(numElements);
      std::vector<int> v_cl(numCorners);
      auto _tl = v_tl.data();
      auto _el = v_el.data();
      auto _cl = v_cl.data();

      std::vector<float> v_x(numVertices), v_y(numVertices), v_z(numVertices);
      float *_x = v_x.data(), *_y = v_y.data(), *_z = v_z.data();
      covReadUNSGRD(fd, numElements, numCorners, numVertices,
            _el, _cl, _tl,
            _x, _y, _z);

      Index *el = usg->el().data();
      unsigned char *tl = &usg->tl()[0];
      for (int index = 0; index < numElements; index ++) {
         el[index] = _el[index];
         tl[index] = (UnstructuredGrid::Type) _tl[index];
      }
      el[numElements] = numCorners;

      Index *cl = usg->cl().data();
      for (int index = 0; index < numCorners; index ++)
         cl[index] = _cl[index];

      // convert to VTK face stream
      for (int index = 0; index < numElements; index ++) {
          if (tl[index] == UnstructuredGrid::POLYHEDRON) {
              Index begin=el[index], end=el[index+1];
              Index faceTerm = InvalidIndex;
              Index faceVert = 0;
              Index faceStart = 0;
              for (Index i=begin; i<end; ++i) {
                  Index v = cl[i];
                  if (faceTerm == InvalidIndex) {
                      faceTerm = v;
                      faceVert = 0;
                      faceStart = i;
                  } else if (faceTerm == v) {
                      cl[faceStart] = faceVert;
                      faceTerm = InvalidIndex;
                      faceVert = 0;
                      faceStart = InvalidIndex;
                  }
                  ++faceVert;
              }
          }
      }

      auto x=usg->x().data(), y=usg->y().data(), z=usg->z().data();
      for (int index = 0; index < numVertices; ++index) {
         x[index] = _x[index];
         y[index] = _y[index];
         z[index] = _z[index];
      }

      usg->checkConvexity();
      return usg;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readUSTSDT(const int fd, const bool skeleton) {

   int numElements=-1;
   covReadSizeUSTSDT(fd, &numElements);
   vassert(numElements>=0);

   if (skeleton) {

      covSkipUSTSDT(fd, numElements);
   } else {

      Vec<Scalar>::ptr array(new Vec<Scalar>(numElements));
      std::vector<float> _x(numElements);
      covReadUSTSDT(fd, numElements, &_x[0]);
      auto x = array->x().data();
      for (int i=0; i<numElements; ++i)
         x[i] = _x[i];

      return array;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readUSTVDT(const int fd, const bool skeleton) {

   int numElements=-1;
   covReadSizeUSTVDT(fd, &numElements);
   vassert(numElements>=0);

   if (skeleton) {

      covSkipUSTVDT(fd, numElements);
   } else {

      Vec<Scalar,3>::ptr array(new Vec<Scalar,3>(numElements));
      std::vector<float> _x(numElements), _y(numElements), _z(numElements);
      covReadUSTVDT(fd, numElements, &_x[0], &_y[0], &_z[0]);
      Scalar *x = array->x().data();
      Scalar *y = array->y().data();
      Scalar *z = array->z().data();
      for (int i=0; i<numElements; ++i) {
         x[i] = _x[i];
         y[i] = _y[i];
         z[i] = _z[i];
      }

      return array;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readSTRSDT(const int fd, const bool skeleton) {

   int numElements=-1;
   int sx=-1, sy=-1, sz=-1;
   covReadSizeSTRSDT(fd, &numElements, &sx, &sy, &sz);
   vassert(sx >= 0);
   vassert(sy >= 0);
   vassert(sz >= 0);
   size_t n = sx*sy*sz;

   if (skeleton) {

      covSkipSTRSDT(fd, numElements, sx, sy, sz);
   } else {

      Vec<Scalar>::ptr array(new Vec<Scalar>(n));
      std::vector<float> _x(n);
      covReadSTRSDT(fd, n, &_x[0], sx, sy, sz);
      auto x = array->x().data();
      for (size_t i=0; i<n; ++i)
         x[i] = _x[i];

      return array;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readSTRVDT(const int fd, const bool skeleton) {

   int numElements=-1;
   int sx=-1, sy=-1, sz=-1;
   covReadSizeSTRVDT(fd, &numElements, &sx, &sy, &sz);
   vassert(sx >= 0);
   vassert(sy >= 0);
   vassert(sz >= 0);
   size_t n = sx*sy*sz;

   if (skeleton) {

      covSkipSTRVDT(fd, numElements, sx, sy, sz);
   } else {

      Vec<Scalar,3>::ptr array(new Vec<Scalar,3>(n));
      std::vector<float> _x(n), _y(n), _z(n);
      covReadSTRVDT(fd, n, &_x[0], &_y[0], &_z[0], sx, sy, sz);
      auto x = array->x().data();
      auto y = array->y().data();
      auto z = array->z().data();
      for (size_t i=0; i<n; ++i) {
         x[i] = _x[i];
         y[i] = _y[i];
         z[i] = _z[i];
      }

      return array;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readPOINTS(const int fd, const bool skeleton) {

   int numCoords=-1;
   covReadSizePOINTS(fd, &numCoords);
   vassert(numCoords>=0);

   if (skeleton) {

      covSkipPOINTS(fd, numCoords);
   } else {

      Points::ptr points(new Points(numCoords));

      std::vector<float> _x(numCoords), _y(numCoords), _z(numCoords);

      covReadPOINTS(fd, numCoords, &_x[0], &_y[0], &_z[0]);

      auto *x = points->x().data();
      auto *y = points->y().data();
      auto *z = points->z().data();
      for (int index = 0; index < numCoords; index ++) {
         x[index] = _x[index];
         y[index] = _y[index];
         z[index] = _z[index];
      }

      return points;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readLINES(const int fd, const bool skeleton) {

   int numElements=-1, numCorners=-1, numVertices=-1;
   covReadSizeLINES(fd, &numElements, &numCorners, &numVertices);
   vassert(numElements>=0);
   vassert(numCorners>=0);
   vassert(numVertices>=0);

   if (skeleton) {

      covSkipLINES(fd, numElements, numCorners, numVertices);
   } else {

      Lines::ptr lines(new Lines(numElements, numCorners, numVertices));

      std::vector<int> _el(numElements);
      std::vector<int> _cl(numCorners);
      std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

      covReadLINES(fd, numElements, &_el[0], numCorners, &_cl[0], numVertices, &_x[0], &_y[0], &_z[0]);

      auto el = lines->el().data();
      for (int index = 0; index < numElements; index ++)
         el[index] = _el[index];
      el[numElements] = numCorners;

      auto cl = lines->cl().data();
      for (int index = 0; index < numCorners; index ++)
         cl[index] = _cl[index];

      auto x = lines->x().data(), y = lines->y().data(), z = lines->z().data();
      for (int index = 0; index < numVertices; index ++) {
         x[index] = _x[index];
         y[index] = _y[index];
         z[index] = _z[index];
      }

      return lines;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readPOLYGN(const int fd, const bool skeleton) {

   int numElements=-1, numCorners=-1, numVertices=-1;
   covReadSizePOLYGN(fd, &numElements, &numCorners, &numVertices);
   vassert(numElements>=0);
   vassert(numCorners>=0);
   vassert(numVertices>=0);

   if (skeleton) {

      covSkipPOLYGN(fd, numElements, numCorners, numVertices);
   } else {

      Polygons::ptr polygons(new Polygons(numElements, numCorners, numVertices));

      std::vector<int> _el(numElements);
      std::vector<int> _cl(numCorners);
      std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

      covReadPOLYGN(fd, numElements, &_el[0], numCorners, &_cl[0], numVertices, &_x[0], &_y[0], &_z[0]);

      auto el = polygons->el().data();
      for (int index = 0; index < numElements; index ++)
         el[index] = _el[index];
      el[numElements] = numCorners;

      auto cl = polygons->cl().data();
      for (int index = 0; index < numCorners; index ++)
         cl[index] = _cl[index];

      auto x = polygons->x().data(), y = polygons->y().data(), z = polygons->z().data();
      for (int index = 0; index < numVertices; index ++) {
         x[index] = _x[index];
         y[index] = _y[index];
         z[index] = _z[index];
      }

      return polygons;
   }

   return Object::ptr();
}

Object::ptr ReadCovise::readGEOTEX(const int fd, const bool skeleton, Element *elem, int timestep) {

   vassert(elem);
   // XXX: handle sets in GEOTEX

   const size_t ncomp = 4;
   int contains[ncomp] = { 0, 0, 0, 0 };
   covReadGeometryBegin(fd, &contains[0], &contains[1], &contains[2], &contains[3]);

   if (skeleton) {

      for (size_t i=0; i<ncomp; ++i) {
         Element *e = new Element();
         e->in_geometry = true;
         e->offset = mytell(fd);
         if (contains[i])
            readSkeleton(fd, e);
         elem->subelems.push_back(e);
      }
   } else {
      vassert(elem->subelems.size() == ncomp);

      DataBase::ptr data;
      Coords::ptr grid;

      if (contains[0]) {
         grid = Coords::as(readObject(fd, elem->subelems[0], timestep));
      }

      if (contains[2]) {
         auto normals = Normals::clone<Vec<Scalar,3>>(Vec<Scalar,3>::as(readObject(fd, elem->subelems[2], timestep)));
         if (grid)
             grid->setNormals(normals);
      }

      if (contains[3]) {
         data = DataBase::as(readObject(fd, elem->subelems[3], timestep));
      }

      if (!data && contains[1]) {
         data = DataBase::as(readObject(fd, elem->subelems[1], timestep));
      }

      if (data) {
          data->setGrid(grid);
          return data;
      }

      return grid;
   }

   return Object::ptr();
}

vistle::Object::ptr ReadCovise::readOBJREF(const int fd, bool skeleton, Element *elem) {

   int objNum = -1;
   if (covReadOBJREF(fd, &objNum) == -1) {
      std::cerr << "ReadCovise: failed to read OBJREF" << std::endl;
      return Object::ptr();
   }

   if (objNum < 0 || size_t(objNum) >= m_objects.size()) {
      std::cerr << "ReadCovise: invalid OBJREF" << std::endl;
      return Object::ptr();
   }

   elem->referenced = m_objects[objNum];
   if (!elem->referenced->obj) {
      //std::cerr << "ReadCovise: OBJREF to SETELE - not supported" << std::endl;
   }
   return elem->referenced->obj;
}

Object::ptr ReadCovise::readObjectIntern(const int fd, const bool skeleton, Element *elem, int timestep) {

   Object::ptr object;

   if (!skeleton) {
      if (elem->objnum < 0)
         return object;

      int block = findBlockNum(*elem);
      if (block % size() != rank())
         return object;

      if ((timestep-m_firstTimestep) % (m_skipTimesteps+1) != 0)
          return object;
      int outputTimestep = (timestep-m_firstTimestep) / (m_skipTimesteps+1);

      if (elem->obj) {
          if (elem->obj->getTimestep() == outputTimestep)
              return elem->obj;
          object = elem->obj->clone();
          object->setTimestep(outputTimestep);
          return object;
      }
   }

   if (skeleton) {
      elem->offset = mytell(fd);
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
   //std::cerr << "ReadCovise::readObject " << type << " @ " << mytell(fd) << std::endl;

   if (type == "OBJREF") {
       object = readOBJREF(fd, skeleton, elem);
       elem->objnum = -1;
       return object;
   } else if (skeleton) {
       m_objects.push_back(elem);
   }

   bool handled = false;
#define HANDLE(t) \
   if (!handled && type == "" #t) { \
      handled = true; \
      object = read##t(fd, skeleton); \
   }
   HANDLE(UNIGRD);
   HANDLE(RCTGRD);
   HANDLE(STRGRD);
   HANDLE(UNSGRD);
   HANDLE(USTSDT);
   HANDLE(USTVDT);
   HANDLE(STRSDT);
   HANDLE(STRVDT);
   HANDLE(POLYGN);
   HANDLE(LINES);
   HANDLE(POINTS);
#undef HANDLE

   if (handled) {
       if (skeleton) {
           elem->objnum = m_numObj;
           ++m_numObj;
       }
   } else {
      if (type == "GEOTEX") {
         object = readGEOTEX(fd, skeleton, elem, timestep);
      } else  if (type == "SETELE") {
            if (skeleton) {
               readSETELE(fd, elem);
            }
      } else {
          std::stringstream str;
          str << "Object type not supported: " << buf;
          std::cerr << "ReadCovise: " << str.str() << std::endl;
          throw vistle::exception(str.str());
      }
   }

   if (skeleton) {
      elem->attribs = readAttributes(fd);
      parseAttributes(elem);
   } else {
      if (object) {
         applyAttributes(object, *elem);
         elem->obj = object;
         std::cerr << "ReadCovise: " << type << " [ b# " << object->getBlock() << ", t# " << object->getTimestep() << " ]" << std::endl;
      }
   }

   return object;
}

Object::ptr ReadCovise::readObject(const int fd, Element *elem, int timestep) {

   return readObjectIntern(fd, false, elem, timestep);
}

bool ReadCovise::readSkeleton(const int fd, Element *elem) {

   readObjectIntern(fd, true, elem, -1);
   return true;
}

bool ReadCovise::readRecursive(const int fd, Element *elem, int timestep) {

   if (cancelRequested()) {
      return true;
   }

   if (Object::ptr obj = readObject(fd, elem, timestep)) {
      // obj is regular
      // do not recurse as subelems are abused for Geometry components
      addObject("grid_out", obj);
   } else {
      const bool inTimeset = elem->is_timeset;
      if (elem->referenced) {
          elem = elem->referenced;
      }
      //std::cerr << "processing SET w/ " << elem->subelems.size() << " elements, timeset=" << inTimeset << std::endl;
      // obj corresponds to a Set, recurse
      for (size_t i=0; i<elem->subelems.size(); ++i) {
         readRecursive(fd, elem->subelems[i], inTimeset ? i : timestep);
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
   try {
      readSkeleton(fd, &elem);

      readRecursive(fd, &elem, -1);
   } catch(vistle::exception &e) {
      covCloseInFile(fd);
      deleteRecursive(elem);

      throw(e);
   }
   covCloseInFile(fd);
   deleteRecursive(elem);

   return true;
}

bool ReadCovise::compute() {

   m_firstTimestep = getIntParameter("first_timestep");
   m_skipTimesteps = getIntParameter("skip_timesteps");
   m_numObj = 0;
   m_objects.clear();
   if (!load(getStringParameter("filename"))) {
      std::cerr << "cannot open " << getStringParameter("filename") << std::endl;
   }
   return true;
}
