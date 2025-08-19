#include "FileSystem.h"
#include "ThirdPartyHeadersBegin.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include "ThirdPartyHeadersEnd.h"
#include "CodeContract.h"
 #if defined(_WIN32)
#include "UnicodeStringUtils.h"
 #endif
namespace tecplot { namespace filesystem { FILE* fileOpen(std::string const& ___1392, std::string const& ___2502) { REQUIRE(!___1392.empty()); REQUIRE(!___2502.empty());
 #if defined(_WIN32)
return _wfopen(tecplot::utf8ToWideString(___1392).c_str(), tecplot::utf8ToWideString(___2502).c_str());
 #else
return fopen(___1392.c_str(), ___2502.c_str());
 #endif
} FILE* fileReopen(std::string const& ___1392, std::string const& ___2502, FILE* file) { REQUIRE(!___1392.empty()); REQUIRE(!___2502.empty()); REQUIRE(VALID_REF(file));
 #if defined(_WIN32)
return _wfreopen(tecplot::utf8ToWideString(___1392).c_str(), tecplot::utf8ToWideString(___2502).c_str(), file);
 #else
return freopen(___1392.c_str(), ___2502.c_str(), file);
 #endif
} int fileRename(std::string const& ___1392, std::string const& newFileName) { REQUIRE(!___1392.empty()); REQUIRE(!newFileName.empty());
 #if defined(_WIN32)
return _wrename(tecplot::utf8ToWideString(___1392).c_str(), tecplot::utf8ToWideString(newFileName).c_str());
 #else
return rename(___1392.c_str(), newFileName.c_str());
 #endif
} int fileRemove(std::string const& ___1392) { REQUIRE(!___1392.empty());
 #if defined(_WIN32)
return _wremove(tecplot::utf8ToWideString(___1392).c_str());
 #else
return remove(___1392.c_str());
 #endif
} bool fileSize(char const* ___1392, uint64_t&   sizeResult) { bool ___3356 = false;
 #if defined(_WIN32)
struct _stat64 s; int statResult = _wstati64(tecplot::utf8ToWideString(___1392).c_str(), &s);
 #else
struct stat s; int statResult = stat(___1392, &s);
 #endif
if (statResult == 0) { ___3356 = true; sizeResult = static_cast<uint64_t>(s.st_size); } return ___3356; } bool fileSize(std::string const& ___1392, uint64_t&          sizeResult) { return fileSize(___1392.c_str(), sizeResult); } std::string homeDirectory() { std::string homedir; char const* home = std::getenv("HOME"); if (home && std::strlen(home)) { homedir = home; }
 #if defined(_WIN32)
else { home = std::getenv("USERPROFILE"); if (home && std::strlen(home)) { homedir = home; } else { char const* hdrive = std::getenv("HOMEDRIVE"); if(!(hdrive && std::strlen(hdrive))) { hdrive = "C:\\"; } char const* hpath = std::getenv("HOMEPATH"); if (hpath && std::strlen(hpath)) { homedir = std::string(hdrive) + hpath; } } }
 #endif
if (homedir.empty()) { char const* user = std::getenv("USERNAME"); if (!(user && std::strlen(user))) { user = std::getenv("USER"); } if (user && std::strlen(user)) {
 #if defined(_WIN32)
char const* hdrive = std::getenv("HOMEDRIVE"); if (!(hdrive && std::strlen(hdrive))) { hdrive = "C:\\"; } homedir = std::string(hdrive) + "Users\\" + user;
 #elif defined(__linux__)
homedir = std::string("/home/") + user;
 #elif defined(__APPLE__)
homedir = std::string("/Users/") + user;
 #endif
} } return homedir; } void convertPathDelimitersForOS(std::string& path) { std::replace(path.begin(), path.end()
 #if defined MSWIN
,'/'
 #else
,'\\'
 #endif
 #if defined MSWIN
,'\\'
 #else
,'/'
 #endif
); }
 #if !defined(NO_THIRD_PARTY_LIBS)
boost::filesystem::path makeAbsolute(boost::filesystem::path const& path, boost::filesystem::path const& basePath) { REQUIRE(!path.empty()); REQUIRE(!basePath.empty()); REQUIRE(boost::filesystem::is_directory(basePath)); return path.is_absolute() ? path : basePath / path; } std::pair<boost::filesystem::file_status,boost::system::error_code> fileStatus( boost::filesystem::path const& filePath, bool                           resolveSymlinks) { boost::system::error_code errorCode{}; auto const fileStatus{resolveSymlinks ? boost::filesystem::status(filePath, errorCode) : boost::filesystem::symlink_status(filePath, errorCode) }; return {fileStatus,errorCode}; } bool fileExists(boost::filesystem::path const& filePath) { REQUIRE("filepath can be valid or empty"); auto const& [status,errorCode] = fileStatus(filePath, true); return !errorCode && boost::filesystem::exists(status) && !boost::filesystem::is_directory(status); } bool dirExists(boost::filesystem::path const& dirPath) { REQUIRE("dirpath can be anything - including empty"); auto const& [status,errorCode] = fileStatus(dirPath, true); return !errorCode && boost::filesystem::exists(status) && boost::filesystem::is_directory(status); } bool isFileOrDirWritable(boost::filesystem::path const& path) { auto const isFileReadWritable{[](boost::filesystem::path filePath, bool ___3331){ auto filename{filePath.make_preferred().string()}; auto* ___1479{fileOpen(filename, "a+")}; bool const isReadWritable{___1479 != nullptr}; if (isReadWritable) fclose(___1479); if (___3331) (void)std::remove(filename.c_str()); return isReadWritable; }}; auto const& [pathStatus,pathErrorCode]{fileStatus(path, true)}; if (!pathErrorCode && boost::filesystem::exists(pathStatus)) { if (boost::filesystem::is_directory(pathStatus)) { boost::system::error_code uniquePathErrorCode{}; auto const tempFilePath{boost::filesystem::unique_path( path/"%%%%-%%%%-%%%%-%%%%", uniquePathErrorCode)}; return !uniquePathErrorCode && isFileReadWritable(tempFilePath, true); } else { return isFileReadWritable(path, false); } } else { return isFileReadWritable(path, true); } }
 #endif
}}
