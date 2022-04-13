#include <iostream>
#include "filesystem.h"
#include "directory.h"
#include "findself.h"

namespace vistle {

namespace directory {

std::string build_type()
{
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

std::string prefix(int argc, char *argv[])
{
    return prefix(getbindir(argc, argv));
}

std::string prefix(const std::string &bindir)
{
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

std::string bin(const std::string &prefix)
{
    if (build_type().empty()) {
        return prefix + "/bin";
    }

    return prefix + "/bin/" + build_type();
}

std::string module(const std::string &prefix)
{
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

std::string share(const std::string &prefix)
{
    return prefix + "/share/vistle";
}

std::string configHome()
{
#ifdef _WIN32
    auto var = "LOCALAPPDATA";
#else
    auto var = "XDG_CONFIG_HOME";
#endif
    if (const char *CONFIG = getenv(var)) {
        return CONFIG + std::string("/vistle");
    } else if (const char *HOME = getenv("HOME")) {
        return HOME + std::string("/.config/vistle");
    }
    return "";
}
bool setVistleRoot(const std::string &vistleRootDir)
{
    std::string vr = "VISTLE_ROOT=" + vistleRootDir;
    //std::cerr << "setting VISTLE_ROOT to " << vr << std::endl;
    char *vistleRoot = strdup(vr.c_str());
    int retval = putenv(vistleRoot);
    if (retval != 0) {
        std::cerr << "failed to set VISTLE_ROOT: " << strerror(errno) << std::endl;
        delete[] vistleRoot;
        return false;
    }
    return true;
}

} // namespace directory

} // namespace vistle
