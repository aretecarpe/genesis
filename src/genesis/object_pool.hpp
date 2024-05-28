#if !defined GENESIS_OBJECT_POOL_HEADER_INCLUDED
#define GENESIS_OBJECT_POOL_HEADER_INCLUDED
#pragma once

#include "genesis/memory.hpp"

#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace genesis {

template <typename T>
class object_pool;

namespace details {

template <typename T>
class pool_node {
private:
	pool_node* next_;
	object_pool<T>* home_;
	alignas(T) std::byte data_[sizeof(T)];  // Raw storage for T

public:
	pool_node(object_pool<T>* init_home, pool_node* init_next) noexcept :
		home_{init_home},
		next_{init_next}
	{ }

	pool_node(const pool_node&) = delete;

	pool_node(pool_node&&) = delete;

	pool_node& operator=(const pool_node&) = delete;

	pool_node& operator=(pool_node&&) = delete;

	[[nodiscard]] pool_node* link(pool_node* next) noexcept { next_ = next; return next; }

	T& operator*() noexcept { return *std::launder(reinterpret_cast<T*>(data_)); }

	const T& operator*() const noexcept { return *std::launder(reinterpret_cast<const T*>(data_)); }

	T* operator->() noexcept { return std::launder(reinterpret_cast<T*>(data_)); }

	const T* operator->() const noexcept { return std::launder(reinterpret_cast<const T*>(data_)); }

	[[nodiscard]] T* data() noexcept { return std::launder(reinterpret_cast<T*>(data_)); }

	[[nodiscard]] const T* data() const noexcept { return std::launder(reinterpret_cast<const T*>(data_)); }

	[[nodiscard]] object_pool<T>* home() noexcept { return home_; }

	void next(pool_node* n) noexcept { next_ = n; }

	[[nodiscard]] pool_node* next() noexcept { return next_; }

	[[nodiscard]] const pool_node* next() const noexcept { return next_; }

	template <typename... Args>
	void construct(Args&&... args) {
		genesis::construct_at(std::launder(reinterpret_cast<T*>(data_)), std::forward<Args>(args)...);
	}

	void destroy() {
		std::destroy_at(std::launder(reinterpret_cast<T*>(data_)));
	}

	friend class object_pool<T>;
};

} // end namespace details

/// @brief The object_pool class allows for the allocation of N number of T objects in a thread safe manner. 
/// An object can be borrowed from the pool and will be returned to the pool upon destruction.
/// @tparam T The type to allocate in the pool
template <typename T>
class object_pool {
public:
	using node = details::pool_node<T>;

private:
	std::pmr::polymorphic_allocator<std::byte> allocator_;
	std::pmr::vector<std::unique_ptr<node>> nodes_;
	std::atomic<node*> head_;
	std::size_t capacity_;
	std::atomic<uint32_t> waiters_;
	std::mutex mutex_;
	std::condition_variable ready_;

public:
	/// @brief Construct a new object_pool object.
	/// @tparam Generator Function that returns a object type T.
	/// @param init_capacity The initial capacity for the object_pool.
	/// @param gen The generaotr function that must return an object of type T.
	/// @param mem_resource The memory resource in which to do the allocations.
	template <
		typename Generator, 
		std::enable_if_t<std::is_invocable_r_v<T, Generator>, int> = 0
	>
	object_pool(
		std::size_t init_capacity,
		Generator gen,
		std::pmr::memory_resource* mem_resource = std::pmr::get_default_resource()
	) :
		allocator_{mem_resource},
		nodes_{allocator_},
		capacity_{init_capacity},
		head_{nullptr},
		waiters_{0}
	{
		initial_allocation(gen);
	}

	/// @brief Construct a new object_pool object with all objects being constructed with the default constructor of type T.
	/// @param init_capacity The initial capacity for the object_pool.
	/// @param mem_resource The memory resource in which to do the allocations.
	explicit object_pool(std::size_t init_capacity, std::pmr::memory_resource* mem_resource = std::pmr::get_default_resource()) :
		allocator_{mem_resource},
		nodes_{allocator_},
		capacity_{init_capacity},
		head_{nullptr},
		waiters_{0}
	{
		initial_allocation([]() { return T{}; });
	}

	~object_pool() {
		for (auto& n : nodes_) {
			n->destroy();
		}
	}

	/// @brief Capacity observor for object_pool.
	/// @return std::size_t the total capactiy of the pool.
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

	/// @brief Const node observor for the head node of the pool.
	/// @return const node* the head of the node.
	[[nodiscard]] const node* head() const noexcept { return head_.load(); }

	/// @brief Node observor for the head of the pool 
	/// @return node* the head of the node.
	[[nodiscard]] node* head() noexcept { return head_.load(); }

	/// @brief Allocates an object from the pool.
	/// If a valid allocation will return a std::optional with a valid shared pointer of type T.
	/// Otherwise will return a std::nullopt.
	/// @return std::optional<std::shared_ptr<T>>
	[[nodiscard]] std::optional<std::shared_ptr<T>> allocate();

	/// @brief Deallocates a node and returns it back to the object_pool
	/// @param n The node to be deleted from the pool
	void deallocate(node* n) noexcept {
		if (n == nullptr) { return; }
		auto old_head = head_.load(std::memory_order_relaxed);
		do {
			n->next_ = old_head;
		} while (!head_.compare_exchange_weak(old_head, n, std::memory_order_release, std::memory_order_relaxed));
		if (waiters_ > 0) {
			std::scoped_lock lock{mutex_};
			ready_.notify_one();
		}
	}

	/// @brief The number of waiters.
	/// @return uint32_T
	[[nodiscard]] uint32_t waiters() const noexcept { return waiters_; }

private:
	template <typename F>
	void initial_allocation(F init) {
		node* prev = nullptr;
		for (std::size_t n = 0; n < capacity_; ++n) {
			auto* raw_memory = allocator_.allocate(sizeof(object_pool::node));
			auto node = genesis::construct_at(reinterpret_cast<object_pool::node*>(raw_memory), this, prev);
			node->construct(init());
			prev = node;
			nodes_.emplace_back(node);
		}
		head_ = prev;
	}

	node* do_allocate() {
		node* p = nullptr;
		while (true) {
			p = head_.load(std::memory_order_acquire);
			if (p == nullptr) return nullptr;
			if (head_.compare_exchange_weak(p, p->next_, std::memory_order_release, std::memory_order_relaxed)) break;
			std::this_thread::yield();
		}
		return p;
	}
};

/// @brief Deleter functor that returns items to the pool on destruction.
/// @tparam T The type T.
template <typename T>
struct pool_deleter {
	using node_type = typename object_pool<T>::node;

	node_type* node_;
	
	pool_deleter(node_type* init_node) : node_{init_node} { }

	void operator()(T*) {
		if (node_ != nullptr) { node_->home()->deallocate(node_); }
	}
};

/// @brief Allocates an object from the pool and returns a std::optional.
/// @tparam T The type T to allocate from the pool
/// @return std::optional<std::shared_ptr<T>>
template <typename T>
[[nodiscard]] std::optional<std::shared_ptr<T>> object_pool<T>::allocate() {
	auto node = do_allocate();
	if (node != nullptr) {
		return std::make_optional(std::shared_ptr<T>{node->data(), pool_deleter<T>{node}});
	}
	++waiters_;
	std::unique_lock<std::mutex> lock{mutex_};
	bool timeout = ready_.wait_for(lock, std::chrono::milliseconds{50}) == std::cv_status::timeout;
	--waiters_;
	if (!timeout) {
		node = do_allocate();
		if (node != nullptr) {
			return std::make_optional(std::shared_ptr<T>{node->data(), pool_deleter<T>{node}});
		}
	}
	return std::nullopt;
}

} // end namespace genesis

#endif