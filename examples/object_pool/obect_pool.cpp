#include "genesis/object_pool.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
	genesis::object_pool<int32_t> pool{42};
	std::vector<std::shared_ptr<int32_t>> objs{};
	for (std::size_t i = 0; i < pool.capacity(); ++i) {
		auto o = pool.allocate();
		objs.push_back(*o);
	}
	auto f = pool.allocate();
	if (f) {
		std::cout << "We still have room..." << std::endl;
	} else {
		std::cout << "Pool is exhausted..." << std::endl;
	}
	return 0;
}