#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "FileIOStream.h"
#include "FileSystem.h"
namespace tecplot { namespace ___3931 { FileIOStream::FileIOStream(std::string const& ___1392) : m_fileHandle(NULL) , ___2459(___1392) , m_isAscii(false) , m_dataFileType(___843) { REQUIRE(!___2459.empty()); } FileIOStream::~FileIOStream() { close(true); } ___372 FileIOStream::___2039() const { return m_fileHandle != NULL; } ___372 FileIOStream::close(bool ___3359) { ___372 ___3356 = ___4224; if (___2039()) { ___4193(m_fileHandle); m_fileHandle = NULL; if (!___3359) { ___476(!___1392().empty()); if (remove(___1392().c_str()) != 0) ___3356 = ___1303; }
 #ifdef PROFILE_FILE_ACCESS
m_statistics.numFSeeksPerformed = 0; m_statistics.numReadWritesPerformed = 0; m_statistics.___2778 = 0;
 #endif
} else { ___3356 = ___1303; } ENSURE(VALID_BOOLEAN(___3356)); return ___3356; } ___1391 FileIOStream::fileLoc() { REQUIRE(___2039()); ___1391 fileLoc = ___1391(___4199(m_fileHandle)); ENSURE(fileLoc != ___330); return fileLoc; } ___372 FileIOStream::___3458() { REQUIRE(___2039());
 #ifdef PROFILE_FILE_ACCESS
m_statistics.numFSeeksPerformed++;
 #endif
return ___4198(m_fileHandle, ___1391(0), SEEK_SET) == 0; } ___372 FileIOStream::___3457(___1391 fileLoc) { REQUIRE(___2039()); REQUIRE(fileLoc != ___330);
 #ifdef PROFILE_FILE_ACCESS
m_statistics.numFSeeksPerformed++;
 #endif
return ___4198(m_fileHandle, fileLoc, SEEK_SET) == 0; } ___372 FileIOStream::seekToFileEnd() { REQUIRE(___2039());
 #ifdef PROFILE_FILE_ACCESS
m_statistics.numFSeeksPerformed++;
 #endif
return ___4198(m_fileHandle, ___1391(0), SEEK_END) == 0; } std::string const& FileIOStream::___1392() const { return ___2459; } void FileIOStream::___3492(___372 ___2000) { REQUIRE(VALID_BOOLEAN(___2000)); m_isAscii = (___2000 == ___4224); } ___372 FileIOStream::___2000() const { return m_isAscii; } void FileIOStream::setDataFileType(DataFileType_e ___842) { REQUIRE(VALID_ENUM(___842, DataFileType_e)); m_dataFileType = ___842; } DataFileType_e FileIOStream::___842() const { return m_dataFileType; } class FileIOStatistics& FileIOStream::statistics() { return m_statistics; } ___372 FileIOStream::open(std::string const& ___2502) { REQUIRE(!___1392().empty()); REQUIRE(!___2039()); m_fileHandle = tecplot::filesystem::fileOpen(___1392(), ___2502); ___372 ___2037 = m_fileHandle != NULL;
 #ifdef PROFILE_FILE_ACCESS
___476(m_statistics.numFSeeksPerformed == 0 && m_statistics.numReadWritesPerformed == 0 && m_statistics.___2778 == 0);
 #endif
return ___2037; } FILE* FileIOStream::handle() const { return m_fileHandle; } }}
