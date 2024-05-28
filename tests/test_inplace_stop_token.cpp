#include "genesis/stop_token.hpp"

#include "catch2/catch_all.hpp"

#include <thread>
#include <chrono>

TEST_CASE("inplace_stop_source stop_requested not stopped test", "[inplace_stop_token][inplace_stop_source]") {
	genesis::inplace_stop_source source{};
	REQUIRE(source.stop_requested() == false);
}

TEST_CASE("stop_source stop_requested stopped test", "[inplace_stop_token][inplace_stop_source]") {
	genesis::inplace_stop_source source{};
	REQUIRE(source.request_stop() == false);
	REQUIRE(source.stop_requested() == true);
	REQUIRE(source.request_stop() == true);
}

TEST_CASE("stop_token state test", "[inplace_stop_token][inplace_stop_token]") {
	genesis::inplace_stop_token token{};
	REQUIRE(token.stop_possible() == false);
	REQUIRE(token.stop_requested() == false);
}

TEST_CASE("stop_token from stop_source test", "[inplace_stop_token][inplace_stop_token]") {
	genesis::inplace_stop_source source{};
	auto token = source.get_token();

	REQUIRE(token.stop_possible() == true);
	REQUIRE(token.stop_requested() == false);
	source.request_stop();
	REQUIRE(token.stop_possible() == true);
	REQUIRE(token.stop_requested() == true);
}

TEST_CASE("stop_source shared between threads", "[inplace_stop_token][inplace_stop_source]") {
	genesis::inplace_stop_source source{};
	bool t1_cleaned_up = false;
	bool t2_cleaned_up = false;

	std::thread t1{[&source, &t1_cleaned_up]{
		auto token = source.get_token();
		while (!token.stop_requested()) { }
		t1_cleaned_up = true;
	}};

	std::thread t2{[&source, &t2_cleaned_up]{
		auto token = source.get_token();
		while (!token.stop_requested()) { }
		t2_cleaned_up = true;
	}};
	REQUIRE(source.get_token().stop_possible() == true);
	std::this_thread::sleep_for(std::chrono::milliseconds{50});
	source.request_stop();
	t1.join();
	t2.join();
	REQUIRE(t1_cleaned_up == true);
	REQUIRE(t2_cleaned_up == true);
}