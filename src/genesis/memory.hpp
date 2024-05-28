#if !defined GENESIS_MEMORY_HEADER_INCLUDED
#define GENESIS_MEMORY_HEADER_INCLUDED
#pragma once

#include <utility>

namespace genesis {

template <typename T, typename... Args>
constexpr auto construct_at(T* location, Args&&... args) noexcept(
	noexcept(::new((void*)0) T(std::declval<Args>()...))
) ->
	decltype(::new((void*)0) T(std::declval<Args>()...))
{
	return ::new((void*)location) T(std::forward<Args>(args)...);
}

} // end namespace genesis

#endif