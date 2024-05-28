#include "genesis/expected.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("expected and_then monad", "[expected][and_then]") {
	{
		genesis::expected<int32_t, int32_t> e = 21;
		auto ret = e.and_then([](int32_t a) { return genesis::expected<int32_t, int32_t>{a * 2};});
		REQUIRE(ret);
		REQUIRE(*ret == 42);
	}
}