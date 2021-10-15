#ifndef VISTLE_NETCDF_H
#define VISTLE_NETCDF_H
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>
#include <netcdf>

namespace vistle {
namespace netcdf {

typedef std::vector<size_t> NcVec_size;
typedef std::vector<ptrdiff_t> NcVec_diff;

/**
 * @brief Thread-safe NcVar wrapper.
 */
struct VNcVar {
    typedef std::shared_ptr<netCDF::NcVar> NcVarPtr;

public:
    VNcVar(NcVarPtr var);
    VNcVar(netCDF::NcVar var);
    ~VNcVar();

    auto getDim(int i) const { return ncVar->getDim(i); }

    template<class T>
    void getVar(const NcVec_size &start, const NcVec_size &count, const NcVec_diff &stride, T *out) const;

private:
    NcVarPtr ncVar;
    mutable std::mutex _mtx;
};

} // namespace netcdf
} // namespace vistle
#endif
