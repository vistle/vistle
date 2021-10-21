#ifndef C20_H
#define C20_H

#include <iterator>
#include <algorithm>

namespace vistle {

/**
 * @brief Shift order of container in value range.
 *
 * @tparam ForwardIt Iterator type
 * @param begin begin iterator.
 * @param end end iterator.
 * @param range subrange in container.
 * @param left shift direction.
 */
template<class ForwardIt>
constexpr void shift_order_in_range(ForwardIt begin, ForwardIt end, unsigned range, bool left = true)
{
    auto sizeIt = range - 1;
    for (ForwardIt i = begin; i != end; i = i + range)
        if (left)
            for (ForwardIt j = i; j != i + sizeIt; ++j)
                iter_swap(j, j + 1);
        else
            for (ForwardIt j = i + sizeIt; j != i; --j)
                iter_swap(j, j - 1);
}

/**
 * @brief std::iter_swap (C++20)
 *
 * @tparam ForwardIt1 iterator type 1
 * @tparam ForwardIt2 iterator type 2
 * @param a iterator
 * @param b iterator
 */
template<class ForwardIt1, class ForwardIt2>
constexpr void iter_swap(ForwardIt1 a, ForwardIt2 b) // constexpr since C++20
{
    std::swap(*a, *b);
}
} // namespace vistle
#endif
