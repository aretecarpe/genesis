#include "genesis/config.hpp"

#include <catch2/catch_all.hpp>

#include <cstdint>

TEST_CASE("Test case for compilation between different platforms", "[config][platforms]") {
	uint32_t gnu_linux = 1;
	uint32_t msvc_microsoft = 2;
	#if GENESIS_LINUX
	REQUIRE(1 == gnu_linux);
	REQUIRE(gnu_linux != msvc_microsoft);
	#elif GENESIS_MICROSOFT
	REQUIRE(2 == msvc_microsoft);
	REQUIRE(msvc_microsoft != gnu_linux);
	#else
	FAIL("Can't find platform");
	#endif
}

TEST_CASE("Test case for detecting architecture", "[config][architecture]") {
	uint32_t aarch64 = 1;
	uint32_t x86_64 = 2;
	#if GENESIS_ARCH_x64
	REQUIRE(2 == x86_64);
	REQUIRE(x86_64 != aarch64);
	#elif GENESIS_ARCH_ARM
	REQUIRE(1 == aarch64);
	REQUIRE(aarch64 != x86_64);
	#endif
}