#include "genesis/object_pool.hpp"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <vector>

TEST_CASE("object_pool construction with no generator", "[object_pool][constructor]") {
	struct foo { };
	genesis::object_pool<foo> pool{42};
	std::vector<std::shared_ptr<foo>> objs{};
	for (std::size_t i = 0; i < pool.capacity(); ++i) {
		auto o = pool.allocate();
		// Ensure that every object in the pool is correctly allocated.
		REQUIRE(o != std::nullopt);
	}
}

TEST_CASE("object_pool construction with generator function", "[object_pool][constructor]") {
	struct foo { };
	auto generator = [] { return foo{}; };
	genesis::object_pool<foo> pool{42, generator};
	std::vector<std::shared_ptr<foo>> objs{};
	for (std::size_t i = 0; i < pool.capacity(); ++i) {
		auto o = pool.allocate();
		REQUIRE(o != std::nullopt);
	}
}

TEST_CASE("object_pool allocations", "[object_pool][allocate]") {
	struct foo { };
	genesis::object_pool<foo> pool{42};
	auto o = pool.allocate();
	REQUIRE(o != std::nullopt);
}

TEST_CASE("object_pool allocations return std::nullopt if exhausted", "[object_pool][allocate]") {
	struct foo { };
	genesis::object_pool<foo> pool{42};
	std::vector<std::shared_ptr<foo>> obj_holder{};
	for (std::size_t i = 0; i < pool.capacity(); ++i) {
		obj_holder.emplace_back(*pool.allocate());
	}
	auto o = pool.allocate();
	REQUIRE(o == std::nullopt);
}

TEST_CASE("object_pool thread safe", "[object_pool][thread_safety]") {
	struct foo { };
	genesis::object_pool<foo> pool{42};
	std::vector<std::shared_ptr<foo>> obj_holder_one{};	
	std::vector<std::shared_ptr<foo>> obj_holder_two{};	
	auto work1 = std::thread{[&pool, &obj_holder_one] {
		for (std::size_t i = 0; i < pool.capacity() / 2; ++i) {
			obj_holder_one.emplace_back(*pool.allocate());
		}
	}};

	auto work2 = std::thread{[&pool, &obj_holder_two] {
		for (std::size_t i = 0; i < pool.capacity() / 2; ++i) {
			obj_holder_two.emplace_back(*pool.allocate());
		}
	}};
	work1.join();
	work2.join();
	REQUIRE(obj_holder_one.size() == 21);
	REQUIRE(obj_holder_two.size() == 21);
}