#include "ThirdPartyHeadersBegin.h"
#include <stdexcept>
#include "ThirdPartyHeadersEnd.h"
namespace tecplot { namespace teciompi { class MPIError : public std::runtime_error { public: explicit MPIError(std::string const& ___1184) : std::runtime_error(___1184) {} virtual ~MPIError() throw() {}; }; }}
