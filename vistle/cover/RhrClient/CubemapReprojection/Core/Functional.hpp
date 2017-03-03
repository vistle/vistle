// Core/Functional.hpp

#ifndef _CORE_FUNCTIONAL_HPP_INCLUDED_
#define _CORE_FUNCTIONAL_HPP_INCLUDED_

#include <type_traits>
#include <functional>
#include <tuple>

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
			-> std::enable_if_t<std::is_function<T>::value &&
			std::is_base_of<Base, std::decay_t<Derived>>::value,
			decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))>
		{
			return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
		}

		template <class Base, class T, class RefWrap, class... Args>
		auto INVOKE(T Base::*pmf, RefWrap&& ref, Args&&... args)
			noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<std::is_function<T>::value &&
			is_reference_wrapper<std::decay_t<RefWrap>>::value,
			decltype((ref.get().*pmf)(std::forward<Args>(args)...))>

		{
			return (ref.get().*pmf)(std::forward<Args>(args)...);
		}

		template <class Base, class T, class Pointer, class... Args>
		auto INVOKE(T Base::*pmf, Pointer&& ptr, Args&&... args)
			noexcept(noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<std::is_function<T>::value &&
			!is_reference_wrapper<std::decay_t<Pointer>>::value &&
			!std::is_base_of<Base, std::decay_t<Pointer>>::value,
			decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))>
		{
			return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
		}

		template <class Base, class T, class Derived>
		auto INVOKE(T Base::*pmd, Derived&& ref)
			noexcept(noexcept(std::forward<Derived>(ref).*pmd))
			-> std::enable_if_t<!std::is_function<T>::value &&
			std::is_base_of<Base, std::decay_t<Derived>>::value,
			decltype(std::forward<Derived>(ref).*pmd)>
		{
			return std::forward<Derived>(ref).*pmd;
		}

		template <class Base, class T, class RefWrap>
		auto INVOKE(T Base::*pmd, RefWrap&& ref)
			noexcept(noexcept(ref.get().*pmd))
			-> std::enable_if_t<!std::is_function<T>::value &&
			is_reference_wrapper<std::decay_t<RefWrap>>::value,
			decltype(ref.get().*pmd)>
		{
			return ref.get().*pmd;
		}

		template <class Base, class T, class Pointer>
		auto INVOKE(T Base::*pmd, Pointer&& ptr)
			noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
			-> std::enable_if_t<!std::is_function<T>::value &&
			!is_reference_wrapper<std::decay_t<Pointer>>::value &&
			!std::is_base_of<Base, std::decay_t<Pointer>>::value,
			decltype((*std::forward<Pointer>(ptr)).*pmd)>
		{
			return (*std::forward<Pointer>(ptr)).*pmd;
		}

		template <class F, class... Args>
		auto INVOKE(F&& f, Args&&... args)
			noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
			-> std::enable_if_t<!std::is_member_pointer<std::decay_t<F>>::value,
			decltype(std::forward<F>(f)(std::forward<Args>(args)...))>
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
		constexpr decltype(auto) Apply_Impl(F&& f, Tuple&& t, std::index_sequence<I...>)
		{
			return Invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
		}
	}

	template <class F, class Tuple>
	constexpr decltype(auto) ApplyFunction(F&& f, Tuple&& t)
	{
		return detail::Apply_Impl(std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
	}

	template <class F, class O, class Tuple>
	constexpr decltype(auto) ApplyMemberFunction(F&& f, O&& o, Tuple&& t)
	{
		return ApplyFunction(std::forward<F>(f), std::tuple_cat(std::forward<O>(o), std::forward<Tuple>(t)));
	}
}

#endif