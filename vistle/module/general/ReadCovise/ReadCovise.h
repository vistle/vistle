#ifndef READCOVISE_H
#define READCOVISE_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/reader.h>

typedef std::vector<std::pair<std::string, std::string> > AttributeList;
struct Element {
   Element()
      : parent(NULL)
      , referenced(NULL)
      , is_timeset(false)
      , in_geometry(false)
      , objnum(-1)
      , index(-1)
      , block(-1)
      , offset(0)
   {
   }

   Element(const Element &other)
      : parent(other.parent)
      , referenced(other.referenced)
      , type(other.type)
      , obj(other.obj)
      , is_timeset(other.is_timeset)
      , in_geometry(other.in_geometry)
      , objnum(other.objnum)
      , index(other.index)
      , block(other.block)
      , offset(other.offset)
      , subelems(other.subelems)
      , attribs(other.attribs)
   {
   }

   Element &operator=(const Element &rhs) {
      if (&rhs != this) {
         parent = rhs.parent;
         referenced = rhs.referenced;
         type = rhs.type;
         obj = rhs.obj;
         is_timeset = rhs.is_timeset;
         in_geometry = rhs.in_geometry;
         objnum = rhs.objnum;
         index = rhs.index;
         block = rhs.block;
         offset = rhs.offset;
         subelems = rhs.subelems;
         attribs = rhs.attribs;
      }
      return *this;
   }

   Element *parent;
   Element *referenced;
   std::string type;
   vistle::Object::ptr obj;
   bool is_timeset;
   bool in_geometry;
   ssize_t objnum;
   int index, block;
   off_t offset;
   std::vector<Element *> subelems;
   AttributeList attribs;

};

class ReadCovise: public vistle::Reader {

   static const int NumPorts = 4;

 public:
   ReadCovise(const std::string &name, int moduleID, mpi::communicator comm);
   ~ReadCovise() override;

   virtual bool examine(const vistle::Parameter *param) override;
   virtual bool prepareRead() override;
   virtual bool read(Token &token, int timestep=-1, int block=-1) override;
   virtual bool finishRead() override;

 private:
   bool readSkeleton(const int port, Element *elem);
   AttributeList readAttributes(const int fd);
   void applyAttributes(Token &token, vistle::Object::ptr obj, const Element &elem, int index=-1);

   bool readSETELE(Token &token, const int port, int fd, Element *parent);
   vistle::Object::ptr readGEOTEX(Token &token, const int port, int fd, bool skeleton, Element *elem, int timestep);
   vistle::Object::ptr readUNIGRD(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readRCTGRD(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readSTRGRD(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readUNSGRD(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readUSTSDT(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readSTRSDT(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readPOLYGN(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readLINES(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readPOINTS(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readUSTVDT(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readSTRVDT(Token &token, const int port, int fd, bool skeleton);
   vistle::Object::ptr readOBJREF(Token &token, const int port, int fd, bool skeleton, Element *elem);

   bool readRecursive(Token &token, int fd[], Element *elem[], int timestep, int targetTimestep);
   void deleteRecursive(Element &elem);
   vistle::Object::ptr readObject(Token &token, const int port, int fd, Element *elem, int timestep);
   vistle::Object::ptr readObjectIntern(Token &token, const int port, int fd, bool skeleton, Element *elem, int timestep);

   //bool load(const std::string & filename);

#if 0
   virtual bool compute();
#endif

   vistle::StringParameter *m_gridFile = nullptr;
   vistle::StringParameter *m_fieldFile[NumPorts];
   vistle::Port *m_out[NumPorts];

   int m_fd[NumPorts];
   std::string m_filename[NumPorts];
   Element m_rootElement[NumPorts];
   size_t m_numObj[NumPorts];
   int m_numTime[NumPorts];
   std::vector<Element *> m_objects[NumPorts];
};

#endif
