#if !defined GENESIS_STOP_TOKEN_HEADER_INCLUDED
#define GENESIS_STOP_TOKEN_HEADER_INCLUDED
#pragma once

#include "genesis/utility.hpp"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <utility>
#include <version>
#include <memory>

namespace genesis {

using stop_state = std::atomic<bool>;
using stop_token_shared_resource = std::shared_ptr<stop_state>;

class stop_token;

class stop_source {
private:
	stop_token_shared_resource state_;

public:
	stop_source() :
		state_{std::make_shared<stop_state>(false)}
	{ }

	explicit stop_source(std::nullptr_t) noexcept :
		state_{}
	{ }

	stop_source(const stop_source&) noexcept = default;

	stop_source(stop_source&&) noexcept = default;

	stop_source& operator=(const stop_source&) = default;

	stop_source& operator=(stop_source&&) = default;

	void swap(stop_source& s) noexcept { state_.swap(s.state_); }

	auto get_token() const noexcept -> stop_token;

	auto stop_possible() const noexcept -> bool { return static_cast<bool>(state_); }

	auto stop_requested() const noexcept -> bool { return stop_possible() && *state_; }

	auto request_stop() noexcept -> bool {
		const auto old_state = stop_requested();
		if (stop_possible()) { *state_ = true; }
		return old_state;
	}
};

class stop_token {
private:
	stop_token_shared_resource state_;

public:
	stop_token() :
		state_{}
	{ }

	stop_token(const stop_token&) = default;

	stop_token(stop_token&&) = default;

	stop_token& operator=(const stop_token&) = default;

	stop_token& operator=(stop_token&&) = default;

	auto stop_requested() const noexcept -> bool {
		return state_ && *state_;
	}

	auto stop_possible() const noexcept -> bool {
		return static_cast<bool>(state_);
	}

private:
	stop_token(const stop_token_shared_resource& init_state) :
		state_{init_state}
	{ }

	friend class stop_source;
};

inline auto stop_source::get_token() const noexcept -> stop_token {
	return { state_ };
}

// [stoptoken.inplace], class inplace_stop_token
class inplace_stop_token;

// [stopsource.inplace], class inplace_stop_source
class inplace_stop_source;

// [stopcallback.inplace], class template inplace_stop_callback
template <class _Callback>
class inplace_stop_callback;

namespace stok {

struct inplace_stop_callback_base {
protected:
	using execute_fn_t = void(inplace_stop_callback_base*) noexcept;

	const inplace_stop_source* source_;
	execute_fn_t* execute_fn_;
	inplace_stop_callback_base* next_;
	inplace_stop_callback_base** prev_ptr_;
	bool* removed_during_callback_;
	std::atomic<bool> callback_completed_;

public:
	void execute() noexcept {
		this->execute_fn_(this);
	}

protected:
	explicit inplace_stop_callback_base(
		const inplace_stop_source* source,
		execute_fn_t* execute
	) noexcept :
		source_{source},
		execute_fn_{execute},
		next_{nullptr},
		prev_ptr_{nullptr},
		removed_during_callback_{nullptr},
		callback_completed_{false}
	{ }

	void register_callback() noexcept;

	friend inplace_stop_source;
};

struct spin_wait {
private:
	static constexpr uint32_t yield_threshold = 20;
	uint32_t count_ = 0;

public:
	spin_wait() noexcept = default;

	void wait() noexcept {
		if (count_++ < yield_threshold) {
		// TODO: _mm_pause();
		} else {
		if (count_ == 0)
			count_ = yield_threshold;
		std::this_thread::yield();
		}
	}
};

template <template <class> class>
struct check_type_alias_exists;

} // namespace stok

// [stoptoken.never], class never_stop_token
struct never_stop_token {
private:
	struct callback_type_t {
		explicit callback_type_t(never_stop_token, ignore_t) noexcept { }
	};

public:
	template <class>
	using callback_type = callback_type_t;

	static constexpr auto stop_requested() noexcept -> bool {
		return false;
	}

	static constexpr auto stop_possible() noexcept -> bool {
		return false;
	}

	friend constexpr bool
	operator==(const never_stop_token&, const never_stop_token&) noexcept {
		return true;
	}

	friend constexpr bool
	operator!=(const never_stop_token&, const never_stop_token&) noexcept {
		return false;
	}
};

template <class _Callback>
class inplace_stop_callback;

// [stopsource.inplace], class inplace_stop_source
class inplace_stop_source {
private:
	mutable std::atomic<uint8_t> state_{0};
	mutable stok::inplace_stop_callback_base *callbacks_ = nullptr;
	std::thread::id notifying_thread_;

	static constexpr uint8_t stop_requested_flag{1};
	static constexpr uint8_t locked_flag{2};

public:
	inplace_stop_source() noexcept = default;

	inplace_stop_source(inplace_stop_source &&) = delete;

	~inplace_stop_source();
	
	auto get_token() const noexcept -> inplace_stop_token;

	auto request_stop() noexcept -> bool;

	auto stop_requested() const noexcept -> bool {
		return (state_.load(std::memory_order_acquire) & stop_requested_flag) != 0;
	}

private:
	friend inplace_stop_token;
	friend stok::inplace_stop_callback_base;
	template <class>
	friend class inplace_stop_callback;

	auto lock() const noexcept -> uint8_t;

	void unlock(uint8_t) const noexcept;

	auto try_lock_unless_stop_requested(bool) const noexcept -> bool;

	auto try_add_callback(stok::inplace_stop_callback_base *) const noexcept -> bool;

	void remove_callback(stok::inplace_stop_callback_base *) const noexcept;
};

// [stoptoken.inplace], class inplace_stop_token
class inplace_stop_token {
private:
	const inplace_stop_source *source_;

public:
	template <class Fun>
	using callback_type = inplace_stop_callback<Fun>;

	inplace_stop_token() noexcept : 
		source_(nullptr) 
	{ }

	inplace_stop_token(const inplace_stop_token &_other) noexcept = default;

	inplace_stop_token(inplace_stop_token &&_other) noexcept : 
		source_(std::exchange(_other.source_, {})) 
	{ }

	auto operator=(const inplace_stop_token &_other) noexcept -> inplace_stop_token & = default;

	auto operator=(inplace_stop_token &&_other) noexcept -> inplace_stop_token & {
		source_ = std::exchange(_other.source_, nullptr);
		return *this;
	}

	[[nodiscard]] auto stop_requested() const noexcept -> bool {
		return source_ != nullptr && source_->stop_requested();
	}

	[[nodiscard]] auto stop_possible() const noexcept -> bool {
		return source_ != nullptr;
	}

	void swap(inplace_stop_token &_other) noexcept {
		std::swap(source_, _other.source_);
	}

	friend bool operator==(const inplace_stop_token &_a, const inplace_stop_token &_b) noexcept {
		return _a.source_ == _b.source_;
	}

	friend bool operator!=(const inplace_stop_token &_a, const inplace_stop_token &_b) noexcept {
		return _a.source_ != _b.source_;
	}

private:
	friend inplace_stop_source;
	template <class>
	friend class inplace_stop_callback;

	explicit inplace_stop_token(const inplace_stop_source *init_source) noexcept : 
		source_(init_source) 
	{ }
};

inline auto inplace_stop_source::get_token() const noexcept -> inplace_stop_token {
	return inplace_stop_token{this};
}

// [stopcallback.inplace], class template inplace_stop_callback
template <class Fun>
class inplace_stop_callback : stok::inplace_stop_callback_base {
public:
	template <class Fun2>
	explicit inplace_stop_callback(inplace_stop_token init_token, Fun2&& init_fun) noexcept(
		std::is_nothrow_constructible_v<Fun, Fun2>
	) :
		stok::inplace_stop_callback_base(
			init_token.source_,
			&inplace_stop_callback::execute_impl
		),
		fun_(static_cast<Fun2 &&>(init_fun)) 
	{
		register_callback();
	}

	~inplace_stop_callback() {
		if (source_ != nullptr)
			source_->remove_callback(this);
	}

private:
	static void execute_impl(stok::inplace_stop_callback_base *cb) noexcept {
		std::move(static_cast<inplace_stop_callback *>(cb)->fun_)();
	}

	Fun fun_;
};

namespace stok {
	inline void inplace_stop_callback_base::register_callback() noexcept {
		if (source_ != nullptr) {
			if (!source_->try_add_callback(this)) {
				source_ = nullptr;
				// Callback not registered because stop_requested() was true.
				// Execute inline here.
				execute();
			}
		}
	}
} // namespace stok

inline inplace_stop_source::~inplace_stop_source() {
	assert((state_.load(std::memory_order_relaxed) & locked_flag) == 0);
	assert(callbacks_ == nullptr);
}

inline auto inplace_stop_source::request_stop() noexcept -> bool {
	if (!try_lock_unless_stop_requested(true))
	return true;

	notifying_thread_ = std::this_thread::get_id();

	// We are responsible for executing callbacks.
	while (callbacks_ != nullptr) {
		auto *callbk = callbacks_;
		callbk->prev_ptr_ = nullptr;
		callbacks_ = callbk->next_;
		if (callbacks_ != nullptr)
			callbacks_->prev_ptr_ = &callbacks_;

		state_.store(stop_requested_flag, std::memory_order_release);

		bool removed_during_callback_ = false;
		callbk->removed_during_callback_ = &removed_during_callback_;

		callbk->execute();

		if (!removed_during_callback_) {
			callbk->removed_during_callback_ = nullptr;
			callbk->callback_completed_.store(true, std::memory_order_release);
		}

		lock();
	}
	state_.store(stop_requested_flag, std::memory_order_release);
	return false;
}

inline auto inplace_stop_source::lock() const noexcept -> uint8_t {
	stok::spin_wait spin;
	auto old_state = state_.load(std::memory_order_relaxed);
	do {
		while ((old_state & locked_flag) != 0) {
			spin.wait();
			old_state = state_.load(std::memory_order_relaxed);
		}
	} while (!state_.compare_exchange_weak(
			old_state,
			old_state | locked_flag,
			std::memory_order_acquire,
			std::memory_order_relaxed
		)
	);

	return old_state;
}

inline void inplace_stop_source::unlock(uint8_t old_state) const noexcept {
	(void) state_.store(old_state, std::memory_order_release);
}

inline auto inplace_stop_source::try_lock_unless_stop_requested(bool set_stop_requested) const noexcept -> bool {
	stok::spin_wait spin;
	auto old_state = state_.load(std::memory_order_relaxed);
	do {
		while (true) {
			if ((old_state & stop_requested_flag) != 0) {
				// Stop already requested.
				return false;
			} else if (old_state == 0) {
				break;
			} else {
				spin.wait();
				old_state = state_.load(std::memory_order_relaxed);
			}
		}
	} while (!state_.compare_exchange_weak(
			old_state,
			set_stop_requested ? (locked_flag | stop_requested_flag) : locked_flag,
			std::memory_order_acq_rel,
			std::memory_order_relaxed
		)
	);
	// Lock acquired successfully
	return true;
}

inline auto inplace_stop_source::try_add_callback(stok::inplace_stop_callback_base* callbk) const noexcept -> bool {
	if (!try_lock_unless_stop_requested(false)) {
		return false;
	}

	callbk->next_ = callbacks_;
	callbk->prev_ptr_ = &callbacks_;
	if (callbacks_ != nullptr) {
		callbacks_->prev_ptr_ = &callbk->next_;
	}
	callbacks_ = callbk;

	unlock(0);

	return true;
}

inline void inplace_stop_source::remove_callback(stok::inplace_stop_callback_base* callbk) const noexcept {
	auto old_state = lock();

	if (callbk->prev_ptr_ != nullptr) {
	// Callback has not been executed yet.
	// Remove from the list.
	*callbk->prev_ptr_ = callbk->next_;
	if (callbk->next_ != nullptr) {
		callbk->next_->prev_ptr_ = callbk->prev_ptr_;
	}
	unlock(old_state);
	} else {
		auto notifying_thread_ = this->notifying_thread_;
		unlock(old_state);

		// Callback has either already been executed or is
		// currently executing on another thread.
		if (std::this_thread::get_id() == notifying_thread_) {
			if (callbk->removed_during_callback_ != nullptr) {
				*callbk->removed_during_callback_ = true;
			}
		} else {
			// Concurrently executing on another thread.
			// Wait until the other thread finishes executing the callback.
			stok::spin_wait spin;
			while (
				!callbk->callback_completed_.load(std::memory_order_acquire)) {
				spin.wait();
			}
		}
	}
}

struct on_stop_request {
	inplace_stop_source &source_;

	void operator()() const noexcept {
		source_.request_stop();
	}
};

template <class Token, class Callback>
using stop_callback_for_t = typename Token::template callback_type<Callback>;

} // end namespace genesis

#endif