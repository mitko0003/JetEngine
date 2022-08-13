#pragma once

#include <Core/Misc/Utility.h>
#include <Core/Misc/Types.h>

template <typename T>
constexpr TRemoveReference_t<T> &&Move(T &&value) noexcept
{
	return static_cast<TRemoveReference_t<T> &&>(value);
}

template <typename T>
constexpr T &&Forward(T &value)
{
	return static_cast<T &&>(value);
}

template <typename T>
T *AddressOf(T &reference)
{
	return &reference;
}

template <class T>
constexpr void DestroyAt(T *object)
{
	if constexpr (IsArray<T>)
		for (auto &elem : *object)
			DestroyAt(AddressOf(elem));
	else
		object->~T();
}

inline void MemorySet(void *first, int32 value, size_t size)
{
	auto *curr = reinterpret_cast<uint8 *>(first);
	for (const auto *end = curr + size; curr != end; ++curr)
		*curr = value;
}

inline int MemoryCompare(const void *lhs, const void *rhs, int32 size)
{
	return 0;
}

template<typename T, typename... TArgs>
inline void ConstructAt(T *object, TArgs &&... args)
{
	::new (static_cast<void *>(object)) T(Forward<TArgs>(args)...);
}

struct
{
	uint8 *Strings = nullptr;
	uint8 *FS = nullptr;
} Pools;

template<uint8 *&pool, uint32 poolSize>
class StackAllocator
{
public:
	StackAllocator() : offset(0) {}
	uint8 *Alloc(int32 size)
	{
		int32 curr = offset;
		offset += size;
		return pool + curr;
	}
	void Free(uint8*) {}
private:
	int32 offset;
};

template<uint8 *&pool, uint32 poolSize>
class TlsfAllocator
{

};

template<uint8 *&pool, uint32 poolSize>
class FixedAllocator
{
public:
	FixedAllocator() { MemorySet(&bits, 0, sizeof(bits)); }
	uint8 *Alloc(int32 size)
	{
		return nullptr;
	}
	void Free(uint8*) {}
private:
	int32 bits[DivCeil(poolSize, CHAR_BIT)];
};

//class PageAllocator
//{
//	struct PagePhysical
//	{
//		HANDLE fileMapping;
//	};
//
//	struct PageVirtual
//	{
//
//	};
//
//	enum
//	{
//		Read,
//		Write
//	};
//
//	PagePhysical AllocPhysical(uint64 size)
//	{
//		HANDLE fileMapping = CreateFileMapping(
//			INVALID_HANDLE_VALUE,
//			nullptr,
//			PAGE_READWRITE,
//			size >> 32ull,
//			size & 0xFFFFFFFFull,
//			nullptr
//		);
//		return PagePhysical{ fileMapping };
//	}
//
//	PageVirtual AllocVirtual(uint64 size)
//	{
//		void *virtualAddress = MapViewOfFile;
//	}
//
//	void FreePhysical();
//	void FreeVirtual();
//
//	void Map();
//	void Unmap();
//};