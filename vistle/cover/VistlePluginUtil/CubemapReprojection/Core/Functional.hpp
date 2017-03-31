// Core/Functional.hpp

#ifndef _CORE_FUNCTIONAL_HPP_INCLUDED_
#define _CORE_FUNCTIONAL_HPP_INCLUDED_

#include <type_traits>
#include <functional>
#include <tuple>

#include <Core/boost_workaround/integer_sequence.hpp>

// Sources:
//
// http://en.cppreference.com/w/cpp/utility/functional/invoke
// http://en.cppreference.com/w/cpp/experimental/apply
//

namespace Core
{
	namespace detail
	{
		template <class T>
		struct is_reference_wrapper : std::false_type {};
		template <class U>
		struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};

		template <class Base, class T, class Derived, class... Args>
		auto INVOKE(T Base::*pmf, Derived&& ref, Args&&... args)
			noexcept(noexcept((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...)))
                        -> typename std::enable_if<std::is_function<T>::value &&
                        std::is_base_of<Base, typename std::decay<Derived>::type>::value,
                        decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))>::type
		{
			return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
		}

		template <class Base, class T, class RefWrap, class... Args>
		auto INVOKE(T Base::*pmf, RefWrap&& ref, Args&&... args)
			noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args)...)))
                        -> typename std::enable_if<std::is_function<T>::value &&
                        is_reference_wrapper<typename std::decay<RefWrap>::type>::value,
                        decltype((ref.get().*pmf)(std::forward<Args>(args)...))>::type

		{
			return (ref.get().*pmf)(std::forward<Args>(args)...);
		}

		template <class Base, class T, class Pointer, class... Args>
		auto INVOKE(T Base::*pmf, Pointer&& ptr, Args&&... args)
			noexcept(noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...)))
                        -> typename std::enable_if<std::is_function<T>::value &&
                        !is_reference_wrapper<typename std::decay<Pointer>::type>::value &&
                        !std::is_base_of<Base, typename std::decay<Pointer>::type>::value,
                        decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))>::type
		{
			return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
		}

		template <class Base, class T, class Derived>
		auto INVOKE(T Base::*pmd, Derived&& ref)
			noexcept(noexcept(std::forward<Derived>(ref).*pmd))
                        -> typename std::enable_if<!std::is_function<T>::value &&
                        std::is_base_of<Base, typename std::decay<Derived>::type>::value,
                        decltype(std::forward<Derived>(ref).*pmd)>::type
		{
			return std::forward<Derived>(ref).*pmd;
		}

		template <class Base, class T, class RefWrap>
		auto INVOKE(T Base::*pmd, RefWrap&& ref)
			noexcept(noexcept(ref.get().*pmd))
                        -> typename std::enable_if<!std::is_function<T>::value &&
                        is_reference_wrapper<typename std::decay<RefWrap>::type>::value,
                        decltype(ref.get().*pmd)>::type
		{
			return ref.get().*pmd;
		}

		template <class Base, class T, class Pointer>
		auto INVOKE(T Base::*pmd, Pointer&& ptr)
			noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
                        -> typename std::enable_if<!std::is_function<T>::value &&
                        !is_reference_wrapper<typename std::decay<Pointer>::type>::value &&
                        !std::is_base_of<Base, typename std::decay<Pointer>::type>::value,
                        decltype((*std::forward<Pointer>(ptr)).*pmd)>::type
		{
			return (*std::forward<Pointer>(ptr)).*pmd;
		}

		template <class F, class... Args>
		auto INVOKE(F&& f, Args&&... args)
			noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
                        -> typename std::enable_if<!std::is_member_pointer<typename std::decay<F>::type>::value,
                        decltype(std::forward<F>(f)(std::forward<Args>(args)...))>::type
		{
			return std::forward<F>(f)(std::forward<Args>(args)...);
		}
	}

	template< class F, class... ArgTypes >
	auto Invoke(F&& f, ArgTypes&&... args)
		// exception specification for QoI
		noexcept(noexcept(detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...)))
		-> decltype(detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...))
	{
		return detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...);
	}

	namespace detail
	{
		template <class F, class Tuple, std::size_t... I>
		constexpr auto Apply_Impl(F&& f, Tuple&& t, boost::spirit::x3::index_sequence<I...>)
			-> decltype(Invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...))
		{
			return Invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
		}
	}

	template <class F, class Tuple>
	constexpr auto ApplyFunction(F&& f, Tuple&& t)
		-> decltype(detail::Apply_Impl(std::forward<F>(f), std::forward<Tuple>(t),
			boost::spirit::x3::make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
	{
		return detail::Apply_Impl(std::forward<F>(f), std::forward<Tuple>(t),
			boost::spirit::x3::make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
	}

	template <class F, class O, class Tuple>
	constexpr auto ApplyMemberFunction(F&& f, O&& o, Tuple&& t)
		-> decltype(ApplyFunction(std::forward<F>(f), std::tuple_cat(std::forward<O>(o), std::forward<Tuple>(t))))
	{
		return ApplyFunction(std::forward<F>(f), std::tuple_cat(std::forward<O>(o), std::forward<Tuple>(t)));
	}
}

#endif
