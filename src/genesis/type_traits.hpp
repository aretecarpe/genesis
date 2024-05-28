#if !defined GENESIS_TYPE_TRAITS_HEADER_INCLUDED
#define GENESIS_TYPE_TRAITS_HEADER_INCLUDED
#pragma once

#include <type_traits>
#include <utility>

namespace genesis {

template <typename... B>
inline constexpr bool or_v = std::__or_<B...>::value;
template <typename... B>
inline constexpr bool and_v = std::__and_<B...>::value;

template <typename T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename From, typename To>
constexpr bool is_nothrow_convertible_impl() {
    if constexpr (std::is_void_v<From> || std::is_function_v<To> || std::is_array_v<To>) {
        return false;
    } else {
        return noexcept(static_cast<To>(std::declval<From>()));
    }
}

template <typename From, typename To>
struct is_nothrow_convertible : std::bool_constant<is_nothrow_convertible_impl<From, To>()> {};

template <typename From, typename To>
constexpr bool is_nothrow_convertible_v = is_nothrow_convertible<From, To>::value;

struct cp {
	template <class Tp>
	using _f = Tp;
};

struct cpc {
	template <class Tp>
	using _f = const Tp;
};

struct cplr {
	template <class Tp>
	using _f = Tp &;
};

struct cprr {
	template <class Tp>
	using _f = Tp &&;
};

struct cpclr {
	template <class Tp>
	using _f = const Tp &;
};

struct cpcrr {
	template <class Tp>
	using _f = const Tp &&;
};

template <class>
extern cp cpcvr;
template <class Tp>
extern cpc cpcvr<const Tp>;
template <class Tp>
extern cplr cpcvr<Tp &>;
template <class Tp>
extern cprr cpcvr<Tp &&>;
template <class Tp>
extern cpclr cpcvr<const Tp &>;
template <class Tp>
extern cpcrr cpcvr<const Tp &&>;
template <class Tp>
using copy_cvref_fn = decltype(cpcvr<Tp>);

template <typename From, typename To>
using copy_cvref_t = typename copy_cvref_fn<From>::template _f<To>;

template <typename T, typename U>
using is_void_or = std::conditional_t<std::is_void<T>::value, std::true_type, U>;

template <typename T>
using is_copy_constructible_or_void =
	is_void_or<T, std::is_copy_constructible<T>>;

template <typename T>
using is_move_constructible_or_void =
	is_void_or<T, std::is_move_constructible<T>>;

template <typename T>
using is_copy_assignable_or_void = is_void_or<T, std::is_copy_assignable<T>>;

template <typename T>
using is_move_assignable_or_void = is_void_or<T, std::is_move_assignable<T>>;

} // end namespace genesis

#endif