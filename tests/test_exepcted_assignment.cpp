#include "genesis/expected.hpp"

#include <catch2/catch_all.hpp>

#include <cstdint>

TEST_CASE("expected simple assignemnts", "[expected][assignments]") {
	genesis::expected<int32_t, int32_t> e1 = 42;
	genesis::expected<int32_t, int32_t> e2 = 777;

	e1 = e2;
	REQUIRE(e1);
	REQUIRE(*e1 == 777);
	REQUIRE(e2);
	REQUIRE(*e2 == 777);

	e1 = std::move(e2);
	REQUIRE(e1);
	REQUIRE(*e1 == 777);
	REQUIRE(e2);
	REQUIRE(*e2 == 777);

	e1 = 42;
	REQUIRE(e1);
	REQUIRE(*e1 == 42);
}