#ifndef VISTLE_XDMF_XDMF_H
#define VISTLE_XDMF_XDMF_H

//xdmf
#include <XdmfArray.hpp>

//vistle
#include <vistle/util/export.h>

namespace vistle {
namespace xdmf {

/**
 * @brief Extract unique points from xdmfarray and store them in a set.
 *
 * @tparam T Set generic parameter.
 * @param arr XdmfArray with values to extract.
 * @param refSet Storage std::set.
 */
template<class T>
void extractUniquePoints(XdmfArray *arr, std::set<T> &refSet)
{
    if (arr)
        for (unsigned i = 0; i < arr->getSize(); ++i)
            refSet.insert(arr->getValue<T>(i));
}

} // namespace xdmf
} // namespace vistle
#endif
