#ifndef UTIL_FILESYSTEM_H
#define UTIL_FILESYSTEM_H

//#ifdef _WIN32
//#include <filesystem>
//#else
#include <boost/filesystem.hpp>
//#endif

namespace vistle {

//#ifdef _WIN32
//namespace filesystem = std::experimental::filesystem;
//#else
namespace filesystem = boost::filesystem;
//#endif

} // namespace vistle
#endif
