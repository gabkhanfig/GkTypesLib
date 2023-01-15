#include <GkTypes/Bitset/Bitset.h>
#include "ConstexprTestUnitTest.h"

#if RUN_TESTS == true
#define test(test, message) \
PRAGMA_MESSAGE([Bitset Unit Test]: test); \
static_assert(test(), "[Bitset Unit Test]: " #test "... " message)
#else
#define test(test, message)
#endif

namespace BitsetUnitTests
{

#pragma region Class

	static_assert(sizeof(gk::bitset<1>) == 1, "Tiny bitset does not occupy only 1 byte");
	static_assert(sizeof(gk::bitset<64>) == 8, "Large bitset does not occupy 8 bytes");

#pragma endregion

#pragma region Default_Construct

	constexpr bool DefaultConstruct() {
		gk::bitset<64> b;
		return b.bits == 0;
	}
	test(DefaultConstruct, "Bitset default constructor does not set all flags to 0");

#pragma endregion

#pragma region Flag_Construct

	constexpr bool FlagsConstruct() {
		gk::bitset<64> b = 1;
		return b.bits & 1;
	}
	test(FlagsConstruct, "Bitset flag constructor does not initialize flags");

#pragma endregion

#pragma region Copy_Construct

	constexpr bool CopyConstructSameSize() {
		gk::bitset<64> a = 2;
		gk::bitset<64> b = a;
		return b.bits & 2;
	}
	test(CopyConstructSameSize, "Copy constructor of same sized bitset does not copy flags properly");

	constexpr bool CopyConstructDifferentSize() {
		gk::bitset<1> a = 1;
		gk::bitset<64> b = a;
		return b.bits & 1;
	}
	test(CopyConstructDifferentSize, "Copy constructor of different sized bitset does not copy flags properly");

#pragma endregion

#pragma region Get_Flag

	constexpr bool GetBit() {
		const size_t bitflag = 1 << 7;
		gk::bitset<64> bit = bitflag;
		return bit.GetBit(7);
	}
	test(GetBit, "Getting a bitflag does not get the correct flag");

	constexpr bool GetBitOperator() {
		const size_t bitflag = 1 << 20;
		gk::bitset<64> bit = bitflag;
		return bit[20];
	}
	test(GetBitOperator, "Getting a bitflag from operator[] does not get the correct flag");

#pragma endregion

#pragma region Set_Flag

	constexpr bool SetBit() {
		gk::bitset<64> b;
		b.SetBit(1);
		return b[1];
	}
	test(SetBit, "Setting specific bitflag does not set the correct one");

	constexpr bool SetBitOutOfRange() {
		gk::bitset<8> b;
		b.SetBit(8);
		return !(bool)(b.bits);
	}
	test(SetBitOutOfRange, "Setting specific bitflag out of range sets bit in range");

#pragma endregion

#pragma region Copy_Operator_=

	constexpr bool CopyOperatorOtherNums() {
		gk::bitset<64> b;
		b = 1 << 6;
		return b[6];
	}
	test(CopyOperatorOtherNums, "Copy operator does not copy bits correctly");

	constexpr bool CopyOperatorSameSizeBitset() {
		gk::bitset<64> a;
		gk::bitset<64> b = 4;
		a = b;
		return a[2];
	}
	test(CopyOperatorSameSizeBitset, "Copy operator on same sized bitset does not copy bits correctly");

	constexpr bool CopyOperatorOtherSizeBitset() {
		gk::bitset<64> a;
		gk::bitset<8> b = 4;
		a = b;
		return a[2];
	}
	test(CopyOperatorSameSizeBitset, "Copy operator on different sized bitset does not copy bits correctly");

#pragma endregion

#pragma region Equals_Operator_==

	constexpr bool EqualsOperatorSameSizeBitset() {
		const size_t bitflags = 255;
		gk::bitset<64> b1 = bitflags;
		gk::bitset<64> b2 = bitflags;
		return b1 == b2;
	}
	test(EqualsOperatorSameSizeBitset, "Equivalency check of same sized bitsets failing");

	constexpr bool EqualsOperatorOtherSizeBitset() {
		const size_t bitflags = 255;
		gk::bitset<64> b1 = bitflags;
		gk::bitset<8> b2 = bitflags;
		return b1 == b2;
	}
	test(EqualsOperatorOtherSizeBitset, "Equivalency check of different sized bitsets failing");

	constexpr bool EqualsOperatorFalseDifferentSize() {
		gk::bitset<64> b1 = 511;
		gk::bitset<8> b2 = 255;
		return b1 != b2;
	}
	test(EqualsOperatorFalseDifferentSize, "False equivalency check passing for bitsets of different sizes with the same bits in the smaller range, but different bits outside of range");

#pragma endregion

}