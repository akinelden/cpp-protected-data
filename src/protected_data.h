#ifndef PROTECTED_DATA
#define PROTECTED_DATA


#include <shared_mutex>
#include <optional>

template<typename T>
class unique_guard
{
	std::unique_lock<std::shared_mutex> lk;
	T& obj;

	unique_guard();

	unique_guard(const unique_guard& other) = delete;
	unique_guard& operator=(unique_guard other) = delete;

public:
	unique_guard(std::shared_mutex& _mut, T& _ptr) : obj(_ptr), lk(std::unique_lock<std::shared_mutex>(_mut)) {};

	T* const operator->()
	{
		return &obj;
	}
};

template<typename T>
class shared_guard
{
	std::shared_lock<std::shared_mutex> lk;
	T const& obj;

	shared_guard();

	shared_guard(const shared_guard& other) = delete;
	shared_guard& operator=(shared_guard other) = delete;

public:
	shared_guard(std::shared_mutex& _mut, T const& _ptr) : obj(_ptr), lk(std::shared_lock<std::shared_mutex>(_mut)) {};

	const T* const operator->()
	{
		return &obj;
	}
};

template<typename T>
class protected_data
{
	mutable std::shared_mutex mut;
	T* obj;

	protected_data(const protected_data& other) = delete;
	protected_data& operator=(protected_data other) = delete;

public:
	template<typename... Args>
	protected_data(Args&&... args)
	{
		obj = new T(std::forward<Args>(args)...);
	}

	protected_data(T* _data) : obj(_data) {};

	~protected_data()
	{
		std::unique_lock lk(mut);
		delete obj;
	}

	unique_guard<T> get_unique() const
	{
		return unique_guard<T>(mut, *obj);
	}

	shared_guard<T> get_shared() const
	{
		return shared_guard<T>(mut, *obj);
	}

	template<typename U>
	std::optional<unique_guard<U>> get_unique_cast()
	{
		if (U* ptr = dynamic_cast<U*>(obj))
			return std::optional < unique_guard<U>>(std::in_place, mut, *ptr);
		else
			return {};
	}

	template<typename U>
	std::optional<shared_guard<U>> get_shared_cast()
	{
		if (U* ptr = dynamic_cast<U*>(obj))
			return std::optional<shared_guard<U>>(std::in_place, mut, *ptr);
		else
			return {};
	}
};

#endif
