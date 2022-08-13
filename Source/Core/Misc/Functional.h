#pragma once

struct TOpAdd
{
	template <typename T>
	inline constexpr T operator()(T lhs, T rhs) const
	{
		return lhs + rhs;
	}
};

struct TOpSub
{
	template <typename T>
	inline constexpr T operator()(T lhs, T rhs) const
	{
		return lhs - rhs;
	}
};

struct TOpDiv
{
	template <typename T>
	inline constexpr T operator()(T lhs, T rhs) const
	{
		return lhs / rhs;
	}
};

struct TOpMul
{
	template <typename T>
	inline constexpr T operator()(T lhs, T rhs) const
	{
		return lhs * rhs;
	}
};

struct TOpNeg
{
	template <typename T>
	inline constexpr T operator()(T value) const
	{
		return -value;
	}
};

struct TOpAnd
{
	inline constexpr bool operator()(bool lhs, bool rhs) const
	{
		return lhs && rhs;
	}
};

struct TOpOr
{
	inline constexpr bool operator()(bool lhs, bool rhs) const
	{
		return lhs || rhs;
	}
};

struct TOpNot
{
	inline constexpr bool operator()(bool value) const
	{
		return !value;
	}
};