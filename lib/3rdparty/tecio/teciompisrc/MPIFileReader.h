 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <boost/scoped_ptr.hpp>
#include <mpi.h>
#include <string>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "basicTypes.h"
#include "ClassMacros.h"
#include "FileReaderInterface.h"
#include "MPIFileIOStream.h"
namespace tecplot { namespace teciompi { class MPIFileReader : public ___3931::___1397 { public: MPIFileReader( std::string const& ___1392, MPI_Comm           comm); virtual ~MPIFileReader(); virtual ___372 ___2039() const; virtual ___372 close(bool ___3359); virtual ___3931::___1391 fileLoc(); virtual ___372 ___3458(); virtual ___372 ___3457(___3931::___1391 fileLoc); virtual ___372 seekToFileEnd(); virtual std::string const& ___1392() const; virtual void ___3492(___372 ___2000); virtual ___372 ___2000() const; virtual void setDataFileType(DataFileType_e ___842); virtual DataFileType_e ___842() const; virtual ___3931::FileIOStatistics& statistics(); virtual ___372 open(); virtual size_t fread(void* ___416, size_t size, size_t count); virtual char* fgets(char* s, int size); virtual int feof(); virtual int getc(); virtual int ungetc(int c); virtual int fscanf(char const* format, void* ___3249); virtual int fscanf(char const* format, void* ptr1, void* ptr2); virtual int fscanf(char const* format, void* ptr1, void* ptr2, void* ptr3); private: MPIFileIOStream m_ioStream; UNCOPYABLE_CLASS(MPIFileReader); }; }}
