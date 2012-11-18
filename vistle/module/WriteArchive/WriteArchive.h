#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <string.h>

#include <module.h>
#include <object.h>

class WriteArchive: public vistle::Module {

 public:
   WriteArchive(const std::string &shmname, int rank, int size, int moduleID);
   ~WriteArchive();

 private:
   virtual bool compute();

   void close();

   std::ofstream *m_ofs;
   boost::archive::binary_oarchive *m_binAr;
   boost::archive::text_oarchive *m_textAr;
   boost::archive::xml_oarchive *m_xmlAr;


};

#endif
