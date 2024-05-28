#include "genesis/expected.hpp"

#include <catch2/catch_all.hpp>

#include <cstdint>

template <class E>
genesis::unexpected<typename std::decay<E>::type> make_unexpected(E &&e) {
  return genesis::unexpected<typename std::decay<E>::type>(std::forward<E>(e));
}

TEST_CASE("expected constructions", "[expected][constructors]") {
	{
		genesis::expected<int32_t, int32_t> e;
		REQUIRE(e);
		REQUIRE(*e == 0);
	}
	{
		genesis::expected<int32_t, int32_t> e = make_unexpected(42);
		REQUIRE(!e);
		REQUIRE(e.error() == 42);
	}
	{
		genesis::expected<int32_t, int32_t> e{genesis::unexpect, 42};
		REQUIRE(!e);
		REQUIRE(e.error() == 42);
	}
}