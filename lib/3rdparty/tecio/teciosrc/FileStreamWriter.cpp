#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "showMessage.h"
#include "FileStreamWriter.h"
namespace tecplot { namespace ___3931 { FileStreamWriter::FileStreamWriter(std::string const& ___1392) : m_fileIOStream(___1392) { } FileStreamWriter::~FileStreamWriter() { m_fileIOStream.close(true); } ___372 FileStreamWriter::___2039() const { return m_fileIOStream.___2039(); } ___372 FileStreamWriter::open(bool update) { REQUIRE(!___2039()); ___372 ___2037; if (update) { ___2037 = m_fileIOStream.open("rb+"); if (!___2037) ___2037 = m_fileIOStream.open("wb+"); } else { ___2037 = m_fileIOStream.open("wb+"); } if (___2037) {
 #if !defined NDEBUG
{ if (m_fileIOStream.___2000()) if (setvbuf(m_fileIOStream.handle(), NULL, _IONBF, 0) != 0) ___2037 = ___1303; }
 #endif
} else { ___1184( "Cannot write to file %s", m_fileIOStream.___1392().c_str()); }
 #ifdef PROFILE_FILE_ACCESS
___476(statistics().numFSeeksPerformed == 0 && statistics().numReadWritesPerformed == 0 && statistics().___2778 == 0);
 #endif
return ___2037; } ___372 FileStreamWriter::close(bool ___3359) { return m_fileIOStream.close(___3359); } ___1391 FileStreamWriter::fileLoc() { REQUIRE(___2039()); return m_fileIOStream.fileLoc(); } ___372 FileStreamWriter::___3458() { REQUIRE(___2039()); return m_fileIOStream.___3458(); } ___372 FileStreamWriter::___3457(___1391 fileLoc) { REQUIRE(___2039()); REQUIRE(fileLoc != ___330); return m_fileIOStream.___3457(fileLoc); } ___372 FileStreamWriter::seekToFileEnd() { REQUIRE(___2039()); return m_fileIOStream.seekToFileEnd(); } std::string const& FileStreamWriter::___1392() const { return m_fileIOStream.___1392(); } void FileStreamWriter::___3492(___372 ___2000) { REQUIRE(VALID_BOOLEAN(___2000)); m_fileIOStream.___3492(___2000); } ___372 FileStreamWriter::___2000() const { return m_fileIOStream.___2000(); } void FileStreamWriter::setDataFileType(DataFileType_e ___842) { REQUIRE(VALID_ENUM(___842, DataFileType_e)); m_fileIOStream.setDataFileType(___842); } DataFileType_e FileStreamWriter::___842() const { return m_fileIOStream.___842(); } class FileIOStatistics& FileStreamWriter::statistics() { return m_fileIOStream.statistics(); } size_t FileStreamWriter::fwrite(void const* ___416, size_t size, size_t count) { REQUIRE(___2039()); REQUIRE(VALID_REF(___416)); size_t ___3356 = ::fwrite(___416, size, count, m_fileIOStream.handle());
 #ifdef PROFILE_FILE_ACCESS
if ( ___3356 > 0 ) { statistics().numReadWritesPerformed++; statistics().___2778 += ___3356*size; }
 #endif
return ___3356; } int FileStreamWriter::fprintf(char const* format, ...) { REQUIRE(___2039()); REQUIRE(VALID_NON_ZERO_LEN_STR(format)); va_list args; va_start(args, format); int ___3356 = ::vfprintf(m_fileIOStream.handle(), format, args); va_end (args);
 #ifdef PROFILE_FILE_ACCESS
if (___3356 > 0) { statistics().numReadWritesPerformed++; statistics().___2778 += ___3356; }
 #endif
return ___3356; } }}
