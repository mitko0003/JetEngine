#pragma once

#include "malloc.h"

#define ASSERT(condition) do { if(!(condition)) { DebugPrint("%s(%d): %s", __FILE__, __LINE__, #condition); DebugBreak(); exit(1); } } while (false)
#define ALLOCA(type, size) static_cast<type*>(_alloca(sizeof(type) * size))

using LibraryType = HMODULE;

template <typename T>
constexpr int32 ArrayLength(const T &arr) 
{
	static_assert(std::is_array<T>::value, "Non-array type!");
	return sizeof(arr) / sizeof(arr[0]);
}

//template <class T>
//void ZeroMemory(T &obj)
//{
//	memset(&obj, 0, sizeof(T));
//}

template <typename T, int32 _size>
struct Array
{
	static constexpr int32 size = _size;
	T elements[size];
};

template <typename T>
inline constexpr T Min(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
inline constexpr T Max(T a, T b)
{
	return a < b ? b : a;
}

template <typename T>
inline constexpr T Clamp(T x, T a, T b)
{
	return Min(Max(x, a), b);
}

template <typename T>
inline constexpr T Saturate(T x)
{
	return Clamp(x, T(0), T(1));
}

template <typename T>
inline constexpr typename std::enable_if<std::is_integral<T>::value, T>::type DivCeil(T x, T y)
{
	return (x + y - 1) / y;
}

template <typename T> inline T CountLeadingZeros(T x);
template <> inline uint32 CountLeadingZeros(uint32 x) { return _lzcnt_u32(x); }
template <> inline uint64 CountLeadingZeros(uint64 x) { return _lzcnt_u64(x); }

template <typename T> inline T CountTrailingZeros(T x);
template <> inline uint32 CountTrailingZeros(uint32 x) { return _tzcnt_u32(x); }
template <> inline uint64 CountTrailingZeros(uint64 x) { return _tzcnt_u64(x); }

template <typename T> inline T CountSetBits(T x);
template <> inline uint32 CountSetBits(uint32 x) { return _mm_popcnt_u32(x); }
template <> inline uint64 CountSetBits(uint64 x) { return _mm_popcnt_u64(x); }

template <typename T> inline uint8 IsBitSet(T a, T b);
template <> inline uint8 IsBitSet(uint32 a, uint32 b) { return _bittest(reinterpret_cast<const LONG*>(&a), reinterpret_cast<LONG&>(b));   }
template <> inline uint8 IsBitSet(uint64 a, uint64 b) { return _bittest64(reinterpret_cast<const LONG64*>(&a), reinterpret_cast<LONG64&>(b)); }

enum LogLevel
{
	logVerbose,
	logDebug,
	logError
};

template <LogLevel level = logDebug>
inline void DebugPrint(const char *format, ...)
{
	va_list argList;
	char buffer[2048];
	
	va_start(argList, format);
		vsnprintf(buffer, ArrayLength(buffer), format, argList);
	va_end(argList);

	OutputDebugString(buffer);
}

template <uint32 size>
class Bitset
{
	bool operator[](uint32 idx) const
	{
		return Data[idx / CHAR_BIT] & (1 << (idx % CHAR_BIT));		 
	}
private:
	uint8 Data[DivCeil(size, CHAR_BIT)];
};

//class CC_ID
//{
//	CC_ID(const char *cStr)
//	{
//
//	}
//
//	bool operator==(const CC_ID &rhs)
//	{
//		return false;
//	}
//private:
//	int64 data;
//};

//template <typename allocator, uint8 *&pool>
//class String
//{
//	String(const char *cStr)
//	{
//		memcpy(data, cStr, 2);
//	}
//
//	bool operator==(const String &rhs) const
//	{
//		return size != rhs.size && !memcmp(data, rhs.data, size);
//	}
//private:
//	union
//	{
//		char embeded[8];
//		struct
//		{
//			char *data;
//			int32 size;
//		};
//	};
//};
//
//void TestConcat()
//{
//	String a = "abc";
//	String b = "bcd";
//	String c = a + b;
//	ASSERT(c == "abcbcd");
//}