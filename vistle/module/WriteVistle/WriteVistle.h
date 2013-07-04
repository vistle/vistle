#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <string>

#include <module/module.h>
#include <core/object.h>

class WriteVistle: public vistle::Module {

 public:
   WriteVistle(const std::string &shmname, int rank, int size, int moduleID);
   ~WriteVistle();

 private:
   virtual bool compute();

   void close();

   std::ofstream *m_ofs;
   boost::archive::binary_oarchive *m_binAr;
   boost::archive::text_oarchive *m_textAr;
   boost::archive::xml_oarchive *m_xmlAr;


};

#endif
