#include "../pch.h"
#include "../../GkTypesLib/GkTypes/String/TestString.h"
#include "../GkTest.h"

struct StringTestExample {
	double a;
	int64 b;

	constexpr StringTestExample() :
		a(0.0), b(0) {}
};

template<>
[[nodiscard]] static constexpr gk::TestString gk::TestString::from<StringTestExample>(const StringTestExample& value) {
	return gk::TestString::fromFloat(value.a) + gk::TestString(", ") + gk::TestString::fromUint(value.b);
}

namespace UnitTests
{
	TEST(TestString, DefaultConstruct) {
		gk::TestString a;
		EXPECT_EQ(a.len(), 0);
	}

	COMPTIME_TEST(TestString, DefaultConstruct, {
		gk::TestString a;
		comptimeAssertEq(a.len(), 0);
	});

	TEST(TestString, ConstructOneCharacter) {
		gk::TestString a = 'c';
		EXPECT_EQ(a.len(), 1);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], '\0');
	}

	COMPTIME_TEST(TestString, ConstructOneCharacter, {
		gk::TestString a = 'c';
		comptimeAssertEq(a.len(), 1);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], '\0');
	});

#pragma region Str_Construct

	TEST(TestString, ConstructStrSmall) {
		gk::TestString a = "hi"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'h');
		EXPECT_EQ(a.cstr()[1], 'i');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(TestString, ConstructStrSmall, {
		gk::TestString a = "hi"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'h');
		comptimeAssertEq(a.cstr()[1], 'i');
		comptimeAssertEq(a.cstr()[2], '\0');
	});

	TEST(TestString, ConstructStrSmallUtf8) {
		gk::TestString a = "aÜ"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 3);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[1], "Ü"[0]);
		EXPECT_EQ(a.cstr()[2], "Ü"[1]);
		EXPECT_EQ(a.cstr()[4], '\0');
	}

	COMPTIME_TEST(TestString, ConstructStrSmallUtf8, {
		gk::TestString a = "aÜ"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 3);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[1], "Ü"[0]);
		comptimeAssertEq(a.cstr()[2], "Ü"[1]);
		comptimeAssertEq(a.cstr()[4], '\0');
	});

	TEST(TestString, ConstructStrLarge) {
		gk::TestString a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(TestString, ConstructStrLarge, {
		gk::TestString a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[39], '\0');
	});

	TEST(TestString, ConstructStrLargeUtf8) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		EXPECT_EQ(a.len(), 29);
		EXPECT_EQ(a.usedBytes(), 37);
		EXPECT_EQ(a.cstr()[0], "Ü"[0]);
		EXPECT_EQ(a.cstr()[1], "Ü"[1]);
		EXPECT_NE(a.cstr()[36], '\0');
		EXPECT_EQ(a.cstr()[37], '\0');
	}

	COMPTIME_TEST(TestString, ConstructStrLargeUtf8, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		comptimeAssertEq(a.len(), 29);
		comptimeAssertEq(a.usedBytes(), 37);
		comptimeAssertEq(a.cstr()[0], "Ü"[0]);
		comptimeAssertEq(a.cstr()[1], "Ü"[1]);
		comptimeAssertNe(a.cstr()[36], '\0');
		comptimeAssertEq(a.cstr()[37], '\0');
	}); 

#pragma endregion

#pragma region Copy_Construct

	TEST(TestString, CopyDefaultConstruct) {
		gk::TestString a;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 0);
	}

	COMPTIME_TEST(TestString, CopyDefaultConstruct, {
		gk::TestString a;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 0);
		});

	TEST(TestString, CopyConstructOneCharacter) {
		gk::TestString a = 'c';
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 1);
		EXPECT_EQ(b.cstr()[0], 'c');
		EXPECT_EQ(b.cstr()[1], '\0');
	}

	COMPTIME_TEST(TestString, CopyConstructOneCharacter, {
		gk::TestString a = 'c';
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 1);
		comptimeAssertEq(b.cstr()[0], 'c');
		comptimeAssertEq(b.cstr()[1], '\0');
		});

	TEST(TestString, CopyConstructStrSmall) {
		gk::TestString a = "hi"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 2);
		EXPECT_EQ(b.cstr()[0], 'h');
		EXPECT_EQ(b.cstr()[1], 'i');
		EXPECT_EQ(b.cstr()[2], '\0');
	}

	COMPTIME_TEST(TestString, CopyConstructStrSmall, {
		gk::TestString a = "hi"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 2);
		comptimeAssertEq(b.cstr()[0], 'h');
		comptimeAssertEq(b.cstr()[1], 'i');
		comptimeAssertEq(b.cstr()[2], '\0');
		});

	TEST(TestString, CopyConstructStrSmallUtf8) {
		gk::TestString a = "aÜ"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 3);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[1], "Ü"[0]);
		EXPECT_EQ(b.cstr()[2], "Ü"[1]);
		EXPECT_EQ(b.cstr()[4], '\0');
	}

	COMPTIME_TEST(TestString, CopyConstructStrSmallUtf8, {
		gk::TestString a = "aÜ"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 3);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[1], "Ü"[0]);
		comptimeAssertEq(b.cstr()[2], "Ü"[1]);
		comptimeAssertEq(b.cstr()[4], '\0');
		});

	TEST(TestString, CopyConstructStrLarge) {
		gk::TestString a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 39);
		EXPECT_EQ(b.usedBytes(), 39);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[39], '\0');
	}

	COMPTIME_TEST(TestString, CopyConstructStrLarge, {
		gk::TestString a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 39);
		comptimeAssertEq(b.usedBytes(), 39);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[39], '\0');
		});

	TEST(TestString, CopyConstructStrLargeUtf8) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 29);
		EXPECT_EQ(b.usedBytes(), 37);
		EXPECT_EQ(b.cstr()[0], "Ü"[0]);
		EXPECT_EQ(b.cstr()[1], "Ü"[1]);
		EXPECT_NE(b.cstr()[36], '\0');
		EXPECT_EQ(b.cstr()[37], '\0');
	}

	COMPTIME_TEST(TestString, CopyConstructStrLargeUtf8, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 29);
		comptimeAssertEq(b.usedBytes(), 37);
		comptimeAssertEq(b.cstr()[0], "Ü"[0]);
		comptimeAssertEq(b.cstr()[1], "Ü"[1]);
		comptimeAssertNe(b.cstr()[36], '\0');
		comptimeAssertEq(b.cstr()[37], '\0');
		});

#pragma endregion

#pragma region Move_Construct

	TEST(TestString, MoveDefaultConstruct) {
		gk::TestString a;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 0);
	}

	COMPTIME_TEST(TestString, MoveDefaultConstruct, {
		gk::TestString a;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 0);
		});

	TEST(TestString, MoveConstructOneCharacter) {
		gk::TestString a = 'c';
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 1);
		EXPECT_EQ(b.cstr()[0], 'c');
		EXPECT_EQ(b.cstr()[1], '\0');
	}

	COMPTIME_TEST(TestString, MoveConstructOneCharacter, {
		gk::TestString a = 'c';
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 1);
		comptimeAssertEq(b.cstr()[0], 'c');
		comptimeAssertEq(b.cstr()[1], '\0');
		});

	TEST(TestString, MoveConstructStrSmall) {
		gk::TestString a = "hi"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 2);
		EXPECT_EQ(b.cstr()[0], 'h');
		EXPECT_EQ(b.cstr()[1], 'i');
		EXPECT_EQ(b.cstr()[2], '\0');
	}

	COMPTIME_TEST(TestString, MoveConstructStrSmall, {
		gk::TestString a = "hi"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 2);
		comptimeAssertEq(b.cstr()[0], 'h');
		comptimeAssertEq(b.cstr()[1], 'i');
		comptimeAssertEq(b.cstr()[2], '\0');
		});

	TEST(TestString, MoveConstructStrSmallUtf8) {
		gk::TestString a = "aÜ"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 3);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[1], "Ü"[0]);
		EXPECT_EQ(b.cstr()[2], "Ü"[1]);
		EXPECT_EQ(b.cstr()[4], '\0');
	}

	COMPTIME_TEST(TestString, MoveConstructStrSmallUtf8, {
		gk::TestString a = "aÜ"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 3);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[1], "Ü"[0]);
		comptimeAssertEq(b.cstr()[2], "Ü"[1]);
		comptimeAssertEq(b.cstr()[4], '\0');
		});

	TEST(TestString, MoveConstructStrLarge) {
		gk::TestString a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 39);
		EXPECT_EQ(b.usedBytes(), 39);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[39], '\0');
	}

	COMPTIME_TEST(TestString, MoveConstructStrLarge, {
		gk::TestString a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 39);
		comptimeAssertEq(b.usedBytes(), 39);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[39], '\0');
		});

	TEST(TestString, MoveConstructStrLargeUtf8) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::TestString b = a;
		EXPECT_EQ(b.len(), 29);
		EXPECT_EQ(b.usedBytes(), 37);
		EXPECT_EQ(b.cstr()[0], "Ü"[0]);
		EXPECT_EQ(b.cstr()[1], "Ü"[1]);
		EXPECT_NE(b.cstr()[36], '\0');
		EXPECT_EQ(b.cstr()[37], '\0');
	}

	COMPTIME_TEST(TestString, MoveConstructStrLargeUtf8, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::TestString b = a;
		comptimeAssertEq(b.len(), 29);
		comptimeAssertEq(b.usedBytes(), 37);
		comptimeAssertEq(b.cstr()[0], "Ü"[0]);
		comptimeAssertEq(b.cstr()[1], "Ü"[1]);
		comptimeAssertNe(b.cstr()[36], '\0');
		comptimeAssertEq(b.cstr()[37], '\0');
		});

#pragma endregion

#pragma region Assign_Char

	TEST(TestString, AssignFromChar) {
		gk::TestString a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
		a = 'c';
		EXPECT_EQ(a.len(), 1);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromChar, {
		gk::TestString a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
		a = 'c';
		comptimeAssertEq(a.len(), 1);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], '\0');
	});

	TEST(TestString, AssignFromCharNullBytesSanityCheck) {
		gk::TestString a = "ha"_str;
		a = 'c';
		EXPECT_EQ(a.len(), 1);
		EXPECT_EQ(a.cstr()[0], 'c');
		for (int i = 1; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}		
	}	
	
	COMPTIME_TEST(TestString, AssignFromCharNullBytesSanityCheck, {
		gk::TestString a = "ha"_str;
		a = 'c';
		comptimeAssertEq(a.len(), 1);
		comptimeAssertEq(a.cstr()[0], 'c');
		for (int i = 1; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
	});

#pragma endregion

#pragma region Assign_Str

	TEST(TestString, AssignFromSmallStr) {
		gk::TestString a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		a = "ca"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromSmallStr, {
		gk::TestString a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		a = "ca"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		comptimeAssertEq(a.cstr()[2], '\0');
		});

	TEST(TestString, AssignFromLargeStr) {
		gk::TestString a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[38], 'd');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromLargeStr, {
		gk::TestString a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[38], 'd');
		comptimeAssertEq(a.cstr()[39], '\0');
		});

	TEST(TestString, AssignFromStrNullBytesSanityCheck) {
		gk::TestString a = "hbb"_str;
		a = "ca"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a'); 
		for (int i = 2; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}
	}

	COMPTIME_TEST(TestString, AssignFromStrNullBytesSanityCheck, {
		gk::TestString a = "hbb"_str;
		a = "ca"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
		});

	TEST(TestString, AssignFromStrReuseAllocation) {
		gk::TestString a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		const char* newBuffer = a.cstr();
		EXPECT_EQ(oldBuffer, newBuffer);
	}

	COMPTIME_TEST(TestString, AssignFromStrReuseAllocation, {
		gk::TestString a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		const char* newBuffer = a.cstr();
		comptimeAssertEq(oldBuffer, newBuffer);
	});

#pragma endregion

#pragma region Assign_Copy

	TEST(TestString, AssignFromSmallCopy) {
		gk::TestString a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::TestString b = "ca"_str;
		a = b;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromSmallCopy, {
		gk::TestString a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::TestString b = "ca"_str;
		a = b;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		comptimeAssertEq(a.cstr()[2], '\0');
		});

	TEST(TestString, AssignFromLargeCopy) {
		gk::TestString a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::TestString b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = b;
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[38], 'd');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromLargeCopy, {
		gk::TestString a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::TestString b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = b;
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[38], 'd');
		comptimeAssertEq(a.cstr()[39], '\0');
		});

	TEST(TestString, AssignFromCopyNullBytesSanityCheck) {
		gk::TestString a = "hbb"_str;
		gk::TestString b = "ca"_str;
		a = b;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}
	}

	COMPTIME_TEST(TestString, AssignFromCopyNullBytesSanityCheck, {
		gk::TestString a = "hbb"_str;
		gk::TestString b = "ca"_str;
		a = b;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
		});

	TEST(TestString, AssignFromCopyReuseAllocation) {
		gk::TestString a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		gk::TestString b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		a = b;
		const char* newBuffer = a.cstr();
		EXPECT_EQ(oldBuffer, newBuffer);
	}

	COMPTIME_TEST(TestString, AssignFromCopyReuseAllocation, {
		gk::TestString a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		gk::TestString b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		a = b;
		const char* newBuffer = a.cstr();
		comptimeAssertEq(oldBuffer, newBuffer);
		});

#pragma endregion

#pragma region Assign_Move

	TEST(TestString, AssignFromSmallMove) {
		gk::TestString a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::TestString b = "ca"_str;
		a = std::move(b);
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromSmallMove, {
		gk::TestString a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::TestString b = "ca"_str;
		a = std::move(b);
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		comptimeAssertEq(a.cstr()[2], '\0');
		});

	TEST(TestString, AssignFromLargeMove) {
		gk::TestString a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::TestString b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = std::move(b);
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[38], 'd');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(TestString, AssignFromLargeMove, {
		gk::TestString a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::TestString b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = std::move(b);
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[38], 'd');
		comptimeAssertEq(a.cstr()[39], '\0');
		});

	TEST(TestString, AssignFromMoveNullBytesSanityCheck) {
		gk::TestString a = "hbb"_str;
		gk::TestString b = "ca"_str;
		a = std::move(b);
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}
	}

	COMPTIME_TEST(TestString, AssignFromMoveNullBytesSanityCheck, {
		gk::TestString a = "hbb"_str;
		gk::TestString b = "ca"_str;
		a = std::move(b);
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
		});

#pragma endregion

#pragma region Equal_Char

	TEST(TestString, EqualChar) {
		gk::TestString a = 'c';
		EXPECT_EQ(a, 'c');
	}

	COMPTIME_TEST(TestString, EqualChar, {
		gk::TestString a = 'c';
		comptimeAssertEq(a, 'c');
		});

	TEST(TestString, NotEqualChar) {
		gk::TestString a = 'b';
		EXPECT_NE(a, 'c');
	}

	COMPTIME_TEST(TestString, NotEqualChar, {
		gk::TestString a = 'b';
		comptimeAssertNe(a, 'c');
		});

	TEST(TestString, NotEqualCharSameFirst) {
		gk::TestString a = "ca"_str;
		EXPECT_NE(a, 'c');
	}

	COMPTIME_TEST(TestString, NotEqualCharSameFirst, {
		gk::TestString a = "ca"_str;
		comptimeAssertNe(a, 'c');
		});

	TEST(TestString, NotEqualCharAndLargeString) {
		gk::TestString a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
		EXPECT_NE(a, 'c');
	}

	COMPTIME_TEST(TestString, NotEqualCharAndLargeString, {
		gk::TestString a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
		comptimeAssertNe(a, 'c');
		});

#pragma endregion

#pragma region Equal_Str

	TEST(TestString, EqualSmallStr) {
		gk::TestString a = "hi"_str;
		EXPECT_EQ(a, "hi"_str);
	}

	COMPTIME_TEST(TestString, EqualSmallStr, {
		gk::TestString a = "hi"_str;
		comptimeAssertEq(a, "hi"_str);
		});

	TEST(TestString, EqualSsoMaxStr) {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_EQ(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	}

	COMPTIME_TEST(TestString, EqualSsoMaxStr, {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertEq(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

	TEST(TestString, EqualLargeStr) {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_EQ(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	}

	COMPTIME_TEST(TestString, EqualLargeStr, {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertEq(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

	TEST(TestString, EqualUtf8SmallStr) {
		gk::TestString a = "ßen"_str;
		EXPECT_EQ(a, "ßen"_str);
	}

	COMPTIME_TEST(TestString, EqualUtf8SmallStr, {
		gk::TestString a = "ßen"_str;
		comptimeAssertEq(a, "ßen"_str);
	});

	TEST(TestString, EqualUtf8LargeStr) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	}
	
	COMPTIME_TEST(TestString, EqualUtf8LargeStr, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

	TEST(TestString, NotEqualSmallStr) {
		gk::TestString a = "hh"_str;
		EXPECT_NE(a, "hi"_str);
	}

	COMPTIME_TEST(TestString, NotEqualSmallStr, {
		gk::TestString a = "hh"_str;
		comptimeAssertNe(a, "hi"_str);
	});

	TEST(TestString, NotEqualSsoMaxStr) {
		gk::TestString a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_NE(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	}

	COMPTIME_TEST(TestString, NotEqualSsoMaxStr, {
		gk::TestString a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertNe(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

	TEST(TestString, NotEqualLargeStr) {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_NE(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	}

	COMPTIME_TEST(TestString, NotEqualLargeStr, {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertNe(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

	TEST(TestString, NotEqualUtf8Small) {
		gk::TestString a = "ßeb"_str;
		EXPECT_NE(a, "ßen"_str);
	}

	COMPTIME_TEST(TestString, NotEqualUtf8Small, {
		gk::TestString a = "ßeb"_str;
		comptimeAssertNe(a, "ßen"_str);
	});

	TEST(TestString, NotEqualUtf8Large) {
		gk::TestString a = "ÜbergrößenträgerÜbargrößenträ"_str;
		EXPECT_NE(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	}
	
	COMPTIME_TEST(TestString, NotEqualUtf8Large, {
		gk::TestString a = "ÜbergrößenträgerÜbargrößenträ"_str;
		comptimeAssertNe(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

#pragma endregion

#pragma region Equal_Other_String

	TEST(TestString, EqualCharOtherString) {
		gk::TestString a = 'c';
		EXPECT_EQ(a, gk::TestString('c'));
	}

	COMPTIME_TEST(TestString, EqualCharOtherString, {
		gk::TestString a = 'c';
		comptimeAssertEq(a, gk::TestString('c'));
	});

	TEST(TestString, EqualSmallOtherString) {
		gk::TestString a = "hi"_str;
		EXPECT_EQ(a, gk::TestString("hi"_str));
	}

	COMPTIME_TEST(TestString, EqualSmallOtherString, {
		gk::TestString a = "hi"_str;
		comptimeAssertEq(a, gk::TestString("hi"_str));
		});

	TEST(TestString, EqualSsoMaxOtherString) {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_EQ(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddg"_str));
	}

	COMPTIME_TEST(TestString, EqualSsoMaxOtherString, {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertEq(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddg"_str));
		});

	TEST(TestString, EqualLargeOtherString) {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_EQ(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	}

	COMPTIME_TEST(TestString, EqualLargeOtherString, {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertEq(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
		});

	TEST(TestString, EqualUtf8SmallOtherString) {
		gk::TestString a = "ßen"_str;
		EXPECT_EQ(a, gk::TestString("ßen"_str));
	}

	COMPTIME_TEST(TestString, EqualUtf8SmallOtherString, {
		gk::TestString a = "ßen"_str;
		comptimeAssertEq(a, gk::TestString("ßen"_str));
		});

	TEST(TestString, EqualUtf8LargeOtherString) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		EXPECT_EQ(a, gk::TestString("ÜbergrößenträgerÜbergrößenträ"_str));
	}

	COMPTIME_TEST(TestString, EqualUtf8LargeOtherString, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträ"_str;
		comptimeAssertEq(a, gk::TestString("ÜbergrößenträgerÜbergrößenträ"_str));
		});

	TEST(TestString, NotEqualSmallStrOtherString) {
		gk::TestString a = "hh"_str;
		EXPECT_NE(a, gk::TestString("hi"_str));
	}

	COMPTIME_TEST(TestString, NotEqualSmallStrOtherString, {
		gk::TestString a = "hh"_str;
		comptimeAssertNe(a, gk::TestString("hi"_str));
		});

	TEST(TestString, NotEqualSsoMaxStrOtherString) {
		gk::TestString a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_NE(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddg"_str));
	}

	COMPTIME_TEST(TestString, NotEqualSsoMaxStrOtherString, {
		gk::TestString a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertNe(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddg"_str));
		});

	TEST(TestString, NotEqualLargeStrOtherString) {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_NE(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	}

	COMPTIME_TEST(TestString, NotEqualLargeStrOtherString, {
		gk::TestString a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertNe(a, gk::TestString("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
		});

	TEST(TestString, NotEqualUtf8SmallOtherString) {
		gk::TestString a = "ßeb"_str;
		EXPECT_NE(a, gk::TestString("ßen"_str));
	}

	COMPTIME_TEST(TestString, NotEqualUtf8SmallOtherString, {
		gk::TestString a = "ßeb"_str;
		comptimeAssertNe(a, gk::TestString("ßen"_str));
		});

	TEST(TestString, NotEqualUtf8LargeOtherString) {
		gk::TestString a = "ÜbergrößenträgerÜbargrößenträ"_str;
		EXPECT_NE(a, gk::TestString("ÜbergrößenträgerÜbergrößenträ"_str));
	}

	COMPTIME_TEST(TestString, NotEqualUtf8LargeOtherString, {
		gk::TestString a = "ÜbergrößenträgerÜbargrößenträ"_str;
		comptimeAssertNe(a, gk::TestString("ÜbergrößenträgerÜbergrößenträ"_str));
		});

#pragma endregion

#pragma region Append

	TEST(TestString, EmptyStringAppendChar) {
		gk::TestString a;
		a.append('c');
		EXPECT_EQ(a, 'c');
		EXPECT_EQ(a, gk::TestString('c')); // for sanity, same with following tests
	}

	COMPTIME_TEST(TestString, EmptyStringAppendChar, {
		gk::TestString a;
		a.append('c');
		comptimeAssertEq(a, 'c');
		comptimeAssertEq(a, gk::TestString('c'));
		});

	TEST(TestString, SmallStringAppendChar) {
		gk::TestString a = "hello"_str;
		a.append('!');
		EXPECT_EQ(a, "hello!"_str);
		EXPECT_EQ(a, gk::TestString("hello!"_str));
	}

	COMPTIME_TEST(TestString, SmallStringAppendChar, {
		gk::TestString a = "hello"_str;
		a.append('!');
		comptimeAssertEq(a, "hello!"_str);
		comptimeAssertEq(a, gk::TestString("hello!"_str));
	});

	TEST(TestString, SmallStringAppendCharMakeHeap) {
		gk::TestString a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append('!');
		EXPECT_EQ(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
		EXPECT_EQ(a, gk::TestString("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
	}

	COMPTIME_TEST(TestString, SmallStringAppendCharMakeHeap, {
		gk::TestString a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append('!');
		comptimeAssertEq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
		comptimeAssertEq(a, gk::TestString("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
	});

	TEST(TestString, LargeStringAppendChar) {
		gk::TestString a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append('a');
		EXPECT_EQ(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
		EXPECT_EQ(a, gk::TestString("1672038761203876102873601287630187263018723601872630187263018723a"_str));
	}

	COMPTIME_TEST(TestString, LargeStringAppendChar, {
		gk::TestString a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append('a');
		comptimeAssertEq(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
		comptimeAssertEq(a, gk::TestString("1672038761203876102873601287630187263018723601872630187263018723a"_str));
	});

	TEST(TestString, SmallUtf8AppendChar) {
		gk::TestString a = "ßeb"_str;
		a.append('?');
		EXPECT_EQ(a, "ßeb?"_str);
		EXPECT_EQ(a, gk::TestString("ßeb?"_str));
	}

	COMPTIME_TEST(TestString, SmallUtf8AppendChar, {
		gk::TestString a = "ßeb"_str;
		a.append('?');
		comptimeAssertEq(a, "ßeb?"_str);
		comptimeAssertEq(a, gk::TestString("ßeb?"_str));
	});

	TEST(TestString, SmallUtf8AppendCharMakeHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergröa"_str;
		a.append('l');
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergröal"_str);
		EXPECT_EQ(a, gk::TestString("ÜbergrößenträgerÜbergröal"_str));
	}

	COMPTIME_TEST(TestString, SmallUtf8AppendCharMakeHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergröa"_str;
		a.append('l');
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergröal"_str);
		comptimeAssertEq(a, gk::TestString("ÜbergrößenträgerÜbergröal"_str));
	});

	TEST(TestString, AppendCharHeapReallocate) {
		gk::TestString a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append('5');
		EXPECT_EQ(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
		EXPECT_EQ(a, gk::TestString("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
	}

	COMPTIME_TEST(TestString, AppendCharHeapReallocate, {
		gk::TestString a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append('5');
		comptimeAssertEq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
		comptimeAssertEq(a, gk::TestString("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
	});

#pragma endregion

#pragma region Append_Str

	TEST(TestString, EmptyStringAppendStr) {
		gk::TestString a;
		a.append("cc"_str);
		EXPECT_EQ(a, "cc"_str);
		EXPECT_EQ(a, gk::TestString("cc"_str)); // for sanity, same with following tests
	}

	COMPTIME_TEST(TestString, EmptyStringAppendStr, {
		gk::TestString a;
		a.append("cc"_str);
		comptimeAssertEq(a, "cc"_str);
		comptimeAssertEq(a, gk::TestString("cc"_str));
		});

	TEST(TestString, SmallStringAppendStr) {
		gk::TestString a = "hello"_str;
		a.append("!!"_str);
		EXPECT_EQ(a, "hello!!"_str);
		EXPECT_EQ(a, gk::TestString("hello!!"_str));
	}

	COMPTIME_TEST(TestString, SmallStringAppendStr, {
		gk::TestString a = "hello"_str;
		a.append("!!"_str);
		comptimeAssertEq(a, "hello!!"_str);
		comptimeAssertEq(a, gk::TestString("hello!!"_str));
		});

	TEST(TestString, SmallStringAppendStrMakeHeap) {
		gk::TestString a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append("!!"_str);
		EXPECT_EQ(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		EXPECT_EQ(a, gk::TestString("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	}

	COMPTIME_TEST(TestString, SmallStringAppendStrMakeHeap, {
		gk::TestString a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append("!!"_str);
		comptimeAssertEq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		comptimeAssertEq(a, gk::TestString("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
		});

	TEST(TestString, LargeStringAppendStr) {
		gk::TestString a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append("aa"_str);
		EXPECT_EQ(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		EXPECT_EQ(a, gk::TestString("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	}

	COMPTIME_TEST(TestString, LargeStringAppendStr, {
		gk::TestString a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append("aa"_str);
		comptimeAssertEq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		comptimeAssertEq(a, gk::TestString("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
		});

	TEST(TestString, SmallUtf8AppendStr) {
		gk::TestString a = "ßeb"_str;
		a.append("??"_str);
		EXPECT_EQ(a, "ßeb??"_str);
		EXPECT_EQ(a, gk::TestString("ßeb??"_str));
	}

	COMPTIME_TEST(TestString, SmallUtf8AppendStr, {
		gk::TestString a = "ßeb"_str;
		a.append("??"_str);
		comptimeAssertEq(a, "ßeb??"_str);
		comptimeAssertEq(a, gk::TestString("ßeb??"_str));
		});

	TEST(TestString, SmallUtf8AppendStrMakeHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergröa"_str;
		a.append("ll"_str);
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergröall"_str);
		EXPECT_EQ(a, gk::TestString("ÜbergrößenträgerÜbergröall"_str));
	}

	COMPTIME_TEST(TestString, SmallUtf8AppendStrMakeHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergröa"_str;
		a.append("ll"_str);
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergröall"_str);
		comptimeAssertEq(a, gk::TestString("ÜbergrößenträgerÜbergröall"_str));
		});

	TEST(TestString, AppendStrHeapReallocate) {
		gk::TestString a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append("55"_str);
		EXPECT_EQ(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		EXPECT_EQ(a, gk::TestString("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	}

	COMPTIME_TEST(TestString, AppendStrHeapReallocate, {
		gk::TestString a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append("55"_str);
		comptimeAssertEq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		comptimeAssertEq(a, gk::TestString("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
		});

#pragma endregion

#pragma region Append_Other_String

	TEST(TestString, EmptyStringAppendOtherString) {
		gk::TestString a;
		a.append(gk::TestString("cc"_str));
		EXPECT_EQ(a, "cc"_str);
		EXPECT_EQ(a, gk::TestString("cc"_str)); // for sanity, same with following tests
	}

	COMPTIME_TEST(TestString, EmptyStringAppendOtherString, {
		gk::TestString a;
		a.append(gk::TestString("cc"_str));
		comptimeAssertEq(a, "cc"_str);
		comptimeAssertEq(a, gk::TestString("cc"_str));
		});

	TEST(TestString, SmallStringAppendOtherString) {
		gk::TestString a = "hello"_str;
		a.append(gk::TestString("!!"_str));
		EXPECT_EQ(a, "hello!!"_str);
		EXPECT_EQ(a, gk::TestString("hello!!"_str));
	}

	COMPTIME_TEST(TestString, SmallStringAppendOtherString, {
		gk::TestString a = "hello"_str;
		a.append(gk::TestString("!!"_str));
		comptimeAssertEq(a, "hello!!"_str);
		comptimeAssertEq(a, gk::TestString("hello!!"_str));
		});

	TEST(TestString, SmallStringAppendOtherStringMakeHeap) {
		gk::TestString a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append(gk::TestString("!!"_str));
		EXPECT_EQ(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		EXPECT_EQ(a, gk::TestString("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	}

	COMPTIME_TEST(TestString, SmallStringAppendOtherStringMakeHeap, {
		gk::TestString a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append(gk::TestString("!!"_str));
		comptimeAssertEq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		comptimeAssertEq(a, gk::TestString("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
		});

	TEST(TestString, LargeStringAppendOtherString) {
		gk::TestString a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append(gk::TestString("aa"_str));
		EXPECT_EQ(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		EXPECT_EQ(a, gk::TestString("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	}

	COMPTIME_TEST(TestString, LargeStringAppendOtherString, {
		gk::TestString a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append(gk::TestString("aa"_str));
		comptimeAssertEq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		comptimeAssertEq(a, gk::TestString("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
		});

	TEST(TestString, SmallUtf8AppendOtherString) {
		gk::TestString a = "ßeb"_str;
		a.append(gk::TestString("??"_str));
		EXPECT_EQ(a, "ßeb??"_str);
		EXPECT_EQ(a, gk::TestString("ßeb??"_str));
	}

	COMPTIME_TEST(TestString, SmallUtf8AppendOtherString, {
		gk::TestString a = "ßeb"_str;
		a.append(gk::TestString("??"_str));
		comptimeAssertEq(a, "ßeb??"_str);
		comptimeAssertEq(a, gk::TestString("ßeb??"_str));
		});

	TEST(TestString, SmallUtf8AppendOtherStringMakeHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergröa"_str;
		a.append(gk::TestString("ll"_str));
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergröall"_str);
		EXPECT_EQ(a, gk::TestString("ÜbergrößenträgerÜbergröall"_str));
	}

	COMPTIME_TEST(TestString, SmallUtf8AppendOtherStringMakeHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergröa"_str;
		a.append(gk::TestString("ll"_str));
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergröall"_str);
		comptimeAssertEq(a, gk::TestString("ÜbergrößenträgerÜbergröall"_str));
		});

	TEST(TestString, AppendOtherStringHeapReallocate) {
		gk::TestString a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append(gk::TestString("55"_str));
		EXPECT_EQ(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		EXPECT_EQ(a, gk::TestString("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	}

	COMPTIME_TEST(TestString, AppendOtherStringHeapReallocate, {
		gk::TestString a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append(gk::TestString("55"_str));
		comptimeAssertEq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		comptimeAssertEq(a, gk::TestString("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
		});

#pragma endregion

#pragma region Concat_Char

	TEST(TestString, ConcatEmptyAndChar) {
		const gk::TestString a;
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, 'c');
		EXPECT_EQ(b, gk::TestString('c'));
	}

	COMPTIME_TEST(TestString, ConcatEmptyAndChar, {
		const gk::TestString a;
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, 'c');
		comptimeAssertEq(b, gk::TestString('c'));
		});

	TEST(TestString, ConcatCharStringAndChar) {
		const gk::TestString a = 'c';
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::TestString("cc"_str));
	}

	COMPTIME_TEST(TestString, ConcatCharStringAndChar, {
		const gk::TestString a = 'c';
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::TestString("cc"_str));
	});

	TEST(TestString, ConcatSmallStringAndCharToHeap) {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
		EXPECT_EQ(b, gk::TestString("aslasdasddkjahldkjahsldkjahsdac"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallStringAndCharToHeap, {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
		comptimeAssertEq(b, gk::TestString("aslasdasddkjahldkjahsldkjahsdac"_str));
	});

	TEST(TestString, ConcatHeapStringAndCharToHeap) {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
		EXPECT_EQ(b, gk::TestString("aslasdasddkjahl55dkjahsldkjahsdac"_str));
	}

	COMPTIME_TEST(TestString, ConcatHeapStringAndCharToHeap, {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
		comptimeAssertEq(b, gk::TestString("aslasdasddkjahl55dkjahsldkjahsdac"_str));
	});

	TEST(TestString, ConcatSmallUtf8AndChar) {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, "Übergrößenträgerc"_str);
		EXPECT_EQ(b, gk::TestString("Übergrößenträgerc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallUtf8AndChar, {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, "Übergrößenträgerc"_str);
		comptimeAssertEq(b, gk::TestString("Übergrößenträgerc"_str));
	});

	TEST(TestString, ConcatSmallUtf8AndCharToHeap) {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, "Übergrößenträgerasjhdgashhc"_str);
		EXPECT_EQ(b, gk::TestString("Übergrößenträgerasjhdgashhc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallUtf8AndCharToHeap, {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, "Übergrößenträgerasjhdgashhc"_str);
		comptimeAssertEq(b, gk::TestString("Übergrößenträgerasjhdgashhc"_str));
	});

	TEST(TestString, ConcatHeapUtf8AndChar) {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a + 'c';
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
		EXPECT_EQ(b, gk::TestString("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
	}

	COMPTIME_TEST(TestString, ConcatHeapUtf8AndChar, {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a + 'c';
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
		comptimeAssertEq(b, gk::TestString("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
	});

#pragma endregion

#pragma region Concat_Char_Inverted

	TEST(TestString, InvertConcatEmptyAndChar) {
		const gk::TestString a;
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, 'c');
		EXPECT_EQ(b, gk::TestString('c'));
	}

	COMPTIME_TEST(TestString, InvertConcatEmptyAndChar, {
		const gk::TestString a;
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, 'c');
		comptimeAssertEq(b, gk::TestString('c'));
		});

	TEST(TestString, InvertConcatCharStringAndChar) {
		const gk::TestString a = 'c';
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::TestString("cc"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatCharStringAndChar, {
		const gk::TestString a = 'c';
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::TestString("cc"_str));
	});

	TEST(TestString, InvertConcatSmallStringAndCharToHeap) {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, "caslasdasddkjahldkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::TestString("caslasdasddkjahldkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatSmallStringAndCharToHeap, {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, "caslasdasddkjahldkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::TestString("caslasdasddkjahldkjahsldkjahsda"_str));
	});

	TEST(TestString, InvertConcatHeapStringAndCharToHeap) {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::TestString("caslasdasddkjahl55dkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatHeapStringAndCharToHeap, {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::TestString("caslasdasddkjahl55dkjahsldkjahsda"_str));
	});

	TEST(TestString, InvertConcatSmallUtf8AndChar) {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, "cÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::TestString("cÜbergrößenträger"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatSmallUtf8AndChar, {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, "cÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::TestString("cÜbergrößenträger"_str));
	});

	TEST(TestString, InvertConcatSmallUtf8AndCharToHeap) {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, "cÜbergrößenträgerasjhdgashh"_str);
		EXPECT_EQ(b, gk::TestString("cÜbergrößenträgerasjhdgashh"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatSmallUtf8AndCharToHeap, {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, "cÜbergrößenträgerasjhdgashh"_str);
		comptimeAssertEq(b, gk::TestString("cÜbergrößenträgerasjhdgashh"_str));
	});

	TEST(TestString, InvertConcatHeapUtf8AndChar) {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = 'c' + a;
		EXPECT_EQ(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::TestString("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatHeapUtf8AndChar, {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = 'c' + a;
		comptimeAssertEq(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::TestString("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	});

#pragma endregion

#pragma region Concat_Str

	TEST(TestString, ConcatEmptyAndStr) {
		const gk::TestString a;
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::TestString("cc"_str));
	}

	COMPTIME_TEST(TestString, ConcatEmptyAndStr, {
		const gk::TestString a;
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::TestString("cc"_str));
		});

	TEST(TestString, ConcatCharStringAndStr) {
		const gk::TestString a = 'c';
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "ccc"_str);
		EXPECT_EQ(b, gk::TestString("ccc"_str));
	}

	COMPTIME_TEST(TestString, ConcatCharStringAndStr, {
		const gk::TestString a = 'c';
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "ccc"_str);
		comptimeAssertEq(b, gk::TestString("ccc"_str));
		});

	TEST(TestString, ConcatSmallStringAndStrToHeap) {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::TestString("aslasdasddkjahldkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallStringAndStrToHeap, {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::TestString("aslasdasddkjahldkjahsldkjahsdacc"_str));
		});

	TEST(TestString, ConcatHeapStringAndStrToHeap) {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::TestString("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(TestString, ConcatHeapStringAndStrToHeap, {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::TestString("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
		});

	TEST(TestString, ConcatSmallUtf8AndStr) {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "Übergrößenträgercc"_str);
		EXPECT_EQ(b, gk::TestString("Übergrößenträgercc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallUtf8AndStr, {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "Übergrößenträgercc"_str);
		comptimeAssertEq(b, gk::TestString("Übergrößenträgercc"_str));
		});

	TEST(TestString, ConcatSmallUtf8AndStrToHeap) {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "Übergrößenträgerasjhdgashhcc"_str);
		EXPECT_EQ(b, gk::TestString("Übergrößenträgerasjhdgashhcc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallUtf8AndStrToHeap, {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "Übergrößenträgerasjhdgashhcc"_str);
		comptimeAssertEq(b, gk::TestString("Übergrößenträgerasjhdgashhcc"_str));
		});

	TEST(TestString, ConcatHeapUtf8AndStr) {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a + "cc"_str;
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		EXPECT_EQ(b, gk::TestString("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	}

	COMPTIME_TEST(TestString, ConcatHeapUtf8AndStr, {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a + "cc"_str;
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		comptimeAssertEq(b, gk::TestString("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
		});

#pragma endregion

#pragma region Concat_Str_Inverted

	TEST(TestString, InvertConcatEmptyAndStr) {
		const gk::TestString a;
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::TestString("cc"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatEmptyAndStr, {
		const gk::TestString a;
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::TestString("cc"_str));
		});

	TEST(TestString, InvertConcatCharStringAndStr) {
		const gk::TestString a = 'c';
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "ccc"_str);
		EXPECT_EQ(b, gk::TestString("ccc"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatCharStringAndStr, {
		const gk::TestString a = 'c';
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "ccc"_str);
		comptimeAssertEq(b, gk::TestString("ccc"_str));
		});

	TEST(TestString, InvertConcatSmallStringAndStrToHeap) {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::TestString("ccaslasdasddkjahldkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatSmallStringAndStrToHeap, {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::TestString("ccaslasdasddkjahldkjahsldkjahsda"_str));
		});

	TEST(TestString, InvertConcatHeapStringAndStrToHeap) {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::TestString("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatHeapStringAndStrToHeap, {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::TestString("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
		});

	TEST(TestString, InvertConcatSmallUtf8AndStr) {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "ccÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::TestString("ccÜbergrößenträger"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatSmallUtf8AndStr, {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "ccÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::TestString("ccÜbergrößenträger"_str));
		});

	TEST(TestString, InvertConcatSmallUtf8AndStrToHeap) {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "ccÜbergrößenträgerasjhdgashh"_str);
		EXPECT_EQ(b, gk::TestString("ccÜbergrößenträgerasjhdgashh"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatSmallUtf8AndStrToHeap, {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "ccÜbergrößenträgerasjhdgashh"_str);
		comptimeAssertEq(b, gk::TestString("ccÜbergrößenträgerasjhdgashh"_str));
		});

	TEST(TestString, InvertConcatHeapUtf8AndStr) {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = "cc"_str + a;
		EXPECT_EQ(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::TestString("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	}

	COMPTIME_TEST(TestString, InvertConcatHeapUtf8AndStr, {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = "cc"_str + a;
		comptimeAssertEq(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::TestString("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
		});

#pragma endregion

#pragma region Concat_Two_Strings

	TEST(TestString, ConcatEmptyAndOtherString) {
		const gk::TestString a;
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::TestString("cc"_str));
	}

	COMPTIME_TEST(TestString, ConcatEmptyAndOtherString, {
		const gk::TestString a;
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::TestString("cc"_str));
		});

	TEST(TestString, ConcatCharStringAndOtherString) {
		const gk::TestString a = 'c';
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "ccc"_str);
		EXPECT_EQ(b, gk::TestString("ccc"_str));
	}

	COMPTIME_TEST(TestString, ConcatCharStringAndOtherString, {
		const gk::TestString a = 'c';
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "ccc"_str);
		comptimeAssertEq(b, gk::TestString("ccc"_str));
		});

	TEST(TestString, ConcatSmallStringAndOtherStringToHeap) {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::TestString("aslasdasddkjahldkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallStringAndOtherStringToHeap, {
		const gk::TestString a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::TestString("aslasdasddkjahldkjahsldkjahsdacc"_str));
		});

	TEST(TestString, ConcatHeapStringAndOtherStringToHeap) {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::TestString("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(TestString, ConcatHeapStringAndOtherStringToHeap, {
		const gk::TestString a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::TestString("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
		});

	TEST(TestString, ConcatSmallUtf8AndOtherString) {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "Übergrößenträgercc"_str);
		EXPECT_EQ(b, gk::TestString("Übergrößenträgercc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallUtf8AndOtherString, {
		const gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "Übergrößenträgercc"_str);
		comptimeAssertEq(b, gk::TestString("Übergrößenträgercc"_str));
		});

	TEST(TestString, ConcatSmallUtf8AndOtherStringToHeap) {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "Übergrößenträgerasjhdgashhcc"_str);
		EXPECT_EQ(b, gk::TestString("Übergrößenträgerasjhdgashhcc"_str));
	}

	COMPTIME_TEST(TestString, ConcatSmallUtf8AndOtherStringToHeap, {
		const gk::TestString a = "Übergrößenträgerasjhdgashh"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "Übergrößenträgerasjhdgashhcc"_str);
		comptimeAssertEq(b, gk::TestString("Übergrößenträgerasjhdgashhcc"_str));
		});

	TEST(TestString, ConcatHeapUtf8AndOtherString) {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		EXPECT_EQ(b, gk::TestString("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	}

	COMPTIME_TEST(TestString, ConcatHeapUtf8AndOtherString, {
		const gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a + gk::TestString("cc"_str);
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		comptimeAssertEq(b, gk::TestString("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
		});

#pragma endregion

#pragma region Concat_Multiple

	TEST(TestString, ChainConcat) {
		gk::TestString a = "hello world!"_str;
		gk::TestString b = a + ' ' + "hmm"_str + " t" + 'h' + gk::TestString("is is") + " a multi concat string thats quite large"_str;
		EXPECT_EQ(b, "hello world! hmm this is a multi concat string thats quite large"_str);
		EXPECT_EQ(b, gk::TestString("hello world! hmm this is a multi concat string thats quite large"_str));
	}

	COMPTIME_TEST(TestString, ChainConcat, {
		gk::TestString a = "hello world!"_str;
		gk::TestString b = a + ' ' + "hmm"_str + " t" + 'h' + gk::TestString("is is") + " a multi concat string thats quite large"_str;
		comptimeAssertEq(b, "hello world! hmm this is a multi concat string thats quite large"_str);
		comptimeAssertEq(b, gk::TestString("hello world! hmm this is a multi concat string thats quite large"_str));
	});

#pragma endregion

#pragma region From_Type

	TEST(TestString, FromBoolTrue) {
		gk::TestString a = gk::TestString::fromBool(true);
		EXPECT_EQ(a, "true"_str);
		EXPECT_EQ(a, gk::TestString("true"_str));
	}

	COMPTIME_TEST(TestString, FromBoolTrue, {
		gk::TestString a = gk::TestString::fromBool(true);
		comptimeAssertEq(a, "true"_str);
		comptimeAssertEq(a, gk::TestString("true"_str));
		});

	TEST(TestString, FromBoolFalse) {
		gk::TestString a = gk::TestString::fromBool(false);
		EXPECT_EQ(a, "false"_str);
		EXPECT_EQ(a, gk::TestString("false"_str));
	}

	COMPTIME_TEST(TestString, FromBoolFalse, {
		gk::TestString a = gk::TestString::fromBool(false);
		comptimeAssertEq(a, "false"_str);
		comptimeAssertEq(a, gk::TestString("false"_str));
	});

	TEST(TestString, FromSignedIntZero) {
		gk::TestString a = gk::TestString::fromInt(0);
		EXPECT_EQ(a, '0');
	}

	COMPTIME_TEST(TestString, FromSignedIntZero, {
		gk::TestString a = gk::TestString::fromInt(0);
		comptimeAssertEq(a, '0');
	});

	TEST(TestString, FromSignedIntSmallValue) {
		gk::TestString a = gk::TestString::fromInt(16);
		EXPECT_EQ(a, "16"_str);
	}

	COMPTIME_TEST(TestString, FromSignedIntSmallValue, {
		gk::TestString a = gk::TestString::fromInt(16);
		comptimeAssertEq(a, "16"_str);
	});

	TEST(TestString, FromSignedIntMaximumValue) {
		gk::TestString a = gk::TestString::fromInt(MAXINT64);
		EXPECT_EQ(a, "9223372036854775807"_str);
	}

	COMPTIME_TEST(TestString, FromSignedIntMaximumValue, {
		gk::TestString a = gk::TestString::fromInt(MAXINT64);
		comptimeAssertEq(a, "9223372036854775807"_str);
	});

	TEST(TestString, FromSignedIntSmallNegativeValue) {
		gk::TestString a = gk::TestString::fromInt(-3);
		EXPECT_EQ(a, "-3"_str);
	}

	COMPTIME_TEST(TestString, FromSignedIntSmallNegativeValue, {
		gk::TestString a = gk::TestString::fromInt(-3);
		comptimeAssertEq(a, "-3"_str);
	});

	TEST(TestString, FromSignedIntMinimumValue) {
		gk::TestString a = gk::TestString::fromInt(MININT64);
		EXPECT_EQ(a, "-9223372036854775808"_str);
	}

	COMPTIME_TEST(TestString, FromSignedIntMinimumValue, {
		gk::TestString a = gk::TestString::fromInt(MININT64);
		comptimeAssertEq(a, "-9223372036854775808"_str);
	});

	TEST(TestString, FromUnsignedIntZero) {
		gk::TestString a = gk::TestString::fromUint(0);
		EXPECT_EQ(a, '0');
	}

	COMPTIME_TEST(TestString, FromUnsignedIntZero, {
		gk::TestString a = gk::TestString::fromUint(0);
		comptimeAssertEq(a, '0');
	});

	TEST(TestString, FromUnsignedIntSmallValue) {
		gk::TestString a = gk::TestString::fromUint(23);
		EXPECT_EQ(a, "23"_str);
	}

	COMPTIME_TEST(TestString, FromUnsignedIntSmallValue, {
		gk::TestString a = gk::TestString::fromUint(23);
		comptimeAssertEq(a, "23"_str);
	});

	TEST(TestString, FromUnsignedIntMaximumValue) {
		gk::TestString a = gk::TestString::fromUint(MAXUINT64);
		EXPECT_EQ(a, "18446744073709551615"_str);
	}

	COMPTIME_TEST(TestString, FromUnsignedIntMaximumValue, {
		gk::TestString a = gk::TestString::fromUint(MAXUINT64);
		comptimeAssertEq(a, "18446744073709551615"_str);
	});

	TEST(TestString, FromFloatZero) {
		gk::TestString a = gk::TestString::fromFloat(0.0);
		EXPECT_EQ(a, "0.0"_str);
	}

	COMPTIME_TEST(TestString, FromFloatZero, {
		gk::TestString a = gk::TestString::fromFloat(0.0);
		comptimeAssertEq(a, "0.0"_str);
	});

	TEST(TestString, FromFloatPositiveInfinity) {
		gk::TestString a = gk::TestString::fromFloat(INFINITY);
		EXPECT_EQ(a, "inf"_str);
	}

	TEST(TestString, FromFloatNegativeInfinity) {
		gk::TestString a = gk::TestString::fromFloat(-1.0 * INFINITY);
		EXPECT_EQ(a, "-inf"_str);
	}

	TEST(TestString, FromFloatNaN) {
		gk::TestString a = gk::TestString::fromFloat(NAN);
		EXPECT_EQ(a, "nan"_str);
	}

	TEST(TestString, FromFloatWholeNumber) {
		gk::TestString a = gk::TestString::fromFloat(100.0);
		EXPECT_EQ(a, "100.0"_str);
	}

	COMPTIME_TEST(TestString, FromFloatWholeNumber, {
		gk::TestString a = gk::TestString::fromFloat(100.0);
		comptimeAssertEq(a, "100.0"_str);
	});

	TEST(TestString, FromFloatWholeNegativeNumber) {
		gk::TestString a = gk::TestString::fromFloat(-100.0);
		EXPECT_EQ(a, "-100.0"_str);
	}

	COMPTIME_TEST(TestString, FromFloatWholeNegativeNumber, {
		gk::TestString a = gk::TestString::fromFloat(-100.0);
		comptimeAssertEq(a, "-100.0"_str);
	});

	TEST(TestString, FromFloatDecimalNumber) {
		gk::TestString a = gk::TestString::fromFloat(100.09999);
		EXPECT_EQ(a, "100.09999"_str);
	}

	COMPTIME_TEST(TestString, FromFloatDecimalNumber, {
		gk::TestString a = gk::TestString::fromFloat(100.09999);
		comptimeAssertEq(a, "100.09999"_str);
	});

	TEST(TestString, FromFloatDecimalNegativeNumber) {
		gk::TestString a = gk::TestString::fromFloat(-100.09999);
		EXPECT_EQ(a, "-100.09999"_str);
	}

	COMPTIME_TEST(TestString, FromFloatDecimalNegativeNumber, {
		gk::TestString a = gk::TestString::fromFloat(-100.09999);
		comptimeAssertEq(a, "-100.09999"_str);
	});

	TEST(TestString, FromFloatDecimalNumberDefaultPrecision) {
		gk::TestString a = gk::TestString::fromFloat(100.12000005);
		EXPECT_EQ(a, "100.12"_str);
	}

	COMPTIME_TEST(TestString, FromFloatDecimalNumberDefaultPrecision, {
		gk::TestString a = gk::TestString::fromFloat(100.12000005);
		comptimeAssertEq(a, "100.12"_str);
	});

	TEST(TestString, FromFloatDecimalNegativeNumberDefaultPrecision) {
		gk::TestString a = gk::TestString::fromFloat(-100.12000005);
		EXPECT_EQ(a, "-100.12"_str);
	}

	COMPTIME_TEST(TestString, FromFloatDecimalNegativeNumberDefaultPrecision, {
		gk::TestString a = gk::TestString::fromFloat(-100.12000005);
		comptimeAssertEq(a, "-100.12"_str);
	});

	TEST(TestString, FromFloatDecimalNumberCustomPrecision) {
		gk::TestString a = gk::TestString::fromFloat(100.12000005, 10);
		EXPECT_EQ(a, "100.12000005"_str);
	}

	COMPTIME_TEST(TestString, FromFloatDecimalNumberCustomPrecision, {
		gk::TestString a = gk::TestString::fromFloat(100.12000005, 10);
		comptimeAssertEq(a, "100.12000005"_str);
	});

	TEST(TestString, FromFloatDecimalNegativeNumberCustomPrecision) {
		gk::TestString a = gk::TestString::fromFloat(-100.12000005, 10);
		EXPECT_EQ(a, "-100.12000005"_str);
	}

	COMPTIME_TEST(TestString, FromFloatDecimalNegativeNumberCustomPrecision, {
		gk::TestString a = gk::TestString::fromFloat(-100.12000005, 10);
		comptimeAssertEq(a, "-100.12000005"_str);
	});

	TEST(TestString, FromTemplateBool) {
		bool b = true;
		gk::TestString a = gk::TestString::from(b);
		EXPECT_EQ(a, "true"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateBool, {
		bool b = true;
		gk::TestString a = gk::TestString::from(b);
		comptimeAssertEq(a, "true"_str);
	});

	TEST(TestString, FromTemplateInt8) {
		int8 num = -56;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "-56"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateInt8, {
		int8 num = -56;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "-56"_str);
	});

	TEST(TestString, FromTemplateUint8) {
		uint8 num = 56;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "56"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateUint8, {
		uint8 num = 56;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "56"_str);
	});

	TEST(TestString, FromTemplateInt16) {
		int16 num = -1000;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "-1000"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateInt16, {
		int16 num = -1000;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "-1000"_str);
	});

	TEST(TestString, FromTemplateUint16) {
		uint16 num = 1000;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "1000"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateUint16, {
		uint16 num = 1000;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "1000"_str);
	});

	TEST(TestString, FromTemplateInt32) {
		int32 num = -99999;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "-99999"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateInt32, {
		int32 num = -99999;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "-99999"_str);
	});

	TEST(TestString, FromTemplateUint32) {
		uint32 num = 99999;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "99999"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateUint32, {
		uint32 num = 99999;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "99999"_str);
	});

	TEST(TestString, FromTemplateInt64) {
		int64 num = -123456789012345;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "-123456789012345"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateInt64, {
		int64 num = -123456789012345;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "-123456789012345"_str);
	});

	TEST(TestString, FromTemplateUint64) {
		uint64 num = 123456789012345;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "123456789012345"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateUint64, {
		uint64 num = 123456789012345;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "123456789012345"_str);
	});

	TEST(TestString, FromTemplateFloat32) {
		float num = -123.45f;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "-123.44999"_str); // slightly imprecise
	}

	COMPTIME_TEST(TestString, FromTemplateFloat32, {
		float num = -123.45f;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "-123.44999"_str); // slightly imprecise
	});

	TEST(TestString, FromTemplateFloat64) {
		double num = -123.45;
		gk::TestString a = gk::TestString::from(num);
		EXPECT_EQ(a, "-123.45"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateFloat64, {
		double num = -123.45;
		gk::TestString a = gk::TestString::from(num);
		comptimeAssertEq(a, "-123.45"_str);
	});

	TEST(TestString, FromTemplateCustomType) {
		StringTestExample e;
		e.a = 1.0;
		e.b = 1;
		gk::TestString a = gk::TestString::from(e);
		EXPECT_EQ(a, "1.0, 1"_str);
	}

	COMPTIME_TEST(TestString, FromTemplateCustomType, {
		StringTestExample e;
		e.a = 1.0;
		e.b = 1;
		gk::TestString a = gk::TestString::from(e);
		comptimeAssertEq(a, "1.0, 1"_str);
	});

#pragma endregion

#pragma region Format

	TEST(TestString, FormatOneArg) {
		int num = 4;
		gk::TestString a = gk::TestString::format<"num: {}">(num);
		EXPECT_EQ(a, "num: 4"_str);
	}

	COMPTIME_TEST(TestString, FormatOneArg, {
		int num = 4;
		gk::TestString a = gk::TestString::format<"num: {}">(num);
		comptimeAssertEq(a, "num: 4"_str);
	});

	TEST(TestString, FormatOneArgWithTextAfter) {
		float num = 4.f;
		gk::TestString a = gk::TestString::format<"num: {}... cool!">(num);
		EXPECT_EQ(a, "num: 4.0... cool!"_str);
	}

	COMPTIME_TEST(TestString, FormatOneArgWithTextAfter, {
		float num = 4.f;
		gk::TestString a = gk::TestString::format<"num: {}... cool!">(num);
		comptimeAssertEq(a, "num: 4.0... cool!"_str);
	});

	TEST(TestString, FormatTwoArgs) {
		int num1 = 5;
		float num2 = 5;
		gk::TestString a = gk::TestString::format<"num1: {}, num2: {}">(num1, num2);
		EXPECT_EQ(a, "num1: 5, num2: 5.0"_str);
	}

	COMPTIME_TEST(TestString, FormatTwoArgs, {
		int num1 = 5;
		float num2 = 5;
		gk::TestString a = gk::TestString::format<"num1: {}, num2: {}">(num1, num2);
		comptimeAssertEq(a, "num1: 5, num2: 5.0"_str);
	});

	TEST(TestString, FormatTwoArgsWithOperation) {
		int num1 = 5;
		float num2 = 5;
		gk::TestString a = gk::TestString::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
		EXPECT_EQ(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
	}

	COMPTIME_TEST(TestString, FormatTwoArgsWithOperation, {
		int num1 = 5;
		float num2 = 5;
		gk::TestString a = gk::TestString::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
		comptimeAssertEq(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
	});

	TEST(TestString, FormatFromCustomType) {
		StringTestExample e;
		e.a = -1.2;
		e.b = 5;
		int count = 2;
		gk::TestString a = gk::TestString::format<"the {} numbers are {}">(count, e);
		EXPECT_EQ(a, "the 2 numbers are -1.19999, 5"_str);
	}

	COMPTIME_TEST(TestString, FormatFromCustomType, {
		StringTestExample e;
		e.a = -1.2;
		e.b = 5;
		int count = 2;
		gk::TestString a = gk::TestString::format<"the {} numbers are {}">(count, e);
		comptimeAssertEq(a, "the 2 numbers are -1.19999, 5"_str);
	});

#pragma endregion

#pragma region Find_Char

	TEST(TestString, FindCharInSso) {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
	}

	COMPTIME_TEST(TestString, FindCharInSso, {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
	});

	TEST(TestString, FindCharInHeap) {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 81);
	}

	COMPTIME_TEST(TestString, FindCharInHeap, {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 81);
	});

	TEST(TestString, NotFindCharInSso) {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindCharInSso, {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	});

	TEST(TestString, NotFindCharInHeap) {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	}		

	COMPTIME_TEST(TestString, NotFindCharInHeap, {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	});


#pragma endregion

#pragma region Find_Str

	TEST(TestString, FindStrInSso) {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 21);
	}

	COMPTIME_TEST(TestString, FindStrInSso, {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
		});

	TEST(TestString, FindStrInHeap) {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 83);
	}

	COMPTIME_TEST(TestString, FindStrInHeap, {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 83);
		});

	TEST(TestString, FindUtf8StrInSso) {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßen"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 9);
	}

	COMPTIME_TEST(TestString, FindUtf8StrInSso, {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßen"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 9);
		});

	TEST(TestString, FindUtf8StrInHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("6Übe"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 141);
	}

	COMPTIME_TEST(TestString, FindUtf8StrInHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("6Übe"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 141);
	});

	TEST(TestString, NotFindStrInSso) {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("ya"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindStrInSso, {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("ya"_str);
		comptimeAssert(opt.none());
		});

	TEST(TestString, NotFindStrInHeap) {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(";5"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindStrInHeap, {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(";5"_str);
		comptimeAssert(opt.none());
		});

	TEST(TestString, NotFindUtf8StrInSso) {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßet"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindUtf8StrInSso, {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßet"_str);
		comptimeAssert(opt.none());
	});

	TEST(TestString, NotFindUtf8StrInHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("5Üba"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindUtf8StrInHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("5Üba"_str);
		comptimeAssert(opt.none());
	});

#pragma endregion

#pragma region Find_Other_String

	TEST(TestString, FindOtherStringInSso) {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
	}

	COMPTIME_TEST(TestString, FindOtherStringInSso, {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
		});

	TEST(TestString, FindOtherStringInHeap) {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 83);
	}

	COMPTIME_TEST(TestString, FindOtherStringInHeap, {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 83);
		});

	TEST(TestString, FindUtf8OtherStringInSso) {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("ßen"_str));
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 9);
	}

	COMPTIME_TEST(TestString, FindUtf8OtherStringInSso, {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("ßen"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 9);
		});

	TEST(TestString, FindUtf8OtherStringInHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("6Übe"_str));
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 141);
	}

	COMPTIME_TEST(TestString, FindUtf8OtherStringInHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("6Übe"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 141);
		});

	TEST(TestString, NotFindOtherStringInSso) {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("ya"_str));
		comptimeAssert(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindOtherStringInSso, {
		gk::TestString a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("ya"_str));
		comptimeAssert(opt.none());
		});

	TEST(TestString, NotFindOtherStringInHeap) {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString(";5"_str));
		comptimeAssert(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindOtherStringInHeap, {
		gk::TestString a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString(";5"_str));
		comptimeAssert(opt.none());
		});

	TEST(TestString, NotFindUtf8OtherStringInSso) {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("ßet"_str));
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindUtf8OtherStringInSso, {
		gk::TestString a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("ßet"_str));
		comptimeAssert(opt.none());
		});

	TEST(TestString, NotFindUtf8OtherStringInHeap) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("5Üba"_str));
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(TestString, NotFindUtf8OtherStringInHeap, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::TestString("5Üba"_str));
		comptimeAssert(opt.none());
		});

#pragma endregion

#pragma region Substring

	TEST(TestString, SubstringSsoStartingFromBeginning) {
		gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a.substring(0, 12);
		EXPECT_EQ(b, "Übergröße"_str);
	}

	COMPTIME_TEST(TestString, SubstringSsoStartingFromBeginning, {
		gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a.substring(0, 12);
		comptimeAssertEq(b, "Übergröße"_str);
		});

	TEST(TestString, SubstringSsoStartingFromOffset) {
		gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a.substring(2, 12);
		EXPECT_EQ(b, "bergröße"_str);
	}

	COMPTIME_TEST(TestString, SubstringSsoStartingFromOffset, {
		gk::TestString a = "Übergrößenträger"_str;
		gk::TestString b = a.substring(2, 12);
		comptimeAssertEq(b, "bergröße"_str);
	});

	TEST(TestString, SubstringHeapToSsoStartingFromBeginning) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(0, 20);
		EXPECT_EQ(b, "Übergrößenträger"_str);
	}

	COMPTIME_TEST(TestString, SubstringHeapToSsoStartingFromBeginning, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(0, 20);
		comptimeAssertEq(b, "Übergrößenträger"_str);
	});

	TEST(TestString, SubstringHeapToSsoStartingFromOffset) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(20, 40);
		EXPECT_EQ(b, "Übergrößenträger"_str);
	}

	COMPTIME_TEST(TestString, SubstringHeapToSsoStartingFromOffset, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(20, 40);
		comptimeAssertEq(b, "Übergrößenträger"_str);
	});

	TEST(TestString, SubstringHeapToHeapStartingFromBeginning) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(0, 40);
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträger"_str);
	}

	COMPTIME_TEST(TestString, SubstringHeapToHeapStartingFromBeginning, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(0, 40);
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträger"_str);
	});

	TEST(TestString, SubstringHeapToHeapStartingFromOffset) {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(20, 80);
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	}

	COMPTIME_TEST(TestString, SubstringHeapToHeapStartingFromOffset, {
		gk::TestString a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::TestString b = a.substring(20, 80);
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	});

#pragma endregion
}