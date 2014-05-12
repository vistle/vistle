#include <iostream>
#include <boost/filesystem.hpp>
#include "directory.h"

namespace vistle {

bool scanModules(const std::string &dir, int hub, AvailableMap &available) {

   namespace bf = boost::filesystem;
   bf::path p(dir);
   try {
      if (!bf::is_directory(p)) {
         std::cerr << "scanModules: " << dir << " is not a directory" << std::endl;
         return false;
      }
   } catch (const bf::filesystem_error &e) {
      std::cerr << "scanModules: error in" << dir << ": " << e.what() << std::endl;
      return false;
   }

   for (bf::directory_iterator it(p);
         it != bf::directory_iterator();
         ++it) {

      bf::path ent(*it);
      std::string stem = ent.stem().string();
      if (stem.size() > ModuleNameLength) {
         std::cerr << "scanModules: skipping " << stem << " - name too long" << std::endl;
      }

      std::string ext = ent.extension().string();

      AvailableModule mod;
      mod.name = stem;
      mod.path = bf::path(*it).string();

      AvailableModule::Key key(hub, stem);
      auto prev = available.find(key);
      if (prev != available.end()) {
         std::cerr << "scanModules: overriding " << stem << ", " << prev->second.path << " -> " << mod.path << std::endl;
      }
      available[key] = mod;
   }

   return true;
}

}
