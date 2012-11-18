#include <cstdio>
#include <cstdlib>

#include <sstream>
#include <iomanip>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <object.h>

#include "WriteArchive.h"

namespace ba = boost::archive;
using namespace vistle;

MODULE_MAIN(WriteArchive)

WriteArchive::WriteArchive(const std::string &shmname, int rank, int size, int moduleID)
   : Module("WriteArchive", shmname, rank, size, moduleID)
   , m_ofs(NULL)
   , m_binAr(NULL)
   , m_textAr(NULL)
   , m_xmlAr(NULL)
{

   createInputPort("grid_in");
   addIntParameter("format", 0);
   addFileParameter("filename", "vistle.archive");
}

WriteArchive::~WriteArchive() {

   close();
}

void WriteArchive::close() {

   delete m_binAr;
   m_binAr = NULL;
   delete m_textAr;
   m_textAr = NULL;
   delete m_xmlAr;
   m_xmlAr = NULL;

   if (m_ofs) {
      *m_ofs << std::endl;
      *m_ofs << "vistle separator" << std::endl;
   }
   delete m_ofs;
   m_ofs = NULL;
}

bool WriteArchive::compute() {

   int count = 0;
   bool trunc = false;
   bool end = false;
   std::string format;
   while (Object::const_ptr obj = takeFirstObject("grid_in")) {
      std::ios_base::openmode flags = std::ios::out;
      if (obj->hasAttribute("_mark_begin")) {
         close();
         trunc = true;
         flags |= std::ios::trunc;
      } else {
         flags |= std::ios::app;
      }
      if (obj->hasAttribute("_mark_end")) {
         end = true;
      }
      switch(getIntParameter("format")) {
         default:
         case 0:
            flags |= std::ios::binary;
            format = "binary";
            break;
         case 1:
            format = "text";
            break;
         case 2:
            format = "xml";
            break;
      }
      if (!m_ofs) {
         m_ofs = new std::ofstream(getFileParameter("filename").c_str(), flags);
      }
      if (trunc)
         *m_ofs << "vistle " << format << " 1 start" << std::endl;

      switch(getIntParameter("format")) {
         default:
         case 0:
         {
            if (!m_binAr)
               m_binAr = new ba::binary_oarchive(*m_ofs);
            obj->save(*m_binAr);
            break;
         }
         case 1:
         {
            if (!m_textAr)
               m_textAr = new ba::text_oarchive(*m_ofs);
            obj->save(*m_textAr);
            break;
         }
         case 2:
         {
            if (!m_xmlAr)
               m_xmlAr = new ba::xml_oarchive(*m_ofs);
            obj->save(*m_xmlAr);
            break;
         }
      }
      ++count;

      close();

      //*m_ofs << std::endl << "vistle separator" << std::endl;
   }
   if (trunc) {
      std::cerr << "saved";
   } else {
      std::cerr << "appended";
   }
   if (end) {
      std::cerr << " final";
      close();
   }
   std::cerr << " [" << count << "] to " << getFileParameter("filename") << " (" << format << ")" << std::endl;

   return true;
}
