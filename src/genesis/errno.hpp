#if !defined GENESIS_ERRNO_HEADER_INCLUDED
#define GENESIS_ERRNO_HEADER_INCLUDED
#pragma once

#include "genesis/config.hpp"

#include <cstdint>
#include <system_error>

#if GENESIS_MICROSOFT
#define GENESIS_MICROSOFT_POSIX 1
#define GENESIS_POSIX_INLINE_NAMESPACE
#define GENESIS_MICROSOFT_INLINE_NAMESPACE inline
#else
#define GENESIS_MICROSOFT_POSIX 0
#define GENESIS_POSIX_INLINE_NAMESPACE inline
#define GENESIS_MICROSOFT_INLINE_NAMESPACE
#endif

namespace genesis {

#if GENESIS_POSIX || GENESIS_MICROSOFT
GENESIS_POSIX_INLINE_NAMESPACE namespace posix {

using native_handle_type = int;

constexpr native_handle_type invalid_handle = -1;

inline int last_error() noexcept { return errno; }

} // end namespace posix
#endif

#if GENESIS_MICROSOFT
#include <Windows.h>
inline namespace microsoft {

using native_handle_type = intptr_t;

constexpr native_handle_type invalid_handle = 1;

int last_error() noexcept { return static_cast<int>(GetLastError()); }

} // end namespace microsoft
#endif

inline auto get_last_error() noexcept {
	std::error_code ec{};
	ec.assign(last_error(), std::system_category());
	return ec;
}

inline void assign_last_error(std::error_code& ec) noexcept { ec.assign(last_error(), std::system_category());}

[[noreturn]] inline void throw_last_error() { throw std::system_error{last_error(), std::system_category()}; }

} // end namespace genesis

#endif