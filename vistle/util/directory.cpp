#include <iostream>
#include "filesystem.h"
#include "directory.h"
#include "findself.h"

namespace vistle {

bool scanModules(const std::string &dir, int hub, AvailableMap &available) {

   namespace bf = vistle::filesystem;
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

   p = bf::canonical(p);

   for (bf::directory_iterator it(p);
         it != bf::directory_iterator();
         ++it) {

      bf::path ent(*it);
      std::string stem = ent.stem().string();
      if (stem.size() > ModuleNameLength) {
         std::cerr << "scanModules: skipping " << stem << " - name too long" << std::endl;
         continue;
      } 
      if (stem.empty()) {
         continue;
      }

      std::string ext = ent.extension().string();

      AvailableModule mod;
      mod.hub = hub;
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

namespace directory {

std::string prefix(int argc, char *argv[]) {

    return prefix(getbindir(argc, argv));
}

std::string prefix(const std::string &bindir) {

   namespace bf = vistle::filesystem;
   bf::path p(bindir);
#ifdef WIN32
   p += "/../..";
#else
   p += "/..";
#endif
   p = bf::canonical(p);

   return p.string();
}

std::string bin(const std::string &prefix) {

#ifdef WIN32
#ifdef _DEBUG
    return prefix + "/Debug/bin";
#else
    return prefix + "/Release/bin";
#endif
#else
    return prefix + "/bin";
#endif
}

std::string module(const std::string &prefix) {

#ifdef WIN32
#ifdef _DEBUG
    return prefix + "/Debug/libexec/module";
#else
    return prefix + "/Release/libexec/module";
#endif
#else
    return prefix + "/libexec/module";
#endif

}

std::string share(const std::string &prefix) {

    return prefix + "/share/vistle";
}

} // namespace directory

}
