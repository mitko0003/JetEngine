#include "pch.h"

#include "Core/Containers/String.h"
#include "Core/Misc/Utility.h"

TEST(TestMinMax, TestMisc) {
	EXPECT_EQ(1, Min(1, 2, 3, 4));
	EXPECT_EQ(1, Min({ 1, 2, 3, 4 }));
	EXPECT_EQ(4, Max(1, 2, 3, 4));
	EXPECT_EQ(4, Max({ 1, 2, 3, 4 }));
	EXPECT_EQ(MakePair(1, 4), MinMax({ 1, 2, 3, 4 }));
	EXPECT_EQ(MakePair(1, 4), MinMax(1, 2, 3, 4));
	EXPECT_EQ(MakePair(-1, 4), MinMax(4, 3, 2, 1, -1));
	EXPECT_EQ(MakePair(-2, 4), MinMax(4, 3, -2));
}

TEST(TestTypeTraits, TestIntegral) {
	EXPECT_EQ(true, IsIntegral<bool>);
	EXPECT_EQ(true, IsIntegral<char>);
	EXPECT_EQ(true, IsIntegral<int8>);
	EXPECT_EQ(true, IsIntegral<int16>);
	EXPECT_EQ(true, IsIntegral<int32>);
	EXPECT_EQ(true, IsIntegral<int64>);
	EXPECT_EQ(true, IsIntegral<uint8>);
	EXPECT_EQ(true, IsIntegral<uint16>);
	EXPECT_EQ(true, IsIntegral<uint32>);
	EXPECT_EQ(true, IsIntegral<uint64>);
	EXPECT_EQ(false, IsIntegral<float>);
	EXPECT_EQ(false, IsIntegral<double>);
}

TEST(TestTypeTraits, TestIsConst) {
	EXPECT_EQ(false, IsConst<int>);
	EXPECT_EQ(false, IsConst<int &>);
	EXPECT_EQ(false, IsConst<volatile int>);
	EXPECT_EQ(false, IsConst<int *>);
	EXPECT_EQ(false, IsConst<volatile int *>);
	EXPECT_EQ(false, IsConst<const int *>);
	EXPECT_EQ(false, IsConst<const volatile int *>);
	EXPECT_EQ(false, IsConst<int const *>);
	EXPECT_EQ(false, IsConst<volatile int const *>);
	EXPECT_EQ(false, IsConst<const int &>);
	EXPECT_EQ(false, IsConst<const volatile int &>);
	EXPECT_EQ(true, IsConst<const int>);
	EXPECT_EQ(true, IsConst<const volatile int>);
	EXPECT_EQ(true, IsConst<int *const>);
	EXPECT_EQ(true, IsConst<volatile int *const>);
}

TEST(TestTypeTraits, TestIsReference) {
	EXPECT_EQ(false, IsReference<int>);
	EXPECT_EQ(false, IsReference<volatile int>);
	EXPECT_EQ(false, IsReference<int *>);
	EXPECT_EQ(false, IsReference<volatile int *>);
	EXPECT_EQ(false, IsReference<const int *>);
	EXPECT_EQ(false, IsReference<const volatile int *>);
	EXPECT_EQ(false, IsReference<int const *>);
	EXPECT_EQ(false, IsReference<volatile int const *>);
	EXPECT_EQ(false, IsReference<const int>);
	EXPECT_EQ(false, IsReference<const volatile int>);
	EXPECT_EQ(false, IsReference<int *const>);
	EXPECT_EQ(false, IsReference<volatile int *const>);
	EXPECT_EQ(true, IsReference<int &>);
	EXPECT_EQ(true, IsReference<const int &>);
	EXPECT_EQ(true, IsReference<const volatile int &>);
}

TEST(TestString, TestMisc) {
	EXPECT_EQ(TString("ali"), TString("ali"));
	EXPECT_EQ(TString("alibali"), TString("ali") + TString("bali"));
	EXPECT_EQ(0, StringCompare("ali", "ali"));
	EXPECT_GT(0, StringCompare("ali1bali", "ali2bali"));
	EXPECT_LT(0, StringCompare("ali2bali", "ali1bali"));
}


TEST(TestToString, TestIntegral) {
	EXPECT_EQ(TString("42"), ToString(42));
	EXPECT_EQ(42, StringTo<int32>("42"));
	EXPECT_EQ(-12423, StringTo<int32>("-12423"));
}