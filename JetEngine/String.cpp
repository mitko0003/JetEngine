#include "Precompiled.h"

#include <string.h>
#include <stdint.h>

#include "String.h"

TString::TString() :
	Size(0), Data(nullptr) {}

TString::TString(const char *input)
{
	Size = int32_t(strlen(input));
	Data = new char[Size + 1];
	memcpy(Data, input, Size + 1);
}

TString::TString(const TString &rhs) :
	Size(rhs.Size), Data(nullptr)
{
	if (Size != 0)
	{
		Data = new char[Size + 1];
		memcpy(Data, rhs.Data, Size + 1);
	}
}

TString::TString(TString &&rhs) :
	Size(0), Data(nullptr)
{
	*this = Move(rhs);
}

TString::~TString()
{
	delete[] Data;
}

TString& TString::operator=(TString &&rhs)
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

TString TString::operator+(const TString &rhs) const
{
	TString result;
	result.Size = Size + rhs.Size;
	if (result.Size > 0)
	{
		result.Data = new char[result.Size + 1];

		memcpy(result.Data, Data, Size);
		if (rhs.Data != nullptr)
			memcpy(result.Data + Size, rhs.Data, rhs.Size + 1);
		else
			result.Data[result.Size + 1] = '\0';
	}
	return result;
}

TString TString::operator+(const char *rhs) const
{
	TString dummy;
	dummy.Data = const_cast<char*>(rhs);
	dummy.Size = int32_t(strlen(rhs));

	TString result;
	result = *this + dummy;

	ASSERT(rhs == dummy.Data);
	dummy.Data = nullptr;
	dummy.Size = 0;

	return result;
}

TString& TString::operator+=(const TString &rhs)
{
	return *this = *this + rhs;
}

TString& TString::operator+=(const char *rhs)
{
	return *this = *this + rhs;
}

int32_t TString::Length() const
{
	return Size;
}

const char *TString::c_str() const
{
	return Data;
}