 #pragma once
#include "ThirdPartyHeadersBegin.h"
#   include <cstdio>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "basicTypes.h"
#include "FileIOStream.h"
#include "FileReaderInterface.h"
namespace tecplot { namespace ___3931 { class FileStreamReader : public ___1397 { public: explicit FileStreamReader(std::string const& ___1392); virtual ~FileStreamReader(); virtual ___372 ___2039() const; virtual ___372 close(bool ___3359); virtual ___1391 fileLoc(); virtual ___372 ___3458(); virtual ___372 ___3457(___1391 fileLoc); virtual ___372 seekToFileEnd(); virtual std::string const& ___1392() const; virtual void ___3492(___372 ___2000); virtual ___372 ___2000() const; virtual void setDataFileType(DataFileType_e ___842); virtual DataFileType_e ___842() const; virtual class FileIOStatistics& statistics(); virtual ___372 open(); virtual size_t fread(void* ___416, size_t size, size_t count); virtual char* fgets(char* s, int size); virtual int feof(); virtual int getc(); virtual int ungetc(int c); virtual int fscanf(char const* format, void* ___3249); virtual int fscanf(char const* format, void* ptr1, void* ptr2); virtual int fscanf(char const* format, void* ptr1, void* ptr2, void* ptr3); private: FileIOStream m_fileIOStream; UNCOPYABLE_CLASS(FileStreamReader); }; }}
