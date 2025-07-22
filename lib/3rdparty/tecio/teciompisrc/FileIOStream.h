 #pragma once
#include "ClassMacros.h"
#include "FileIOStatistics.h"
#include "FileIOStreamInterface.h"
namespace tecplot { namespace ___3931 { class FileIOStream : public FileIOStreamInterface { public: explicit FileIOStream(std::string const& ___1392); virtual ~FileIOStream(); virtual ___372 ___2039() const; virtual ___372 close(bool ___3359); virtual ___1391 fileLoc(); virtual ___372 ___3458(); virtual ___372 ___3457(___1391 fileLoc); virtual ___372 seekToFileEnd(); virtual std::string const& ___1392() const; virtual void ___3492(___372 ___2000); virtual ___372 ___2000() const; virtual void setDataFileType(DataFileType_e ___842); virtual DataFileType_e ___842() const; virtual class FileIOStatistics& statistics(); ___372 open(std::string const& ___2502); FILE* handle() const; private: FileIOStatistics  m_statistics; FILE*             m_fileHandle; std::string const ___2459; bool              m_isAscii; DataFileType_e    m_dataFileType; UNCOPYABLE_CLASS(FileIOStream); }; }}
