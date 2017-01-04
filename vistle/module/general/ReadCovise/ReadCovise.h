#ifndef READCOVISE_H
#define READCOVISE_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/module.h>

typedef std::vector<std::pair<std::string, std::string> > AttributeList;
struct Element {
   Element()
      : parent(NULL)
      , referenced(NULL)
      , is_timeset(false)
      , in_geometry(false)
      , objnum(-1)
      , index(-1)
      , offset(0)
   {
   }

   Element(const Element &other)
      : parent(other.parent)
      , referenced(other.referenced)
      , obj(other.obj)
      , is_timeset(other.is_timeset)
      , in_geometry(other.in_geometry)
      , objnum(other.objnum)
      , index(other.index)
      , offset(other.offset)
      , subelems(other.subelems)
      , attribs(other.attribs)
   {
   }

   Element &operator=(const Element &rhs) {
      if (&rhs != this) {
         parent = rhs.parent;
         referenced = rhs.referenced;
         obj = rhs.obj;
         is_timeset = rhs.is_timeset;
         in_geometry = rhs.in_geometry;
         objnum = rhs.objnum;
         index = rhs.index;
         offset = rhs.offset;
         subelems = rhs.subelems;
         attribs = rhs.attribs;
      }
      return *this;
   }

   Element *parent;
   Element *referenced;
   vistle::Object::ptr obj;
   bool is_timeset;
   bool in_geometry;
   ssize_t objnum;
   int index, block;
   off_t offset;
   std::vector<Element *> subelems;
   AttributeList attribs;

};

class ReadCovise: public vistle::Module {

 public:
   ReadCovise(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadCovise();

 private:
   bool readSkeleton(const int fd, Element *elem);
   AttributeList readAttributes(const int fd);
   void applyAttributes(vistle::Object::ptr obj, const Element &elem, int index=-1);

   bool readSETELE(const int fd, Element *parent);
   vistle::Object::ptr readGEOTEX(const int fd, bool skeleton, Element *elem, int timestep);
   vistle::Object::ptr readUNIGRD(const int fd, bool skeleton);
   vistle::Object::ptr readRCTGRD(const int fd, bool skeleton);
   vistle::Object::ptr readSTRGRD(const int fd, bool skeleton);
   vistle::Object::ptr readUNSGRD(const int fd, bool skeleton);
   vistle::Object::ptr readUSTSDT(const int fd, bool skeleton);
   vistle::Object::ptr readSTRSDT(const int fd, bool skeleton);
   vistle::Object::ptr readPOLYGN(const int fd, bool skeleton);
   vistle::Object::ptr readLINES(const int fd, bool skeleton);
   vistle::Object::ptr readPOINTS(const int fd, bool skeleton);
   vistle::Object::ptr readUSTVDT(const int fd, bool skeleton);
   vistle::Object::ptr readSTRVDT(const int fd, bool skeleton);
   vistle::Object::ptr readOBJREF(const int fd, bool skeleton, Element *elem);

   bool readRecursive(const int fd, Element *elem, int timestep);
   void deleteRecursive(Element &elem);
   vistle::Object::ptr readObject(const int fd, Element *elem, int timestep);
   vistle::Object::ptr readObjectIntern(const int fd, bool skeleton, Element *elem, int timestep);

   bool load(const std::string & filename);

   virtual bool compute();

   size_t m_numObj;
   std::vector<Element *> m_objects;
   int m_firstTimestep, m_skipTimesteps;
};

#endif
