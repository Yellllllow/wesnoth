/*
 Copyright (C) by the Battle for Wesnoth Project http://www.wesnoth.org/
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.
 
 See the COPYING file for more details.
 */

/*
 * Wrapper header to allow use of placeholder variables without a namespace.
 */

#ifndef INCL_FUNCTIONAL_HPP_
#define INCL_FUNCTIONAL_HPP_

#include "global.hpp"
#include <functional>
#include <boost/bind.hpp> // Because std::bind is just not flexible enough

// We'd like to just say "using namespace std::placeholders", but unfortunately
// that clashes with Boost.Bind's placeholders in some cases (even if bind.hpp is not included).
// Instead, we specialize std::is_placeholder for the Boost placeholders,
// so that Boost placeholders can be passed to std::bind.

template<int N>
struct std::is_placeholder<boost::arg<N>> : public std::integral_constant<int, N> {};

namespace detail {
	template<typename Ret, typename... T>
	struct apply {
		using result_type = void;
		apply(std::function<Ret(T...)> fcn) : fcn(fcn) {}
		void operator()(T... params) {
			fcn(std::forward<T>(params)...);
		}
	private:
		std::function<Ret(T...)> fcn;
	};
	
	template<typename Ret, typename... T>
	apply<Ret, T...> make_apply(std::function<Ret(T...)> fcn) {
		return apply<Ret, T...>(fcn);
	}
	
	template<typename F>
	struct function_base {
		using type = typename function_base<decltype(&F::operator())>::type;
	};
	
	template<typename Ret, typename... P>
	struct function_base<Ret(P...)> {
		using type = Ret(P...);
	};
	
	template<typename Ret, typename... P>
	struct function_base<Ret(*)(P...)> {
		using type = Ret(P...);
	};
	
	template<typename Ret, typename Class, typename... P>
	struct function_base<Ret(Class::*)(P...)> {
		using type = Ret(Class,P...);
	};
	
	template<typename Ret, typename Class, typename... P>
	struct function_base<Ret(Class::*)(P...)const> {
		using type = Ret(const Class,P...);
	};
	
	template<typename Ret, typename Class, typename... P>
	struct function_base<Ret(Class::*)(P...)volatile > {
		using type = Ret(volatile Class,P...);
	};
	
	template<typename Ret, typename Class, typename... P>
	struct function_base<Ret(Class::*)(P...)const volatile> {
		using type = Ret(const volatile Class,P...);
	};
	
	template<typename Ret, typename... P>
	struct function_base<std::function<Ret(P...)>> {
		using type = Ret(P...);
	};
}

template<typename F, typename... P>
auto bind_void(F fcn, P... bindings)
#ifndef HAVE_CXX14
-> decltype(boost::bind(
	detail::make_apply(std::function<typename detail::function_base<F>::type>(fcn)), bindings...)
)
#endif
{
	using T = typename detail::function_base<F>::type;
	return boost::bind(detail::make_apply(std::function<T>(fcn)), bindings...);
}

template<typename F, typename... P>
auto bind_void_exact(F fcn, P... bindings)
#ifndef HAVE_CXX14
-> decltype(
	boost::bind(detail::make_apply(std::function<typename detail::function_base<F>::type>(fcn)), bindings...)
)
#endif
{
	using T = typename detail::function_base<F>::type;
	return std::bind(detail::make_apply(std::function<T>(fcn)), bindings...);
}

/* A note on why std::bind is not flexible enough:

1. The functions produced do not silently consume extra parameters passed to them.
This is not a bad thing per se, but some of Wesnoth's code relied on it.
It's useful behaviour, as well.

2. A function that returns a value cannot be bound in a function type that returns void.
This is also relied upon in several places.

If behaviour #1 is needed, we need to use boost::bind. For behaviour #2, this won't work;
instead, the bind_void function is provided. (This also provides behaviour #1.
To get #2 without #1, use bind_void_exact.)
*/

#endif
