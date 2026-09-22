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

GridInterface::Interpolator::Interpolator(std::vector<Scalar> &weights, std::vector<Index> &indices)
: weights(std::move(weights)), indices(std::move(indices))
{
#ifndef NDEBUG
    check();
#endif
}

Scalar GridInterface::Interpolator::operator()(const Scalar *field) const
{
    Scalar ret(0);
    for (size_t i = 0; i < weights.size(); ++i)
        ret += field[indices[i]] * weights[i];
    return ret;
}

Vector3 GridInterface::Interpolator::operator()(const Scalar *f0, const Scalar *f1, const Scalar *f2) const
{
    Vector3 ret(0, 0, 0);
    for (size_t i = 0; i < weights.size(); ++i) {
        const Index ind(indices[i]);
        const Scalar w(weights[i]);
        ret += Vector3(f0[ind], f1[ind], f2[ind]) * w;
    }
    return ret;
}

GridInterface::Interpolator GridInterface::getInterpolator(const Vector3 &point, DataBase::Mapping mapping,
                                                           InterpolationMode mode) const
{
    const Index elem = findCell(point);
    if (elem == InvalidIndex) {
        return Interpolator();
    }
    return getInterpolator(elem, point, mapping, mode);
}

} // namespace vistle
