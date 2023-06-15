#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Bitset/Bitset.h"

namespace UnitTests {
#pragma region Class

	static_assert(sizeof(gk::bitset<1>) == 1, "Tiny bitset does not occupy only 1 byte");
	static_assert(sizeof(gk::bitset<64>) == 8, "Large bitset does not occupy 8 bytes");
	static_assert(sizeof(gk::bitset<65>) == 16, "Bitset with a size greater than 64 bits doesn't occupy 12 bytes");

	static_assert(gk::bitset<1>::GetBitArrayCount() == 1, "Tiny should have only one union array element");
	static_assert(gk::bitset<64>::GetBitArrayCount() == 1, "64 bit bitset should have only one union array element");
	static_assert(gk::bitset<65>::GetBitArrayCount() > 1, "Bitset with a size greater than 64 bits should have more than one union array element");

#pragma endregion

#pragma region Default_Construct

	TEST(Bitset, DefaultConstruct) {
		gk::bitset<64> b;
		ASSERT_EQ(b.bits, 0);
	}

	TEST(Bitset, DefaultConstructArray) {
		gk::bitset<65> b;
		EXPECT_EQ(b.bitsArray[0], 0);
		EXPECT_EQ(b.bitsArray[1], 0);
	}

#pragma endregion

#pragma region Flag_Construct

	TEST(Bitset, FlagsConstruct) {
		gk::bitset<64> b = 1;
		EXPECT_EQ(b.bits, 1);
		EXPECT_EQ(b.bits & 1, 1);
	}

	TEST(Bitset, FlagsConstructArrayEqualSize) {
		std::array<unsigned long long, 2> flags = { 1, 3 };
		gk::bitset<65> b = flags;
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
	}

	TEST(Bitset, FlagsConstructArraySmallerSize) {
		std::array<unsigned long long, 2> flags = { 1, 3 };
		gk::bitset<129> b = flags;
		EXPECT_EQ(b.GetBitArrayCount(), 3);
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
		EXPECT_EQ(b.bitsArray[2], 0);
	}

	TEST(Bitset, FlagsConstructArrayLargerSize) {
		std::array<unsigned long long, 4> flags = { 1, 3, 5, 7 };
		gk::bitset<129> b = flags;
		EXPECT_EQ(b.GetBitArrayCount(), 3);
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
		EXPECT_EQ(b.bitsArray[2], 5);
	}

#pragma endregion

#pragma region Copy_Construct

	TEST(Bitset, CopyConstructSameSize) {
		gk::bitset<64> a = 2;
		gk::bitset<64> b = a;
		ASSERT_EQ(b.bits & 2, 1 << 1);
	}

	TEST(Bitset, CopyConstructDifferentSize) {
		gk::bitset<1> a = 1;
		gk::bitset<64> b = a;
		ASSERT_EQ(b.bits & 1, 1);
	}

	TEST(Bitset, CopyConstructArraySameSize) {
		std::array<unsigned long long, 2> flags = { 1, 3 };
		gk::bitset<65> a = flags;
		gk::bitset<65> b = a;
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
	}

	TEST(Bitset, CopyConstructArraySmallerSize) {
		std::array<unsigned long long, 2> flags = { 1, 3 };
		gk::bitset<129> a = flags;
		gk::bitset<129> b = a;
		EXPECT_EQ(b.GetBitArrayCount(), 3);
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
		EXPECT_EQ(b.bitsArray[2], 0);
	}

	TEST(Bitset, CopyConstructArrayLargerSize) {
		std::array<unsigned long long, 4> flags = { 1, 3, 5, 7 };
		gk::bitset<129> a = flags;
		gk::bitset<129> b = a;
		EXPECT_EQ(b.GetBitArrayCount(), 3);
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
		EXPECT_EQ(b.bitsArray[2], 5);
	}

	TEST(Bitset, CopyConstructBitsetTemplateArraySmallerSize) {
		std::array<unsigned long long, 4> flags = { 1, 3, 5, 7 };
		gk::bitset<129> a = flags;
		gk::bitset<65> b = a;
		EXPECT_EQ(b.GetBitArrayCount(), 2);
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
	}

	TEST(Bitset, CopyConstructBitsetTemplateArrayLargerSize) {
		std::array<unsigned long long, 4> flags = { 1, 3, 5, 7 };
		gk::bitset<129> a = flags;
		gk::bitset<512> b = a;
		EXPECT_EQ(b.GetBitArrayCount(), 8);
		EXPECT_EQ(b.bitsArray[0], 1);
		EXPECT_EQ(b.bitsArray[1], 3);
		EXPECT_EQ(b.bitsArray[2], 5);
		EXPECT_EQ(b.bitsArray[3], 0);
	}

#pragma endregion

#pragma region Get_Flag

	TEST(Bitset, GetBit) {
		const size_t bitflag = 1 << 7;
		gk::bitset<64> bit = bitflag;
		ASSERT_TRUE(bit.GetBit(7));
	}

	TEST(Bitset, GetBitOperator) {
		const size_t bitflag = 1 << 20;
		gk::bitset<64> bit = bitflag;
		ASSERT_TRUE(bit[20]);
	}

	TEST(Bitset, GetBit65Bitset) {
		gk::bitset<65> bit = std::array<unsigned long long, 2>{1, 0};
		ASSERT_TRUE(bit.GetBit(0));
		ASSERT_FALSE(bit.GetBit(64));
	}

	TEST(Bitset, GetBit65Bitset65thBitTrue) {
		gk::bitset<65> bit = std::array<unsigned long long, 2>{0, 1};
		ASSERT_TRUE(bit.GetBit(64));
		ASSERT_FALSE(bit.GetBit(0));
	}

#pragma endregion

#pragma region Set_Flag

	TEST(Bitset, SetBit) {
		gk::bitset<64> b;
		b.SetBit(1);
		ASSERT_TRUE(b[1]);
	}

	TEST(Bitset, SetBitOutOfRange) {
		gk::bitset<8> b;
		b.SetBit(8);
		ASSERT_EQ(b.bits, 0);
	}

	TEST(Bitset, SetBitToFalse) {
		gk::bitset<8> b = 0b100;
		b.SetBit(2, false);
		EXPECT_FALSE(b.GetBit(2));
		EXPECT_EQ(b.bits, 0);
	}

	TEST(Bitset, SetBit65Bitset) {
		gk::bitset<65> b;
		b.SetBit(1);
		b.SetBit(64);
		EXPECT_TRUE(b[1]);
		EXPECT_TRUE(b[64]);
		EXPECT_FALSE(b[0]);
	}

	TEST(Bitset, SetBit65BitsetFalse) {
		gk::bitset<65> b;
		b.SetBit(1);
		b.SetBit(64);
		EXPECT_TRUE(b[1]);
		EXPECT_TRUE(b[64]);
		EXPECT_FALSE(b[0]);
		b.SetBit(1, false);
		b.SetBit(64, false);
		EXPECT_FALSE(b[1]);
		EXPECT_FALSE(b[64]);
		EXPECT_FALSE(b[0]);
	}


#pragma endregion

#pragma region Copy_Operator_=

	TEST(Bitset, CopyOperatorOtherNums) {
		gk::bitset<64> b;
		b = 1 << 6;
		ASSERT_TRUE(b[6]);
	}

	TEST(Bitset, CopyOperatorSameSizeBitset) {
		gk::bitset<64> a;
		gk::bitset<64> b = 4;
		a = b;
		ASSERT_TRUE(a[2]);
	}

	TEST(Bitset, CopyOperatorOtherSizeBitset) {
		gk::bitset<64> a;
		gk::bitset<8> b = 4;
		a = b;
		ASSERT_TRUE(a[2]);
	}

#pragma endregion

#pragma region Equals_Operator_==

	TEST(Bitset, EqualsOperatorSameSizeBitset) {
		const size_t bitflags = 255;
		gk::bitset<64> b1 = bitflags;
		gk::bitset<64> b2 = bitflags;
		ASSERT_TRUE(b1 == b2);
	}

	TEST(Bitset, EqualsOperatorOtherSizeBitset) {
		const size_t bitflags = 255;
		gk::bitset<64> b1 = bitflags;
		gk::bitset<8> b2 = bitflags;
		ASSERT_TRUE(b1 == b2);
	}

	TEST(Bitset, EqualsOperatorFalseDifferentSize) {
		gk::bitset<64> b1 = 511;
		gk::bitset<8> b2 = 255;
		ASSERT_FALSE(b1 == b2);
	}

#pragma endregion

	TEST(Bitset, MemcpyTo32BitIntegers) {
		std::array<unsigned long long, 2> flags = { 1, 3 };
		gk::bitset<128> b = flags;
		unsigned int nums[4];
		memcpy(nums, b.bitsArray, 16);
		EXPECT_EQ(nums[0], 1);
		EXPECT_EQ(nums[1], 0);
		EXPECT_EQ(nums[2], 3);
		EXPECT_EQ(nums[3], 0);
	}

	TEST(Bitset, CopyTo32BitBufferTiny) {
		gk::bitset<8> b = 4;
		unsigned int nums[1];
		b.CopyTo32BitBuffer<1>(nums);
		ASSERT_EQ(nums[0], 4);
	}

	TEST(Bitset, CopyTo32BitBufferTinyWithLargerBuffer) {
		gk::bitset<8> b = 4;
		unsigned int nums[8];
		b.CopyTo32BitBuffer<8>(nums);
		ASSERT_EQ(nums[0], 4);
	}

	TEST(Bitset, CopyTo32BitBuffer32Bitset) {
		gk::bitset<32> b = 4;
		unsigned int nums[1];
		b.CopyTo32BitBuffer<1>(nums);
		ASSERT_EQ(nums[0], 4);
	}

	TEST(Bitset, CopyTo32BitBuffer33Bitset) {
		gk::bitset<33> b = 4;
		unsigned int nums[1];
		b.CopyTo32BitBuffer<1>(nums);
		ASSERT_EQ(nums[0], 4);
	}

	TEST(Bitset, CopyTo32BitBuffer33BitsetLargerBuffer) {
		gk::bitset<33> b = 4;
		unsigned int nums[4];
		b.CopyTo32BitBuffer<4>(nums);
		ASSERT_EQ(nums[0], 4);
	}

	TEST(Bitset, CopyTo32BitBuffer64Bitset) {
		gk::bitset<64> b = 0b1000000000000000000000000000000000ULL;
		unsigned int nums[4];
		b.CopyTo32BitBuffer<4>(nums);
		EXPECT_EQ(nums[0], 0);
		EXPECT_EQ(nums[1], 2);
	}

	TEST(Bitset, CopyTo32BitBuffer128Bitset) {
		gk::bitset<128> b = std::array<unsigned long long, 2>{1, 2};
		unsigned int nums[4];
		b.CopyTo32BitBuffer<4>(nums);
		EXPECT_EQ(nums[0], 1);
		EXPECT_EQ(nums[1], 0);
		EXPECT_EQ(nums[2], 2);
		EXPECT_EQ(nums[3], 0);
	}

	TEST(Bitset, CopyTo32BitBuffer128BitsetSmallerBuffer) {
		gk::bitset<128> b = std::array<unsigned long long, 2>{1, 2};
		unsigned int nums[3];
		b.CopyTo32BitBuffer<3>(nums);
		EXPECT_EQ(nums[0], 1);
		EXPECT_EQ(nums[1], 0);
		EXPECT_EQ(nums[2], 2);
	}

	TEST(Bitset, CopyTo32BitBuffer128BitsetLargerBuffer) {
		gk::bitset<128> b = std::array<unsigned long long, 2>{1, 2};
		unsigned int nums[6] = { 55, 55, 55, 55, 55, 57 };
		b.CopyTo32BitBuffer<6>(nums);
		EXPECT_EQ(nums[0], 1);
		EXPECT_EQ(nums[1], 0);
		EXPECT_EQ(nums[2], 2);
		EXPECT_EQ(nums[3], 0);
		EXPECT_EQ(nums[4], 55);
		EXPECT_EQ(nums[5], 57);
	}
}