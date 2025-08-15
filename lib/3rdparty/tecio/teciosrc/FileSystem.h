 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <stdio.h>
#include <string>
 #if !defined(NO_THIRD_PARTY_LIBS)
#include <utility>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
 #endif
#include "ThirdPartyHeadersEnd.h"
#include "StandardIntegralTypes.h"
namespace tecplot { namespace filesystem { FILE* fileOpen(std::string const& ___1392, std::string const& ___2502); FILE* fileReopen(std::string const& ___1392, std::string const& ___2502, FILE* file); int   fileRename(std::string const& ___1392, std::string const& newFileName); int   fileRemove(std::string const& ___1392); bool  fileSize(char const* ___1392, uint64_t& sizeResult); bool  fileSize(std::string const& ___1392, uint64_t& sizeResult); std::string homeDirectory(); void convertPathDelimitersForOS(std::string&);
 #if !defined NO_THIRD_PARTY_LIBS
boost::filesystem::path makeAbsolute(boost::filesystem::path const& file, boost::filesystem::path const& basePath); std::pair<boost::filesystem::file_status,boost::system::error_code> fileStatus( boost::filesystem::path const& filePath, bool                           resolveSymlinks); bool fileExists(boost::filesystem::path const& filePath); bool dirExists(boost::filesystem::path const& dirPath); bool isFileOrDirWritable(boost::filesystem::path const& path);
 #endif
}}
