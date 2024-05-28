#include "genesis/stop_token.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
	genesis::inplace_stop_source source;

	std::thread task{[&source] {
		auto token = source.get_token();
		while (!token.stop_requested()) {
			std::cout << "Hi thread one" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds{100});
		}
		std::cout << "Bye from thread one." << std::endl;
	}};

	std::thread task1{[&source] {
		auto token = source.get_token();
		while (!token.stop_requested()) {
			std::cout << "Hi thread two" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds{50});
		}
		std::cout << "Bye from thread two." << std::endl;
	}};
	std::this_thread::sleep_for(std::chrono::milliseconds{1000});
	source.request_stop();
	task.join();
	task1.join();
	return 0;
}