#pragma once

#include "Utility.h"

template <typename TFirst, typename TSecond>
class TPair
{
public:
	TPair() {}
	TPair(TFirst first, TSecond second) :
		First(first), Second(second) {}
	TPair(const TPair &rhs) :
		First(rhs.First), Second(rhs.Second) {}
	TPair(TPair &&rhs) noexcept :
		First(Move(rhs.First)), Second(Move(rhs.Second)) {}

	TPair &operator=(const TPair &rhs)
	{
		if (this != &rhs)
		{
			First = rhs.First;
			Second = rhs.Second;
		}
		return *this;
	}

	TPair &operator=(TPair &&rhs)
	{
		if (this != &rhs)
		{
			Swap(First, rhs.First);
			Swap(Second, rhs.Second);
		}
		return *this;
	}

	~TPair()
	{
		DestroyAt(&First);
		DestroyAt(&Second);
	}

	union
	{
		TFirst First;
		TFirst x;
	};
	union
	{
		TSecond Second;
		TSecond y;
	};
};

template <typename TFirst, typename TSecond>
constexpr TPair<TFirst, TSecond> MakePair(TFirst first, TSecond second)
{
	return TPair<TFirst, TSecond>(first, second);
}

template <typename TFirst, typename TSecond>
constexpr TPair<TFirst, TSecond> MakePair(TFirst &&first, TSecond &&second)
{
	return TPair<TFirst, TSecond>(Move(first), Move(second));
}