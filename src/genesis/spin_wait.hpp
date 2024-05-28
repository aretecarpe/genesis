#if !defined GENESIS_SPIN_WAIT_HEADER_INCLUDED
#define GENESIS_SPIN_WAIT_HEADER_INCLUDED
#pragma once

#include "genesis/details/thread.hpp"

#include <cstdint>
#include <thread>

namespace genesis {

struct spin_wait {
private:
	static constexpr uint32_t yield_threshold{20};
	uint32_t count_{yield_threshold};

public:
	spin_wait() noexcept = default;

	void wait() noexcept {
		if (count_ == 0) {
			std::this_thread::yield();
		} else {
			--count_;
			mm_pause();
		}
	}
};

} // end namespace genesis

#endif