#include <cstdio>
#include <cstdlib>

#include <sstream>
#include <iomanip>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <object.h>

#include "WriteArchive.h"

namespace ba = boost::archive;
using namespace vistle;

MODULE_MAIN(WriteArchive)

WriteArchive::WriteArchive(const std::string &shmname, int rank, int size, int moduleID)
   : Module("WriteArchive", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   addFileParameter("filename", "vistle.archive");
}

void WriteArchive::save(const std::string & name, vistle::Object::const_ptr object) {

#if 0
   int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
   if (fd == -1) {
      std::cout << "ERROR WriteArchive::save could not open file [" << name
                << "]" << std::endl;
      return;
   }
#endif

#if 0
   catalogue c;
   createCatalogue(object, c);
   printCatalogue(c);

   write_char(fd, (char *) "VISTLE", 6);
   header h('l', 1, 0, 0);
   write_char(fd, (char *) &h, sizeof(header));

   saveCatalogue(fd, c);
#endif
   //saveItemInfo(oa, object);
}

bool WriteArchive::compute() {

#if 0
   ObjectList objects = getObjects("grid_in");

   if (objects.size() == 1) {
      std::cerr << "saving to " << getFileParameter("filename") << std::endl;
      save(getFileParameter("filename"), objects.front());
   } else {
      std::cerr << "NOT saving to " << getFileParameter("filename") << " (" << objects.size() << " objects)" << std::endl;
   }
#else
   std::cerr << "saving to " << getFileParameter("filename") << std::endl;
   std::ofstream ofs(getFileParameter("filename").c_str(), std::ios::out | std::ios::app);
   ba::text_oarchive oa(ofs);
   size_t objcount = 0;
   while (Object::const_ptr obj = takeFirstObject("grid_in")) {
      oa << *obj;
      ++objcount;
      //save(getFileParameter("filename"), obj);
   }
   std::cout << "saved #" << objcount << " [" << name << "]" << std::endl;

#endif

   return true;
}
