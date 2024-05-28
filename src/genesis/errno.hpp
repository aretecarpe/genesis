#if !defined GENESIS_ERRNO_HEADER_INCLUDED
#define GENESIS_ERRNO_HEADER_INCLUDED
#pragma once

#include "genesis/config.hpp"

#include <cstdint>
#include <system_error>

namespace genesis {

#if GENESIS_POSIX 
inline namespace posix {

inline int last_error() noexcept { return errno; }

} // end namespace posix
#endif

#if GENESIS_MICROSOFT
#include <windows.h>
inline namespace microsoft {

inline int last_error() noexcept { return static_cast<int>(GetLastError()); }

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