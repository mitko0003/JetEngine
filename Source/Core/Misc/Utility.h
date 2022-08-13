#pragma once

#include <cstdint>
#include "TypeTraits.h"

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
inline constexpr typename TEnableIf_t<TIsIntegral_v<T>, T> DivCeil(T x, T y)
{
	return (x + y - 1) / y;
}

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

template <typename T>
T *AddressOf(T &reference)
{
	return &reference;
}

template <class T>
constexpr void DestroyAt(T *object)
{
	if constexpr (TIsArray_v<T>)
		for (auto &elem : *object)
			DestroyAt(AddressOf(elem));
	else
		object->~T();
}

template<typename T, typename... TArgs>
inline void ConstructAt(T *object, TArgs &&... args)
{
	::new (static_cast<void *>(object)) T(Forward<TArgs>(args)...);
}