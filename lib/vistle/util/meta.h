#ifndef VISTLE_UTIL_META_H
#define VISTLE_UTIL_META_H

#include <type_traits>
#include <cstdlib>
#include <utility>
#include <typeindex>
#include <typeinfo>

namespace vistle {
namespace meta {

namespace detail {
// returns a function that shifts the integral constant argument by N
template<std::size_t N, class F>
auto _shift(F func)
{
    return [&, func](auto i) {
        return func(std::integral_constant<std::size_t, i() + N>{});
    };
}
} // namespace detail

// invoke func for every index in supplied integer_sequence
template<class F, std::size_t... Is>
void _for(F func, std::integer_sequence<std::size_t, Is...>)
{
    (func(std::integral_constant<std::size_t, Is>{}), ...);
}

// invoke func for indices 0..N-1
template<std::size_t N, typename F>
void _for(F func)
{
    _for(func, std::make_integer_sequence<std::size_t, N>());
}

// invoke func for indices B..E-1
template<std::size_t B, std::size_t E, typename F>
void _for(F func)
{
    _for<E - B>(detail::_shift<B>(func));
}

namespace detail {
template<typename Tuple, typename T, typename F, std::size_t... I>
bool dispatch_by_type_impl(const T &obj, F &&f, std::index_sequence<I...>)
{
    const std::type_index actual(typeid(obj));

    return ((actual == std::type_index(typeid(std::tuple_element_t<I, Tuple>)) &&
             (std::forward<F>(f).template operator()<std::tuple_element_t<I, Tuple>>(), true)) ||
            ...);
}
} // namespace detail

// call f.template operator()<T>() where T is the type of obj and is in Tuple
// f can be a lamda: [&]<typename T>() { ... }
// returns true if a match was found, false otherwise
template<typename Tuple, typename T, typename F>
bool dispatch_by_type(const T &obj, F &&f)
{
    return detail::dispatch_by_type_impl<Tuple>(obj, std::forward<F>(f),
                                                std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}


} // namespace meta
} // namespace vistle
#endif
