#pragma once

void* operator new(size_t) {
	ASSERT(!"Not implemented!");
	return nullptr;
}

void* operator new[](size_t) {
	ASSERT(!"Not implemented!");
	return nullptr;
}

void operator delete(void*) {
	ASSERT(!"Not implemented!");
}

void operator delete[](void*) {
	ASSERT(!"Not implemented!");
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
	FixedAllocator() { memset(&bits, 0, sizeof(bits)); }
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