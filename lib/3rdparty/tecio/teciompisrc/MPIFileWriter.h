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
#include "FileWriterInterface.h"
namespace tecplot { namespace teciompi { class MPIFileWriter : public ___3931::FileWriterInterface { public: MPIFileWriter( std::string const& ___1392, MPI_Comm           comm, MPI_Info           info, size_t             bufferSizeInMB = 8); virtual ~MPIFileWriter(); virtual ___372 ___2039() const; virtual ___372 close(bool ___3359); virtual ___3931::___1391 fileLoc(); virtual ___372 ___3458(); virtual ___372 ___3457(___3931::___1391 fileLoc); virtual ___372 seekToFileEnd(); virtual std::string const& ___1392() const; virtual void ___3492(___372 ___2000); virtual ___372 ___2000() const; virtual void setDataFileType(DataFileType_e ___842); virtual DataFileType_e ___842() const; virtual ___3931::FileIOStatistics& statistics(); virtual ___372 open(bool update); virtual size_t fwrite(void const* ___416, size_t size, size_t count); virtual int fprintf(char const* format, ...); class ScopedCaching { public: ScopedCaching(MPIFileWriter& fileWriter, size_t cacheSize); virtual ~ScopedCaching(); private: MPIFileWriter& m_fileWriter; }; private: struct Impl; boost::scoped_ptr<Impl> const m_impl; UNCOPYABLE_CLASS(MPIFileWriter); }; }}
