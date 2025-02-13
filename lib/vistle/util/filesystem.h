#ifndef VISTLE_UTIL_FILESYSTEM_H
#define VISTLE_UTIL_FILESYSTEM_H

//#ifdef _WIN32
//#define USE_STD_FILESYSTEM
//#endif

#ifdef USE_STD_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem.hpp>
#endif

namespace vistle {

#ifdef USE_STD_FILESYSTEM
namespace filesystem = std::filesystem;
#else
namespace filesystem = boost::filesystem;
#endif

} // namespace vistle
#endif
