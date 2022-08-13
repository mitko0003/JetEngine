#pragma once

#include <cstdint>
#include <float.h>

template <typename T>
struct TNumericLimits {};

template <>
struct TNumericLimits<int>
{
	static constexpr int Min() noexcept
	{
		return INT32_MIN;
	}

	static constexpr int Max() noexcept
	{
		return INT32_MAX;
	}
};

template <>
struct TNumericLimits<unsigned int>
{
	static constexpr unsigned int Min() noexcept
	{
		return 0;
	}

	static constexpr unsigned int Max() noexcept
	{
		return UINT32_MAX;
	}
};

template <>
struct TNumericLimits<long long>
{
	static constexpr long long Min() noexcept
	{
		return INT64_MIN;
	}

	static constexpr long long Max() noexcept
	{
		return INT64_MAX;
	}
};

template <>
struct TNumericLimits<unsigned long long>
{
	static constexpr unsigned long long Min() noexcept
	{
		return 0;
	}

	static constexpr unsigned long long Max() noexcept
	{
		return UINT64_MAX;
	}
};

template <>
struct TNumericLimits<float>
{
	static constexpr float Min() noexcept
	{
		return FLT_MIN;
	}

	static constexpr float Max() noexcept
	{
		return FLT_MAX;
	}

	static constexpr float Epsilon() noexcept
	{
		return FLT_EPSILON;
	}

	static constexpr double Infinity() noexcept
	{
		return __builtin_huge_valf();
	}
};

template <>
struct TNumericLimits<double>
{
	static constexpr double Min() noexcept
	{
		return DBL_MIN;
	}

	static constexpr double Max() noexcept
	{
		return DBL_MAX;
	}

	static constexpr double Epsilon() noexcept
	{
		return DBL_EPSILON;
	}

	static constexpr double Infinity() noexcept
	{
		return __builtin_huge_val();
	}
};