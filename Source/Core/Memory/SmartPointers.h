#pragma once

template <typename T>
struct TDefaultDeleter
{
	void operator()(T *ptr) const
	{
		delete ptr;
	}
};

template <typename T>
struct TDefaultDeleter<T[]>
{
	void operator()(T *ptr) const
	{
		delete[] ptr;
	}
};

template <typename T, typename TDeleter = TDefaultDeleter<T>>
class TUniquePtrBase
{
public:
	constexpr TUniquePtrBase() :
		Pointer(nullptr) {}
	constexpr TUniquePtrBase(T *pointer) :
		Pointer(pointer) {}

	constexpr TUniquePtrBase(TUniquePtrBase &&rhs) :
		Pointer(rhs.Pointer)
	{
		rhs.Pointer = nullptr;
	}

	~TUniquePtrBase()
	{
		TDeleter()(Pointer);
	}

	T* operator->() { return Pointer; }
	T& operator*() { return *Pointer; }

	constexpr operator T*() const {
		return Pointer;
	}
	constexpr T* Get() const {
		return Pointer;
	}
	T* const & GetRef() const {
		return Pointer;
	}

	bool operator==(TNullptr) const {
		return Pointer == nullptr;
	}
	bool operator!=(TNullptr) const {
		return Pointer != nullptr;
	}
	operator bool() const {
		return Pointer != nullptr;
	}

protected:
	T *Pointer;
};

template <typename T, typename TDeleter = TDefaultDeleter<T>>
class TUniquePtr : public TUniquePtrBase<T, TDeleter>
{
public:
	using TUniquePtrBase<T, TDeleter>::TUniquePtrBase;

	TUniquePtr(const TUniquePtr &rhs) = delete;
	TUniquePtr& operator=(const TUniquePtr &rhs) = delete;
};

template <typename T, typename TDeleter>
class TUniquePtr<T[], TDeleter> : public TUniquePtrBase<T, TDeleter>
{
public:
	using TUniquePtrBase<T, TDeleter>::TUniquePtrBase;

	TUniquePtr(const TUniquePtr &rhs) = delete;
	TUniquePtr& operator=(const TUniquePtr &rhs) = delete;
};

template <typename T, typename TDeleter>
constexpr bool operator==(const TUniquePtr<T, TDeleter> &lhs, const TUniquePtr<T, TDeleter> &rhs) {
	return lhs.Get() == rhs.Get();
}
template <typename T, typename TDeleter>
constexpr bool operator!=(const TUniquePtr<T, TDeleter> &lhs, const TUniquePtr<T, TDeleter> &rhs) {
	return lhs.Get() != rhs.Get();
}

class TRefCount
{
public:
	TRefCount() :
		StrongRefs(1) {}

	void AddStrongRef() {
		++StrongRefs;
	}
	void AddWeakRef() {
		++WeakRefs;
	}

private:
	uint32 StrongRefs;
	uint32 WeakRefs;
};

template <typename T, typename TDeleter>
class TSharedPtrBase
{
public:
	constexpr TSharedPtrBase() :
		Pointer(nullptr) {}
	constexpr TSharedPtrBase(T *pointer) :
		Pointer(pointer)
	{
		if (Pointer != nullptr)
		{
			RefCount = new TRefCount();
		}
	}

	constexpr T* operator->() const { return Pointer; }
	constexpr T& operator*() const { return *Pointer; }

	constexpr operator T*() const {
		return Pointer;
	}
	constexpr T* Get() const {
		return Pointer;
	}

	TSharedPtrBase(const TSharedPtrBase &rhs) :
		Pointer(rhs.Pointer), RefCount(rhs.RefCount)
	{
		if (Pointer != nullptr)
			RefCount->AddStrongRef();
	}

	TSharedPtrBase& operator=(const TSharedPtrBase &rhs)
	{
		Pointer = rhs.Pointer;
		RefCount = rhs.RefCount;
		RefCount->AddStrongRef();
		return *this;
	}

protected:
	T *Pointer;
	TRefCount *RefCount;
};

template <typename T, typename TDeleter = TDefaultDeleter<T>>
class TSharedPtr : public TSharedPtrBase<T, TDeleter>
{
public:
	using TSharedPtrBase<T, TDeleter>::TSharedPtrBase;
};

template <typename T, typename TDeleter>
class TSharedPtr<T[], TDeleter> : public TSharedPtrBase<T, TDeleter>
{
public:
	using TSharedPtrBase<T, TDeleter>::TSharedPtrBase;
};

template <typename T, typename underlying, uint8 *pool>
struct OffsetPtr
{
	constexpr OffsetPtr() : offset(nulloff) {}
	constexpr OffsetPtr(TNullptr) : offset(nulloff) {}
	constexpr OffsetPtr(const OffsetPtr &rhs) : offset(rhs.offset) {}
	OffsetPtr(const T *rhs) : offset(rhs == nullptr ? nulloff : static_cast<underlying>(rhs - pool)) {}

	constexpr bool operator==(TNullptr) const { return offset == nulloff; }
	constexpr bool operator==(const OffsetPtr &rhs) const { return offset == rhs.offset; }

	constexpr bool operator!=(TNullptr) const { return offset != nulloff; }
	constexpr bool operator!=(const OffsetPtr &rhs) const { return offset != rhs.offset; }

	template <typename PtrT>
	operator PtrT*() const { return reinterpret_cast<PtrT>(pool + offset); }
	constexpr operator bool() const { return offset != nulloff; }

	T& operator->() { return *this->operator T*(); }
private:
	static constexpr underlying nulloff = TNumericalLimits<underlying>::Max();
	underlying offset;
};