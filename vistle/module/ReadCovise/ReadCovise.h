#ifndef READCOVISE_H
#define READCOVISE_H

#include <string.h>
#include "module.h"

typedef std::vector<std::pair<std::string, std::string> > AttributeList;
struct Element {
   Element()
      : parent(NULL)
      , index(-1)
      , offset(0)
   {
   }

   Element(const Element &other)
      : parent(other.parent)
      , index(other.index)
      , offset(other.offset)
      , subelems(other.subelems)
      , attribs(other.attribs)
   {
   }

   Element &operator=(const Element &rhs) {
      if (&rhs != this) {
         parent = rhs.parent;
         index = rhs.index;
         offset = rhs.offset;
         subelems = rhs.subelems;
         attribs = rhs.attribs;
      }
      return *this;
   }

   Element *parent;
   int index;
   off_t offset;
   std::vector<Element *> subelems;
   AttributeList attribs;

};

class ReadCovise: public vistle::Module {

 public:
   ReadCovise(const std::string &shmname, int rank, int size, int moduleID);
   ~ReadCovise();

 private:
   bool readSkeleton(const int fd, Element *elem);
   AttributeList readAttributes(const int fd);
   void applyAttributes(vistle::Object::ptr obj, const Element &elem, int index=-1);

   bool readSETELE(const int fd, Element *parent);
   vistle::Object::ptr readGEOTEX(const int fd, bool skeleton, Element *elem);
   vistle::Object::ptr readUNSGRD(const int fd, bool skeleton);
   vistle::Object::ptr readUSTSDT(const int fd, bool skeleton);
   vistle::Object::ptr readPOLYGN(const int fd, bool skeleton);
   vistle::Object::ptr readUSTVDT(const int fd, bool skeleton);

   bool readRecursive(const int fd, const Element &elem);
   void deleteRecursive(Element &elem);
   vistle::Object::ptr readObject(const int fd, const Element &elem);
   vistle::Object::ptr readObjectIntern(const int fd, bool skeleton, Element *elem);

   bool load(const std::string & filename);

   virtual bool compute();
};

#endif
