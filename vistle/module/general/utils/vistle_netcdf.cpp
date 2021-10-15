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

VNcVar::~VNcVar()
{}

template<class T>
void VNcVar::getVar(const NcVec_size &start, const NcVec_size &count, const NcVec_diff &stride, T *out) const
{
    std::lock_guard<std::mutex> locker(_mtx);
    ncVar->getVar(start, count, stride, out);
}

} // namespace netcdf
} // namespace vistle
