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

WriteArchive::WriteArchive(int rank, int size, int moduleID)
   : Module("WriteArchive", rank, size, moduleID) {

   createInputPort("grid_in");
   addFileParameter("filename", "");
}

void WriteArchive::save(const std::string & name, vistle::Object * object) {

   std::ofstream ofs(name.c_str());
   ba::text_oarchive oa(ofs);

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
   oa << object;

   std::cout << "saved [" << name << "]" << std::endl;
}

bool WriteArchive::compute() {

   std::list<vistle::Object *> objects = getObjects("grid_in");

   if (objects.size() == 1) {
      save(getFileParameter("filename"), objects.front());
   }

   return true;
}
