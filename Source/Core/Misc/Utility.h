#pragma once

#include "Core/Misc/TypeTraits.h"
#include "Core/Misc/Concepts.h"
#include "Core/Misc/Tuple.h"
#include "Core/Misc/Functional.h"

template <typename TIterator>
struct TIteratorValue;

template <typename T>
struct TIteratorValue<T*>
{
	using Type = T;
};

template <typename TIterator>
struct TIteratorTraits
{
	using TValue = TIteratorValue<TRemoveConst_t<TIterator>>::Type;
};

template <typename TForwardIterator, typename TBinaryOperation, typename TValue = typename TIteratorTraits<TForwardIterator>::TValue>
inline constexpr TValue Reduce(TForwardIterator begin, TForwardIterator end, TValue initialValue, TBinaryOperation binaryOperation)
{
	for (; begin != end; ++begin)
		initialValue = binaryOperation(initialValue, *begin);
	return initialValue;
}

inline constexpr bool Any(std::initializer_list<bool> values)
{
	return Reduce(values.begin(), values.end(), false, TOpOr());
}

inline constexpr bool All(std::initializer_list<bool> values)
{
	return Reduce(values.begin(), values.end(), true, TOpAnd());
}

template <typename TForwardIterator>
inline constexpr TForwardIterator MinElement(TForwardIterator begin, TForwardIterator end)
{
	if (begin == end)
		return end;

	auto result = begin++;
	for (; begin != end; ++begin)
	{
		if (*begin < *result)
			result = begin;
	}
	return result;
}

template <typename T>
inline constexpr T Min(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
inline constexpr T Min(std::initializer_list<T> values)
{
	return *MinElement(values.begin(), values.end());
}

template <typename THead, typename ... TTail>
inline constexpr THead Min(const THead &head, const TTail &... tail)
{
	return Min(head, Min(tail...));
}

template <typename TForwardIterator>
inline constexpr TForwardIterator MaxElement(TForwardIterator begin, TForwardIterator end)
{
	if (begin == end)
		return end;

	auto result = begin++;
	for (; begin != end; ++begin)
	{
		if (*begin > *result)
			result = begin;
	}
	return result;
}

template <typename T>
inline constexpr T Max(T a, T b)
{
	return a < b ? b : a;
}

template <typename T>
inline constexpr T Max(std::initializer_list<T> values)
{
	return *MaxElement(values.begin(), values.end());
}

template <typename THead, typename ... TTail>
inline constexpr THead Max(const THead &head, const TTail &... tail)
{
	return Max(head, Max(tail...));
}

template <typename TForwardIterator>
inline constexpr TPair<TForwardIterator, TForwardIterator> MinMaxElement(TForwardIterator begin, TForwardIterator end)
{
	auto result = MakePair(begin, begin);
	if (begin == end || ++begin == end)
		return result;
	
	if (*begin < *result.First)
		result.First = begin;
	else if (*result.Second < *begin)
		result.Second = begin;

	for (auto prev = begin; begin != end; prev = ++begin)
	{
		if (++begin == end)
		{
			if (*prev < *result.First)
				result.First = prev;
			else if (*result.Second < *prev)
				result.Second = prev;
			break;
		}
		else
		{
			if (*prev < *begin)
			{
				if (*prev < *result.First)
					result.First = prev;
				if (*result.Second < *begin)
					result.Second = begin;
			}
			else
			{
				if (*begin < *result.First)
					result.First = begin;
				if (*result.Second < *prev)
					result.Second = prev;
			}
		}
	}
	return result;
}

template <typename T>
inline constexpr TPair<T, T> MinMax(T value)
{
	return MakePair(value, value);
}

template <typename T>
inline constexpr TPair<T, T> MinMax(T lhs, T rhs)
{
	return lhs < rhs ? MakePair(lhs, rhs) : MakePair(rhs, lhs);
}

template <typename T>
inline constexpr TPair<T, T> MinMax(std::initializer_list<T> values)
{
	const auto result = MinMaxElement(values.begin(), values.end());
	return MakePair(*result.First, *result.Second);
}

template <typename THead, typename ... TTail>
inline constexpr TPair<THead, THead> MinMax(const THead &first, const THead &second, const TTail &... tail)
{
	auto result = MinMax(tail...);
	if (first < second)
	{
		if (first < result.First)
			result.First = first;
		if (result.Second < second)
			result.Second = second;
	}
	else
	{
		if (second < result.First)
			result.First = second;
		if (result.Second < first)
			result.Second = first;
	}
	return result;
}

template <typename TOutputIterator, typename TSize, typename TValue>
inline constexpr TOutputIterator FillN(TOutputIterator first, TSize n, const TValue &value)
{
	for (int32 i = 0; i < n; ++i)
		*first++ = value;
	return first;
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

template <CIntegral T>
inline constexpr T DivCeil(T x, T y)
{
	return (x + y - 1) / y;
}

template <typename T>
constexpr void Swap(T &lhs, T &rhs)
{
	auto temp = Move(lhs);
	lhs = Move(rhs);
	rhs = Move(temp);
}

template <typename T>
constexpr void Swap(T &&lhs, T &&rhs)
{
	auto temp = Move(lhs);
	lhs = Move(rhs);
	rhs = Move(temp);
}

inline void MemCopy(void *dst, const void *src, int32 size)
{
	memcpy(dst, src, size);
}

constexpr bool IsWhiteSpace(const char symbol)
{
	return
		symbol == ' '  ||
		symbol == '\t' ||
		symbol == '\n' ||
		symbol == '\v' ||
		symbol == '\f' ||
		symbol == '\r';
}

constexpr bool IsDigit(const char symbol)
{
	return symbol >= '0' && symbol <= '9';
}

constexpr bool IsUpper(const char symbol)
{
	return symbol >= 'A' && symbol <= 'Z';
}

constexpr bool IsLower(const char symbol)
{
	return symbol >= 'a' && symbol <= 'z';
}

constexpr bool IsAlpha(const char symbol)
{
	return IsUpper(symbol) || IsLower(symbol);
}

constexpr char ToUpper(const char symbol)
{
	if (!IsLower(symbol))
		return symbol;
	return symbol + ('A' - 'a');
}

constexpr char ToLower(const char symbol)
{
	if (!IsUpper(symbol))
		return symbol;
	return symbol + ('a' - 'A');
}