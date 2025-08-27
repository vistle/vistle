#ifndef VISTLE_CORE_YAS_SPAN_H
#define VISTLE_CORE_YAS_SPAN_H

#include <yas/detail/type_traits/type_traits.hpp>
#include <yas/detail/type_traits/serializer.hpp>
#include <yas/detail/io/serialization_exceptions.hpp>

#include <yas/types/concepts/array.hpp>

#include <boost/core/span.hpp>

namespace yas {
namespace detail {

template<std::size_t F, typename T, std::size_t E>
struct serializer<type_prop::not_a_fundamental, ser_case::use_internal_serializer, F, boost::span<T, E>> {
    template<typename Archive>
    static Archive &save(Archive &ar, const boost::span<T, E> &span)
    {
        return concepts::array::save<F>(ar, span);
    }

    template<typename Archive>
    static Archive &load(Archive &ar, boost::span<T, E> &span)
    {
        return concepts::array::load<F>(ar, span);
    }
};

} // namespace detail
} // namespace yas

#endif
