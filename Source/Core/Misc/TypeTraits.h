#pragma once

using TNullptr = decltype(nullptr);
using TVoid = void;

template <typename T>
struct TRemoveReference
{
	using Type = T;
};

template <typename T>
struct TRemoveReference<T &>
{
	using Type = T;
};

template <typename T>
struct TRemoveReference<T &&>
{
	using Type = T;
};

template <typename T>
using TRemoveReference_t = typename TRemoveReference<T>::Type;

template <bool Value>
struct TBoolTrait
{
	static constexpr bool Value = Value;
};

using TTrueTrait = TBoolTrait<true>;
using TFalseTrait = TBoolTrait<false>;

template <bool Value, typename T = void>
struct TEnableIf {};

template <typename T>
struct TEnableIf<true, T>
{
	using Type = T;
};

template <bool Value, typename T = void>
using TEnableIf_t = typename TEnableIf<Value, T>::Type;

template <typename, typename>
struct TAreSame :
	public TBoolTrait<false> {};

template <typename TLhs>
struct TAreSame<TLhs, TLhs> :
	public TBoolTrait<true> {};

template <typename TLhs, typename TRhs>
constexpr bool TAreSame_v = TAreSame<TLhs, TRhs>::Value;

template <typename TLhs, typename TRhs, typename ... TRemaining>
struct TIsAnyOf :
	public TBoolTrait<TAreSame_v<TLhs, TRhs> || TIsAnyOf<TLhs, TRemaining...>::Value> {};

template <typename TLhs, typename TRhs>
struct TIsAnyOf<TLhs, TRhs> :
	public TBoolTrait<TAreSame_v<TLhs, TRhs>> {};

template <typename T>
struct TIsIntegral :
	public TBoolTrait<TIsAnyOf<T, bool, char, int, unsigned int>::Value> {};

template <typename T>
constexpr bool TIsIntegral_v = TIsIntegral<T>::Value;

template <typename T>
struct TIsArray :
	public TBoolTrait<false> {};

template <typename T>
struct TIsArray<T[]> :
	public TBoolTrait<true> {};

template <typename T, size_t Size>
struct TIsArray<T[Size]> :
	public TBoolTrait<true> {};

template <typename T>
constexpr bool TIsArray_v = TIsArray<T>::Value;

template <typename T>
class TIsClass :
	public TBoolTrait<__is_class(T)> {};

template <typename T>
class TIsStandardLayout :
	public TBoolTrait<__is_standard_layout(T)> {};

template <typename T>
class TIsMoveAssignable :
	public TBoolTrait<__is_assignable(T&, T&&)> {};

template <typename T>
constexpr bool TIsMoveAssignable_v = TIsMoveAssignable<T>::Value;

template <typename TLhs, typename TRhs>
class TIsAssignable :
	public TBoolTrait<__is_assignable(TLhs&, const TRhs&)> {};

template <typename TLhs, typename TRhs>
constexpr bool TIsAssignable_v = TIsAssignable<TLhs, TRhs>::Value;

template <typename T>
class TIsCopyAssignable :
	public TBoolTrait<__is_assignable(T&, const T&)> {};

template <typename T>
constexpr bool TIsCopyAssignable_v = TIsCopyAssignable<T>::Value;

template <typename T>
class TIsCopyConstructible :
	public TBoolTrait<__is_constructible(T, const T&)> {};

template <typename T>
constexpr bool TIsCopyConstructible_v = TIsCopyConstructible<T>::Value;

template <typename T>
class TIsDefaultConstructible :
	public TBoolTrait<__is_constructible(T)> {};

template <typename T>
constexpr bool TIsDefaultConstructible_v = TIsDefaultConstructible<T>::Value;