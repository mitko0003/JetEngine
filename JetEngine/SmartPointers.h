#pragma once

template <typename T, typename Deleter>
struct UniquePtr
{
	constexpr UniquePtr() : ptr(nullptr) {}
	constexpr UniquePtr(T *rhs) : ptr(rhs) {}
	
	UniquePtr(const UniquePtr &rhs) = delete;
	UniquePtr& operator=(const UniquePtr &rhs) = delete;

	~UniquePtr() { Deleter(ptr); }

	T* operator->() { return *ptr; }
	T& operator*() { return *ptr; }
private:
	T *ptr
};

template <typename T>
struct SharedPtr
{
	constexpr T* operator->() { return reinterpret_cast<T*>(ptr); }
	constexpr T& operator*() { return *reinterpret_cast<T*>(ptr); }
private:
	uint64 ref : 16;
	uint64 ptr : 48;
};

static_assert(sizeof(SharedPtr<void>) == sizeof(uint64), "Unexpected size!");

template <typename T, typename underlying, uint8 *pool>
struct OffsetPtr
{
	constexpr OffsetPtr() : offset(nulloff) {}
	constexpr OffsetPtr(std::nullptr_t) : offset(nulloff) {}
	constexpr OffsetPtr(const OffsetPtr &rhs) : offset(rhs.offset) {}
	OffsetPtr(const T *rhs) : offset(rhs == nullptr ? nulloff : static_cast<underlying>(rhs - pool)) {}

	constexpr bool operator==(std::nullptr_t) const { return offset == nulloff; }
	constexpr bool operator==(const OffsetPtr &rhs) const { return offset == rhs.offset; }

	constexpr bool operator!=(std::nullptr_t) const { return offset != nulloff; }
	constexpr bool operator!=(const OffsetPtr &rhs) const { return offset != rhs.offset; }

	template <typename PtrT>
	operator PtrT*() const { return reinterpret_cast<PtrT>(pool + offset); }
	constexpr operator bool() const { return offset != nulloff; }

	T& operator->() { return *(T*)(this*); }
private:
	static constexpr underlying nulloff = std::numerical_limits<underlying>::max();
	underlying offset;
};