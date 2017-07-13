#include <iostream>
#include "filesystem.h"
#include "directory.h"
#include "findself.h"

namespace vistle {

#ifdef CMAKE_INTDIR
const std::string build_type = CMAKE_INTDIR;
#else
#ifdef _WIN32
#ifdef CMAKE_BUILD_TYPE
const std::string build_type = CMAKE_BUILD_TYPE;
#else
#ifdef _DEBUG
const std::string build_type = "Debug";
#else
const std::string build_type = "Release";
#endif
#endif
#else
const std::string build_type = "";
#endif
#endif

bool scanModules(const std::string &dir, int hub, AvailableMap &available) {

   namespace bf = vistle::filesystem;
   bf::path p(dir);
   if (!build_type.empty()) {
#ifdef MODULE_THREAD
       std::cerr << dir + "/../../../lib/module/" + build_type << std::endl;
       p = dir + "/../../../lib/module/" + build_type;
#else
	   std::cerr << dir + "/../../../libexec/module/" + build_type << std::endl;
	   p = dir + "/../../../libexec/module/" + build_type;
#endif
   }
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

#ifdef MODULE_THREAD
      std::string ext = ent.extension().string();
#ifdef _WIN32
      if (ext != ".dll") {
          //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
          continue;
      }
#else
      if (ext != ".so") {
          //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
          continue;
      }
#endif
#else
#ifdef _WIN32
      std::string ext = ent.extension().string();
	  if (ext != ".exe") {
		  //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
		  continue;
	  }
#endif
#endif

      AvailableModule mod;
      mod.hub = hub;
#ifdef MODULE_THREAD
      if (stem.find("lib") == 0)
          mod.name = stem.substr(3);
      else
          mod.name = stem;
#else
      mod.name = stem;
#endif
      mod.path = bf::path(*it).string();

      AvailableModule::Key key(hub, mod.name);
      //std::cerr << "scanModules: new " << mod.name << " on hub " << hub << std::endl;
      auto prev = available.find(key);
      if (prev != available.end()) {
         std::cerr << "scanModules: overriding " << mod.name << ", " << prev->second.path << " -> " << mod.path << std::endl;
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
   if (build_type.empty()) {
	   p += "/..";
   } else {
	   p += "/../..";
   }
   p = bf::canonical(p);

   return p.string();
}

std::string bin(const std::string &prefix) {

	if (build_type.empty()) {
		return prefix + "/bin";
	}

	return prefix + "/bin/" + build_type;
}

std::string module(const std::string &prefix) {

#ifdef MODULE_THREAD
    if (build_type.empty()) {
        return prefix + "/lib/module";
    }

    return prefix + "/" + build_type + "/lib/module";
#else
	if (build_type.empty()) {
		return prefix + "/libexec/module";
	}

	return prefix + "/" + build_type + "/libexec/module";
#endif
}

std::string share(const std::string &prefix) {

    return prefix + "/share/vistle";
}

} // namespace directory

}
