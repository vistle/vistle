 #pragma once
#include "ThirdPartyHeadersBegin.h"
#   include <string>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "basicTypes.h"
namespace tecplot { namespace ___3931 { class FileIOStreamInterface { public: virtual ___372 ___2039() const = 0; virtual ___372 close(bool ___3359) = 0; virtual ___1391 fileLoc() = 0; virtual ___372 ___3458() = 0; virtual ___372 ___3457(___1391 fileLoc) = 0; virtual ___372 seekToFileEnd() = 0; virtual std::string const& ___1392() const = 0; virtual void ___3492(___372 ___2000) = 0; virtual ___372 ___2000() const = 0; virtual void setDataFileType(DataFileType_e ___842) = 0; virtual DataFileType_e ___842() const = 0; virtual class FileIOStatistics& statistics() = 0; virtual ~FileIOStreamInterface() {} }; }}
