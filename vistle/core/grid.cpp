#include "grid.h"
#include "scalar.h"

#include <cmath>

namespace vistle {

bool GridInterface::Interpolator::check() const
{
#ifndef NDEBUG
    bool ok = true;
    for (const auto w: weights) {
        if (!std::isfinite(w)) {
            std::cerr << "invalid interpolation weight" << std::endl;
            ok = false;
        }
    }

    Scalar total = 0;
    for (const auto w: weights) {
        if (w < -1e-3)
            ok = false;
        total += w;
    }
    if (fabs(total - 1) > 1e-4)
        ok = false;

#ifndef INTERPOL_DEBUG
    if (!ok)
#endif
    {
        if (!ok) {
            std::cerr << "GridInterface::Interpolator: PROBLEM: ";
        }
        std::cerr << "weights:";
        for (const auto w: weights) {
            std::cerr << " " << w;
        }
        std::cerr << ", total: " << total << std::endl;
    }
    return ok;
#endif

    return true;
}

} // namespace vistle
