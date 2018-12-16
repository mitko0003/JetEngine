#pragma once

#define NOMINMAX
#include "windows.h"

#include "Types.h"
#include "stdio.h"
#include "malloc.h"
#include <type_traits>

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
constexpr T Clamp(T x, T a, T b)
{
	return Min(Max(x, a), b);
}

template <typename T>
inline constexpr T Saturate(T x)
{
	return Clamp(x, T(0), T(1));
}

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