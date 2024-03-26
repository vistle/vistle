#include "version.h"

#include <boost/preprocessor.hpp>
#include <sstream>


#ifndef VISTLE_VERSION_TAG
#error "VISTLE_VERSION_TAG not defined"
#endif

#ifndef VISTLE_VERSION_HASH
#error "VISTLE_VERSION_HASH not defined
#endif

namespace vistle {
namespace version {

std::string tag()
{
    return BOOST_PP_STRINGIZE(VISTLE_VERSION_TAG);
}

std::string hash()
{
    return BOOST_PP_STRINGIZE(VISTLE_VERSION_HASH);
}

std::string string()
{
    return tag() + "-" + hash();
}

std::string copyright()
{
    return "2012 - 2024, the Vistle authors";
}

std::string license()
{
    return "LGPL-2.1-or-later";
}

std::string flags()
{
    std::stringstream str;
    str
#ifdef MODULE_THREAD
        << "single-process"
#else
        << "multi-process"
#endif

#ifdef MODULE_STATIC
        << " static"
#endif
#ifndef NO_SHMEM
        << " shm"
#endif

#ifdef VISTLE_USE_CUDA
        << " cuda"
#endif
#ifdef VISTLE_SCALAR_DOUBLE
        << " double"
#else
        << " float"
#endif
#ifdef VISTLE_INDEX_64BIT
        << " idx64"
#else
        << " idx32"
#endif
        ;
    return str.str();
}

std::string os()
{
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "apple";
#elif defined(__linux__)
    return "linux";
#elif defined(__FreeBSD__)
    return "freebsd";
#else
    return "other";
#endif
}

std::string arch()
{
#if defined(__aarch64__)
    return "ARM64";
#elif defined(__arm__)
    return "ARM";
#elif defined(__x86_64)
    return "AMD64";
#elif defined(__i386__)
    return "x86";
#else
    return "Unknown";
#endif
}

std::string homepage()
{
    return "https://vistle.io";
}

std::string github()
{
    return "https://github.com/vistle/vistle";
}

std::string banner()
{
    std::stringstream str;
    str << "Vistle version: " << version::string() << std::endl;
    str << "         flags: " << version::flags() << std::endl;
    str << "      platform: " << version::os() << " " << version::arch() << std::endl;
    str << "   " << version::copyright() << " - " << version::homepage() << std::endl;
    return str.str();
}

} // namespace version
} // namespace vistle
