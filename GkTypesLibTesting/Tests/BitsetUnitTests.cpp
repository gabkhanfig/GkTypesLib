#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Bitset/Bitset.h"

namespace UnitTests {
#pragma region Class

	static_assert(sizeof(gk::bitset<1>) == 1, "Tiny bitset does not occupy only 1 byte");
	static_assert(sizeof(gk::bitset<64>) == 8, "Large bitset does not occupy 8 bytes");

#pragma endregion

#pragma region Default_Construct

	TEST(Bitset, DefaultConstruct) {
		gk::bitset<64> b;
		ASSERT_EQ(b.bits, 0);
	}

#pragma endregion

#pragma region Flag_Construct

	TEST(Bitset, FlagsConstruct) {
		gk::bitset<64> b = 1;
		ASSERT_EQ(b.bits, 1);
		ASSERT_TRUE(b.bits & 1 == 1);
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
		//return !(bool)(b.bits);
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
}