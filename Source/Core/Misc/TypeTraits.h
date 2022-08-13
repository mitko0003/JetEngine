#pragma once

#include "Core/Misc/Types.h"

using TNullptr = decltype(nullptr);
using TVoid = void;

template <typename T>
struct TTypeIdentity
{
	using Type = T;
};

template <typename T>
using TTypeIdentity_t = typename TTypeIdentity<T>::Type;

template <bool Test, typename TTrue, typename TFalse>
struct TConditional :
	public TTypeIdentity<TTrue> {};

template <typename TTrue, typename TFalse>
struct TConditional<false, TTrue, TFalse> :
	public TTypeIdentity<TFalse> {};

template <bool Test, typename TTrue, typename TFalse>
using TConditional_t = TConditional<Test, TTrue, TFalse>::Type;

template <typename T>
struct TRemoveReference :
	public TTypeIdentity<T> {};

template <typename T>
struct TRemoveReference<T &> :
	public TTypeIdentity<T> {};

template <typename T>
struct TRemoveReference<T &&> :
	public TTypeIdentity<T> {};

template <typename T>
using TRemoveReference_t = typename TRemoveReference<T>::Type;

template <typename T>
struct TRemovePointer :
	public TTypeIdentity<T> {};

template <typename T>
struct TRemovePointer<T *> :
	public TTypeIdentity<T> {};

template <typename T>
using TRemovePointer_t = typename TRemovePointer<T>::Type;

template <typename T>
struct TRemoveExtent :
	public TTypeIdentity<T> {};

template <typename T, size_t Size>
struct TRemoveExtent<T[Size]> :
	public TTypeIdentity<T> {};

template <typename T>
struct TRemoveExtent<T[]> :
	public TTypeIdentity<T> {};

template <typename T>
using TRemoveExtent_t = typename TRemoveExtent<T>::Type;

template <typename T>
struct TRemoveConst :
	public TTypeIdentity<T> {};

template <typename T>
struct TRemoveConst<const T> :
	public TTypeIdentity<T> {};

template <typename T>
using TRemoveConst_t = typename TRemoveConst<T>::Type;

template <typename T>
struct TRemoveVolatile :
	public TTypeIdentity<T> {};

template <typename T>
struct TRemoveVolatile<volatile T> :
	public TTypeIdentity<T> {};

template <typename T>
using TRemoveVolatile_t = typename TRemoveVolatile<T>::Type;

template <typename T>
struct TRemoveConstVolatile :
	public TRemoveVolatile<TRemoveConst_t<T>> {};

template <typename T>
using TRemoveConstVolatile_t = typename TRemoveConstVolatile<T>::Type;

template <typename T>
auto TryAddPointer(int)->TTypeIdentity<T *>;
template <typename T>
auto TryAddPointer(...)->TTypeIdentity<TRemoveReference_t<T> *>;

template <typename T>
using TAddPointer = decltype(TryAddPointer<T>(0));

template <typename T>
using TAddPointer_t = TAddPointer<T>::Type;

template <typename T, T Value>
struct TIntegralConstant
{
	static constexpr T Value = Value;
};

template <bool Value>
using TBoolTrait = TIntegralConstant<bool, Value>;

using TTrueTrait = TBoolTrait<true>;
using TFalseTrait = TBoolTrait<false>;

template <bool Value, typename T = void>
struct TEnableIf {};

template <typename T>
struct TEnableIf<true, T> :
	public TTypeIdentity<T> {};

template <bool Value, typename T = void>
using TEnableIf_t = typename TEnableIf<Value, T>::Type;

template <typename, typename>
struct TAreSame :
	public TBoolTrait<false> {};

template <typename TLhs>
struct TAreSame<TLhs, TLhs> :
	public TBoolTrait<true> {};

template <typename TLhs, typename TRhs>
constexpr bool AreSame = TAreSame<TLhs, TRhs>::Value;

template <typename TLhs, typename TRhs, typename ... TRemaining>
struct TIsAnyOf :
	public TBoolTrait<AreSame<TLhs, TRhs> || TIsAnyOf<TLhs, TRemaining...>::Value> {};

template <typename TLhs, typename TRhs>
struct TIsAnyOf<TLhs, TRhs> :
	public TBoolTrait<AreSame<TLhs, TRhs>> {};

template <typename TLhs, typename ... TAll>
constexpr bool IsAnyOf = TIsAnyOf<TLhs, TAll...>::Value;

template <typename T>
struct TIsIntegral :
	public TBoolTrait<IsAnyOf<T, bool, char, signed char, short, int, long long, unsigned char, unsigned short, unsigned int, unsigned long long>> {};

template <typename T>
constexpr bool IsIntegral = TIsIntegral<T>::Value;

template <typename T>              struct TIsArray          : public TBoolTrait<false> {};
template <typename T>              struct TIsArray<T[]>     : public TBoolTrait<true> {};
template <typename T, size_t Size> struct TIsArray<T[Size]> : public TBoolTrait<true> {};
template <typename T> constexpr bool IsArray = TIsArray<T>::Value;

template <typename T> struct TIsConst          : public TBoolTrait<false> {};
template <typename T> struct TIsConst<const T> : public TBoolTrait<true> {};
template <typename T> constexpr bool IsConst = TIsConst<T>::Value;

template <typename T> struct TIsReference     : public TBoolTrait<false> {};
template <typename T> struct TIsReference<T&> : public TBoolTrait<true> {};
template <typename T> constexpr bool IsReference = TIsReference<T>::Value;

template <typename T> struct TIsFunction : public TBoolTrait<!IsConst<const T> && !IsReference<T>> {};
template <typename T> constexpr bool IsFunction = TIsFunction<T>::Value;

template <typename T> class TIsClass : public TBoolTrait<__is_class(T)> {};
template <typename T> constexpr bool IsClass = TIsClass<T>::Value;

template <typename T> class TIsStandardLayout : public TBoolTrait<__is_standard_layout(T)> {};
template <typename T> constexpr bool IsStandardLayout = TIsStandardLayout<T>::Value;

template <typename T> class TIsMoveAssignable : public TBoolTrait<__is_assignable(T &, T &&)> {};
template <typename T> constexpr bool IsMoveAssignable = TIsMoveAssignable<T>::Value;

template <typename TLhs, typename TRhs> class TIsAssignable : public TBoolTrait<__is_assignable(TLhs &, const TRhs &)> {};
template <typename TLhs, typename TRhs> constexpr bool IsAssignable = TIsAssignable<TLhs, TRhs>::Value;

template <typename T>
class TIsCopyAssignable :
	public TBoolTrait<__is_assignable(T &, const T &)> {};

template <typename T>
constexpr bool IsCopyAssignable = TIsCopyAssignable<T>::Value;

template <typename T>
class TIsCopyConstructible :
	public TBoolTrait<__is_constructible(T, const T &)> {};

template <typename T>
constexpr bool IsCopyConstructible = TIsCopyConstructible<T>::Value;

template <typename T>
class TIsDefaultConstructible :
	public TBoolTrait<__is_constructible(T)> {};

template <typename T>
constexpr bool IsDefaultConstructible = TIsDefaultConstructible<T>::Value;

template <typename T>
struct TDecay
{
private:
	using _TTemp1 = TRemoveReference_t<T>;
	using _TTemp2 = TConditional_t<IsFunction<_TTemp1>, TAddPointer_t<_TTemp1>, TRemoveConstVolatile_t<_TTemp1>>;
public:
	using Type = TConditional_t<IsArray<_TTemp1>, TAddPointer_t<TRemoveExtent_t<_TTemp1>>, _TTemp2>;
};

template <typename T>
using TDecay_t = TDecay<T>::Type;