 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <boost/scoped_ptr.hpp>
#include <mpi.h>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "basicTypes.h"
#include "ClassMacros.h"
#include "FileIOStreamInterface.h"
namespace tecplot { namespace ___3931 { class FileIOStatistics; } } namespace tecplot { namespace teciompi { class MPIFileIOStream : public ___3931::FileIOStreamInterface { public: MPIFileIOStream(std::string const& ___1392, MPI_Comm comm, MPI_Info info = MPI_INFO_NULL); virtual ~MPIFileIOStream(); virtual ___372 ___2039() const; virtual ___372 close(bool ___3359); virtual ___3931::___1391 fileLoc(); virtual ___372 ___3458(); virtual ___372 ___3457(___3931::___1391 fileLoc); virtual ___372 seekToFileEnd(); virtual std::string const& ___1392() const; virtual void ___3492(___372 ___2000); virtual ___372 ___2000() const; virtual void setDataFileType(DataFileType_e ___842); virtual DataFileType_e ___842() const; virtual ___3931::FileIOStatistics& statistics(); ___372 open(int ___2502); MPI_File fileHandle() const; MPI_Comm comm() const; private: struct Impl; boost::scoped_ptr<Impl> m_impl; UNCOPYABLE_CLASS(MPIFileIOStream); }; }}
