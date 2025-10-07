#include <iostream>
#include <fstream>
#include "filesystem.h"
#include "directory.h"
#include "findself.h"

namespace vistle {

namespace directory {

std::string build_type()
{
#ifdef CMAKE_INTDIR
    return CMAKE_INTDIR;
#endif
    return "";
}

namespace {

std::string prefix(const std::string &bindir, const std::string &buildtype)
{
    namespace bf = vistle::filesystem;
    bf::path p(bindir);
    p += "/..";
    if (!buildtype.empty()) {
        p += "/..";
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

std::string covisedir(const std::string &prefix)
{
    if (auto cd = getenv("COVISEDIR")) {
        return cd;
    }

    namespace bf = vistle::filesystem;

    auto dir = bf::path(prefix);
    for (;;) {
        auto covisedir = dir / "covise";
        if (bf::exists(covisedir)) {
            return covisedir.string();
        }
        if (!dir.has_parent_path()) {
            break;
        }
        dir = dir.parent_path();
    }
    return "";
}

} // namespace

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

static bool setvar(const std::string &varval)
{
    char *cvv = strdup(varval.c_str());
    int retval = putenv(cvv);
    if (retval != 0) {
        std::cerr << "failed to set " << varval << ": " << strerror(errno) << std::endl;
        free(cvv);
        return false;
    }
    //std::cerr << varval << std::endl;
    return true;
}

static bool setvar(const std::string &var, const std::string &val)
{
    std::string vv = var + "=" + val;
    return setvar(vv);
}

static bool addpath(const char *var, const std::string &add)
{
    auto p = getenv(var);
    if (!p) {
        return setvar(var, add);
    }

#ifdef _WIN32
    return setvar(var, add + ";" + p);
#else
    return setvar(var, add + ":" + p);
#endif
}

bool setVistleRoot(const std::string &vistleRootDir, const std::string &buildtype)
{
    //std::cerr << "setting VISTLE_ROOT to " << vistleRootDir << ", VISTLE_BUILDTYPE to " << buildtype << std::endl;
    return setvar("VISTLE_ROOT", vistleRootDir) && setvar("VISTLE_BUILDTYPE", buildtype);
}

bool setEnvironment(const std::string &prefix)
{
#ifdef __APPLE__
    std::string libpath = "DYLD_LIBRARY_PATH";
    auto pathadd = bin(prefix) + ":" + "/opt/homebrew/bin";
#else
    std::string libpath = "LD_LIBRARY_PATH";
    auto pathadd = bin(prefix);
#endif
    setvar(libpath, prefix + "/lib");
    addpath("PATH", pathadd);

    setvar("COVISEDIR", covisedir(prefix)); // for finding COVER and example data
    addpath("COVISE_PATH", prefix); // for finding Vistle COVER plugin

    auto envfile = std::ifstream(prefix + "/vistle-env.txt");
    while (envfile.good()) {
        std::string line;
        std::getline(envfile, line);
        setvar(line);
    }
    return true;
}

} // namespace directory

Directory::Directory(int argc, char *argv[])
{
    if (auto buildtype = getenv("VISTLE_BUILDTYPE")) {
        m_buildType = buildtype;
    } else {
        m_buildType = directory::build_type();
    }

    if (auto prefix = getenv("VISTLE_ROOT")) {
        m_prefix = prefix;
    } else {
        auto bindir = getbindir(argc, argv);
        while (!bindir.empty() && bindir.back() == '/')
            bindir.pop_back();
        if (bindir.length() < m_buildType.length()) {
            m_buildType.clear();
        } else if (bindir.rfind(m_buildType) != bindir.length() - m_buildType.length()) {
            m_buildType.clear();
        }
        m_prefix = directory::prefix(bindir, m_buildType);
    }

    canonical();
}

Directory::Directory(const std::string &prefix, const std::string &buildtype): m_prefix(prefix), m_buildType(buildtype)
{
    canonical();
}

Directory::Directory()
: Directory(getenv("VISTLE_ROOT") ? getenv("VISTLE_ROOT") : "",
            getenv("VISTLE_BUILDTYPE") ? getenv("VISTLE_BUILDTYPE") : "")
{}

void Directory::canonical()
{
    if (m_prefix.empty() || m_prefix.back() != '/')
        m_prefix += "/";

    if (!m_buildType.empty())
        m_buildTypeSuffix = m_buildType + "/";
}

std::string Directory::buildType() const
{
    return m_buildType;
}

std::string Directory::prefix() const
{
    return m_prefix;
}

std::string Directory::bin() const
{
    return m_prefix + "bin/" + m_buildTypeSuffix;
}

std::string Directory::module() const
{
#ifdef MODULE_THREAD
    std::string moduleDir = "lib/module/";
#else
    std::string moduleDir = "libexec/module/";
#endif
    return m_prefix + moduleDir + m_buildTypeSuffix;
}

std::string Directory::moduleplugin() const
{
    std::string moduleDir = "lib/module/";
    return m_prefix + moduleDir + m_buildTypeSuffix;
}


std::string Directory::share() const
{
    return m_prefix + "share/vistle/";
}

std::string Directory::covisedir() const
{
    return directory::covisedir(prefix());
}

} // namespace vistle
