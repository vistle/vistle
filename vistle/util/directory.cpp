#include <iostream>
#include "filesystem.h"
#include "directory.h"
#include "findself.h"

namespace vistle {

#ifdef _WIN32
#ifdef CMAKE_BUILD_TYPE
//const std::string build_type = CMAKE_BUILD_TYPE;
const std::string build_type = "RelWithDebInfo";
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

bool scanModules(const std::string &dir, int hub, AvailableMap &available) {

   namespace bf = vistle::filesystem;
   bf::path p(dir);
   if (!build_type.empty()) {
	   std::cerr << dir + "/../../../libexec/module/" + build_type << std::endl;
	   p = dir + "/../../../libexec/module/" + build_type;
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

#ifdef _WIN32
      std::string ext = ent.extension().string();
	  if (ext != ".exe") {
		  //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
		  continue;
	  }
#endif

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

	if (build_type.empty()) {
		return prefix + "/libexec/module";
	}

	return prefix + "/" + build_type + "/libexec/module";
}

std::string share(const std::string &prefix) {

    return prefix + "/share/vistle";
}

} // namespace directory

}
