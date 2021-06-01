#include <iostream>
#include "filesystem.h"
#include "directory.h"
#include "findself.h"

namespace vistle {

namespace directory {

std::string build_type() {

#ifdef CMAKE_INTDIR
    const std::string btype = CMAKE_INTDIR;
#else
#ifdef _WIN32
    #ifdef CMAKE_BUILD_TYPE
const std::string btype = CMAKE_BUILD_TYPE;
#else
#ifdef _DEBUG
const std::string btype = "Debug";
#else
const std::string btype = "Release";
#endif
#endif
#else
    const std::string btype = "";
#endif
#endif

    return btype;
}

std::string prefix(int argc, char *argv[]) {

    return prefix(getbindir(argc, argv));
}

std::string prefix(const std::string &bindir) {

   namespace bf = vistle::filesystem;
   bf::path p(bindir);
   if (build_type().empty()) {
	   p += "/..";
   } else {
	   p += "/../..";
   }
   p = bf::canonical(p);

   return p.string();
}

std::string bin(const std::string &prefix) {

	if (build_type().empty()) {
		return prefix + "/bin";
	}

	return prefix + "/bin/" + build_type();
}

std::string module(const std::string &prefix) {

#ifdef MODULE_THREAD
    std::string moduleDir = "/lib/module";
#else
    std::string moduleDir = "/libexec/module";
#endif
    if (build_type().empty()) {
        return prefix + moduleDir;
    }
    return prefix + moduleDir + "/" + build_type();
}

std::string share(const std::string &prefix) {

    return prefix + "/share/vistle";
}

} // namespace directory

}
