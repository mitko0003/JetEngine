#pragma once

#include <string.h>

#include "Core/Misc/TypeTraits.h"
#include "Core/Misc/Utility.h"
#include "Core/Misc/Tuple.h"

template <typename T>
class TVarArray
{
public:
	TVarArray() :
		Size(0), Capacity(0), Data(nullptr) {}

	TVarArray(size_t count) : Size(0), Capacity(count)
	{
		Data = reinterpret_cast<T*>(new char[sizeof(T)*count]);
	}

	TVarArray(TVarArray&& other) :
		Size(other.Size), Capacity(other.Capacity), Data(other.Data)
	{
		other.Data = nullptr;
		other.Size = 0;
		other.Capacity = 0;
	}

	TVarArray(const TVarArray& other) :
		Size(other.Size), Capacity(other.Size)
	{
		copy(other);
	}

	TVarArray<T>& operator=(const TVarArray<T>& other)
	{
		if (this != &other) {
			del();
			copy(other);
			Size = other.Size;
			Capacity = other.Size;
		}
		return *this;
	}

	TVarArray<T>& operator=(TVarArray<T>&& other)
	{
		if (this != &other) {
			Swap(Size, other.Size);
			Swap(Capacity, other.Capacity);
			Swap(Data, other.Data);
		}
		return *this;
	}

	T& operator [](size_t pos)
	{
		return Data[pos];
	}

	const T& operator [](size_t pos) const
	{
		return Data[pos];
	}

	T* data() const
	{
		return Data;
	}

	T& front() {
		return Data[0];
	}
	const T& front() const {
		return Data[0];
	}

	T& back() {
		return Data[Size - 1];
	}
	const T& back() const {
		return Data[Size - 1];
	}

	T& push_back(const T& value)
	{
		if (Size == Capacity)
			reserve(getNextSize());
		return Create(&Data[Size++], value);
	}

	TEnableIf_t<IsMoveAssignable<T>, T>& push_back(T&& value)
	{
		if (Size == Capacity)
			reserve(getNextSize());
		return Create(&Data[Size++], Move(value));
	}

	void pop_back()
	{
		if (empty())
			return;
		(Data + --Size)->~T();
	}

	void resize(size_t count)
	{
		if (count < Size)
			for (size_t i = count; i < Size; i++)
				(Data + i)->~T();
		else if (count > Size) {
			reserve(count);
			for (size_t i = Size; i < count; i++)
				::new (Data + i) T();
		}
		Size = count;
	}

	void reserve(size_t new_cap)
	{
		if (new_cap > Capacity) {
			T* temp = reinterpret_cast<T*>(new char[sizeof(T) * new_cap]);
			for (size_t i = 0; i < Size; i++)
			{
				Create(&temp[i], Data[i]);
				Data[i].~T();
			}
			delete[] reinterpret_cast<char*>(Data);
			Capacity = new_cap;
			Data = temp;
		}
	}

	void shrink_to_fit()
	{
		T* temp = reinterpret_cast<T*>(new char[sizeof(T) * Size]);
		for (size_t i = 0; i < Size; i++)
		{
			Create(temp + i, Data[i]);
			Data[i].~T();
		}
		delete[] reinterpret_cast<char*>(Data);
		Capacity = Size;
		Data = temp;
	}

	size_t size() const
	{
		return Size;
	}

	size_t capacity() const
	{
		return Capacity;
	}

	bool empty() const
	{
		return !static_cast<bool>(Size);
	}

private:
	T* Data;
	size_t Size;
	size_t Capacity;
	static_assert(IsCopyConstructible<T> ||
		(IsDefaultConstructible<T> && IsAssignable<T, const T&>),
		"Vector requires copy constructor or default constructor + assignment operator");

	// Used to create objects depending on constructor-operator combination

	T& Create(T* mem, const T& value)
	{
		if constexpr (IsCopyConstructible<T>)
		{
			ConstructAt(mem, value);
		}
		else
		{
			ConstructAt(mem);
			*mem = value;
		}
		return *mem;
	}

	TEnableIf_t<IsMoveAssignable<T>, T>& Create(T* mem, T&& value)
	{
		if constexpr (IsCopyConstructible<T>)
		{
			ConstructAt(mem, Move(value));
		}
		else
		{
			ConstructAt(mem);
			*mem = Move(value);
		}
		return *mem;
	}

	void copy(const TVarArray& other)
	{
		Data = reinterpret_cast<T*>(new char[sizeof(T) * other.Size]);
		for (size_t i = 0; i < other.Size; i++)
			Create(&Data[i], other.Data[i]);
	}

	void del()
	{
		for (size_t i = 0; i < Size; ++i)
			(Data + i)->~T();
		delete[] reinterpret_cast<char*>(Data);
	}

	size_t getNextSize() const
	{
		return (Capacity * 3) / 2 + 1;
	}
};

template <typename TChar>
class TBasicString
{
public:
	TBasicString() :
		Size(0), Data(nullptr) {}

	explicit TBasicString(const TChar *input)
	{
		Size = int32_t(strlen(input));
		Data = new char[Size + 1];
		MemCopy(Data, input, Size + 1);
	}

	TBasicString(const TBasicString &rhs) :
		Size(rhs.Size), Data(nullptr)
	{
		if (Size != 0)
		{
			Data = new TChar[Size + 1];
			MemCopy(Data, rhs.Data, Size + 1);
		}
	}

	TBasicString(TBasicString &&rhs) :
		Size(0), Data(nullptr)
	{
		*this = Move(rhs);
	}

	TBasicString &operator=(TBasicString &&rhs)
	{
		if (this != &rhs)
		{
			delete[] Data;
			Data = nullptr;
			Size = 0;

			Swap(Data, rhs.Data);
			Swap(Size, rhs.Size);
		}
		return *this;
	}

	template <CIntegral TIndex>
	constexpr TChar &operator[](TIndex i) {
		return Data[i];
	}
	template <CIntegral TIndex>
	constexpr const TChar &operator[](TIndex i) const {
		return Data[i];
	}

	TBasicString operator+(const TBasicString &rhs) const
	{
		TBasicString result;
		result.Size = Size + rhs.Size;
		if (result.Size > 0)
		{
			result.Data = new char[result.Size + 1];

			MemCopy(result.Data, Data, Size);
			if (rhs.Data != nullptr)
				MemCopy(result.Data + Size, rhs.Data, rhs.Size + 1);
			else
				result.Data[result.Size + 1] = TChar('\0');
		}
		return result;
	}

	TBasicString operator+(const TChar *rhs) const
	{
		TBasicString dummy;
		dummy.Data = const_cast<char *>(rhs);
		dummy.Size = int32_t(strlen(rhs));

		TBasicString result;
		result = *this + dummy;

		dummy.Data = nullptr;
		dummy.Size = 0;

		return result;
	}

	TBasicString &operator+=(const TBasicString &rhs)
	{
		return *this = *this + rhs;
	}
	TBasicString &operator+=(const TChar *rhs)
	{
		return *this = *this + rhs;
	}

	bool operator==(const TBasicString &rhs) const
	{
		if (Size != rhs.Size)
			return false;

		for (int32 i = 0; i < Size; ++i)
		{
			if ((*this)[i] != rhs[i])
				return false;
		}

		return true;
	}

	bool operator!=(const TBasicString &rhs) const
	{
		return !(*this == rhs);
	}

	int32 Length() const {
		return Size;
	}
	const TChar *c_str() const {
		return Data;
	}

private:
	TChar *Data;
	int32 Size;
};

using TString = TBasicString<char8>;

inline TString ToString(int32 integer)
{
	char temporary[10];
	auto last = temporary;

	do
	{
		*last++ = '0' + integer % 10;
	} while (integer /= 10);

	char result[10];
	auto curr = result;

	do
	{
		*curr++ = *--last;
	} while (last != temporary);
	*curr = '\0';

	return TString(result);
}

template <typename T>
inline T StringTo(const char*);

template <>
inline uint32 StringTo(const char *data)
{
	int32 result = 0;
	for (; *data != '\0' && *data >= '0' && *data <= '9'; ++data)
		result = result * 10 + (*data - '0');
	return result;
}

template <>
inline int32 StringTo(const char *data)
{
	int32 sign = 1;
	if (*data == '-')
	{
		sign = -1;
		++data;
	}
	return sign * StringTo<uint32>(data);
}

inline int32 StringCompare(const char *lhs, const char *rhs)
{
	int32 i = 0;
	while (lhs[i] == rhs[i] && lhs[i] != '\0' && rhs[i] != '\0')
		++i;
	if (lhs[i] == rhs[i])
		return 0;
	return lhs[i] > rhs[i] ? 1 : -1;
}

inline int32 StringCompareN(const char *lhs, const char *rhs, int32 n)
{
	int32 i = 0;
	while (lhs[i] == rhs[i] && lhs[i] != '\0' && rhs[i] != '\0' && i < n - 1)
		++i;
	if (lhs[i] == rhs[i])
		return 0;
	return lhs[i] > rhs[i] ? 1 : -1;
}