#include "vistle_netcdf.h"

#include <memory>
#include <mutex>

namespace vistle {
namespace netcdf {

// VNcVar
VNcVar::VNcVar(NcVarPtr var): ncVar(var)
{}

VNcVar::VNcVar(netCDF::NcVar var)
{
    ncVar = std::make_shared<netCDF::NcVar>(var);
}

} // namespace netcdf
} // namespace vistle
