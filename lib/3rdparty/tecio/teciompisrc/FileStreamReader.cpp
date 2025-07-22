#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "showMessage.h"
#include "FileStreamReader.h"
namespace tecplot { namespace ___3931 { FileStreamReader::FileStreamReader(std::string const& ___1392) : m_fileIOStream(___1392) { } FileStreamReader::~FileStreamReader() { m_fileIOStream.close(true); } ___372 FileStreamReader::___2039() const { return m_fileIOStream.___2039(); } ___372 FileStreamReader::open() { REQUIRE(!___2039()); ___372 ___2037 = m_fileIOStream.open("rb"); if (!___2037) ___1184("Cannot read file %s", ___1392().c_str());
 #ifdef PROFILE_FILE_ACCESS
___476(statistics().numFSeeksPerformed == 0 && statistics().numReadWritesPerformed == 0 && statistics().___2778 == 0);
 #endif
return ___2037; } ___372 FileStreamReader::close(bool ___3359) { return m_fileIOStream.close(___3359); } ___1391 FileStreamReader::fileLoc() { REQUIRE(___2039()); return m_fileIOStream.fileLoc(); } ___372 FileStreamReader::___3458() { REQUIRE(___2039()); return m_fileIOStream.___3458(); } ___372 FileStreamReader::___3457(___1391 fileLoc) { REQUIRE(___2039()); REQUIRE(fileLoc != ___330); return m_fileIOStream.___3457(fileLoc); } ___372 FileStreamReader::seekToFileEnd() { REQUIRE(___2039()); return m_fileIOStream.seekToFileEnd(); } std::string const& FileStreamReader::___1392() const { return m_fileIOStream.___1392(); } void FileStreamReader::___3492(___372 ___2000) { REQUIRE(VALID_BOOLEAN(___2000)); m_fileIOStream.___3492(___2000); } ___372 FileStreamReader::___2000() const { return m_fileIOStream.___2000(); } void FileStreamReader::setDataFileType(DataFileType_e ___842) { REQUIRE(VALID_ENUM(___842, DataFileType_e)); m_fileIOStream.setDataFileType(___842); } DataFileType_e FileStreamReader::___842() const { return m_fileIOStream.___842(); } class FileIOStatistics& FileStreamReader::statistics() { return m_fileIOStream.statistics(); } size_t FileStreamReader::fread(void* ___416, size_t size, size_t count) { REQUIRE(VALID_REF(___416)); REQUIRE(___2039()); size_t ___3356 = ::fread(___416, size, count, m_fileIOStream.handle());
 #ifdef PROFILE_FILE_ACCESS
if ( ___3356 > 0 ) { statistics().numReadWritesPerformed++; statistics().___2778 += ___3356*size; }
 #endif
return ___3356; } char* FileStreamReader::fgets(char* s, int size) { REQUIRE(VALID_REF(s)); REQUIRE(size >= 0); REQUIRE(___2039()); char* ___3356 = ::fgets(s, size, m_fileIOStream.handle());
 #ifdef PROFILE_FILE_ACCESS
if ( ___3356 != NULL ) { statistics().numReadWritesPerformed++; statistics().___2778 += ___3356 ? strlen(s) : 0; }
 #endif
return ___3356; } int FileStreamReader::feof() { REQUIRE(___2039()); return ::feof(m_fileIOStream.handle()); } int FileStreamReader::getc() { REQUIRE(___2039()); int ___3356 = ::getc(m_fileIOStream.handle());
 #ifdef PROFILE_FILE_ACCESS
statistics().numReadWritesPerformed++; statistics().___2778++;
 #endif
return ___3356; } int FileStreamReader::ungetc(int c) { REQUIRE(___2039()); int ___3356 = ::ungetc(c, m_fileIOStream.handle());
 #ifdef PROFILE_FILE_ACCESS
___476(statistics().___2778>0); statistics().___2778--;
 #endif
return ___3356; } int FileStreamReader::fscanf(char const* format, void* ___3249) { REQUIRE(___2039()); REQUIRE(VALID_NON_ZERO_LEN_STR(format)); REQUIRE(VALID_REF(___3249));
 #ifdef PROFILE_FILE_ACCESS
___1391 startLoc = fileLoc();
 #endif
int ___3356 = ::fscanf(m_fileIOStream.handle(), format, ___3249);
 #ifdef PROFILE_FILE_ACCESS
___1391 endLoc = fileLoc(); statistics().numReadWritesPerformed++; statistics().___2778 += (endLoc-startLoc);
 #endif
return ___3356; } int FileStreamReader::fscanf(char const* format, void* ptr1, void* ptr2) { REQUIRE(___2039()); REQUIRE(VALID_NON_ZERO_LEN_STR(format)); REQUIRE(VALID_REF(ptr1)); REQUIRE(VALID_REF(ptr2));
 #ifdef PROFILE_FILE_ACCESS
___1391 startLoc = fileLoc();
 #endif
int ___3356 = ::fscanf(m_fileIOStream.handle(), format, ptr1, ptr2);
 #ifdef PROFILE_FILE_ACCESS
___1391 endLoc = fileLoc(); statistics().numReadWritesPerformed++; statistics().___2778 += (endLoc-startLoc);
 #endif
return ___3356; } int FileStreamReader::fscanf(char const* format, void* ptr1, void* ptr2, void* ptr3) { REQUIRE(___2039()); REQUIRE(VALID_NON_ZERO_LEN_STR(format)); REQUIRE(VALID_REF(ptr1)); REQUIRE(VALID_REF(ptr2)); REQUIRE(VALID_REF(ptr3));
 #ifdef PROFILE_FILE_ACCESS
___1391 startLoc = fileLoc();
 #endif
int ___3356 = ::fscanf(m_fileIOStream.handle(), format, ptr1, ptr2, ptr3);
 #ifdef PROFILE_FILE_ACCESS
___1391 endLoc = fileLoc(); statistics().numReadWritesPerformed++; statistics().___2778 += (endLoc-startLoc);
 #endif
return ___3356; } }}
