/// Implementation based on https://github.com/TartanLlama/expected from: Sy Brand (tartanllama@gmail.com, @TartanLlama)
#if !defined GENESIS_EXPECTED_HEADER_INCLUDED
#define GENESIS_EXPECTED_HEADER_INCLUDED
#pragma once

#include "genesis/type_traits.hpp"

#include <cassert>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace genesis {

template <typename T, typename E> 
class expected;

constexpr struct in_place_t {
  explicit in_place_t() = default;

} in_place {};

template <typename E>
class unexpected {
private:
	E val_;

public:
	static_assert(!std::is_same<E, void>::value, "E must not be void");

	unexpected() = delete;

	constexpr explicit unexpected(const E &e) : val_(e) { }

	constexpr explicit unexpected(E &&e) : val_(std::move(e)) { }

	template <
		typename... Args, 
		std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0
	>
	constexpr explicit unexpected(Args&&...args)
		: val_(std::forward<Args>(args)...)
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_constructible_v<E, std::initializer_list<U>&, Args &&...>, int
		> = 0
	>
	constexpr explicit unexpected(std::initializer_list<U> il, Args&&...args) :
		val_(il, std::forward<Args>(args)...)
	{ }

	[[nodiscard]] constexpr const E &value() const & { return val_; }

	[[nodiscard]] constexpr E &value() & { return val_; }

	[[nodiscard]] constexpr E &&value() && { return std::move(val_); }

	[[nodiscard]] constexpr const E &&value() const && { return std::move(val_); }
};

/// Template deduction guide.
template <typename E>
unexpected(E) -> unexpected<E>;

template <typename E>
constexpr bool operator==(const unexpected<E>& lhs, const unexpected<E>& rhs) {
	return lhs.value() == rhs.value();
}
template <typename E>
constexpr bool operator!=(const unexpected<E>& lhs, const unexpected<E>& rhs) {
	return lhs.value() != rhs.value();
}
template <typename E>
constexpr bool operator<(const unexpected<E>& lhs, const unexpected<E>& rhs) {
	return lhs.value() < rhs.value();
}
template <typename E>
constexpr bool operator<=(const unexpected<E>& lhs, const unexpected<E>& rhs) {
	return lhs.value() <= rhs.value();
}
template <typename E>
constexpr bool operator>(const unexpected<E>& lhs, const unexpected<E>& rhs) {
	return lhs.value() > rhs.value();
}
template <typename E>
constexpr bool operator>=(const unexpected<E>& lhs, const unexpected<E>& rhs) {
	return lhs.value() >= rhs.value();
}

constexpr struct unexpect_t {
	unexpect_t() = default;
} unexpect { };

namespace details {

template <typename E>
[[noreturn]] constexpr void throw_exception(E &&e) {
	throw std::forward<E>(e);
	(void)e;
}

template<typename _Tp>
constexpr bool is_expected = false;
template<typename _Tp, typename _Er>
constexpr bool is_expected<expected<_Tp, _Er>> = true;

template <typename T, typename E, typename U>
inline constexpr bool expected_enable_forward_value = (
	std::is_constructible_v<T, U&&> &&
	!std::is_same_v<std::decay_t<U>, in_place_t> &&
	!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
	!std::is_same_v<unexpected<E>, std::decay_t<U>>
);

template <typename T, typename E, typename U, typename G, typename UR, typename GR>
inline constexpr bool expected_enable_from_otherr = (
	std::is_constructible_v<T, UR> &&
	std::is_constructible_v<E, GR> &&
	!std::is_constructible_v<T, expected<U, G> &> &&
	!std::is_constructible_v<T, expected<U, G> &&> &&
	!std::is_constructible_v<T, const expected<U, G> &> &&
	!std::is_constructible_v<T, const expected<U, G> &&> &&
	!std::is_convertible_v<expected<U, G> &, T> &&
	!std::is_convertible_v<expected<U, G> &&, T> &&
	!std::is_convertible_v<const expected<U, G> &, T> &&
	!std::is_convertible_v<const expected<U, G> &&, T>
);

} // namespace details

namespace details {

struct no_init_t {};
static constexpr no_init_t no_init{};

// Implements the storage of the values, and ensures that the destructor is
// trivial if it can be.
//
// This specialization is for where neither `T` or `E` is trivially
// destructible, so the destructors must be called on destruction of the
// `expected`
template <
	typename T, 
	typename E, 
	bool = std::is_trivially_destructible_v<T>,
	bool = std::is_trivially_destructible_v<E>
>
struct expected_storage_base {
	constexpr expected_storage_base() :
  		val_(T{}), 
		has_val_(true) 
	{ }

	constexpr expected_storage_base(no_init_t) : 
		no_init_(), 
		has_val_(false) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>, int> = 0>
	constexpr expected_storage_base(in_place_t, Args &&...args) : 
  		val_(std::forward<Args>(args)...), 
		has_val_(true)
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args &&...args) :
		val_(il, std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, Args &&...args) : 
		unex_(std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args &&...args) :
		unex_(il, std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	~expected_storage_base() {
		if (has_val_) {
			val_.~T();
		} else {
			unex_.~unexpected<E>();
		}
	}

	union {
		T val_;
		unexpected<E> unex_;
		char no_init_;
	};

  	bool has_val_;
};

// This specialization is for when both `T` and `E` are trivially-destructible,
// so the destructor of the `expected` can be trivial.
template <typename T, typename E> 
struct expected_storage_base<T, E, true, true> {
	constexpr expected_storage_base() :
		val_(T{}), 
		has_val_(true) 
	{ }

	constexpr expected_storage_base(no_init_t) : 
		no_init_(), 
		has_val_(false) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>, int> = 0>
	constexpr expected_storage_base(in_place_t, Args &&...args) :
		val_(std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <typename U, typename... Args, std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>, int> = 0>
	constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args &&...args) :
		val_(il, std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, Args &&...args) :
		unex_(std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	template <typename U, typename... Args, std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&& ...args) :
		unex_(il, std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	~expected_storage_base() = default;

	union {
		T val_;
		unexpected<E> unex_;
		char no_init_;
	};
	
	bool has_val_;
};

// T is trivial, E is not.
template <typename T, typename E> struct expected_storage_base<T, E, true, false> {
	constexpr expected_storage_base() : 
		val_(T{}), 
		has_val_(true) 
	{ }

	constexpr expected_storage_base(no_init_t) :
		no_init_(), 
		has_val_(false) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>, int> = 0>
	constexpr expected_storage_base(in_place_t, Args &&...args) :
		val_(std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <typename U, typename... Args, std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args...>, int> = 0>
	constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args &&...args) :
		val_(il, std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, Args &&...args) :
		unex_(std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	template <typename U, typename... Args, std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&& ...args) :
		unex_(il, std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	~expected_storage_base() {
		if (!has_val_) {
		unex_.~unexpected<E>();
		}
	}

	union {
		T val_;
		unexpected<E> unex_;
		char no_init_;
	};

  	bool has_val_;
};

// E is trivial, T is not.
template <typename T, typename E> struct expected_storage_base<T, E, false, true> {
	constexpr expected_storage_base() : val_(T{}), has_val_(true) {}

	constexpr expected_storage_base(no_init_t) : no_init_(), has_val_(false) {}

	template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>, int> = 0>
	constexpr expected_storage_base(in_place_t, Args &&...args) :
		val_(std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <
		typename U, 
		typename... Args, 
		std::enable_if_t<
			std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args &&...args) :
		val_(il, std::forward<Args>(args)...), 
		has_val_(true) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, Args &&...args) :
		unex_(std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args &&...args) :
		unex_(il, std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	~expected_storage_base() {
		if (has_val_) {
			val_.~T();
		}
	}

	union {
		T val_;
		unexpected<E> unex_;
		char no_init_;
	};
	
	bool has_val_;
};

// `T` is `void`, `E` is trivially-destructible
template <typename E> struct expected_storage_base<void, E, false, true> {
	constexpr expected_storage_base() : has_val_(true) {}
		
	constexpr expected_storage_base(no_init_t) : val_(), has_val_(false) {}

	constexpr expected_storage_base(in_place_t) : has_val_(true) {}

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, Args &&...args) :
		unex_(std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int> = 0
	>
	constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args &&...args) :
		unex_(il, std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	~expected_storage_base() = default;

	struct dummy {};
	union {
		unexpected<E> unex_;
		dummy val_;
	};
	bool has_val_;
};

// `T` is `void`, `E` is not trivially-destructible
template <typename E> struct expected_storage_base<void, E, false, false> {
	constexpr expected_storage_base() : dummy_(), has_val_(true) {}

	constexpr expected_storage_base(no_init_t) : dummy_(), has_val_(false) {}

	constexpr expected_storage_base(in_place_t) : dummy_(), has_val_(true) {}

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected_storage_base(unexpect_t, Args &&...args) : 
		unex_(std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int> = 0
	>
	constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args &&...args) :
		unex_(il, std::forward<Args>(args)...), 
		has_val_(false) 
	{ }

	~expected_storage_base() {
		if (!has_val_) {
			unex_.~unexpected<E>();
		}
	}

	union {
		unexpected<E> unex_;
		char dummy_;
	};
	bool has_val_;
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <typename T, typename E>
struct expected_operations_base : expected_storage_base<T, E> {
	using expected_storage_base<T, E>::expected_storage_base;

	template <typename... Args> void construct(Args &&...args) noexcept {
		new (std::addressof(this->val_)) T(std::forward<Args>(args)...);
		this->has_val_ = true;
	}

	template <class Rhs> void construct_with(Rhs &&rhs) noexcept {
		new (std::addressof(this->val_)) T(std::forward<Rhs>(rhs).get());
		this->has_val_ = true;
	}

	template <typename... Args> void construct_error(Args &&...args) noexcept {
		new (std::addressof(this->unex_)) unexpected<E>(std::forward<Args>(args)...);
		this->has_val_ = false;
	}

	// If exceptions are disabled then we can just copy-construct
	void assign(const expected_operations_base &rhs) noexcept {
		if (!this->has_val_ && rhs.has_val_) {
			geterr().~unexpected<E>();
			construct(rhs.get());
		} else {
			assign_common(rhs);
		}
	}

	void assign(expected_operations_base &&rhs) noexcept {
		if (!this->has_val_ && rhs.has_val_) {
			geterr().~unexpected<E>();
			construct(std::move(rhs).get());
		} else {
			assign_common(std::move(rhs));
		}
	}

	// The common part of move/copy assigning
	template <class Rhs> void assign_common(Rhs &&rhs) {
		if (this->has_val_) {
			if (rhs.has_val_) {
			get() = std::forward<Rhs>(rhs).get();
			} else {
			destroy_val();
			construct_error(std::forward<Rhs>(rhs).geterr());
			}
		} else {
			if (!rhs.has_val_) {
			geterr() = std::forward<Rhs>(rhs).geterr();
			}
		}
	}

	[[nodiscard]] bool has_value() const { return this->has_val_; }

	[[nodiscard]] constexpr T &get() & { return this->val_; }

	[[nodiscard]] constexpr const T &get() const & { return this->val_; }

	[[nodiscard]] constexpr T &&get() && { return std::move(this->val_); }

	[[nodiscard]] constexpr const T &&get() const && { return std::move(this->val_); }

	[[nodiscard]] constexpr unexpected<E> &geterr() & {
		return this->unex_;
	}

	[[nodiscard]] constexpr const unexpected<E> &geterr() const & { return this->unex_; }
	
	[[nodiscard]] constexpr unexpected<E> &&geterr() && {
		return std::move(this->unex_);
	}
	
	[[nodiscard]] constexpr const unexpected<E> &&geterr() const && {
		return std::move(this->unex_);
	}

	constexpr void destroy_val() { get().~T(); }
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <typename E>
struct expected_operations_base<void, E> : expected_storage_base<void, E> {
	using expected_storage_base<void, E>::expected_storage_base;

	template <typename... Args> 
	void construct() noexcept { this->has_val_ = true; }

	// This function doesn't use its argument, but needs it so that code in
	// levels above this can work independently of whether T is void
	template <class Rhs>
	void construct_with(Rhs &&) noexcept {
		this->has_val_ = true;
	}

	template <typename... Args>
	void construct_error(Args &&...args) noexcept {
		new (std::addressof(this->unex_)) unexpected<E>(std::forward<Args>(args)...);
		this->has_val_ = false;
	}

	template <class Rhs> 
	void assign(Rhs &&rhs) noexcept {
		if (!this->has_val_) {
			if (rhs.has_val_) {
				geterr().~unexpected<E>();
				construct();
			} else {
				geterr() = std::forward<Rhs>(rhs).geterr();
			}
		} else {
			if (!rhs.has_val_) {
				construct_error(std::forward<Rhs>(rhs).geterr());
			}
		}
	}

	[[nodiscard]] bool has_value() const { return this->has_val_; }

	[[nodiscard]] constexpr unexpected<E> &geterr() & {
		return this->unex_;
	}

	[[nodiscard]] constexpr const unexpected<E> &geterr() const & { return this->unex_; }
	
	[[nodiscard]] constexpr unexpected<E> &&geterr() && {
		return std::move(this->unex_);
	}

	[[nodiscard]] constexpr const unexpected<E> &&geterr() const && {
		return std::move(this->unex_);
	}

	constexpr void destroy_val() {
	// no-op
	}
};

// This class manages conditionally having a trivial copy constructor
// This specialization is for when T and E are trivially copy constructible
template <typename T, typename E, bool = is_void_or<T, std::is_trivially_copy_constructible<T>>::value && std::is_trivially_copy_constructible<E>::value>
struct expected_copy_base : expected_operations_base<T, E> {
	using expected_operations_base<T, E>::expected_operations_base;
};

// This specialization is for when T or E are not trivially copy constructible
template <typename T, typename E>
struct expected_copy_base<T, E, false> : expected_operations_base<T, E> {
	using expected_operations_base<T, E>::expected_operations_base;

	expected_copy_base() = default;

	expected_copy_base(const expected_copy_base &rhs) : 
		expected_operations_base<T, E>(no_init) 
	{
		if (rhs.has_value()) {
			this->construct_with(rhs);
		} else {
			this->construct_error(rhs.geterr());
		}
	}

	expected_copy_base(expected_copy_base &&rhs) = default;

	expected_copy_base &operator=(const expected_copy_base &rhs) = default;

	expected_copy_base &operator=(expected_copy_base &&rhs) = default;
};

// doesn't implement an analogue to std::is_trivially_move_constructible. We
// have to make do with a non-trivial move constructor even if T is trivially
// move constructible
template <
	typename T, typename E,
		  bool = is_void_or<T, std::is_trivially_move_constructible<T>>::value
			  &&std::is_trivially_move_constructible<E>::value>
struct expected_move_base : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;
};

template <typename T, typename E>
struct expected_move_base<T, E, false> : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;

  expected_move_base() = default;
  expected_move_base(const expected_move_base &rhs) = default;

  expected_move_base(expected_move_base &&rhs) noexcept(
	  std::is_nothrow_move_constructible<T>::value)
	  : expected_copy_base<T, E>(no_init) {
	if (rhs.has_value()) {
	  this->construct_with(std::move(rhs));
	} else {
	  this->construct_error(std::move(rhs.geterr()));
	}
  }
  expected_move_base &operator=(const expected_move_base &rhs) = default;
  expected_move_base &operator=(expected_move_base &&rhs) = default;
};

// This class manages conditionally having a trivial copy assignment operator
template <
	typename T, 
	typename E,
	bool = is_void_or<
			  T, std::conjunction<std::is_trivially_copy_assignable<T>,
							 std::is_trivially_copy_constructible<T>,
							 std::is_trivially_destructible<T>>>::value
			  &&std::is_trivially_copy_assignable<E>::value
				  &&std::is_trivially_copy_constructible<E>::value
					  &&std::is_trivially_destructible<E>::value>
struct expected_copy_assign_base : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;
};

template <typename T, typename E>
struct expected_copy_assign_base<T, E, false> : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;

  expected_copy_assign_base() = default;
  expected_copy_assign_base(const expected_copy_assign_base &rhs) = default;

  expected_copy_assign_base(expected_copy_assign_base &&rhs) = default;
  expected_copy_assign_base &operator=(const expected_copy_assign_base &rhs) {
	this->assign(rhs);
	return *this;
  }
  expected_copy_assign_base &
  operator=(expected_copy_assign_base &&rhs) = default;
};

// This class manages conditionally having a trivial move assignment operator
// Unfortunately there's no way to achieve this in GCC < 5 AFAIK, since it
// doesn't implement an analogue to std::is_trivially_move_assignable. We have
// to make do with a non-trivial move assignment operator even if T is trivially
// move assignable
#ifndef TL_EXPECTED_GCC49
template <typename T, typename E,
		  bool =
			  is_void_or<T, std::conjunction<std::is_trivially_destructible<T>,
										std::is_trivially_move_constructible<T>,
										std::is_trivially_move_assignable<T>>>::
				  value &&std::is_trivially_destructible<E>::value
					  &&std::is_trivially_move_constructible<E>::value
						  &&std::is_trivially_move_assignable<E>::value>
struct expected_move_assign_base : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;
};
#else
template <typename T, typename E, bool = false> struct expected_move_assign_base;
#endif

template <typename T, typename E>
struct expected_move_assign_base<T, E, false>
	: expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;

  expected_move_assign_base() = default;
  expected_move_assign_base(const expected_move_assign_base &rhs) = default;

  expected_move_assign_base(expected_move_assign_base &&rhs) = default;

  expected_move_assign_base &
  operator=(const expected_move_assign_base &rhs) = default;

  expected_move_assign_base &
  operator=(expected_move_assign_base &&rhs) noexcept(
	  std::is_nothrow_move_constructible<T>::value
		  &&std::is_nothrow_move_assignable<T>::value) {
	this->assign(std::move(rhs));
	return *this;
  }
};

// expected_delete_ctor_base will conditionally delete copy and move
// constructors depending on whether T is copy/move constructible
template <typename T, typename E,
		  bool EnableCopy = (is_copy_constructible_or_void<T>::value &&
							 std::is_copy_constructible<E>::value),
		  bool EnableMove = (is_move_constructible_or_void<T>::value &&
							 std::is_move_constructible<E>::value)>
struct expected_delete_ctor_base {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = default;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_ctor_base<T, E, true, false> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = delete;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_ctor_base<T, E, false, true> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = delete;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = default;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_ctor_base<T, E, false, false> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = delete;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = delete;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

// expected_delete_assign_base will conditionally delete copy and move
// constructors depending on whether T and E are copy/move constructible +
// assignable
template <typename T, typename E,
		  bool EnableCopy = (is_copy_constructible_or_void<T>::value &&
							 std::is_copy_constructible<E>::value &&
							 is_copy_assignable_or_void<T>::value &&
							 std::is_copy_assignable<E>::value),
		  bool EnableMove = (is_move_constructible_or_void<T>::value &&
							 std::is_move_constructible<E>::value &&
							 is_move_assignable_or_void<T>::value &&
							 std::is_move_assignable<E>::value)>
struct expected_delete_assign_base {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
	  default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = default;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_assign_base<T, E, true, false> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
	  default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = default;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = delete;
};

template <typename T, typename E>
struct expected_delete_assign_base<T, E, false, true> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
	  default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = delete;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_assign_base<T, E, false, false> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
	  default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = delete;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = delete;
};

// This is needed to be able to construct the expected_default_ctor_base which
// follows, while still conditionally deleting the default constructor.
struct default_constructor_tag {
  explicit constexpr default_constructor_tag() = default;
};

// expected_default_ctor_base will ensure that expected has a deleted default
// consturctor if T is not default constructible.
// This specialization is for when T is default constructible
template <typename T, typename E,
		  bool Enable =
			  std::is_default_constructible<T>::value || std::is_void<T>::value>
struct expected_default_ctor_base {
  constexpr expected_default_ctor_base() noexcept = default;
  constexpr expected_default_ctor_base(
	  expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
	  default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base const &) noexcept = default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};

// This specialization is for when T is not default constructible
template <typename T, typename E> struct expected_default_ctor_base<T, E, false> {
  constexpr expected_default_ctor_base() noexcept = delete;
  constexpr expected_default_ctor_base(
	  expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
	  default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base const &) noexcept = default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};
} // namespace details

template <typename E> class bad_expected_access : public std::exception {
public:
  explicit bad_expected_access(E e) : val_(std::move(e)) {}

  virtual const char *what() const noexcept override {
	return "Bad expected access";
  }

  const E &error() const & { return val_; }
  E &error() & { return val_; }
  const E &&error() const && { return std::move(val_); }
  E &&error() && { return std::move(val_); }

private:
  E val_;
};

/// An `expected<T, E>` object is an object that contains the storage for
/// another object and manages the lifetime of this contained object `T`.
/// Alternatively it could contain the storage for another unexpected object
/// `E`. The contained object may not be initialized after the expected object
/// has been initialized, and may not be destroyed before the expected object
/// has been destroyed. The initialization state of the contained object is
/// tracked by the expected object.
template <typename T, typename E>
class expected : private details::expected_move_assign_base<T, E>,
				 private details::expected_delete_ctor_base<T, E>,
				 private details::expected_delete_assign_base<T, E>,
				 private details::expected_default_ctor_base<T, E> 
{
public:
	using value_type = T;
	using error_type = E;
	using unexpected_type = unexpected<E>;
	
private:
	static_assert(!std::is_reference_v<T>, "T must not be a reference");
	static_assert(!std::is_same_v<T, std::remove_cv_t<in_place_t>>, "T must not be in_place_t");
	static_assert(!std::is_same_v<T, std::remove_cv_t<unexpect_t>>, "T must not be unexpect_t");
	static_assert(!std::is_same_v<T, std::remove_cv_t<unexpected<E>>>, "T must not be unexpected<E>");
	static_assert(!std::is_reference_v<E>, "E must not be a reference");

	using impl_base = details::expected_move_assign_base<T, E>;

	using ctor_base = details::expected_default_ctor_base<T, E>;

	[[nodiscard]] T* valptr() { return std::addressof(this->val_); }
	
	[[nodiscard]] const T* valptr() const { return std::addressof(this->val_); }

	[[nodiscard]] unexpected<E>* errptr() { return std::addressof(this->unex_); }

	[[nodiscard]] const unexpected<E>* errptr() const { return std::addressof(this->unex_); }

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	[[nodiscard]] constexpr U& val() { return this->val_; }

	[[nodiscard]] constexpr unexpected<E>& err() { return this->unex_; }

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	[[nodiscard]] constexpr const U& val() const { return this->val_; }

	[[nodiscard]] constexpr const unexpected<E>& err() const { return this->unex_; }

public:
	constexpr expected() = default;

	constexpr expected(const expected &rhs) = default;

	constexpr expected(expected &&rhs) = default;

	expected &operator=(const expected &rhs) = default;

	expected &operator=(expected &&rhs) = default;

	template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>, int> = 0>
	constexpr expected(in_place_t, Args &&...args) : 
		impl_base(in_place, std::forward<Args>(args)...),
		ctor_base(details::default_constructor_tag{}) 
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	constexpr expected(in_place_t, std::initializer_list<U> il, Args &&...args) : 
		impl_base(in_place, il, std::forward<Args>(args)...),
		ctor_base(details::default_constructor_tag{}) 
	{ }

	template <
		typename G = E,
		std::enable_if_t<
			std::is_constructible_v<E, const G&> &&
			!std::is_convertible_v<const G&, E>, int
		> = 0
	>
	explicit constexpr expected(const unexpected<G>& e) :
		impl_base(unexpect, e.value()),
		ctor_base(details::default_constructor_tag{})
	{ }

	template <
		typename G = E,
		std::enable_if_t<
			std::is_convertible_v<E, const G&> &&
			std::is_convertible_v<const G&, E>, int
		> = 0
	>
	constexpr expected(unexpected<G> const &e) :
		impl_base(unexpect, e.value()),
		ctor_base(details::default_constructor_tag{})
	{ }

	template <
		typename G = E,
		std::enable_if_t<
			std::is_convertible_v<E, G&&> &&
			!std::is_convertible_v<G&&, E>, int
		> = 0
	>
	explicit constexpr expected(unexpected<G> &&e) noexcept(
		std::is_nothrow_constructible_v<E, G &&>
	) :
		impl_base(unexpect, std::move(e.value())),
		ctor_base(details::default_constructor_tag{}) 
	{ }

	template <
		typename G = E,
		std::enable_if_t<
			std::is_constructible_v<E, G&&> &&
			std::is_convertible_v<G&&, E>, int
		> = 0
	>
	constexpr expected(unexpected<G> &&e) noexcept(
		std::is_nothrow_constructible_v<E, G&&>
	) : 
		impl_base(unexpect, std::move(e.value())),
		ctor_base(details::default_constructor_tag{}) 
	{ }

	template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, int> = 0>
	constexpr explicit expected(unexpect_t, Args&&... args) :
		impl_base(unexpect, std::forward<Args>(args)...),
		ctor_base(details::default_constructor_tag{}) 
	{ }

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args&& ...args) :
		impl_base(unexpect, il, std::forward<Args>(args)...),
		ctor_base(details::default_constructor_tag{})
	{ }

	template <
		typename U, 
		typename G,
		std::enable_if_t<
			!(std::is_convertible_v<const U&, T> && std::is_convertible_v<const G&, E>), int
		> = 0
	>
	explicit constexpr expected(const expected<U, G> &rhs) :
		ctor_base(details::default_constructor_tag{}) 
	{
		if (rhs.has_value()) {
			this->construct(*rhs);
		} else {
			this->construct_error(rhs.error());
		}
	}

	template <
		typename U, 
		typename G,
		std::enable_if_t<
			(std::is_convertible_v<const U&, T> && std::is_convertible_v<const G&, E>), int
		> = 0
	>
	constexpr expected(const expected<U, G> &rhs) :
		ctor_base(details::default_constructor_tag{}) 
	{
		if (rhs.has_value()) {
			this->construct(*rhs);
		} else {
			this->construct_error(rhs.error());
		}
	}

	template <
		typename U, 
		typename G,
		std::enable_if_t<
			!(std::is_convertible_v<U&&, T> && std::is_convertible_v<G&&, E>), int
		> = 0
	>
	explicit constexpr expected(expected<U, G> &&rhs) :
		ctor_base(details::default_constructor_tag{})
	{
		if (rhs.has_value()) {
			this->construct(std::move(*rhs));
		} else {
			this->construct_error(std::move(rhs.error()));
		}
	}

	template <
		typename U, 
		typename G,
		std::enable_if_t<
			(std::is_convertible_v<U&&, T> && std::is_convertible_v<G&&, E>), int
		> = 0
	>
	constexpr expected(expected<U, G> &&rhs) :
		ctor_base(details::default_constructor_tag{}) 
	{
		if (rhs.has_value()) {
			this->construct(std::move(*rhs));
		} else {
			this->construct_error(std::move(rhs.error()));
		}
	}

	template <
		typename U = T,
		std::enable_if_t<
			!std::is_convertible_v<U&&, T> &&
			details::expected_enable_forward_value<T, E, U>, int
		> = 0
	>
	explicit constexpr expected(U &&v) :
		expected(in_place, std::forward<U>(v)) 
	{ }

	template <
		typename U = T,
		std::enable_if_t<
			std::is_convertible_v<U&&, T> &&
			details::expected_enable_forward_value<T, E, U>, int
		> = 0
	>
	constexpr expected(U &&v) :
		expected(in_place, std::forward<U>(v)) 
	{ }

	template <
		typename U = T, 
		typename G = T,
		std::enable_if_t<
			std::is_nothrow_constructible_v<T, U&&> &&
			!std::is_void_v<G> &&
			(
				!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
				!(std::is_scalar_v<T> && std::is_same_v<T, std::decay_t<U>>) &&
				std::is_constructible_v<T, U> &&
				std::is_assignable_v<G&, U> &&
				std::is_nothrow_move_constructible_v<E>
			), int
		> = 0
	>
	expected &operator=(U &&v) {
		if (has_value()) {
			val() = std::forward<U>(v);
		} else {
			err().~unexpected<E>();
			::new (valptr()) T(std::forward<U>(v));
			this->has_val_ = true;
		}
		return *this;
	}

	template <
		typename U = T, 
		typename G = T,
		std::enable_if_t<
			!std::is_nothrow_constructible_v<T, U&&> &&
			!std::is_void_v<U> &&
			(
				!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
				!(std::is_scalar_v<T> && std::is_same_v<T, std::decay_t<U>>) &&
				std::is_constructible_v<T, U> &&
				std::is_assignable_v<G&, U> &&
				std::is_nothrow_move_constructible_v<E>
			), int
		> = 0
	>
	expected &operator=(U &&v) {
		if (has_value()) {
			val() = std::forward<U>(v);
		} else {
			auto tmp = std::move(err());
			err().~unexpected<E>();
			::new (valptr()) T(std::forward<U>(v));
			this->has_val_ = true;
		}
		return *this;
	}

	template <
		typename G = E,
		std::enable_if_t<std::is_nothrow_copy_constructible_v<G> && std::is_assignable_v<G&, G>, int> = 0
	>
	expected &operator=(const unexpected<G> &rhs) {
		if (!has_value()) {
			err() = rhs;
		} else {
			this->destroy_val();
			::new (errptr()) unexpected<E>(rhs);
			this->has_val_ = false;
		}
		return *this;
	}

	template <
		typename G = E,
		std::enable_if_t<
			std::is_nothrow_move_constructible_v<G> && std::is_move_assignable_v<G>, int
		> = 0
	>
	expected &operator=(unexpected<G> &&rhs) noexcept {
		if (!has_value()) {
			err() = std::move(rhs);
		} else {
			this->destroy_val();
			::new (errptr()) unexpected<E>(std::move(rhs));
			this->has_val_ = false;
		}
		return *this;
	}

	template <typename F>
	constexpr auto and_then(F &&f) & -> decltype(
		and_then_impl(std::declval<expected &>(), std::forward<F>(f))
	)
	{
		return and_then_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto and_then(F &&f) && -> decltype(and_then_impl(std::declval<expected &&>(), std::forward<F>(f))) {
		return and_then_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto and_then(F &&f) const & -> decltype(
		and_then_impl(std::declval<expected const &>(), std::forward<F>(f))
	)
	{
		return and_then_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto and_then(F &&f) const&& -> decltype(
		and_then_impl(std::declval<expected const &&>(), std::forward<F>(f))
	) 
	{
		return and_then_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map(F &&f) & -> decltype(
		expected_map_impl(std::declval<expected &>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map(F &&f) && -> decltype(
		expected_map_impl(std::declval<expected>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map(F &&f) const & -> decltype(
		expected_map_impl(std::declval<const expected &>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map(F &&f) const&& -> decltype(
		expected_map_impl(std::declval<const expected &&>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform(F &&f) & -> decltype(
		expected_map_impl(std::declval<expected &>(), std::declval<F &&>())
	) 
	{
		return expected_map_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform(F &&f) && -> decltype(
		expected_map_impl(std::declval<expected>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform(F &&f) const & -> decltype(
		expected_map_impl(std::declval<const expected &>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform(F &&f) const && -> decltype(
		expected_map_impl(std::declval<const expected &&>(), std::declval<F &&>())
	)
	{
		return expected_map_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map_error(F &&f) & -> decltype(
		map_error_impl(std::declval<expected &>(), std::declval<F &&>())
	)
	{
		return map_error_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map_error(F &&f) && ->  decltype(
		map_error_impl(std::declval<expected &&>(), std::declval<F &&>())
	)
	{
		return map_error_impl(std::move(*this), std::forward<F>(f));
	}
	
	template <typename F>
	constexpr auto map_error(F &&f) const &  -> decltype(
		map_error_impl(std::declval<const expected &>(), std::declval<F &&>())
	)
	{
		return map_error_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto map_error(F &&f) const && -> decltype(
		map_error_impl(std::declval<const expected &&>(), std::declval<F &&>())
	)
	{
		return map_error_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform_error(F &&f) & -> decltype(
		map_error_impl(std::declval<expected &>(), std::declval<F &&>())
	)
	{
		return map_error_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform_error(F &&f) && -> decltype(
		map_error_impl(std::declval<expected &&>(), std::declval<F &&>())
	)
	{
		return map_error_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform_error(F &&f) const & -> decltype(
		map_error_impl(std::declval<const expected &>(), std::declval<F &&>())
	)
	{
		return map_error_impl(*this, std::forward<F>(f));
	}

	template <typename F>
	constexpr auto transform_error(F &&f) const && -> decltype(
		map_error_impl(std::declval<const expected &&>(), std::declval<F &&>())
	)
	{
		return map_error_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F> 
	expected constexpr or_else(F &&f) & {
		return or_else_impl(*this, std::forward<F>(f));
	}

	template <typename F> 
	expected constexpr or_else(F &&f) && {
		return or_else_impl(std::move(*this), std::forward<F>(f));
	}

	template <typename F> 
	expected constexpr or_else(F &&f) const & {
		return or_else_impl(*this, std::forward<F>(f));
	}

	template <typename F> 
	expected constexpr or_else(F &&f) const && {
		return or_else_impl(std::move(*this), std::forward<F>(f));
	}

	template <
		typename... Args,
		std::enable_if_t<
			std::is_nothrow_constructible_v<T, Args&&...>, int
		> = 0
	>
	void emplace(Args &&...args) {
		if (has_value()) {
			val().~T();
		} else {
			err().~unexpected<E>();
			this->has_val_ = true;
		}
		::new (valptr()) T(std::forward<Args>(args)...);
	}

	template <typename... Args, std::enable_if_t<!std::is_nothrow_constructible_v<T, Args&&...>, int> = 0>
	void emplace(Args &&...args) {
		if (has_value()) {
			val().~T();
			::new (valptr()) T(std::forward<Args>(args)...);
		} else {
			auto tmp = std::move(err());
			err().~unexpected<E>();
			::new (valptr()) T(std::forward<Args>(args)...);
			this->has_val_ = true;
		}
	}

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	void emplace(std::initializer_list<U> il, Args &&...args) {
		if (has_value()) {
			T t(il, std::forward<Args>(args)...);
			val() = std::move(t);
		} else {
			err().~unexpected<E>();
			::new (valptr()) T(il, std::forward<Args>(args)...);
			this->has_val_ = true;
		}
	}

	template <
		typename U, 
		typename... Args,
		std::enable_if_t<
			!std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>, int
		> = 0
	>
	void emplace(std::initializer_list<U> il, Args&&... args) {
		if (has_value()) {
			T t(il, std::forward<Args>(args)...);
			val() = std::move(t);
		} else {
			auto tmp = std::move(err());
			err().~unexpected<E>();
			::new (valptr()) T(il, std::forward<Args>(args)...);
			this->has_val_ = true;
		}
	}

private:
	using t_is_void = std::true_type;
	using t_is_not_void = std::false_type;
	using t_is_nothrow_move_constructible = std::true_type;
	using move_constructing_t_can_throw = std::false_type;
	using e_is_nothrow_move_constructible = std::true_type;
	using move_constructing_e_can_throw = std::false_type;

	void swap_where_both_have_value(expected&, t_is_void) noexcept {
		// swapping void is a no-op
	}

	void swap_where_both_have_value(expected &rhs, t_is_not_void) {
		using std::swap;
		swap(val(), rhs.val());
	}

	void swap_where_only_one_has_value(expected& rhs, t_is_void) noexcept(
		std::is_nothrow_move_constructible_v<E>
	) 
	{
		::new (errptr()) unexpected_type(std::move(rhs.err()));
		rhs.err().~unexpected_type();
		std::swap(this->has_val_, rhs.has_val_);
	}

	void swap_where_only_one_has_value(expected &rhs, t_is_not_void) {
		swap_where_only_one_has_value_and_t_is_not_void(
			rhs, typename std::is_nothrow_move_constructible<T>::type{},
			typename std::is_nothrow_move_constructible<E>::type{}
		);
	}

	void swap_where_only_one_has_value_and_t_is_not_void(
		expected &rhs, t_is_nothrow_move_constructible,
		e_is_nothrow_move_constructible) noexcept 
	{
		auto temp = std::move(val());
		val().~T();
		::new (errptr()) unexpected_type(std::move(rhs.err()));
		rhs.err().~unexpected_type();
		::new (rhs.valptr()) T(std::move(temp));
		std::swap(this->has_val_, rhs.has_val_);
	}

	void swap_where_only_one_has_value_and_t_is_not_void(
		expected &rhs, t_is_nothrow_move_constructible,
		move_constructing_e_can_throw) 
	{
		auto temp = std::move(val());
		val().~T();
		::new (errptr()) unexpected_type(std::move(rhs.err()));
		rhs.err().~unexpected_type();
		::new (rhs.valptr()) T(std::move(temp));
		std::swap(this->has_val_, rhs.has_val_);
	}

	void swap_where_only_one_has_value_and_t_is_not_void(
		expected &rhs, move_constructing_t_can_throw,
		e_is_nothrow_move_constructible)
	{
		auto temp = std::move(rhs.err());
		rhs.err().~unexpected_type();
		::new (rhs.valptr()) T(std::move(val()));
		val().~T();
		::new (errptr()) unexpected_type(std::move(temp));
		std::swap(this->has_val_, rhs.has_val_);
	}

public:
	template <
		class OT = T, 
		class OE = E, 
		std::enable_if_t<
			std::is_swappable_v<OT> &&
			std::is_swappable_v<OE> &&
			(
				std::is_nothrow_move_constructible_v<OT> ||
				std::is_nothrow_move_constructible_v<OE>
			), int
		> = 0
	>
	auto swap(expected &rhs) noexcept(
		std::is_nothrow_move_constructible_v<T> &&
		std::is_nothrow_swappable_v<T> &&
		std::is_nothrow_move_constructible_v<E> &&
		std::is_nothrow_swappable_v<E>
	) 
	{
		if (has_value() && rhs.has_value()) {
			swap_where_both_have_value(rhs, typename std::is_void<T>::type{});
		} else if (!has_value() && rhs.has_value()) {
			rhs.swap(*this);
		} else if (has_value()) {
			swap_where_only_one_has_value(rhs, typename std::is_void<T>::type{});
		} else {
			using std::swap;
			swap(err(), rhs.err());
		}
	}

	constexpr const T *operator->() const {
		assert(has_value());
		return valptr();
	}

	constexpr T *operator->() {
		assert(has_value());
		return valptr();
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr const U &operator*() const & {
		assert(has_value());
		return val();
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr U &operator*() & {
		assert(has_value());
		return val();
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr const U &&operator*() const && {
		assert(has_value());
		return std::move(val());
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr U &&operator*() && {
		assert(has_value());
		return std::move(val());
	}

	constexpr bool has_value() const noexcept { return this->has_val_; }

	constexpr explicit operator bool() const noexcept { return this->has_val_; }

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr const U &value() const & {
		if (!has_value())
			details::throw_exception(bad_expected_access<E>(err().value()));
		return val();
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr U &value() & {
		if (!has_value())
			details::throw_exception(bad_expected_access<E>(err().value()));
		return val();
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr const U &&value() const && {
		if (!has_value())
			details::throw_exception(bad_expected_access<E>(std::move(err()).value()));
		return std::move(val());
	}

	template <typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
	constexpr U &&value() && {
		if (!has_value())
			details::throw_exception(bad_expected_access<E>(std::move(err()).value()));
		return std::move(val());
	}

	constexpr const E &error() const & {
		assert(!has_value());
		return err().value();
	}

	constexpr E &error() & {
		assert(!has_value());
		return err().value();
	}
	
	constexpr const E &&error() const && {
		assert(!has_value());
		return std::move(err().value());
	}

	constexpr E &&error() && {
		assert(!has_value());
		return std::move(err().value());
	}

	template <typename U> 
	constexpr T value_or(U &&v) const & {
		static_assert(std::is_copy_constructible<T>::value &&
							std::is_convertible<U &&, T>::value,
						"T must be copy-constructible and convertible to from U&&");
		return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
	}

	template <typename U> 
	constexpr T value_or(U &&v) && {
		static_assert(std::is_move_constructible<T>::value &&
							std::is_convertible<U &&, T>::value,
						"T must be move-constructible and convertible to from U&&");
		return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
	}
};

namespace details {

template <typename Exp> 
using exp_t = typename std::decay_t<Exp>::value_type;
template <typename Exp> 
using err_t = typename std::decay_t<Exp>::error_type;
template <typename Exp, typename Ret> 
using ret_t = expected<Ret, err_t<Exp>>;

template <typename> struct TC;

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), *std::declval<Exp>())),
	std::enable_if_t<!std::is_void_v<exp_t<Exp>>, int> = 0
>
auto and_then_impl(Exp &&exp, F &&f) -> Ret {
	static_assert(details::is_expected<Ret>, "F must return an expected");

	return exp.has_value()
			 ? std::invoke(std::forward<F>(f), *std::forward<Exp>(exp))
			 : Ret(unexpect, std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>())),
	std::enable_if_t<std::is_void_v<exp_t<Exp>>, int> = 0
>
constexpr auto and_then_impl(Exp &&exp, F &&f) -> Ret {
	static_assert(details::is_expected<Ret>, "F must return an expected");

	return exp.has_value() ? std::invoke(std::forward<F>(f))
						 : Ret(unexpect, std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), *std::declval<Exp>())),
	std::enable_if_t<
		!std::is_void_v<exp_t<Exp>> && !std::is_void_v<Ret>, int
	> = 0
>
constexpr auto expected_map_impl(Exp &&exp, F &&f) -> ret_t<Exp, std::decay_t<Ret>> {
	using result = ret_t<Exp, std::decay_t<Ret>>;

	return exp.has_value() ? result(std::invoke(std::forward<F>(f),
												 *std::forward<Exp>(exp)))
						 : result(unexpect, std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), *std::declval<Exp>())),
	std::enable_if_t<
		!std::is_void_v<exp_t<Exp>> && std::is_void_v<Ret>, int
	> = 0
>
auto expected_map_impl(Exp &&exp, F &&f) -> expected<void, err_t<Exp>> {
	if (exp.has_value()) {
		std::invoke(std::forward<F>(f), *std::forward<Exp>(exp));
		return {};
	}

	return unexpected<err_t<Exp>>(std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>())),
	std::enable_if_t<
		std::is_void_v<exp_t<Exp>> && !std::is_void_v<Ret>, int
	> = 0
>
constexpr auto expected_map_impl(Exp &&exp, F &&f) -> ret_t<Exp, std::decay_t<Ret>> {
	using result = ret_t<Exp, std::decay_t<Ret>>;

	return exp.has_value() ? result(std::invoke(std::forward<F>(f)))
						 : result(unexpect, std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>())),
	std::enable_if_t<
		std::is_void_v<exp_t<Exp>> && std::is_void_v<Ret>, int
	> = 0
>
auto expected_map_impl(Exp &&exp, F &&f) -> expected<void, err_t<Exp>> {
	if (exp.has_value()) {
		std::invoke(std::forward<F>(f));
		return {};
	}

	return unexpected<err_t<Exp>>(std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
	std::enable_if_t<
		!std::is_void_v<exp_t<Exp>> && !std::is_void_v<Ret>, int
	> = 0
>
constexpr auto map_error_impl(Exp &&exp, F &&f) -> expected<exp_t<Exp>, std::decay_t<Ret>> {
	using result = expected<exp_t<Exp>, std::decay_t<Ret>>;

	return exp.has_value()
			 ? result(*std::forward<Exp>(exp))
			 : result(unexpect, std::invoke(std::forward<F>(f),
											   std::forward<Exp>(exp).error()));
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
	std::enable_if_t<
		!std::is_void_v<exp_t<Exp>> && std::is_void_v<Ret>, int
	> = 0
>
auto map_error_impl(Exp &&exp, F &&f) -> expected<exp_t<Exp>, std::monostate> {
	using result = expected<exp_t<Exp>, std::monostate>;
	if (exp.has_value()) {
		return result(*std::forward<Exp>(exp));
  	}

  	std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
	return result(unexpect, std::monostate{});
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
	std::enable_if_t<
		std::is_void_v<exp_t<Exp>> && !std::is_void_v<Ret>, int
	> = 0
>
constexpr auto map_error_impl(Exp &&exp, F &&f) -> expected<exp_t<Exp>, std::decay_t<Ret>> {
	using result = expected<exp_t<Exp>, std::decay_t<Ret>>;

	return exp.has_value()
			 ? result()
			 : result(unexpect, std::invoke(std::forward<F>(f),
											   std::forward<Exp>(exp).error()));
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
	std::enable_if_t<
		std::is_void_v<exp_t<Exp>> && std::is_void_v<Ret>, int
	> = 0
>
auto map_error_impl(Exp &&exp, F &&f) -> expected<exp_t<Exp>, std::monostate> {
	using result = expected<exp_t<Exp>, std::monostate>;
	if (exp.has_value()) {
		return result();
	}

	std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
	return result(unexpect, std::monostate{});
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
	std::enable_if_t<
		!std::is_void_v<Ret>, int
	> = 0
>
auto or_else_impl(Exp &&exp, F &&f) -> Ret {
	static_assert(details::is_expected<Ret>, "F must return an expected");
	return exp.has_value() ? std::forward<Exp>(exp)
						 : std::invoke(std::forward<F>(f),
										  std::forward<Exp>(exp).error());
}

template <
	typename Exp, 
	typename F,
	typename Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
	std::enable_if_t<std::is_void_v<Ret>, int> = 0
>
std::decay_t<Exp> or_else_impl(Exp &&exp, F &&f) {
	return exp.has_value() ? std::forward<Exp>(exp) : (
		std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error()), std::forward<Exp>(exp)
	);
}

} // namespace details

template <typename T, typename E, typename U, typename F>
constexpr bool operator==(const expected<T, E> &lhs, const expected<U, F> &rhs) {
	return (lhs.has_value() != rhs.has_value())
			 ? false
			 : (!lhs.has_value() ? lhs.error() == rhs.error() : *lhs == *rhs);
}
template <typename T, typename E, typename U, typename F>
constexpr bool operator!=(const expected<T, E> &lhs, const expected<U, F> &rhs) {
	return (lhs.has_value() != rhs.has_value())
			 ? true
			 : (!lhs.has_value() ? lhs.error() != rhs.error() : *lhs != *rhs);
}
template <typename E, typename F>
constexpr bool operator==(const expected<void, E> &lhs, const expected<void, F> &rhs) {
	return (lhs.has_value() != rhs.has_value())
			 ? false
			 : (!lhs.has_value() ? lhs.error() == rhs.error() : true);
}
template <typename E, typename F>
constexpr bool operator!=(const expected<void, E> &lhs, const expected<void, F> &rhs) {
	return (lhs.has_value() != rhs.has_value())
			 ? true
			 : (!lhs.has_value() ? lhs.error() == rhs.error() : false);
}

template <typename T, typename E, typename U>
constexpr bool operator==(const expected<T, E> &x, const U &v) {
	return x.has_value() ? *x == v : false;
}

template <typename T, typename E, typename U>
constexpr bool operator==(const U &v, const expected<T, E> &x) {
	return x.has_value() ? *x == v : false;
}

template <typename T, typename E, typename U>
constexpr bool operator!=(const expected<T, E> &x, const U &v) {
	return x.has_value() ? *x != v : true;
}

template <typename T, typename E, typename U>
constexpr bool operator!=(const U &v, const expected<T, E> &x) {
	return x.has_value() ? *x != v : true;
}

template <typename T, typename E>
constexpr bool operator==(const expected<T, E> &x, const unexpected<E> &e) {
	return x.has_value() ? false : x.error() == e.value();
}

template <typename T, typename E>
constexpr bool operator==(const unexpected<E> &e, const expected<T, E> &x) {
	return x.has_value() ? false : x.error() == e.value();
}

template <typename T, typename E>
constexpr bool operator!=(const expected<T, E> &x, const unexpected<E> &e) {
	return x.has_value() ? true : x.error() != e.value();
}

template <typename T, typename E>
constexpr bool operator!=(const unexpected<E> &e, const expected<T, E> &x) {
	return x.has_value() ? true : x.error() != e.value();
}

template <
	typename T, 
	typename E,
	std::enable_if_t<
		(
			std::is_void_v<T> ||
			std::is_move_constructible_v<T>
		) &&
		std::is_swappable_v<T> &&
		std::is_move_constructible_v<E> &&
		std::is_swappable_v<E>, int
	> = 0
>
void swap(expected<T, E> &lhs, expected<T, E> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}

} // namespace genesis

#endif