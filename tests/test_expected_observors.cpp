#include "genesis/expected.hpp"

#include <catch2/catch_all.hpp>

#include <cstdint>

TEST_CASE("expected observors", "[expected][observors]") {
	genesis::expected<int32_t, int32_t> e1 = 42;
	genesis::expected<int32_t, int32_t> e2 = genesis::unexpected{-1};

	REQUIRE(*e1 == 42);
	REQUIRE(*e1 == e1.value());
	REQUIRE(e2.value_or(42) == 42);
	REQUIRE(e2.error() == -1);

	auto res = std::is_same_v<decltype(e1.value()), std::add_lvalue_reference_t<int32_t>>;
	REQUIRE(res == true);
}