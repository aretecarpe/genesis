#if !defined GENESIS_UTILITY_HEADER_INCLUDED
#define GENESIS_UTILITY_HEADER_INCLUDED
#pragma once

#include <initializer_list>
#include <type_traits>

namespace genesis{ 

struct ignore_t {
	template <typename... As>
	constexpr ignore_t(As&& ...) noexcept { } 
};

template <typename...>
struct undefined;

struct empty { };

struct nil { };

struct immovable {
	immovable() = default;
	immovable(const immovable&) = delete;
	immovable& operator=(const immovable&) = delete;
};

inline constexpr std::size_t max(std::initializer_list<std::size_t> il) noexcept {
	std::size_t max = 0;
	for (auto i : il) {
		if (i > max) {
			max = i;
		}
	}
	return max;
}

template <typename T, typename... Ts>
inline constexpr std::size_t index_of() noexcept {
	constexpr bool same[] {std::is_same_v<T, Ts>...};
	for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
		if (same[i]) { return i; }
	}
	return static_cast<std::size_t>(-1ul);
}

} // end namespace genesis

#endif