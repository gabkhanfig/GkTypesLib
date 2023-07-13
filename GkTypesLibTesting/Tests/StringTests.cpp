#include "../pch.h"
#include "../../GkTypesLib/GkTypes/String/String.h"
#include "../GkTest.h"

struct StringTestExample {
	double a;
	double b;
};

template<>
[[nodiscard]] static gk::String gk::String::From<StringTestExample>(const StringTestExample& value) {
	return gk::String::FromFloat(value.a) + gk::String(", ") + gk::String::FromFloat(value.b);
}

namespace UnitTests {

#pragma region Construct_Destruct

	TEST(String, DefaultConstructor) {
		gk::String str;
		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());
	}

	TEST(String, ConstructOneCharacter) {
		gk::String str = 'c';
		EXPECT_EQ(str.Len(), 1);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'c');
		EXPECT_EQ(str.CStr()[1], '\0');
	}

	TEST(String, ConstructConstCharSmall) {
		gk::String str = "abcdefg";
		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, ConstructConstCharSmallOneOffLong) {
		gk::String str = "012345678901234";
		EXPECT_EQ(str.Len(), 15);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], '0');
		EXPECT_EQ(str.CStr()[15], '\0'); // Ensure last byte is the null terminator
	}

	TEST(String, ConstructConstSegmentSmall) {
		const char* chars = "abcdefg";
		EXPECT_TRUE(gk::IsDataInConstSegment(chars)); // This might fail in test explorer. Should work when running application.
		gk::String str = chars;
		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, ConstructConstSegmentLong) {
		const char* chars = "a1234567890123456789012345678901";
		EXPECT_TRUE(gk::IsDataInConstSegment(chars)); // This might fail in test explorer. Should work when running application.
		gk::String str = chars;
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
	}

	TEST(String, ConstructLongHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
		for (uint64 i = 33; i < 64; i++) {
			EXPECT_EQ(str.CStr()[i], '\0');
		}
	}

	TEST(String, ConstructVeryLongHeap) {
		const char* chars = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		char buffer[121];
		memcpy(buffer, chars, 121);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 120);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		for (uint64 i = 0; i < 120; i += 10) {
			EXPECT_EQ(str.CStr()[i], '0');
			EXPECT_EQ(str.CStr()[i + 1], '1');
			EXPECT_EQ(str.CStr()[i + 2], '2');
			EXPECT_EQ(str.CStr()[i + 3], '3');
			EXPECT_EQ(str.CStr()[i + 4], '4');
			EXPECT_EQ(str.CStr()[i + 5], '5');
			EXPECT_EQ(str.CStr()[i + 6], '6');
			EXPECT_EQ(str.CStr()[i + 7], '7');
			EXPECT_EQ(str.CStr()[i + 8], '8');
			EXPECT_EQ(str.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str.CStr()[120], '\0');
		EXPECT_EQ(str.CStr()[121], '\0');
		EXPECT_EQ(str.CStr()[122], '\0');
		EXPECT_EQ(str.CStr()[123], '\0');
		EXPECT_EQ(str.CStr()[124], '\0');
		EXPECT_EQ(str.CStr()[125], '\0');
		EXPECT_EQ(str.CStr()[126], '\0');
		EXPECT_EQ(str.CStr()[127], '\0');
	}

	TEST(String, CopyConstructDefault) {
		gk::String defaultStr;
		gk::String str = defaultStr;

		EXPECT_EQ(defaultStr.Len(), 0);
		EXPECT_TRUE(defaultStr.IsSmallString());

		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());
	}

	TEST(String, CopyConstructOneCharacter) {
		gk::String charStr = 'c';
		gk::String str = charStr;

		EXPECT_EQ(charStr.Len(), 1);
		EXPECT_TRUE(charStr.IsSmallString());
		EXPECT_EQ(charStr.CStr()[0], 'c');
		EXPECT_EQ(charStr.CStr()[1], '\0');

		EXPECT_EQ(str.Len(), 1);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'c');
		EXPECT_EQ(str.CStr()[1], '\0');
	}

	TEST(String, CopyConstructConstCharSmall) {
		gk::String smallStr = "abcdefg";
		gk::String str = smallStr;

		EXPECT_EQ(smallStr.Len(), 7);
		EXPECT_TRUE(smallStr.IsSmallString());
		EXPECT_EQ(smallStr.CStr()[0], 'a');
		EXPECT_EQ(smallStr.CStr()[1], 'b');
		EXPECT_EQ(smallStr.CStr()[2], 'c');
		EXPECT_EQ(smallStr.CStr()[3], 'd');
		EXPECT_EQ(smallStr.CStr()[4], 'e');
		EXPECT_EQ(smallStr.CStr()[5], 'f');
		EXPECT_EQ(smallStr.CStr()[6], 'g');
		EXPECT_EQ(smallStr.CStr()[7], '\0');

		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, CopyConstructConstCharSmallOneOffLong) {
		gk::String oneOffLong = "012345678901234";
		gk::String str = oneOffLong;

		EXPECT_EQ(oneOffLong.Len(), 15);
		EXPECT_TRUE(oneOffLong.IsSmallString());
		EXPECT_EQ(oneOffLong.CStr()[0], '0');
		EXPECT_EQ(oneOffLong.CStr()[15], '\0'); // Ensure last byte is the null terminator

		EXPECT_EQ(str.Len(), 15);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], '0');
		EXPECT_EQ(str.CStr()[15], '\0'); // Ensure last byte is the null terminator
	}

	TEST(String, CopyConstructLongHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String heapStr = buffer;
		gk::String str = heapStr;

		EXPECT_EQ(heapStr.Len(), 32);
		EXPECT_TRUE(heapStr.IsHeapString());
		EXPECT_NE(heapStr.CStr(), chars);
		EXPECT_EQ(heapStr.CStr()[0], 'a');
		EXPECT_EQ(heapStr.CStr()[31], '1');
		EXPECT_EQ(heapStr.CStr()[32], '\0');
		for (uint64 i = 33; i < 64; i++) {
			EXPECT_EQ(heapStr.CStr()[i], '\0');
		}

		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
		for (uint64 i = 33; i < 64; i++) {
			EXPECT_EQ(str.CStr()[i], '\0');
		}
	}

	TEST(String, CopyConstructVeryLongHeap) {
		const char* chars = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		char buffer[121];
		memcpy(buffer, chars, 121);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String heapStr = buffer;
		gk::String str = heapStr;

		EXPECT_EQ(heapStr.Len(), 120);
		EXPECT_TRUE(heapStr.IsHeapString());
		EXPECT_NE(heapStr.CStr(), chars);
		for (uint64 i = 0; i < 120; i += 10) {
			EXPECT_EQ(heapStr.CStr()[i], '0');
			EXPECT_EQ(heapStr.CStr()[i + 1], '1');
			EXPECT_EQ(heapStr.CStr()[i + 2], '2');
			EXPECT_EQ(heapStr.CStr()[i + 3], '3');
			EXPECT_EQ(heapStr.CStr()[i + 4], '4');
			EXPECT_EQ(heapStr.CStr()[i + 5], '5');
			EXPECT_EQ(heapStr.CStr()[i + 6], '6');
			EXPECT_EQ(heapStr.CStr()[i + 7], '7');
			EXPECT_EQ(heapStr.CStr()[i + 8], '8');
			EXPECT_EQ(heapStr.CStr()[i + 9], '9');
		}
		EXPECT_EQ(heapStr.CStr()[120], '\0');
		EXPECT_EQ(heapStr.CStr()[121], '\0');
		EXPECT_EQ(heapStr.CStr()[122], '\0');
		EXPECT_EQ(heapStr.CStr()[123], '\0');
		EXPECT_EQ(heapStr.CStr()[124], '\0');
		EXPECT_EQ(heapStr.CStr()[125], '\0');
		EXPECT_EQ(heapStr.CStr()[126], '\0');
		EXPECT_EQ(heapStr.CStr()[127], '\0');

		EXPECT_EQ(str.Len(), 120);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		for (uint64 i = 0; i < 120; i += 10) {
			EXPECT_EQ(str.CStr()[i], '0');
			EXPECT_EQ(str.CStr()[i + 1], '1');
			EXPECT_EQ(str.CStr()[i + 2], '2');
			EXPECT_EQ(str.CStr()[i + 3], '3');
			EXPECT_EQ(str.CStr()[i + 4], '4');
			EXPECT_EQ(str.CStr()[i + 5], '5');
			EXPECT_EQ(str.CStr()[i + 6], '6');
			EXPECT_EQ(str.CStr()[i + 7], '7');
			EXPECT_EQ(str.CStr()[i + 8], '8');
			EXPECT_EQ(str.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str.CStr()[120], '\0');
		EXPECT_EQ(str.CStr()[121], '\0');
		EXPECT_EQ(str.CStr()[122], '\0');
		EXPECT_EQ(str.CStr()[123], '\0');
		EXPECT_EQ(str.CStr()[124], '\0');
		EXPECT_EQ(str.CStr()[125], '\0');
		EXPECT_EQ(str.CStr()[126], '\0');
		EXPECT_EQ(str.CStr()[127], '\0');
	}

	TEST(String, MoveConstructDefault) {
		gk::String defaultStr;
		gk::String str = std::move(defaultStr);
		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());
	}

	TEST(String, MoveConstructOneCharacter) {
		gk::String toMove = 'c';
		gk::String str = std::move('c');
		EXPECT_EQ(str.Len(), 1);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'c');
		EXPECT_EQ(str.CStr()[1], '\0');
	}

	TEST(String, MoveConstructConstCharSmall) {
		gk::String toMove = "abcdefg";
		gk::String str = std::move(toMove);
		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, MoveConstructConstCharSmallOneOffLong) {
		gk::String toMove = "012345678901234";
		gk::String str = std::move(toMove);
		EXPECT_EQ(str.Len(), 15);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], '0');
		EXPECT_EQ(str.CStr()[15], '\0'); // Ensure last byte is the null terminator
	}

	TEST(String, MoveConstructConstSegmentSmall) {
		const char* chars = "abcdefg";
		EXPECT_TRUE(gk::IsDataInConstSegment(chars)); // This might fail in test explorer. Should work when running application.
		gk::String toMove = chars;
		gk::String str = std::move(toMove);
		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, MoveConstructConstSegmentLong) {
		const char* chars = "a1234567890123456789012345678901";
		EXPECT_TRUE(gk::IsDataInConstSegment(chars)); // This might fail in test explorer. Should work when running application.
		gk::String toMove = chars;
		gk::String str = std::move(toMove);
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
	}

	TEST(String, MoveConstructLongHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String toMove = buffer;
		gk::String str = std::move(toMove);

		EXPECT_EQ(toMove.CStr(), nullptr);

		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
		for (uint64 i = 33; i < 64; i++) {
			EXPECT_EQ(str.CStr()[i], '\0');
		}
	}

	TEST(String, MoveConstructVeryLongHeap) {
		const char* chars = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		char buffer[121];
		memcpy(buffer, chars, 121);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String toMove = buffer;
		gk::String str = std::move(toMove);

		EXPECT_EQ(toMove.CStr(), nullptr);

		EXPECT_EQ(str.Len(), 120);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		for (uint64 i = 0; i < 120; i += 10) {
			EXPECT_EQ(str.CStr()[i], '0');
			EXPECT_EQ(str.CStr()[i + 1], '1');
			EXPECT_EQ(str.CStr()[i + 2], '2');
			EXPECT_EQ(str.CStr()[i + 3], '3');
			EXPECT_EQ(str.CStr()[i + 4], '4');
			EXPECT_EQ(str.CStr()[i + 5], '5');
			EXPECT_EQ(str.CStr()[i + 6], '6');
			EXPECT_EQ(str.CStr()[i + 7], '7');
			EXPECT_EQ(str.CStr()[i + 8], '8');
			EXPECT_EQ(str.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str.CStr()[120], '\0');
		EXPECT_EQ(str.CStr()[121], '\0');
		EXPECT_EQ(str.CStr()[122], '\0');
		EXPECT_EQ(str.CStr()[123], '\0');
		EXPECT_EQ(str.CStr()[124], '\0');
		EXPECT_EQ(str.CStr()[125], '\0');
		EXPECT_EQ(str.CStr()[126], '\0');
		EXPECT_EQ(str.CStr()[127], '\0');
	}

	TEST(String, RangeConstructSmallNullTerminated) {
		const char* chars = "abcdefg";
		gk::String str = gk::String(chars, chars + 7);

		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, RangeConstructSmallNotNullTerminated) {
		const char* chars = "abcdefg";
		gk::String str = gk::String(chars, chars + 6);

		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, RangeConstructSmallStartOffsetNullTerminated) {
		const char* chars = "abcdefg";
		gk::String str = gk::String(chars + 1, chars + 7);

		EXPECT_EQ(str.Len(), 6);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'b');
		EXPECT_EQ(str.CStr()[1], 'c');
		EXPECT_EQ(str.CStr()[2], 'd');
		EXPECT_EQ(str.CStr()[3], 'e');
		EXPECT_EQ(str.CStr()[4], 'f');
		EXPECT_EQ(str.CStr()[5], 'g');
		EXPECT_EQ(str.CStr()[6], '\0');
	}

	TEST(String, RangeConstructSmallStartOffsetNotNullTerminated) {
		const char* chars = "abcdefg";
		gk::String str = gk::String(chars + 1, chars + 6);

		EXPECT_EQ(str.Len(), 6);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'b');
		EXPECT_EQ(str.CStr()[1], 'c');
		EXPECT_EQ(str.CStr()[2], 'd');
		EXPECT_EQ(str.CStr()[3], 'e');
		EXPECT_EQ(str.CStr()[4], 'f');
		EXPECT_EQ(str.CStr()[5], 'g');
		EXPECT_EQ(str.CStr()[6], '\0');
	}

	TEST(String, RangeConstructConstSegmentNullTerminated) {
		const char* chars = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		gk::String str = gk::String(chars + 10, chars + 120);

		EXPECT_TRUE(gk::IsDataInConstSegment(chars));
		EXPECT_TRUE(gk::IsDataInConstSegment(str.CStr()));

		EXPECT_EQ(str.Len(), 110);
		EXPECT_EQ(str.CStr()[0], '0');
		for (uint64 i = 0; i < 110; i += 10) {
			EXPECT_EQ(str.CStr()[i], '0');
			EXPECT_EQ(str.CStr()[i + 1], '1');
			EXPECT_EQ(str.CStr()[i + 2], '2');
			EXPECT_EQ(str.CStr()[i + 3], '3');
			EXPECT_EQ(str.CStr()[i + 4], '4');
			EXPECT_EQ(str.CStr()[i + 5], '5');
			EXPECT_EQ(str.CStr()[i + 6], '6');
			EXPECT_EQ(str.CStr()[i + 7], '7');
			EXPECT_EQ(str.CStr()[i + 8], '8');
			EXPECT_EQ(str.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str.CStr()[110], '\0');
	}

	TEST(String, RangeConstructConstSegmentIsHeapDueToNotNullTerminated) {
		const char* chars = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		gk::String str = gk::String(chars + 10, chars + 119);

		EXPECT_TRUE(gk::IsDataInConstSegment(chars));
		EXPECT_FALSE(gk::IsDataInConstSegment(str.CStr()));

		EXPECT_EQ(str.Len(), 110);
		EXPECT_EQ(str.CStr()[0], '0');
		for (uint64 i = 0; i < 100; i += 10) {
			EXPECT_EQ(str.CStr()[i], '0');
			EXPECT_EQ(str.CStr()[i + 1], '1');
			EXPECT_EQ(str.CStr()[i + 2], '2');
			EXPECT_EQ(str.CStr()[i + 3], '3');
			EXPECT_EQ(str.CStr()[i + 4], '4');
			EXPECT_EQ(str.CStr()[i + 5], '5');
			EXPECT_EQ(str.CStr()[i + 6], '6');
			EXPECT_EQ(str.CStr()[i + 7], '7');
			EXPECT_EQ(str.CStr()[i + 8], '8');
			EXPECT_EQ(str.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str.CStr()[100], '0');
		EXPECT_EQ(str.CStr()[101], '1');
		EXPECT_EQ(str.CStr()[102], '2');
		EXPECT_EQ(str.CStr()[103], '3');
		EXPECT_EQ(str.CStr()[104], '4');
		EXPECT_EQ(str.CStr()[105], '5');
		EXPECT_EQ(str.CStr()[106], '6');
		EXPECT_EQ(str.CStr()[107], '7');
		EXPECT_EQ(str.CStr()[108], '8');
		EXPECT_EQ(str.CStr()[109], '9');
		EXPECT_EQ(str.CStr()[110], '\0');
	}

	TEST(String, NoMemoryLeakHeapString) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String str = buffer;
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_FALSE(gk::IsDataInConstSegment(str.CStr()));
	}

#pragma endregion

#pragma region Copy_Move_=

	TEST(String, SetEqualSingleCharacter) {
		char c = 'a';
		gk::String str;

		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());

		str = c;

		EXPECT_EQ(str.Len(), 1);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
	}

	TEST(String, SetEqualSingleCharacterDeleteHeap) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		EXPECT_FALSE(gk::IsDataInConstSegment(buffer));
		gk::String str = buffer;

		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
		for (uint64 i = 33; i < 64; i++) {
			EXPECT_EQ(str.CStr()[i], '\0');
		}

		str = 'a';
		EXPECT_EQ(str.Len(), 1);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		leakDetector.CheckLeak();
	}

	TEST(String, SetEqualConstCharSmall) {
		const char* chars = "abcdefg";
		gk::String str;
		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());

		str = chars;
		EXPECT_EQ(str.Len(), 7);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], 'd');
		EXPECT_EQ(str.CStr()[4], 'e');
		EXPECT_EQ(str.CStr()[5], 'f');
		EXPECT_EQ(str.CStr()[6], 'g');
		EXPECT_EQ(str.CStr()[7], '\0');
	}

	TEST(String, SetEqualConstCharConstSegmentLong) {
		const char* chars = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		gk::String str;
		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());

		str = chars;
		EXPECT_EQ(str.Len(), 120);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str.CStr(), chars);
		for (uint64 i = 0; i < 120; i += 10) {
			EXPECT_EQ(str.CStr()[i], '0');
			EXPECT_EQ(str.CStr()[i + 1], '1');
			EXPECT_EQ(str.CStr()[i + 2], '2');
			EXPECT_EQ(str.CStr()[i + 3], '3');
			EXPECT_EQ(str.CStr()[i + 4], '4');
			EXPECT_EQ(str.CStr()[i + 5], '5');
			EXPECT_EQ(str.CStr()[i + 6], '6');
			EXPECT_EQ(str.CStr()[i + 7], '7');
			EXPECT_EQ(str.CStr()[i + 8], '8');
			EXPECT_EQ(str.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str.CStr()[120], '\0');
	}


	TEST(String, SetEqualConstCharNotConstSegmentLong) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str; 
		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());

		str = buffer; 
		
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_NE(str.CStr(), chars);
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');
		for (uint64 i = 33; i < 64; i++) {
			EXPECT_EQ(str.CStr()[i], '\0');
		}
	}

	TEST(String, SetEqualCopySmall) {
		const gk::String str1 = "abcdefg";
		gk::String str2;

		EXPECT_EQ(str2.Len(), 0);
		EXPECT_TRUE(str2.IsSmallString());

		str2 = str1;

		EXPECT_EQ(str1.Len(), 7);
		EXPECT_TRUE(str1.IsSmallString());
		EXPECT_EQ(str1.CStr()[0], 'a');
		EXPECT_EQ(str1.CStr()[1], 'b');
		EXPECT_EQ(str1.CStr()[2], 'c');
		EXPECT_EQ(str1.CStr()[3], 'd');
		EXPECT_EQ(str1.CStr()[4], 'e');
		EXPECT_EQ(str1.CStr()[5], 'f');
		EXPECT_EQ(str1.CStr()[6], 'g');
		EXPECT_EQ(str1.CStr()[7], '\0');

		EXPECT_EQ(str2.Len(), 7);
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[1], 'b');
		EXPECT_EQ(str2.CStr()[2], 'c');
		EXPECT_EQ(str2.CStr()[3], 'd');
		EXPECT_EQ(str2.CStr()[4], 'e');
		EXPECT_EQ(str2.CStr()[5], 'f');
		EXPECT_EQ(str2.CStr()[6], 'g');
		EXPECT_EQ(str2.CStr()[7], '\0');
	}

	TEST(String, SetEqualCopyConstSegment) {
		const gk::String str1 = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		gk::String str2;

		EXPECT_EQ(str2.Len(), 0);
		EXPECT_TRUE(str2.IsSmallString());

		str2 = str1;

		EXPECT_EQ(str2.Len(), 120);
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_EQ(str2.CStr(), str1.CStr());
		for (uint64 i = 0; i < 120; i += 10) {
			EXPECT_EQ(str2.CStr()[i], '0');
			EXPECT_EQ(str2.CStr()[i + 1], '1');
			EXPECT_EQ(str2.CStr()[i + 2], '2');
			EXPECT_EQ(str2.CStr()[i + 3], '3');
			EXPECT_EQ(str2.CStr()[i + 4], '4');
			EXPECT_EQ(str2.CStr()[i + 5], '5');
			EXPECT_EQ(str2.CStr()[i + 6], '6');
			EXPECT_EQ(str2.CStr()[i + 7], '7');
			EXPECT_EQ(str2.CStr()[i + 8], '8');
			EXPECT_EQ(str2.CStr()[i + 9], '9');
		}
		EXPECT_EQ(str2.CStr()[120], '\0');
	}

	TEST(String, SetEqualCopyHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		const gk::String str1 = buffer;
		gk::String str2;

		EXPECT_EQ(str2.Len(), 0);
		EXPECT_TRUE(str2.IsSmallString());

		str2 = str1; 
		EXPECT_NE(str1.CStr(), str2.CStr());
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_NE(str2.CStr(), chars);
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');

		for (uint64 i = 0; i < 64; i++) {
			EXPECT_EQ(str1.CStr()[i], str2.CStr()[i]);
		}
	}

	TEST(String, SetEqualCopyHeapNoMemoryLeak) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		const gk::String str1 = buffer;
		gk::String str2;

		str2 = str1;
	}

	TEST(String, SetEqualMoveSmall) {
		gk::String str1 = "abc";
		gk::String str2;

		EXPECT_EQ(str2.Len(), 0);
		EXPECT_TRUE(str2.IsSmallString());

		str2 = std::move(str1);

		EXPECT_EQ(str2.Len(), 3);
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[1], 'b');
		EXPECT_EQ(str2.CStr()[2], 'c');
		EXPECT_EQ(str2.CStr()[3], '\0');
	}

	TEST(String, SetEqualMoveConstSegmentLong) {
		gk::String str1 = "a1234567890123456789012345678901";
		gk::String str2;

		EXPECT_EQ(str2.Len(), 0);
		EXPECT_TRUE(str2.IsSmallString());

		str2 = std::move(str1);
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');
	}

	TEST(String, SetEqualMoveHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = buffer;
		gk::String str2;

		EXPECT_EQ(str2.Len(), 0);
		EXPECT_TRUE(str2.IsSmallString());

		str2 = std::move(str1);
		EXPECT_FALSE(gk::IsDataInConstSegment(str2.CStr()));
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');
	}

	TEST(String, SetEqualMoveSmallFromHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = "abc";
		gk::String str2 = buffer;

		EXPECT_FALSE(gk::IsDataInConstSegment(str2.CStr()));
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');

		str2 = std::move(str1);

		EXPECT_EQ(str2.Len(), 3);
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[1], 'b');
		EXPECT_EQ(str2.CStr()[2], 'c');
		EXPECT_EQ(str2.CStr()[3], '\0');
	}

	TEST(String, SetEqualMoveSmallFromHeapNoMemoryLeak) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = "abc";
		gk::String str2 = buffer;

		str2 = std::move(str1);
	}

	TEST(String, SetEqualMoveConstSegmentFromHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = chars;
		gk::String str2 = buffer;

		EXPECT_FALSE(gk::IsDataInConstSegment(str2.CStr()));
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');

		str2 = std::move(str1);
		EXPECT_TRUE(gk::IsDataInConstSegment(str2.CStr()));
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');
	}

	TEST(String, SetEqualMoveConstSegmentFromHeapNoMemoryLeak) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = chars;
		gk::String str2 = buffer;

		str2 = std::move(str1);
	}

	TEST(String, SetEqualMoveHeapFromHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = buffer;
		gk::String str2 = buffer;

		EXPECT_NE(str1.CStr(), str2.CStr());

		EXPECT_FALSE(gk::IsDataInConstSegment(str2.CStr()));
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');

		str2 = std::move(str1);
		EXPECT_FALSE(gk::IsDataInConstSegment(str2.CStr()));
		EXPECT_EQ(str2.Len(), 32);
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str2.CStr()[0], 'a');
		EXPECT_EQ(str2.CStr()[31], '1');
		EXPECT_EQ(str2.CStr()[32], '\0');
	}

	TEST(String, SetEqualMoveHeapFromHeapNoMemoryLeak) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str1 = buffer;
		gk::String str2 = buffer;

		str2 = std::move(str1);
	}

	TEST(String, CopySmallStringDoesntUseSameCstr) {
		gk::String str1 = "abc";
		gk::String str2;

		EXPECT_TRUE(str1.IsSmallString());
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_EQ(str1.Len(), 3);
		EXPECT_EQ(str2.Len(), 0);

		str2 = str1;
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_EQ(str2.Len(), str1.Len());
		EXPECT_NE(str1.CStr(), str2.CStr());
	}

#pragma endregion

#pragma region Empty

	TEST(String, EmptySmall) {
		gk::String str = "abc";

		EXPECT_EQ(str.Len(), 3);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[1], 'b');
		EXPECT_EQ(str.CStr()[2], 'c');
		EXPECT_EQ(str.CStr()[3], '\0');

		str.Empty();

		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());
	}

	TEST(String, EmptyHeap) {
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str = buffer;

		EXPECT_FALSE(gk::IsDataInConstSegment(str.CStr()));
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');

		str.Empty();

		EXPECT_EQ(str.Len(), 0);
		EXPECT_TRUE(str.IsSmallString());
	}

	TEST(String, EmptyHeapNoMemoryLeak) {
		MemoryLeakDetector leakDetector;

		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);
		gk::String str = buffer;

		str.Empty();
	}

#pragma endregion

#pragma region Equal

	TEST(String, EqualChar) {
		gk::String str = 'a';
		EXPECT_EQ(str, 'a');
	}

	TEST(String, NotEqualChar) {
		gk::String str = 'a';
		EXPECT_NE(str, 'b');
	}

	TEST(String, NotEqualCharLengthGreaterThan1) {
		gk::String str = "bb";
		EXPECT_NE(str, 'b');
	}

	TEST(String, EqualConstCharSmall) {
		gk::String str = "abcdefg";
		EXPECT_EQ(str, "abcdefg");
	}

	TEST(String, EqualConstCharLength1) {
		gk::String str = 'a';
		const char* asChars = "a";
		EXPECT_EQ(str, asChars);
	}

	TEST(String, EqualConstCharConstSegment) {
		const char* chars = "a1234567890123456789012345678901";
		gk::String str = chars;
		EXPECT_EQ(str, chars);
	}

	TEST(String, EqualConstCharConstSegmentSanity) { // Just in case they both use the same string, but in different const segment locations.
		gk::String str = "a1234567890123456789012345678901";
		const char* chars = "a1234567890123456789012345678901";

		EXPECT_TRUE(gk::IsDataInConstSegment(str.CStr()));
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str.CStr()[0], 'a');
		EXPECT_EQ(str.CStr()[31], '1');
		EXPECT_EQ(str.CStr()[32], '\0');

		EXPECT_EQ(str, chars);
	}

	TEST(String, NotEqualConstCharConstSegment) {
		gk::String str = "a1234567890123456789012345678901";
		const char* chars = "a123456789012345a789012345678901";

		EXPECT_NE(str, chars);
	}

	TEST(String, NotEqualConstCharDifferentLength) {
		gk::String str = "a123456789012345678901234567890";
		const char* chars = "a1234567890123456789012345678901";

		EXPECT_NE(str, chars);
	}

	TEST(String, NotEqualConstCharDifferentLengthAlt) {
		gk::String str = "a1234567890123456789012345678901";
		const char* chars = "a123456789012345678901234567890";

		EXPECT_NE(str, chars);
	}


	TEST(String, EqualConstCharConstSegmentAndHeap) {
		gk::String str = "a1234567890123456789012345678901";
		const char* chars = "a1234567890123456789012345678901";
		char buffer[33];
		memcpy(buffer, chars, 33);

		EXPECT_EQ(str, buffer);
	}
	
	TEST(String, EqualSmallAndHeap) {
		gk::String str = "a12345678901234567890";
		const char* chars = "a12345678901234567890";
		char buffer[33];
		memcpy(buffer, chars, 22);

		EXPECT_EQ(str, buffer);
	}

	TEST(String, NotEqualConstCharSegmentAndHeap) {
		gk::String str = "a1234567890123456789012345678901";
		const char* chars = "a1234567890123456789012345678902";
		char buffer[33];
		memcpy(buffer, chars, 33);

		EXPECT_NE(str, buffer);
	}

	TEST(String, NotEqualSmallAndHeap) {
		gk::String str = "a12345678901234567890";
		const char* chars = "a12345678901234567891";
		char buffer[33];
		memcpy(buffer, chars, 22);

		EXPECT_NE(str, buffer);
	}

	TEST(String, EqualSingleCharacterStrings) {
		gk::String str1 = 'z';
		gk::String str2 = 'z';
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualSingleCharacterStrings) {
		gk::String str1 = 'z';
		gk::String str2 = 'y';
		EXPECT_NE(str1, str2);
	}

	TEST(String, EqualTwoCharacterStrings) {
		gk::String str1 = "ba";
		gk::String str2 = "ba";
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualTwoCharacterStrings) {
		gk::String str1 = "ba";
		gk::String str2 = "ab";
		EXPECT_NE(str1, str2);
	}

	TEST(String, EqualFullSsoBufferStrings) {
		const char* chars = "a12345678901234";
		gk::String str1 = chars;
		gk::String str2 = chars;

		EXPECT_TRUE(str1.IsSmallString());
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualFullSsoBufferStrings) {
		gk::String str1 = "a12345678901234";
		gk::String str2 = "a12345678901o34";

		EXPECT_TRUE(str1.IsSmallString());
		EXPECT_TRUE(str2.IsSmallString());
		EXPECT_NE(str1, str2);
	}

	TEST(String, EqualStringsBothConstSegment) {
		const char* chars = "a1234567890123456789012345678901";
		gk::String str1 = chars;
		gk::String str2 = chars;

		EXPECT_TRUE(str1.IsConstSegmentString());
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualStringsBothConstSegment) {
		const char* chars = "a1234567890123456789012345678901";
		gk::String str1 = chars;
		gk::String str2 = "a1234567890123456789012345678902";

		EXPECT_TRUE(str1.IsConstSegmentString());
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_NE(str1, str2);
	}
	
	TEST(String, EqualStringsBothConstSegmentSanity) {
		gk::String str1 = "a1234567890123456789012345678901";
		gk::String str2 = "a1234567890123456789012345678901";

		EXPECT_TRUE(str1.IsConstSegmentString());
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualStringsBothConstSegmentSanity) {
		gk::String str1 = "a1234567890123456789012345678901";
		gk::String str2 = "a1234567890123456789012345678902";

		EXPECT_TRUE(str1.IsConstSegmentString());
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_NE(str1, str2);
	}

	TEST(String, EqualStringsBothHeap) {
		const char* chars = "a1234567890123456789012345678902";
		char buffer[33];
		memcpy(buffer, chars, 33);

		gk::String str1 = buffer;
		gk::String str2 = buffer;

		EXPECT_TRUE(str1.IsHeapString());
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualStringsBothHeap) {
		const char* chars = "a1234567890123456789012345678902";
		char buffer[33];
		memcpy(buffer, chars, 33);

		const char* chars2 = "a12345678901234i6789012345678902";
		char buffer2[33];
		memcpy(buffer2, chars2, 33);

		gk::String str1 = buffer;
		gk::String str2 = buffer2;

		EXPECT_TRUE(str1.IsHeapString());
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_NE(str1, str2);
	}

	TEST(String, NotEqualFullSsoBufferAndConstSegment) {
		gk::String str1 = "a12345678901234";
		gk::String str2 = "a123456789012345";

		EXPECT_TRUE(str1.IsSmallString());
		EXPECT_TRUE(str2.IsConstSegmentString());
		EXPECT_NE(str1, str2);
	}

	TEST(String, NotEqualFullSsoBufferAndHeap) {
		const char* heap = "a123456789012345";
		char buffer[17];
		memcpy(buffer, heap, 17);

		gk::String str1 = "a12345678901234";
		gk::String str2 = buffer;

		EXPECT_TRUE(str1.IsSmallString());
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_NE(str1, str2);
	}

	TEST(String, EqualConstSegmentAndHeap) {
		const char* constSeg = "a123456789012345";
		char buffer[17];
		memcpy(buffer, constSeg, 17);

		gk::String str1 = constSeg;
		gk::String str2 = buffer;

		EXPECT_TRUE(str1.IsConstSegmentString());
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_EQ(str1, str2);
	}

	TEST(String, NotEqualConstSegmentAndHeap) {
		const char* constSeg = "a123456789012345";
		const char* heap = "a1234567890123450";
		char buffer[18];
		memcpy(buffer, heap, 17);

		gk::String str1 = constSeg;
		gk::String str2 = buffer;

		EXPECT_TRUE(str1.IsConstSegmentString());
		EXPECT_TRUE(str2.IsHeapString());
		EXPECT_NE(str1, str2);
	}

#pragma endregion

#pragma region Append

	TEST(String, SsoAppendChar) {
		gk::String str = "abc";
		EXPECT_EQ(str.Len(), 3);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abc");

		str.Append('d');
		EXPECT_EQ(str.Len(), 4);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcd");
	}

	TEST(String, SsoAppendCharConvertToHeap) {
		gk::String str = "abcdefghijklmno";
		EXPECT_EQ(str.Len(), 15);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcdefghijklmno");

		str.Append('p');
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");
	}

	TEST(String, HeapAppendChar) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");

		str.Append('q');
		EXPECT_EQ(str.Len(), 17);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnopq");
	}

	TEST(String, HeapAppendCharReallocate) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz01234";
		char buffer[32];
		memcpy(buffer, chars, 32);
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 31);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz01234");

		str.Append('5');
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 64);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, ConstSegmentAppendChar) {
		gk::String str = "abcdefghijklmnop";
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str, "abcdefghijklmnop");

		str.Append('q');
		EXPECT_EQ(str.Len(), 17);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnopq");
	}

	TEST(String, ConstSegmentAppendCharLargerHeapCapacity) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz01234";
		EXPECT_EQ(str.Len(), 31);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz01234");

		str.Append('5');
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 64);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, SsoAppendConstChar) {
		gk::String str = "abc";
		EXPECT_EQ(str.Len(), 3);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abc");

		str.Append("de");
		EXPECT_EQ(str.Len(), 5);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcde");
	}

	TEST(String, SsoAppendConstCharConvertToHeap) {
		gk::String str = "abcdefghijklmn";
		EXPECT_EQ(str.Len(), 14);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcdefghijklmn");

		str.Append("op");
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");
	}

	TEST(String, HeapAppendConstChar) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");

		str.Append("qr");
		EXPECT_EQ(str.Len(), 18);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnopqr");
	}

	TEST(String, HeapAppendConstCharReallocate) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz0123";
		char buffer[31];
		memcpy(buffer, chars, 31);
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 30);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz0123");

		str.Append("45");
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 64);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, ConstSegmentAppendConstChar) {
		gk::String str = "abcdefghijklmnop";
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str, "abcdefghijklmnop");

		str.Append("qr");
		EXPECT_EQ(str.Len(), 18);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnopqr");
	}

	TEST(String, ConstSegmentAppendConstCharLargerHeapCapacity) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123";
		EXPECT_EQ(str.Len(), 30);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz0123");

		str.Append("45");
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 64);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, SsoAppendOtherString) {
		gk::String str = "abc";
		EXPECT_EQ(str.Len(), 3);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abc");

		gk::String other = "de";
		str.Append(other);
		EXPECT_EQ(str.Len(), 5);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcde");
	}

	TEST(String, SsoAppendOtherStringConvertToHeap) {
		gk::String str = "abcdefghijklmn";
		EXPECT_EQ(str.Len(), 14);
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcdefghijklmn");

		gk::String other = "op";
		str.Append(other);
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");
	}

	TEST(String, HeapAppendOtherString) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");

		gk::String other = "qr";
		str.Append(other);
		EXPECT_EQ(str.Len(), 18);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnopqr");
	}

	TEST(String, HeapAppendOtherStringReallocate) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz0123";
		char buffer[31];
		memcpy(buffer, chars, 31);
		gk::String str = buffer;
		EXPECT_EQ(str.Len(), 30);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz0123");

		gk::String other = "45";
		str.Append(other);
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 64);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, ConstSegmentAppendOtherString) {
		gk::String str = "abcdefghijklmnop";
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str, "abcdefghijklmnop");

		gk::String other = "qr";
		str.Append(other);
		EXPECT_EQ(str.Len(), 18);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnopqr");
	}

	TEST(String, ConstSegmentAppendOtherStringLargerHeapCapacity) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123";
		EXPECT_EQ(str.Len(), 30);
		EXPECT_TRUE(str.IsConstSegmentString());
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz0123");

		gk::String other = "45";
		str.Append(other);
		EXPECT_EQ(str.Len(), 32);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 64);
		EXPECT_EQ(str, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, ChainAppendCharSso) {
		gk::String str = "abc";
		str.Append('d').Append('e').Append('f');
		EXPECT_TRUE(str.IsSmallString());
		EXPECT_EQ(str, "abcdef");
		EXPECT_EQ(str.CStr()[6], '\0');
	}

	TEST(String, ChainAppendSsoToHeap) {
		gk::String str = "abcdefg";
		str.Append('h').Append('i').Append("jklmnop");
		EXPECT_EQ(str.Len(), 16);
		EXPECT_TRUE(str.IsHeapString());
		EXPECT_EQ(str, "abcdefghijklmnop");
	}

	TEST(String, ChainAppendStrings) {
		gk::String str1 = "abcdefg";
		gk::String str2 = "hijklmnop";
		gk::String str3 = "qrstuv";
		gk::String str4 = "wxyz";
		str1.Append(str2).Append(str3).Append(str4);
		EXPECT_EQ(str1.Len(), 26);
		EXPECT_TRUE(str1.IsHeapString());
		EXPECT_EQ(str1, "abcdefghijklmnopqrstuvwxyz");
	}

#pragma endregion

#pragma region Concat

	TEST(String, ConcatSsoAndChar) {
		gk::String str = "abc";
		gk::String concat = str + 'd';

		EXPECT_EQ(concat.Len(), 4);
		EXPECT_TRUE(concat.IsSmallString());
		EXPECT_EQ(concat, "abcd");
	}

	TEST(String, ConcatSsoAndCharToHeap) {
		gk::String str = "abcdefghijklmno";
		gk::String concat = str + 'p';

		EXPECT_EQ(concat.Len(), 16);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnop");
	}

	TEST(String, ConcatHeapAndChar) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;

		gk::String concat = str + 'q';

		EXPECT_EQ(concat.Len(), 17);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnopq");
	}

	TEST(String, ConcatHeapAndCharDifferentHeapCapacity) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz01234";
		char buffer[32];
		memcpy(buffer, chars, 32);
		gk::String str = buffer;

		gk::String concat = str + '5';
		EXPECT_EQ(concat.Len(), 32);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(concat.HeapCapacity(), 64);
		EXPECT_EQ(concat, "abcdefghijklmnopqrstuvwxyz012345");
	}

	TEST(String, ConcatConstSegmentAndChar) {
		gk::String str = "abcdefghijklmnop";

		gk::String concat = str + 'q';
		EXPECT_EQ(concat.Len(), 17);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnopq");
	}

	TEST(String, ConcatSsoAndConstChar) {
		gk::String str = "abc";
		gk::String concat = str + "de";

		EXPECT_EQ(concat.Len(), 5);
		EXPECT_TRUE(concat.IsSmallString());
		EXPECT_EQ(concat, "abcde");
	}

	TEST(String, ConcatSsoAndConstCharToHeap) {
		gk::String str = "abcdefghijklmn";
		gk::String concat = str + "op";

		EXPECT_EQ(concat.Len(), 16);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnop");
	}

	TEST(String, ConcatHeapAndConstChar) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;

		gk::String concat = str + "qr";

		EXPECT_EQ(concat.Len(), 18);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnopqr");
	}

	TEST(String, ConcatHeapAndConstCharDifferentHeapCapacity) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz01234";
		char buffer[32];
		memcpy(buffer, chars, 32);
		gk::String str = buffer;

		gk::String concat = str + "56";
		EXPECT_EQ(concat.Len(), 33);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(concat.HeapCapacity(), 64);
		EXPECT_EQ(concat, "abcdefghijklmnopqrstuvwxyz0123456");
	}

	TEST(String, ConcatConstSegmentAndConstChar) {
		gk::String str = "abcdefghijklmnop";

		gk::String concat = str + "qr";
		EXPECT_EQ(concat.Len(), 18);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnopqr");
	}

	TEST(String, ConcatSsoAndString) {
		gk::String str = "abc";
		gk::String other = "de";
		gk::String concat = str + other;

		EXPECT_EQ(concat.Len(), 5);
		EXPECT_TRUE(concat.IsSmallString());
		EXPECT_EQ(concat, "abcde");
	}

	TEST(String, ConcatSsoAndStringToHeap) {
		gk::String str = "abcdefghijklmn";
		gk::String other = "op";
		gk::String concat = str + other;

		EXPECT_EQ(concat.Len(), 16);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnop");
	}

	TEST(String, ConcatHeapAndString) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;
		gk::String other = "qr";
		gk::String concat = str + other;

		EXPECT_EQ(concat.Len(), 18);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnopqr");
	}

	TEST(String, ConcatHeapAndStringDifferentHeapCapacity) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz01234";
		char buffer[32];
		memcpy(buffer, chars, 32);
		gk::String str = buffer;
		gk::String other = "56";
		gk::String concat = str + other;
		EXPECT_EQ(concat.Len(), 33);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(concat.HeapCapacity(), 64);
		EXPECT_EQ(concat, "abcdefghijklmnopqrstuvwxyz0123456");
	}

	TEST(String, ConcatConstSegmentAndString) {
		gk::String str = "abcdefghijklmnop";
		gk::String other = "qr";
		gk::String concat = str + other;
		EXPECT_EQ(concat.Len(), 18);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnopqr");
	}

	TEST(String, ConcatCharAndSso) {
		gk::String str = "abc";
		gk::String concat = 'd' + str;
		EXPECT_EQ(concat.Len(), 4);
		EXPECT_TRUE(concat.IsSmallString());
		EXPECT_EQ(concat, "dabc");
	}

	TEST(String, ConcatCharAndSsoToHeap) {
		gk::String str = "bcdefghijklmnop";
		gk::String concat = 'a' + str;

		EXPECT_EQ(concat.Len(), 16);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnop");
	}

	TEST(String, ConcatCharAndHeap) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;
		gk::String concat = 's' + str;

		EXPECT_EQ(concat.Len(), 17);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "sabcdefghijklmnop");
	}

	TEST(String, ConcatCharAndHeapDifferentCapacity) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz01234";
		char buffer[32];
		memcpy(buffer, chars, 32);
		gk::String str = buffer;
		gk::String concat = 'p' + str;
		EXPECT_EQ(concat.Len(), 32);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(concat.HeapCapacity(), 64);
		EXPECT_EQ(concat, "pabcdefghijklmnopqrstuvwxyz01234");
	}

	TEST(String, ConcatCharAndConstSegment) {
		gk::String str = "abcdefghijklmnop";
		gk::String concat = 'a' + str;
		EXPECT_EQ(concat.Len(), 17);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "aabcdefghijklmnop");
	}

	TEST(String, ConcatConstCharAndSso) {
		gk::String str = "abc";
		gk::String concat = "de" + str;
		EXPECT_EQ(concat.Len(), 5);
		EXPECT_TRUE(concat.IsSmallString());
		EXPECT_EQ(concat, "deabc");
	}

	TEST(String, ConcatConstCharAndSsoToHeap) {
		gk::String str = "cdefghijklmnop";
		gk::String concat = "ab" + str;

		EXPECT_EQ(concat.Len(), 16);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "abcdefghijklmnop");
	}

	TEST(String, ConcatConstCharAndHeap) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String str = buffer;
		gk::String concat = "sp" + str;

		EXPECT_EQ(concat.Len(), 18);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "spabcdefghijklmnop");
	}

	TEST(String, ConcatConstCharAndHeapDifferentCapacity) {
		const char* chars = "abcdefghijklmnopqrstuvwxyz01234";
		char buffer[32];
		memcpy(buffer, chars, 32);
		gk::String str = buffer;
		gk::String concat = "pp" + str;
		EXPECT_EQ(concat.Len(), 33);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(str.HeapCapacity(), 32);
		EXPECT_EQ(concat.HeapCapacity(), 64);
		EXPECT_EQ(concat, "ppabcdefghijklmnopqrstuvwxyz01234");
	}

	TEST(String, ConcatConstCharAndConstSegment) {
		gk::String str = "abcdefghijklmnop";
		gk::String concat = "ab" + str;
		EXPECT_EQ(concat.Len(), 18);
		EXPECT_TRUE(concat.IsHeapString());
		EXPECT_EQ(concat, "ababcdefghijklmnop");
	}

#pragma endregion

#pragma region Substring

	TEST(String, SubstringSmallToEnd) {
		gk::String str = "abcdefg";
		gk::String sub = str.Substring(2);
		EXPECT_EQ(sub.Len(), 5);
		EXPECT_TRUE(sub.IsSmallString());
		EXPECT_EQ(sub, "cdefg");
	}

	TEST(String, SubstringSmallToEndSanity) {
		gk::String str = "abcdefg";
		gk::String sub = str.Substring(2, str.Len());
		EXPECT_EQ(sub.Len(), 5);
		EXPECT_TRUE(sub.IsSmallString());
		EXPECT_EQ(sub, "cdefg");
	}

	TEST(String, SubstringSmallNotToEnd) {
		gk::String str = "abcdefg";
		gk::String sub = str.Substring(1, 6);
		EXPECT_EQ(sub.Len(), 5);
		EXPECT_TRUE(sub.IsSmallString());
		EXPECT_EQ(sub, "bcdef");
	}

	TEST(String, SubstringSmallFromHeap) {
		const char* chars = "abcdefghijklmnopqrs";
		char buffer[20];
		memcpy(buffer, chars, 20);
		gk::String str = buffer;
		gk::String sub = str.Substring(2, 5);
		EXPECT_EQ(sub.Len(), 3);
		EXPECT_TRUE(sub.IsSmallString());
		EXPECT_EQ(sub, "cde");
	}

	TEST(String, SubstringHeapFromHeap) {
		const char* chars = "abcdefghijklmnopqrs";
		char buffer[20];
		memcpy(buffer, chars, 20);
		gk::String str = buffer;
		gk::String sub = str.Substring(2, 18);
		EXPECT_EQ(sub.Len(), 16);
		EXPECT_TRUE(sub.IsHeapString());
		EXPECT_EQ(sub, "cdefghijklmnopqr");
	}

	TEST(String, SubstringHeapFromHeapSanity) {
		const char* chars = "abcdefghijklmnopqrs";
		char buffer[20];
		memcpy(buffer, chars, 20);
		gk::String str = buffer;
		gk::String sub = str.Substring(2);
		EXPECT_EQ(sub.Len(), 17);
		EXPECT_TRUE(sub.IsHeapString());
		EXPECT_EQ(sub, "cdefghijklmnopqrs");
	}

	TEST(String, SubstringSmallFromConstSegment) {
		gk::String str = "abcdefghijklmnopqrs";
		gk::String sub = str.Substring(2, 5);
		EXPECT_EQ(sub.Len(), 3);
		EXPECT_TRUE(sub.IsSmallString());
		EXPECT_EQ(sub, "cde");
	}

	TEST(String, SubstringHeapFromConstSegment) {
		gk::String str = "abcdefghijklmnopqrs";
		gk::String sub = str.Substring(2, 18);
		EXPECT_EQ(sub.Len(), 16);
		EXPECT_TRUE(sub.IsHeapString());
		EXPECT_EQ(sub, "cdefghijklmnopqr");
	}

	TEST(String, SubstringConstSegmentFromConstSegment) {
		gk::String str = "abcdefghijklmnopqrs";
		gk::String sub = str.Substring(2);
		EXPECT_EQ(sub.Len(), 17);
		EXPECT_TRUE(sub.IsConstSegmentString());
		EXPECT_EQ(sub, "cdefghijklmnopqrs");
	}

#pragma endregion

#pragma region Find

	TEST(String, FindCharInSmall) {
		gk::String str = "abcdefg";
		EXPECT_EQ(str.Find('d').Get(), 3);
	}

	TEST(String, FindCharInLong) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123456789asjdlakjshdlakjshdlkajhsldaksd????123123";
		EXPECT_EQ(str.Find('?').Get(), 66);
	}

	TEST(String, FindCharDoesntExist) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123456789asjdlakjshdlakjshdlkajhsldaksd????123123";
		EXPECT_FALSE(str.Find('!').IsValidIndex());
	}

	TEST(String, FindSubstringConstChar) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123456789asjdlakjshdlakjshdlkajhsldaksd????123123";
		EXPECT_EQ(str.Find("dl").Get(), 39);
	}

	TEST(String, FindSubstringConstCharSecondFirstCharOccurrence) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789asjdlakjshdlakjshd?lkajhsldaksd????123123";
		EXPECT_EQ(str.Find("?789").Get(), 34);
	}

	TEST(String, FindSubstringConstCharLateOccurrence) {
		gk::String str = "abcdefghijk-lmnopqrstu?vw--xyz01234-56?-789asjdlakjshd-lakjshd?lk-ajhsldaksd?-???123-123";
		EXPECT_EQ(str.Find("-1").Get(), 84);
	}

	TEST(String, FindSubstringDoesntExistConstChar) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789asjdlakjshdlakjshd?lkajhsldaksd????123123";
		EXPECT_FALSE(str.Find("?789b").IsValidIndex());
	}

	TEST(String, FindLargeSubstring) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789alksjdhyl;akjshd;aiouywhp;diuajhysd;iuyp91827y30-981723-98yupaiushfliajhsdp98710-2394871-92847-0192847-1982y3epoaiujshdlkajhsdlkjahsdlkjahsodkjahsopd9i8u170-394871-938754-193857-2398utyhlaijshdflkajshdlkajshep928734-9812734-9quiashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiu7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498yq1-498yqw- 8 y=- 8       98q21y4=-918	2yasjdlakjshdlakjshd?lkajhsldaksd????123123";
		EXPECT_EQ(str.Find("ashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiu7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498").Get(), 266);
	}

	TEST(String, FindLargeSubstringDoesntExist) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789alksjdhyl;akjshd;aiouywhp;diuajhysd;iuyp91827y30-981723-98yupaiushfliajhsdp98710-2394871-92847-0192847-1982y3epoaiujshdlkajhsdlkjahsdlkjahsodkjahsopd9i8u170-394871-938754-193857-2398utyhlaijshdflkajshdlkajshep928734-9812734-9quiashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiu7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498yq1-498yqw- 8 y=- 8       98q21y4=-918	2yasjdlakjshdlakjshd?lkajhsldaksd????123123";
		EXPECT_FALSE(str.Find("ashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiusdas7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498").IsValidIndex());
	}

	TEST(String, FindStringCharInSmall) {
		gk::String str = "abcdefg";
		gk::String find = 'd';
		EXPECT_EQ(str.Find(find).Get(), 3);
	}

	TEST(String, FindStringCharInLong) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123456789asjdlakjshdlakjshdlkajhsldaksd????123123";
		gk::String find = '?';
		EXPECT_EQ(str.Find(find).Get(), 66);
	}

	TEST(String, FindStringCharDoesntExist) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123456789asjdlakjshdlakjshdlkajhsldaksd????123123";
		gk::String find = '!';
		EXPECT_FALSE(str.Find(find).IsValidIndex());
	}

	TEST(String, FindSubstringString) {
		gk::String str = "abcdefghijklmnopqrstuvwxyz0123456789asjdlakjshdlakjshdlkajhsldaksd????123123";
		gk::String find = "dl";
		EXPECT_EQ(str.Find(find).Get(), 39);
	}

	TEST(String, FindSubstringStringSecondFirstCharOccurrence) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789asjdlakjshdlakjshd?lkajhsldaksd????123123";
		gk::String find = "?789";
		EXPECT_EQ(str.Find("?789").Get(), 34);
	}

	TEST(String, FindSubstringStringLateOccurrence) {
		gk::String str = "abcdefghijk-lmnopqrstu?vw--xyz01234-56?-789asjdlakjshd-lakjshd?lk-ajhsldaksd?-???123-123";
		gk::String find = "-1";
		EXPECT_EQ(str.Find(find).Get(), 84);
	}

	TEST(String, FindSubstringDoesntExistString) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789asjdlakjshdlakjshd?lkajhsldaksd????123123";
		gk::String find = "?789b";
		EXPECT_FALSE(str.Find(find).IsValidIndex());
	}

	TEST(String, FindLargeSubstringString) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789alksjdhyl;akjshd;aiouywhp;diuajhysd;iuyp91827y30-981723-98yupaiushfliajhsdp98710-2394871-92847-0192847-1982y3epoaiujshdlkajhsdlkjahsdlkjahsodkjahsopd9i8u170-394871-938754-193857-2398utyhlaijshdflkajshdlkajshep928734-9812734-9quiashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiu7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498yq1-498yqw- 8 y=- 8       98q21y4=-918	2yasjdlakjshdlakjshd?lkajhsldaksd????123123";
		gk::String find = "ashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiu7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498";
		EXPECT_EQ(str.Find(find).Get(), 266);
	}

	TEST(String, FindLargeSubstringStringDoesntExist) {
		gk::String str = "abcdefghijklmnopqrstu?vwxyz0123456?789alksjdhyl;akjshd;aiouywhp;diuajhysd;iuyp91827y30-981723-98yupaiushfliajhsdp98710-2394871-92847-0192847-1982y3epoaiujshdlkajhsdlkjahsdlkjahsodkjahsopd9i8u170-394871-938754-193857-2398utyhlaijshdflkajshdlkajshep928734-9812734-9quiashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiu7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498yq1-498yqw- 8 y=- 8       98q21y4=-918	2yasjdlakjshdlakjshd?lkajhsldaksd????123123";
		gk::String find = "ashdlkajhsdlkajshdlkajhsldkjahsdlkjahsldkjahspdiusdas7y1-398471-9384y-a9w8uhr-098u1y-49v871y-2498";
		EXPECT_FALSE(str.Find(find).IsValidIndex());
	}

#pragma endregion

#pragma region From

	TEST(String, FromBoolTrue) {
		gk::String str = gk::String::FromBool(true);
		EXPECT_EQ(str, "true");
	}

	TEST(String, FromBoolFalse) {
		gk::String str = gk::String::FromBool(false);
		EXPECT_EQ(str, "false");
	}

	TEST(String, FromSignedIntZero) {
		gk::String str = gk::String::FromInt(0);
		EXPECT_EQ(str, '0');
	}

	TEST(String, FromSignedIntNegative) {
		gk::String str = gk::String::FromInt(-11);
		EXPECT_EQ(str, "-11");
	}

	TEST(String, FromSignedIntMaxValue) {
		gk::String str = gk::String::FromInt(MAXINT64);
		EXPECT_EQ(str, "9223372036854775807");
	}

	TEST(String, FromSignedIntMinValue) {
		gk::String str = gk::String::FromInt(MININT64);
		EXPECT_EQ(str, "-9223372036854775808");
	}

	TEST(String, FromUnsignedIntMaxValue) {
		gk::String str = gk::String::FromUint(MAXUINT64);
		EXPECT_EQ(str, "18446744073709551615");
	}

	TEST(String, FromFloatZero) {
		gk::String str = gk::String::FromFloat(0.0);
		EXPECT_EQ(str, "0.0");
	}

	TEST(String, FromFloatInf) {
		gk::String str = gk::String::FromFloat(INFINITY);
		EXPECT_EQ(str, "inf");
	}

	TEST(String, FromFloatNegativeInf) {
		gk::String str = gk::String::FromFloat(-INFINITY);
		EXPECT_EQ(str, "-inf");
	}

	TEST(String, FromFloatNaN) {
		gk::String str = gk::String::FromFloat(NAN);
		EXPECT_EQ(str, "nan");
	}

	TEST(String, FromFloatPositiveNoDecimal) {
		gk::String str = gk::String::FromFloat(1.0);
		EXPECT_EQ(str, "1.0");
	}

	TEST(String, FromFloatNegativeNoDecimal) {
		gk::String str = gk::String::FromFloat(-1.0);
		EXPECT_EQ(str, "-1.0");
	}

	TEST(String, FromFloatPositiveNoDecimalManyDigits) {
		gk::String str = gk::String::FromFloat(175.0);
		EXPECT_EQ(str, "175.0");
	}

	TEST(String, FromFloatNegativeNoDecimalManyDigits) {
		gk::String str = gk::String::FromFloat(-175.0);
		EXPECT_EQ(str, "-175.0");
	}

	TEST(String, FromFloatPositiveWithDecimal) {
		gk::String str = gk::String::FromFloat(1.6);
		EXPECT_EQ(str, "1.6");
	}

	TEST(String, FromFloatNegativeWithDecimal) {
		gk::String str = gk::String::FromFloat(-1.6);
		EXPECT_EQ(str, "-1.6");
	}

	TEST(String, FromFloatPositiveWithDecimalEndingInZeroes) {
		gk::String str = gk::String::FromFloat(1.6000);
		EXPECT_EQ(str, "1.6");
	}

	TEST(String, FromFloatNegativeWithDecimalEndingInZeroes) {
		gk::String str = gk::String::FromFloat(-1.6000);
		EXPECT_EQ(str, "-1.6");
	}

	TEST(String, FromFloatWithZero) {
		gk::String str = gk::String::FromFloat(-12.034);
		EXPECT_EQ(str, "-12.034");
	}

	TEST(String, FromFloatWithManyZeroes) {
		gk::String str = gk::String::FromFloat(5.0004);
		EXPECT_EQ(str, "5.00039"); // Floating point inaccuracy, string ends up representing it as this. Close enough.
	}

	TEST(String, FromFloatLargePrecision) {
		gk::String str = gk::String::FromFloat(4.006000000442003001, 15);
		EXPECT_EQ(str, "4.006000000442003");
	}

	TEST(String, FromTemplateOverloads) {
		gk::String strBool = gk::String::From(true);
		EXPECT_EQ(strBool, "true");

		gk::String strInt8 = gk::String::From(int8(-45));
		EXPECT_EQ(strInt8, "-45");

		gk::String strInt16 = gk::String::From(int16(-1000));
		EXPECT_EQ(strInt16, "-1000");

		gk::String strInt32 = gk::String::From(int(-1234560));
		EXPECT_EQ(strInt32, "-1234560");

		gk::String strInt64 = gk::String::From(int64(MININT64));
		EXPECT_EQ(strInt64, "-9223372036854775808");

		gk::String strUint8 = gk::String::From(uint8(45));
		EXPECT_EQ(strUint8, "45");

		gk::String strUint16 = gk::String::From(uint16(1000));
		EXPECT_EQ(strUint16, "1000");

		gk::String strUint32 = gk::String::From(uint32(1234560));
		EXPECT_EQ(strUint32, "1234560");

		gk::String strUint64 = gk::String::From(uint64(MAXINT64));
		EXPECT_EQ(strUint64, "9223372036854775807");

		gk::String strFloat = gk::String::From(float(-12.5601));
		EXPECT_EQ(strFloat, "-12.56009"); // Floating point inaccuracy, string ends up representing it as this. Close enough.

		gk::String strDouble = gk::String::From(double(-12.5601));
		EXPECT_EQ(strDouble, "-12.5601");
	}

	TEST(String, UserDefinedStringFromTemplateType) {
		StringTestExample example;
		example.a = 1.05;
		example.b = -99.4004;
		gk::String strStringTestExample = gk::String::From(example);
		EXPECT_EQ(strStringTestExample, "1.05, -99.4004");
	}

#pragma endregion

#pragma region To

	TEST(String, ToBoolTrue) {
		gk::String str = gk::String::FromBool(true);
		EXPECT_TRUE(str.ToBool());
	}

	TEST(String, ToBoolFalse) {
		gk::String str = gk::String::FromBool(false);
		EXPECT_FALSE(str.ToBool());
	}

	TEST(String, ToBoolFalseNotFalseString) {
		gk::String str = "test";
		EXPECT_FALSE(str.ToBool());
	}

#pragma endregion

#pragma region Hash

	TEST(String, HashConstSegAndHeap) {
		const char* chars = "abcdefghijklmnop";
		char buffer[17];
		memcpy(buffer, chars, 17);
		gk::String strConstSeg = chars;
		gk::String strHeap = buffer;
		EXPECT_EQ(strConstSeg.ComputeHash(), strHeap.ComputeHash());
	}

	TEST(String, HashSsoAppended) {
		gk::String strNotModified = "blahblahlol";
		gk::String strToModify = "blahblah";
		strToModify.Append("lol");
		EXPECT_EQ(strNotModified.ComputeHash(), strToModify.ComputeHash());
	}

#pragma endregion

}