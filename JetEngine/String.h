#pragma once

#include "TypeTraits.h"
#include "Utility.h"
#include "Tuple.h"

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

	TEnableIf_t<TIsMoveAssignable_v<T>, T>& push_back(T&& value)
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
	static_assert(TIsCopyConstructible_v<T> ||
		(TIsDefaultConstructible_v<T> && TIsAssignable_v<T, const T&>),
		"Vector requires copy constructor or default constructor + assignment operator");

	// Used to create objects depending on constructor-operator combination

	T& Create(T* mem, const T& value)
	{
		if constexpr (TIsCopyConstructible_v<T>)
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

	TEnableIf_t<TIsMoveAssignable_v<T>, T>& Create(T* mem, T&& value)
	{
		if constexpr (TIsCopyConstructible_v<T>)
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

class TString
{
public:
	TString();
	explicit TString(const char *input);
	TString(const TString &rhs);
	TString(TString &&rhs);

	~TString();

	TString& operator=(TString &&rhs);

	TString operator+(const TString &rhs) const;
	TString operator+(const char *rhs) const;
	TString& operator+=(const TString &rhs);
	TString& operator+=(const char *rhs);

	int32_t Length() const;
	const char *c_str() const;

private:
	char *Data;
	int32_t Size;
};