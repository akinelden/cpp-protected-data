#ifndef PROTECTED_DATA
#define PROTECTED_DATA

#include <optional>
#include <memory>
#include <concepts>

template <typename M>
concept Lockable = requires(M mutex) {
	{ mutex.lock() } -> std::same_as<void>;
	{ mutex.unlock() } -> std::same_as<void>;
};

template <typename M>
concept SharedLockable = requires(M mutex) {
	requires Lockable<M>;
	{ mutex.lock_shared() } -> std::same_as<void>;
	{ mutex.unlock_shared() } -> std::same_as<void>;
};

// forward declaration of protected data class
template<typename T, typename M>
requires Lockable<M>
class protected_data;

// unique_guard<T, M>
// 
// T : contained object type
// M : mutex type
// 
// Provides RAII style container for thread-safe object manipulation.
// Holds the lock and object reference during its lifetime.
// All functions of the object can be called via ->
template<typename T, typename M>
	requires Lockable<M>
class unique_guard
{
	std::unique_lock<M> lock_;
	T& object_;

	friend class protected_data<T, M>;

	unique_guard();

	unique_guard(const unique_guard& other) = delete;
	unique_guard& operator=(unique_guard other) = delete;

public:
	unique_guard(protected_data<T, M>& pd);
	unique_guard(M& mutex, T& object) : lock_(mutex), object_(object) {};

	T& operator*()
	{
		return object_;
	}

	T* const operator->()
	{
		return &object_;
	}
};

// shared_guard<T, M>
// 
// T : contained object type
// M : mutex type
// 
// Provides RAII style container for thread-safe object manipulation.
// Holds the lock and const object reference during its lifetime.
// Only the const functions of the object can be called via ->
template<typename T, typename M>
	requires SharedLockable<M>
class shared_guard
{
	std::shared_lock<M> lock_;
	T const& object_;

	friend class protected_data<T, M>;

	shared_guard();

	shared_guard(const shared_guard& other) = delete;
	shared_guard& operator=(shared_guard other) = delete;

public:
	shared_guard(const protected_data<T, M>& pd);
	shared_guard(M& mutex_, T const& object) : lock_(mutex_), object_(object) {};

	const T& operator*()
	{
		return object_;
	}

	const T* const operator->()
	{
		return &object_;
	}
};

// protected_data<T, M>
// 
// T : contained object type
// M : mutex type
// 
// Provides RAII style container for thread-safe object handling
// via returning shared_guard class instances upon
// get_unique() function call
template<typename T, typename M>
requires Lockable<M>
class protected_data
{
	M mutex_;
	T object_;

	friend class unique_guard<T, M>;

	protected_data(const protected_data& other) = delete;
	protected_data& operator=(protected_data other) = delete;

public:
	template<typename... Args>
	protected_data(Args&&... args) : object_(std::forward<Args>(args)...) {};

	protected_data(T&& object) : object_(object) {};

	~protected_data() {}

	// get_unique()
	// 
	// Acquires the unique_lock of the mutex and returns a unique_guard
	unique_guard<T, M> get_unique()
	{
		return unique_guard<T, M>(mutex_, object_);
	}
	
	// get_shared()
	// 
	// Acquires the shared_lock of the mutex and returns a shared_guard
	// Mutex must be SharedLockable
	//shared_guard<T, M> get_shared() const;

	// can_cast_to<U>()
	// 
	// Returns whether the data can be dynamically cast to type U
	template<typename U>
	bool can_cast_to() const
	{
		const U* ptr = dynamic_cast<const U*>(&object_);
		return ptr != nullptr;
	}

	// get_unique_cast()
	// 
	// Tries to dynamically cast the data to type U and returns a 
	// unique_guard of type U without any lock
	template<typename U>
	std::optional<unique_guard<U, M>> get_unique_cast()
	{
		if (U* ptr = dynamic_cast<U*>(&object_))
			return std::optional<unique_guard<U, M>>(std::in_place, mutex_, *ptr);
		else
			return {};
	}
};

// protected_data<T, M>
// 
// T : contained object type
// M : shared mutex type
// 
// Provides RAII style container for thread-safe object handling
// via returning unique_guard and shared_guard class instances upon
// get_unique() and get_shared() function calls
template<typename T, typename M>
requires SharedLockable<M>
class protected_data<T, M>
{
	mutable M mutex_;
	T object_;

	friend class unique_guard<T, M>;
	friend class shared_guard<T, M>;

	protected_data(const protected_data& other) = delete;
	protected_data& operator=(protected_data other) = delete;

public:
	template<typename... Args>
	protected_data(Args&&... args) : object_(std::forward<Args>(args)...) {};

	protected_data(T&& object) : object_(object) {};

	~protected_data() {}

	// get_unique()
	// 
	// Acquires the unique_lock of the mutex and returns a unique_guard
	unique_guard<T, M> get_unique()
	{
		return unique_guard<T, M>(mutex_, object_);
	}

	// get_shared()
	// 
	// Acquires the shared_lock of the mutex and returns a shared_guard
	// Mutex must be SharedLockable
	shared_guard<T, M> get_shared() const
	{
		return shared_guard(*this);
	}

	// can_cast_to<U>()
	// 
	// Returns whether the data can be dynamically cast to type U
	template<typename U>
	bool can_cast_to() const
	{
		const U* ptr = dynamic_cast<const U*>(&object_);
		return ptr != nullptr;
	}

	// get_unique_cast()
	// 
	// Tries to dynamically cast the data to type U and returns a 
	// unique_guard of type U without any lock
	template<typename U>
	std::optional<unique_guard<U, M>> get_unique_cast()
	{
		if (U* ptr = dynamic_cast<U*>(&object_))
			return std::optional<unique_guard<U, M>>(std::in_place, mutex_, *ptr);
		else
			return {};
	}

	// get_shared_cast()
	// 
	// Tries to dynamically cast the data to type U and returns a 
	// shared_guard of type U
	template<typename U>
	std::optional<shared_guard<U, M>> get_shared_cast() const
	{
		if (const U* ptr = dynamic_cast<const U*>(&object_))
			return std::optional<shared_guard<U, M>>(std::in_place, mutex_, *ptr);
		else
			return {};
	}
};

// ----- protected_data pointer cast functions


// get_shared_cast()
// 
// Tries to dynamically cast the data to type U and returns a 
// shared_guard of type U
//template<typename U>
//std::optional<shared_guard<U, M>> get_shared_cast() const;

// cast_shared_ptr_protected_data()
// 
// Creates an instance of shared_ptr<protected_data<U, M>> from shared_ptr<protected_data<T, M>>
// if the cast is possible
template<typename U, typename T, typename M>
std::optional<std::shared_ptr<protected_data<U, M>>> cast_shared_ptr_protected_data(const std::shared_ptr<protected_data<T, M>>& p)
{
	if (p->can_cast_to<U>())
	{
		return std::reinterpret_pointer_cast<protected_data<U, M>>(p);
	}
	return {};
}

// ------- Method definitions for unique_guard and shared_guard constructors
template <typename T, typename M>
requires Lockable<M>
unique_guard<T,M>::unique_guard(protected_data<T, M>& pd) : lock_(pd.mutex_), object_(pd.object_) {};

template <typename T, typename M>
requires SharedLockable<M>
shared_guard<T, M>::shared_guard(const protected_data<T, M>& pd) : lock_(pd.mutex_), object_(pd.object_) {};
#endif