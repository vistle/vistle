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
   : Module("WriteArchive", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   addIntParameter("format", 0);
   addFileParameter("filename", "vistle.archive");
}

bool WriteArchive::compute() {

   int count = 0;
   bool trunc = false;
   std::string format;
   while (Object::const_ptr obj = takeFirstObject("grid_in")) {
      std::ios_base::openmode flags = std::ios::out;
      if (obj->hasAttribute("mark_begin")) {
         trunc = true;
         flags |= std::ios::trunc;
      } else {
         flags |= std::ios::app;
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
      std::ofstream ofs(getFileParameter("filename").c_str(), flags);
      switch(getIntParameter("format")) {
         default:
         case 0:
         {
            ba::binary_oarchive oa(ofs);
            obj->serialize(oa);
            break;
         }
         case 1:
         {
            ba::text_oarchive oa(ofs);
            obj->serialize(oa);
            break;
         }
         case 2:
         {
            ba::xml_oarchive oa(ofs);
            obj->serialize(oa);
            break;
         }
      }
      ++count;
   }
   if (trunc) {
      std::cerr << "saved";
   } else {
      std::cerr << "appended";
   }
   std::cerr << " [" << count << "] to " << getFileParameter("filename") << " (" << format << ")" << std::endl;

   return true;
}
