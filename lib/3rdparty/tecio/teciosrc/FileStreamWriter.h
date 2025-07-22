 #pragma once
#include "ThirdPartyHeadersBegin.h"
#   include <string>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "basicTypes.h"
#include "FileIOStream.h"
#include "FileWriterInterface.h"
namespace tecplot { namespace ___3931 { class FileStreamWriter : public FileWriterInterface { public: explicit FileStreamWriter(std::string const& ___1392); virtual ~FileStreamWriter(); virtual ___372 ___2039() const; virtual ___372 close(bool ___3359); virtual ___1391 fileLoc(); virtual ___372 ___3458(); virtual ___372 ___3457(___1391 fileLoc); virtual ___372 seekToFileEnd(); virtual std::string const& ___1392() const; virtual void ___3492(___372 ___2000); virtual ___372 ___2000() const; virtual void setDataFileType(DataFileType_e ___842); virtual DataFileType_e ___842() const; virtual class FileIOStatistics& statistics(); virtual ___372 open(bool update); virtual size_t fwrite(void const* ___416, size_t size, size_t count); virtual int fprintf(char const* format, ...); private: FileIOStream m_fileIOStream; UNCOPYABLE_CLASS(FileStreamWriter); }; }}
