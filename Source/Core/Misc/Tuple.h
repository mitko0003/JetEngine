#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Misc/Utility.h"

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

	constexpr bool operator==(const TPair<TFirst, TSecond> &rhs) const
	{
		return First == rhs.First && Second == rhs.Second;
	}

	constexpr bool operator!=(const TPair<TFirst, TSecond> &rhs) const
	{
		return First != rhs.First || Second != rhs.Second;
	}

	struct
	{
		TFirst First;
		TFirst x;
	};
	struct
	{
		TSecond Second;
		TSecond y;
	};
};

template <typename TFirst, typename TSecond>
constexpr TPair<TFirst, TSecond> MakePair(const TFirst &first, const TSecond &second)
{
	return TPair<TFirst, TSecond>(first, second);
}

template <typename TFirst, typename TSecond>
constexpr TPair<TDecay_t<TFirst>, TDecay_t<TSecond>> MakePair(TFirst &&first, TSecond &&second)
{
	return TPair<TDecay_t<TFirst>, TDecay_t<TSecond>>(Move(first), Move(second));
}

// In order for C++17 structured bindings to work we need to specialize std::tuple_size, std::tuple_element, std::get.
// This is yet another language/compiler feature bound to the std library.

#include <tuple>

namespace std
{
	template <typename TFirst, typename TSecond>
	struct tuple_size<TPair<TFirst, TSecond>> :
		public integral_constant<size_t, 2> {};

	template <typename TFirst, typename TSecond>
	struct tuple_element<0, TPair<TFirst, TSecond>> :
		public type_identity<TFirst> {};

	template <typename TFirst, typename TSecond>
	struct tuple_element<1, TPair<TFirst, TSecond>> :
		public type_identity<TSecond> {};

	template <size_t I, typename TFirst, typename TSecond>
	tuple_element_t<I, TPair<TFirst, TSecond>> &get(TPair<TFirst, TSecond> &pair)
	{
		if constexpr (I == 0)
			return pair.First;
		else
			return pair.Second;
	}

	template <size_t I, typename TFirst, typename TSecond>
	const tuple_element_t<I, TPair<TFirst, TSecond>> &get(const TPair<TFirst, TSecond> &pair)
	{
		if constexpr (I == 0)
			return pair.First;
		else
			return pair.Second;
	}

	template <size_t I, typename TFirst, typename TSecond>
	tuple_element_t<I, TPair<TFirst, TSecond>> &&get(TPair<TFirst, TSecond> &&pair)
	{
		if constexpr (I == 0)
			return move(pair.First);
		else
			return move(pair.Second);
	}
}