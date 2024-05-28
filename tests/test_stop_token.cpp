#include "genesis/stop_token.hpp"

#include "catch2/catch_all.hpp"

#include <thread>
#include <chrono>

TEST_CASE("stop_source stop_requested not stopped test", "[stop_token][stop_source]") {
	genesis::stop_source source{};
	REQUIRE(source.stop_requested() == false);
	REQUIRE(source.stop_possible() == true);
}

TEST_CASE("stop_source stop_requested stopped test", "[stop_token][stop_source]") {
	genesis::stop_source source{};
	REQUIRE(source.request_stop() == false);
	REQUIRE(source.stop_requested() == true);
	REQUIRE(source.request_stop() == true);
}

TEST_CASE("stop_source states with invalid value", "[stop_token][stop_source]") {
	genesis::stop_source source{nullptr};
	REQUIRE(source.stop_requested() == false);
	REQUIRE(source.stop_possible() == false);
	REQUIRE(source.request_stop() == false);
	REQUIRE(source.stop_requested() == false);
	REQUIRE(source.stop_possible() == false);
}

TEST_CASE("stop_source swap", "[stop_token][stop_source]") {
	genesis::stop_source source1{nullptr};
	genesis::stop_source source2{};

	source1.swap(source2);
	REQUIRE(source2.stop_requested() == false);
	REQUIRE(source2.stop_possible() == false);
	REQUIRE(source2.request_stop() == false);
	REQUIRE(source2.stop_requested() == false);
	REQUIRE(source2.stop_possible() == false);

	REQUIRE(source1.stop_requested() == false);
	REQUIRE(source1.stop_possible() == true);
	REQUIRE(source1.request_stop() == false);
	REQUIRE(source1.stop_requested() == true);
	REQUIRE(source1.stop_possible() == true);
}

TEST_CASE("stop_token state test", "[stop_token][stop_token]") {
	genesis::stop_token token{};
	REQUIRE(token.stop_possible() == false);
	REQUIRE(token.stop_requested() == false);
}

TEST_CASE("stop_token from stop_source test", "[stop_token][stop_token]") {
	genesis::stop_source source{};
	auto token = source.get_token();

	REQUIRE(token.stop_possible() == true);
	REQUIRE(token.stop_requested() == false);
	source.request_stop();
	REQUIRE(token.stop_possible() == true);
	REQUIRE(token.stop_requested() == true);
}

TEST_CASE("stop_source shared between threads", "[stop_token][stop_source]") {
	genesis::stop_source source{};
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
	REQUIRE(source.stop_possible() == true);
	std::this_thread::sleep_for(std::chrono::milliseconds{50});
	source.request_stop();
	t1.join();
	t2.join();
	REQUIRE(t1_cleaned_up == true);
	REQUIRE(t2_cleaned_up == true);
}