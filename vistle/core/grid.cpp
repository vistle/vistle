#include "grid.h"
#include "scalar.h"

namespace vistle {

bool GridInterface::Interpolator::check() const {

#ifndef NDEBUG
    Scalar total = 0;
    bool ok = true;
    for (const auto w: weights) {
        if (w < -1e-4)
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

}
