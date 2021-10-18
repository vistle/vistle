#ifndef VISTLE_NETCDF_H
#define VISTLE_NETCDF_H

#include "vistle/util/export.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>
#include <netcdf>

namespace vistle {
namespace netcdf {

/**
 * @brief Thread-safe NcVar wrapper.
 */
class V_UTILEXPORT VNcVar {
public:
    typedef std::shared_ptr<netCDF::NcVar> NcVarPtr;
    typedef std::vector<size_t> NcVec_size;
    typedef std::vector<ptrdiff_t> NcVec_diff;

    VNcVar(NcVarPtr var);
    VNcVar(netCDF::NcVar var);

    auto getDim(int i) const { return ncVar->getDim(i); }

    template<class T>
    void getVar(const NcVec_size &start, const NcVec_size &count, const NcVec_diff &stride, T *out) const
    {
        std::lock_guard<std::mutex> locker(_mtx);
        ncVar->getVar(start, count, stride, out);
    }

private:
    NcVarPtr ncVar;
    mutable std::mutex _mtx;
};

} // namespace netcdf
} // namespace vistle
#endif
