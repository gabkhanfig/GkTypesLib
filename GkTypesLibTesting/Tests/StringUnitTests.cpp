#include "../pch.h"
#include <windows.h>
#include "../../GkTypesLib/GkTypes/String/String.h"
#include "../GkTest.h"

struct StringTestExample {
	double a;
	int64 b;

	constexpr StringTestExample() :
		a(0.0), b(0) {}
};

template<>
[[nodiscard]] static constexpr gk::String gk::String::from<StringTestExample>(const StringTestExample& value) {
	return gk::String::fromFloat(value.a) + gk::String(", ") + gk::String::fromUint(value.b);
}

namespace UnitTests
{
	TEST(String, DefaultConstruct) {
		gk::String a;
		EXPECT_EQ(a.len(), 0);
	}

	COMPTIME_TEST(String, DefaultConstruct, {
		gk::String a;
		comptimeAssertEq(a.len(), 0);
	});

	TEST(String, ConstructOneCharacter) {
		gk::String a = 'c';
		EXPECT_EQ(a.len(), 1);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], '\0');
	}

	COMPTIME_TEST(String, ConstructOneCharacter, {
		gk::String a = 'c';
		comptimeAssertEq(a.len(), 1);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], '\0');
	});

#pragma region Str_Construct

	TEST(String, ConstructStrSmall) {
		gk::String a = "hi"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'h');
		EXPECT_EQ(a.cstr()[1], 'i');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(String, ConstructStrSmall, {
		gk::String a = "hi"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'h');
		comptimeAssertEq(a.cstr()[1], 'i');
		comptimeAssertEq(a.cstr()[2], '\0');
	});

	TEST(String, ConstructStrSmallUtf8) {
		gk::String a = "aÜ"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 3);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[1], "Ü"[0]);
		EXPECT_EQ(a.cstr()[2], "Ü"[1]);
		EXPECT_EQ(a.cstr()[4], '\0');
	}

	COMPTIME_TEST(String, ConstructStrSmallUtf8, {
		gk::String a = "aÜ"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 3);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[1], "Ü"[0]);
		comptimeAssertEq(a.cstr()[2], "Ü"[1]);
		comptimeAssertEq(a.cstr()[4], '\0');
	});

	TEST(String, ConstructStrLarge) {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(String, ConstructStrLarge, {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[39], '\0');
	});

	TEST(String, ConstructStrLargeUtf8) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		EXPECT_EQ(a.len(), 29);
		EXPECT_EQ(a.usedBytes(), 37);
		EXPECT_EQ(a.cstr()[0], "Ü"[0]);
		EXPECT_EQ(a.cstr()[1], "Ü"[1]);
		EXPECT_NE(a.cstr()[36], '\0');
		EXPECT_EQ(a.cstr()[37], '\0');
	}

	COMPTIME_TEST(String, ConstructStrLargeUtf8, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		comptimeAssertEq(a.len(), 29);
		comptimeAssertEq(a.usedBytes(), 37);
		comptimeAssertEq(a.cstr()[0], "Ü"[0]);
		comptimeAssertEq(a.cstr()[1], "Ü"[1]);
		comptimeAssertNe(a.cstr()[36], '\0');
		comptimeAssertEq(a.cstr()[37], '\0');
	}); 

#pragma endregion

#pragma region Copy_Construct

	TEST(String, CopyDefaultConstruct) {
		gk::String a;
		gk::String b = a;
		EXPECT_EQ(b.len(), 0);
	}

	COMPTIME_TEST(String, CopyDefaultConstruct, {
		gk::String a;
		gk::String b = a;
		comptimeAssertEq(b.len(), 0);
		});

	TEST(String, CopyConstructOneCharacter) {
		gk::String a = 'c';
		gk::String b = a;
		EXPECT_EQ(b.len(), 1);
		EXPECT_EQ(b.cstr()[0], 'c');
		EXPECT_EQ(b.cstr()[1], '\0');
	}

	COMPTIME_TEST(String, CopyConstructOneCharacter, {
		gk::String a = 'c';
		gk::String b = a;
		comptimeAssertEq(b.len(), 1);
		comptimeAssertEq(b.cstr()[0], 'c');
		comptimeAssertEq(b.cstr()[1], '\0');
		});

	TEST(String, CopyConstructStrSmall) {
		gk::String a = "hi"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 2);
		EXPECT_EQ(b.cstr()[0], 'h');
		EXPECT_EQ(b.cstr()[1], 'i');
		EXPECT_EQ(b.cstr()[2], '\0');
	}

	COMPTIME_TEST(String, CopyConstructStrSmall, {
		gk::String a = "hi"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 2);
		comptimeAssertEq(b.cstr()[0], 'h');
		comptimeAssertEq(b.cstr()[1], 'i');
		comptimeAssertEq(b.cstr()[2], '\0');
		});

	TEST(String, CopyConstructStrSmallUtf8) {
		gk::String a = "aÜ"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 3);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[1], "Ü"[0]);
		EXPECT_EQ(b.cstr()[2], "Ü"[1]);
		EXPECT_EQ(b.cstr()[4], '\0');
	}

	COMPTIME_TEST(String, CopyConstructStrSmallUtf8, {
		gk::String a = "aÜ"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 3);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[1], "Ü"[0]);
		comptimeAssertEq(b.cstr()[2], "Ü"[1]);
		comptimeAssertEq(b.cstr()[4], '\0');
		});

	TEST(String, CopyConstructStrLarge) {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 39);
		EXPECT_EQ(b.usedBytes(), 39);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[39], '\0');
	}

	COMPTIME_TEST(String, CopyConstructStrLarge, {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 39);
		comptimeAssertEq(b.usedBytes(), 39);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[39], '\0');
		});

	TEST(String, CopyConstructStrLargeUtf8) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 29);
		EXPECT_EQ(b.usedBytes(), 37);
		EXPECT_EQ(b.cstr()[0], "Ü"[0]);
		EXPECT_EQ(b.cstr()[1], "Ü"[1]);
		EXPECT_NE(b.cstr()[36], '\0');
		EXPECT_EQ(b.cstr()[37], '\0');
	}

	COMPTIME_TEST(String, CopyConstructStrLargeUtf8, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 29);
		comptimeAssertEq(b.usedBytes(), 37);
		comptimeAssertEq(b.cstr()[0], "Ü"[0]);
		comptimeAssertEq(b.cstr()[1], "Ü"[1]);
		comptimeAssertNe(b.cstr()[36], '\0');
		comptimeAssertEq(b.cstr()[37], '\0');
		});

#pragma endregion

#pragma region Move_Construct

	TEST(String, MoveDefaultConstruct) {
		gk::String a;
		gk::String b = a;
		EXPECT_EQ(b.len(), 0);
	}

	COMPTIME_TEST(String, MoveDefaultConstruct, {
		gk::String a;
		gk::String b = a;
		comptimeAssertEq(b.len(), 0);
		});

	TEST(String, MoveConstructOneCharacter) {
		gk::String a = 'c';
		gk::String b = a;
		EXPECT_EQ(b.len(), 1);
		EXPECT_EQ(b.cstr()[0], 'c');
		EXPECT_EQ(b.cstr()[1], '\0');
	}

	COMPTIME_TEST(String, MoveConstructOneCharacter, {
		gk::String a = 'c';
		gk::String b = a;
		comptimeAssertEq(b.len(), 1);
		comptimeAssertEq(b.cstr()[0], 'c');
		comptimeAssertEq(b.cstr()[1], '\0');
		});

	TEST(String, MoveConstructStrSmall) {
		gk::String a = "hi"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 2);
		EXPECT_EQ(b.cstr()[0], 'h');
		EXPECT_EQ(b.cstr()[1], 'i');
		EXPECT_EQ(b.cstr()[2], '\0');
	}

	COMPTIME_TEST(String, MoveConstructStrSmall, {
		gk::String a = "hi"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 2);
		comptimeAssertEq(b.cstr()[0], 'h');
		comptimeAssertEq(b.cstr()[1], 'i');
		comptimeAssertEq(b.cstr()[2], '\0');
		});

	TEST(String, MoveConstructStrSmallUtf8) {
		gk::String a = "aÜ"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 2);
		EXPECT_EQ(b.usedBytes(), 3);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[1], "Ü"[0]);
		EXPECT_EQ(b.cstr()[2], "Ü"[1]);
		EXPECT_EQ(b.cstr()[4], '\0');
	}

	COMPTIME_TEST(String, MoveConstructStrSmallUtf8, {
		gk::String a = "aÜ"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 2);
		comptimeAssertEq(b.usedBytes(), 3);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[1], "Ü"[0]);
		comptimeAssertEq(b.cstr()[2], "Ü"[1]);
		comptimeAssertEq(b.cstr()[4], '\0');
		});

	TEST(String, MoveConstructStrLarge) {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 39);
		EXPECT_EQ(b.usedBytes(), 39);
		EXPECT_EQ(b.cstr()[0], 'a');
		EXPECT_EQ(b.cstr()[39], '\0');
	}

	COMPTIME_TEST(String, MoveConstructStrLarge, {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 39);
		comptimeAssertEq(b.usedBytes(), 39);
		comptimeAssertEq(b.cstr()[0], 'a');
		comptimeAssertEq(b.cstr()[39], '\0');
		});

	TEST(String, MoveConstructStrLargeUtf8) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::String b = a;
		EXPECT_EQ(b.len(), 29);
		EXPECT_EQ(b.usedBytes(), 37);
		EXPECT_EQ(b.cstr()[0], "Ü"[0]);
		EXPECT_EQ(b.cstr()[1], "Ü"[1]);
		EXPECT_NE(b.cstr()[36], '\0');
		EXPECT_EQ(b.cstr()[37], '\0');
	}

	COMPTIME_TEST(String, MoveConstructStrLargeUtf8, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::String b = a;
		comptimeAssertEq(b.len(), 29);
		comptimeAssertEq(b.usedBytes(), 37);
		comptimeAssertEq(b.cstr()[0], "Ü"[0]);
		comptimeAssertEq(b.cstr()[1], "Ü"[1]);
		comptimeAssertNe(b.cstr()[36], '\0');
		comptimeAssertEq(b.cstr()[37], '\0');
		});

#pragma endregion

#pragma region Assign_Char

	TEST(String, AssignFromChar) {
		gk::String a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
		a = 'c';
		EXPECT_EQ(a.len(), 1);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], '\0');
	}

	COMPTIME_TEST(String, AssignFromChar, {
		gk::String a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
		a = 'c';
		comptimeAssertEq(a.len(), 1);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], '\0');
	});

	TEST(String, AssignFromCharNullBytesSanityCheck) {
		gk::String a = "ha"_str;
		a = 'c';
		EXPECT_EQ(a.len(), 1);
		EXPECT_EQ(a.cstr()[0], 'c');
		for (int i = 1; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}		
	}	
	
	COMPTIME_TEST(String, AssignFromCharNullBytesSanityCheck, {
		gk::String a = "ha"_str;
		a = 'c';
		comptimeAssertEq(a.len(), 1);
		comptimeAssertEq(a.cstr()[0], 'c');
		for (int i = 1; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
	});

#pragma endregion

#pragma region Assign_Str

	TEST(String, AssignFromSmallStr) {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		a = "ca"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(String, AssignFromSmallStr, {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		a = "ca"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		comptimeAssertEq(a.cstr()[2], '\0');
		});

	TEST(String, AssignFromLargeStr) {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[38], 'd');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(String, AssignFromLargeStr, {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[38], 'd');
		comptimeAssertEq(a.cstr()[39], '\0');
		});

	TEST(String, AssignFromStrNullBytesSanityCheck) {
		gk::String a = "hbb"_str;
		a = "ca"_str;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a'); 
		for (int i = 2; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}
	}

	COMPTIME_TEST(String, AssignFromStrNullBytesSanityCheck, {
		gk::String a = "hbb"_str;
		a = "ca"_str;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
		});

	TEST(String, AssignFromStrReuseAllocation) {
		gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		const char* newBuffer = a.cstr();
		EXPECT_EQ(oldBuffer, newBuffer);
	}

	COMPTIME_TEST(String, AssignFromStrReuseAllocation, {
		gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		const char* newBuffer = a.cstr();
		comptimeAssertEq(oldBuffer, newBuffer);
	});

#pragma endregion

#pragma region Assign_Copy

	TEST(String, AssignFromSmallCopy) {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::String b = "ca"_str;
		a = b;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(String, AssignFromSmallCopy, {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::String b = "ca"_str;
		a = b;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		comptimeAssertEq(a.cstr()[2], '\0');
		});

	TEST(String, AssignFromLargeCopy) {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = b;
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[38], 'd');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(String, AssignFromLargeCopy, {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = b;
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[38], 'd');
		comptimeAssertEq(a.cstr()[39], '\0');
		});

	TEST(String, AssignFromCopyNullBytesSanityCheck) {
		gk::String a = "hbb"_str;
		gk::String b = "ca"_str;
		a = b;
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}
	}

	COMPTIME_TEST(String, AssignFromCopyNullBytesSanityCheck, {
		gk::String a = "hbb"_str;
		gk::String b = "ca"_str;
		a = b;
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			comptimeAssertEq(a.cstr()[i], '\0');
		}
		});

	TEST(String, AssignFromCopyReuseAllocation) {
		gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		gk::String b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		a = b;
		const char* newBuffer = a.cstr();
		EXPECT_EQ(oldBuffer, newBuffer);
	}

	COMPTIME_TEST(String, AssignFromCopyReuseAllocation, {
		gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		gk::String b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		a = b;
		const char* newBuffer = a.cstr();
		comptimeAssertEq(oldBuffer, newBuffer);
		});

#pragma endregion

#pragma region Assign_Move

	TEST(String, AssignFromSmallMove) {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::String b = "ca"_str;
		a = std::move(b);
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		EXPECT_EQ(a.cstr()[2], '\0');
	}

	COMPTIME_TEST(String, AssignFromSmallMove, {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::String b = "ca"_str;
		a = std::move(b);
		comptimeAssertEq(a.len(), 2);
		comptimeAssertEq(a.usedBytes(), 2);
		comptimeAssertEq(a.cstr()[0], 'c');
		comptimeAssertEq(a.cstr()[1], 'a');
		comptimeAssertEq(a.cstr()[2], '\0');
		});

	TEST(String, AssignFromLargeMove) {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = std::move(b);
		EXPECT_EQ(a.len(), 39);
		EXPECT_EQ(a.usedBytes(), 39);
		EXPECT_EQ(a.cstr()[0], 'a');
		EXPECT_EQ(a.cstr()[38], 'd');
		EXPECT_EQ(a.cstr()[39], '\0');
	}

	COMPTIME_TEST(String, AssignFromLargeMove, {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = std::move(b);
		comptimeAssertEq(a.len(), 39);
		comptimeAssertEq(a.usedBytes(), 39);
		comptimeAssertEq(a.cstr()[0], 'a');
		comptimeAssertEq(a.cstr()[38], 'd');
		comptimeAssertEq(a.cstr()[39], '\0');
		});

	TEST(String, AssignFromMoveNullBytesSanityCheck) {
		gk::String a = "hbb"_str;
		gk::String b = "ca"_str;
		a = std::move(b);
		EXPECT_EQ(a.len(), 2);
		EXPECT_EQ(a.usedBytes(), 2);
		EXPECT_EQ(a.cstr()[0], 'c');
		EXPECT_EQ(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			EXPECT_EQ(a.cstr()[i], '\0');
		}
	}

	COMPTIME_TEST(String, AssignFromMoveNullBytesSanityCheck, {
		gk::String a = "hbb"_str;
		gk::String b = "ca"_str;
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

	TEST(String, EqualChar) {
		gk::String a = 'c';
		EXPECT_EQ(a, 'c');
	}

	COMPTIME_TEST(String, EqualChar, {
		gk::String a = 'c';
		comptimeAssertEq(a, 'c');
		});

	TEST(String, NotEqualChar) {
		gk::String a = 'b';
		EXPECT_NE(a, 'c');
	}

	COMPTIME_TEST(String, NotEqualChar, {
		gk::String a = 'b';
		comptimeAssertNe(a, 'c');
		});

	TEST(String, NotEqualCharSameFirst) {
		gk::String a = "ca"_str;
		EXPECT_NE(a, 'c');
	}

	COMPTIME_TEST(String, NotEqualCharSameFirst, {
		gk::String a = "ca"_str;
		comptimeAssertNe(a, 'c');
		});

	TEST(String, NotEqualCharAndLargeString) {
		gk::String a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
		EXPECT_NE(a, 'c');
	}

	COMPTIME_TEST(String, NotEqualCharAndLargeString, {
		gk::String a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
		comptimeAssertNe(a, 'c');
		});

#pragma endregion

#pragma region Equal_Str

	TEST(String, EqualSmallStr) {
		gk::String a = "hi"_str;
		EXPECT_EQ(a, "hi"_str);
	}

	COMPTIME_TEST(String, EqualSmallStr, {
		gk::String a = "hi"_str;
		comptimeAssertEq(a, "hi"_str);
		});

	TEST(String, EqualSsoMaxStr) {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_EQ(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	}

	COMPTIME_TEST(String, EqualSsoMaxStr, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertEq(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

	TEST(String, EqualLargeStr) {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_EQ(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	}

	COMPTIME_TEST(String, EqualLargeStr, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertEq(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

	TEST(String, EqualUtf8SmallStr) {
		gk::String a = "ßen"_str;
		EXPECT_EQ(a, "ßen"_str);
	}

	COMPTIME_TEST(String, EqualUtf8SmallStr, {
		gk::String a = "ßen"_str;
		comptimeAssertEq(a, "ßen"_str);
	});

	TEST(String, EqualUtf8LargeStr) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	}
	
	COMPTIME_TEST(String, EqualUtf8LargeStr, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

	TEST(String, NotEqualSmallStr) {
		gk::String a = "hh"_str;
		EXPECT_NE(a, "hi"_str);
	}

	COMPTIME_TEST(String, NotEqualSmallStr, {
		gk::String a = "hh"_str;
		comptimeAssertNe(a, "hi"_str);
	});

	TEST(String, NotEqualSsoMaxStr) {
		gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_NE(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	}

	COMPTIME_TEST(String, NotEqualSsoMaxStr, {
		gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertNe(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

	TEST(String, NotEqualLargeStr) {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_NE(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	}

	COMPTIME_TEST(String, NotEqualLargeStr, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertNe(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

	TEST(String, NotEqualUtf8Small) {
		gk::String a = "ßeb"_str;
		EXPECT_NE(a, "ßen"_str);
	}

	COMPTIME_TEST(String, NotEqualUtf8Small, {
		gk::String a = "ßeb"_str;
		comptimeAssertNe(a, "ßen"_str);
	});

	TEST(String, NotEqualUtf8Large) {
		gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		EXPECT_NE(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	}
	
	COMPTIME_TEST(String, NotEqualUtf8Large, {
		gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		comptimeAssertNe(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

#pragma endregion

#pragma region Equal_Other_String

	TEST(String, EqualCharOtherString) {
		gk::String a = 'c';
		EXPECT_EQ(a, gk::String('c'));
	}

	COMPTIME_TEST(String, EqualCharOtherString, {
		gk::String a = 'c';
		comptimeAssertEq(a, gk::String('c'));
	});

	TEST(String, EqualSmallOtherString) {
		gk::String a = "hi"_str;
		EXPECT_EQ(a, gk::String("hi"_str));
	}

	COMPTIME_TEST(String, EqualSmallOtherString, {
		gk::String a = "hi"_str;
		comptimeAssertEq(a, gk::String("hi"_str));
		});

	TEST(String, EqualSsoMaxOtherString) {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_EQ(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
	}

	COMPTIME_TEST(String, EqualSsoMaxOtherString, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertEq(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
		});

	TEST(String, EqualLargeOtherString) {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_EQ(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	}

	COMPTIME_TEST(String, EqualLargeOtherString, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertEq(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
		});

	TEST(String, EqualUtf8SmallOtherString) {
		gk::String a = "ßen"_str;
		EXPECT_EQ(a, gk::String("ßen"_str));
	}

	COMPTIME_TEST(String, EqualUtf8SmallOtherString, {
		gk::String a = "ßen"_str;
		comptimeAssertEq(a, gk::String("ßen"_str));
		});

	TEST(String, EqualUtf8LargeOtherString) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		EXPECT_EQ(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
	}

	COMPTIME_TEST(String, EqualUtf8LargeOtherString, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		comptimeAssertEq(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
		});

	TEST(String, NotEqualSmallStrOtherString) {
		gk::String a = "hh"_str;
		EXPECT_NE(a, gk::String("hi"_str));
	}

	COMPTIME_TEST(String, NotEqualSmallStrOtherString, {
		gk::String a = "hh"_str;
		comptimeAssertNe(a, gk::String("hi"_str));
		});

	TEST(String, NotEqualSsoMaxStrOtherString) {
		gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		EXPECT_NE(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
	}

	COMPTIME_TEST(String, NotEqualSsoMaxStrOtherString, {
		gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertNe(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
		});

	TEST(String, NotEqualLargeStrOtherString) {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		EXPECT_NE(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	}

	COMPTIME_TEST(String, NotEqualLargeStrOtherString, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertNe(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
		});

	TEST(String, NotEqualUtf8SmallOtherString) {
		gk::String a = "ßeb"_str;
		EXPECT_NE(a, gk::String("ßen"_str));
	}

	COMPTIME_TEST(String, NotEqualUtf8SmallOtherString, {
		gk::String a = "ßeb"_str;
		comptimeAssertNe(a, gk::String("ßen"_str));
		});

	TEST(String, NotEqualUtf8LargeOtherString) {
		gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		EXPECT_NE(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
	}

	COMPTIME_TEST(String, NotEqualUtf8LargeOtherString, {
		gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		comptimeAssertNe(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
		});

#pragma endregion

#pragma region Append

	TEST(String, EmptyStringAppendChar) {
		gk::String a;
		a.append('c');
		EXPECT_EQ(a, 'c');
		EXPECT_EQ(a, gk::String('c')); // for sanity, same with following tests
	}

	COMPTIME_TEST(String, EmptyStringAppendChar, {
		gk::String a;
		a.append('c');
		comptimeAssertEq(a, 'c');
		comptimeAssertEq(a, gk::String('c'));
		});

	TEST(String, SmallStringAppendChar) {
		gk::String a = "hello"_str;
		a.append('!');
		EXPECT_EQ(a, "hello!"_str);
		EXPECT_EQ(a, gk::String("hello!"_str));
	}

	COMPTIME_TEST(String, SmallStringAppendChar, {
		gk::String a = "hello"_str;
		a.append('!');
		comptimeAssertEq(a, "hello!"_str);
		comptimeAssertEq(a, gk::String("hello!"_str));
	});

	TEST(String, SmallStringAppendCharMakeHeap) {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append('!');
		EXPECT_EQ(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
		EXPECT_EQ(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
	}

	COMPTIME_TEST(String, SmallStringAppendCharMakeHeap, {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append('!');
		comptimeAssertEq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
		comptimeAssertEq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
	});

	TEST(String, LargeStringAppendChar) {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append('a');
		EXPECT_EQ(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
		EXPECT_EQ(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723a"_str));
	}

	COMPTIME_TEST(String, LargeStringAppendChar, {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append('a');
		comptimeAssertEq(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
		comptimeAssertEq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723a"_str));
	});

	TEST(String, SmallUtf8AppendChar) {
		gk::String a = "ßeb"_str;
		a.append('?');
		EXPECT_EQ(a, "ßeb?"_str);
		EXPECT_EQ(a, gk::String("ßeb?"_str));
	}

	COMPTIME_TEST(String, SmallUtf8AppendChar, {
		gk::String a = "ßeb"_str;
		a.append('?');
		comptimeAssertEq(a, "ßeb?"_str);
		comptimeAssertEq(a, gk::String("ßeb?"_str));
	});

	TEST(String, SmallUtf8AppendCharMakeHeap) {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append('l');
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergröal"_str);
		EXPECT_EQ(a, gk::String("ÜbergrößenträgerÜbergröal"_str));
	}

	COMPTIME_TEST(String, SmallUtf8AppendCharMakeHeap, {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append('l');
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergröal"_str);
		comptimeAssertEq(a, gk::String("ÜbergrößenträgerÜbergröal"_str));
	});

	TEST(String, AppendCharHeapReallocate) {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append('5');
		EXPECT_EQ(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
		EXPECT_EQ(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
	}

	COMPTIME_TEST(String, AppendCharHeapReallocate, {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append('5');
		comptimeAssertEq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
		comptimeAssertEq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
	});

#pragma endregion

#pragma region Append_Str

	TEST(String, EmptyStringAppendStr) {
		gk::String a;
		a.append("cc"_str);
		EXPECT_EQ(a, "cc"_str);
		EXPECT_EQ(a, gk::String("cc"_str)); // for sanity, same with following tests
	}

	COMPTIME_TEST(String, EmptyStringAppendStr, {
		gk::String a;
		a.append("cc"_str);
		comptimeAssertEq(a, "cc"_str);
		comptimeAssertEq(a, gk::String("cc"_str));
		});

	TEST(String, SmallStringAppendStr) {
		gk::String a = "hello"_str;
		a.append("!!"_str);
		EXPECT_EQ(a, "hello!!"_str);
		EXPECT_EQ(a, gk::String("hello!!"_str));
	}

	COMPTIME_TEST(String, SmallStringAppendStr, {
		gk::String a = "hello"_str;
		a.append("!!"_str);
		comptimeAssertEq(a, "hello!!"_str);
		comptimeAssertEq(a, gk::String("hello!!"_str));
		});

	TEST(String, SmallStringAppendStrMakeHeap) {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append("!!"_str);
		EXPECT_EQ(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		EXPECT_EQ(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	}

	COMPTIME_TEST(String, SmallStringAppendStrMakeHeap, {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append("!!"_str);
		comptimeAssertEq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		comptimeAssertEq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
		});

	TEST(String, LargeStringAppendStr) {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append("aa"_str);
		EXPECT_EQ(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		EXPECT_EQ(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	}

	COMPTIME_TEST(String, LargeStringAppendStr, {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append("aa"_str);
		comptimeAssertEq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		comptimeAssertEq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
		});

	TEST(String, SmallUtf8AppendStr) {
		gk::String a = "ßeb"_str;
		a.append("??"_str);
		EXPECT_EQ(a, "ßeb??"_str);
		EXPECT_EQ(a, gk::String("ßeb??"_str));
	}

	COMPTIME_TEST(String, SmallUtf8AppendStr, {
		gk::String a = "ßeb"_str;
		a.append("??"_str);
		comptimeAssertEq(a, "ßeb??"_str);
		comptimeAssertEq(a, gk::String("ßeb??"_str));
		});

	TEST(String, SmallUtf8AppendStrMakeHeap) {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append("ll"_str);
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergröall"_str);
		EXPECT_EQ(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
	}

	COMPTIME_TEST(String, SmallUtf8AppendStrMakeHeap, {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append("ll"_str);
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergröall"_str);
		comptimeAssertEq(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
		});

	TEST(String, AppendStrHeapReallocate) {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append("55"_str);
		EXPECT_EQ(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		EXPECT_EQ(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	}

	COMPTIME_TEST(String, AppendStrHeapReallocate, {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append("55"_str);
		comptimeAssertEq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		comptimeAssertEq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
		});

#pragma endregion

#pragma region Append_Other_String

	TEST(String, EmptyStringAppendOtherString) {
		gk::String a;
		a.append(gk::String("cc"_str));
		EXPECT_EQ(a, "cc"_str);
		EXPECT_EQ(a, gk::String("cc"_str)); // for sanity, same with following tests
	}

	COMPTIME_TEST(String, EmptyStringAppendOtherString, {
		gk::String a;
		a.append(gk::String("cc"_str));
		comptimeAssertEq(a, "cc"_str);
		comptimeAssertEq(a, gk::String("cc"_str));
		});

	TEST(String, SmallStringAppendOtherString) {
		gk::String a = "hello"_str;
		a.append(gk::String("!!"_str));
		EXPECT_EQ(a, "hello!!"_str);
		EXPECT_EQ(a, gk::String("hello!!"_str));
	}

	COMPTIME_TEST(String, SmallStringAppendOtherString, {
		gk::String a = "hello"_str;
		a.append(gk::String("!!"_str));
		comptimeAssertEq(a, "hello!!"_str);
		comptimeAssertEq(a, gk::String("hello!!"_str));
		});

	TEST(String, SmallStringAppendOtherStringMakeHeap) {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append(gk::String("!!"_str));
		EXPECT_EQ(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		EXPECT_EQ(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	}

	COMPTIME_TEST(String, SmallStringAppendOtherStringMakeHeap, {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append(gk::String("!!"_str));
		comptimeAssertEq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		comptimeAssertEq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
		});

	TEST(String, LargeStringAppendOtherString) {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append(gk::String("aa"_str));
		EXPECT_EQ(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		EXPECT_EQ(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	}

	COMPTIME_TEST(String, LargeStringAppendOtherString, {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append(gk::String("aa"_str));
		comptimeAssertEq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		comptimeAssertEq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
		});

	TEST(String, SmallUtf8AppendOtherString) {
		gk::String a = "ßeb"_str;
		a.append(gk::String("??"_str));
		EXPECT_EQ(a, "ßeb??"_str);
		EXPECT_EQ(a, gk::String("ßeb??"_str));
	}

	COMPTIME_TEST(String, SmallUtf8AppendOtherString, {
		gk::String a = "ßeb"_str;
		a.append(gk::String("??"_str));
		comptimeAssertEq(a, "ßeb??"_str);
		comptimeAssertEq(a, gk::String("ßeb??"_str));
		});

	TEST(String, SmallUtf8AppendOtherStringMakeHeap) {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append(gk::String("ll"_str));
		EXPECT_EQ(a, "ÜbergrößenträgerÜbergröall"_str);
		EXPECT_EQ(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
	}

	COMPTIME_TEST(String, SmallUtf8AppendOtherStringMakeHeap, {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append(gk::String("ll"_str));
		comptimeAssertEq(a, "ÜbergrößenträgerÜbergröall"_str);
		comptimeAssertEq(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
		});

	TEST(String, AppendOtherStringHeapReallocate) {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append(gk::String("55"_str));
		EXPECT_EQ(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		EXPECT_EQ(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	}

	COMPTIME_TEST(String, AppendOtherStringHeapReallocate, {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append(gk::String("55"_str));
		comptimeAssertEq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		comptimeAssertEq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
		});

#pragma endregion

#pragma region Concat_Char

	TEST(String, ConcatEmptyAndChar) {
		const gk::String a;
		gk::String b = a + 'c';
		EXPECT_EQ(b, 'c');
		EXPECT_EQ(b, gk::String('c'));
	}

	COMPTIME_TEST(String, ConcatEmptyAndChar, {
		const gk::String a;
		gk::String b = a + 'c';
		comptimeAssertEq(b, 'c');
		comptimeAssertEq(b, gk::String('c'));
		});

	TEST(String, ConcatCharStringAndChar) {
		const gk::String a = 'c';
		gk::String b = a + 'c';
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::String("cc"_str));
	}

	COMPTIME_TEST(String, ConcatCharStringAndChar, {
		const gk::String a = 'c';
		gk::String b = a + 'c';
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::String("cc"_str));
	});

	TEST(String, ConcatSmallStringAndCharToHeap) {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + 'c';
		EXPECT_EQ(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
		EXPECT_EQ(b, gk::String("aslasdasddkjahldkjahsldkjahsdac"_str));
	}

	COMPTIME_TEST(String, ConcatSmallStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + 'c';
		comptimeAssertEq(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
		comptimeAssertEq(b, gk::String("aslasdasddkjahldkjahsldkjahsdac"_str));
	});

	TEST(String, ConcatHeapStringAndCharToHeap) {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + 'c';
		EXPECT_EQ(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
		EXPECT_EQ(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdac"_str));
	}

	COMPTIME_TEST(String, ConcatHeapStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + 'c';
		comptimeAssertEq(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
		comptimeAssertEq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdac"_str));
	});

	TEST(String, ConcatSmallUtf8AndChar) {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + 'c';
		EXPECT_EQ(b, "Übergrößenträgerc"_str);
		EXPECT_EQ(b, gk::String("Übergrößenträgerc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallUtf8AndChar, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + 'c';
		comptimeAssertEq(b, "Übergrößenträgerc"_str);
		comptimeAssertEq(b, gk::String("Übergrößenträgerc"_str));
	});

	TEST(String, ConcatSmallUtf8AndCharToHeap) {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + 'c';
		EXPECT_EQ(b, "Übergrößenträgerasjhdgashhc"_str);
		EXPECT_EQ(b, gk::String("Übergrößenträgerasjhdgashhc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallUtf8AndCharToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + 'c';
		comptimeAssertEq(b, "Übergrößenträgerasjhdgashhc"_str);
		comptimeAssertEq(b, gk::String("Übergrößenträgerasjhdgashhc"_str));
	});

	TEST(String, ConcatHeapUtf8AndChar) {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + 'c';
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
		EXPECT_EQ(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
	}

	COMPTIME_TEST(String, ConcatHeapUtf8AndChar, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + 'c';
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
		comptimeAssertEq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
	});

#pragma endregion

#pragma region Concat_Char_Inverted

	TEST(String, InvertConcatEmptyAndChar) {
		const gk::String a;
		gk::String b = 'c' + a;
		EXPECT_EQ(b, 'c');
		EXPECT_EQ(b, gk::String('c'));
	}

	COMPTIME_TEST(String, InvertConcatEmptyAndChar, {
		const gk::String a;
		gk::String b = 'c' + a;
		comptimeAssertEq(b, 'c');
		comptimeAssertEq(b, gk::String('c'));
		});

	TEST(String, InvertConcatCharStringAndChar) {
		const gk::String a = 'c';
		gk::String b = 'c' + a;
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::String("cc"_str));
	}

	COMPTIME_TEST(String, InvertConcatCharStringAndChar, {
		const gk::String a = 'c';
		gk::String b = 'c' + a;
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::String("cc"_str));
	});

	TEST(String, InvertConcatSmallStringAndCharToHeap) {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = 'c' + a;
		EXPECT_EQ(b, "caslasdasddkjahldkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::String("caslasdasddkjahldkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(String, InvertConcatSmallStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = 'c' + a;
		comptimeAssertEq(b, "caslasdasddkjahldkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::String("caslasdasddkjahldkjahsldkjahsda"_str));
	});

	TEST(String, InvertConcatHeapStringAndCharToHeap) {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = 'c' + a;
		EXPECT_EQ(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::String("caslasdasddkjahl55dkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(String, InvertConcatHeapStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = 'c' + a;
		comptimeAssertEq(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::String("caslasdasddkjahl55dkjahsldkjahsda"_str));
	});

	TEST(String, InvertConcatSmallUtf8AndChar) {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = 'c' + a;
		EXPECT_EQ(b, "cÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::String("cÜbergrößenträger"_str));
	}

	COMPTIME_TEST(String, InvertConcatSmallUtf8AndChar, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = 'c' + a;
		comptimeAssertEq(b, "cÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::String("cÜbergrößenträger"_str));
	});

	TEST(String, InvertConcatSmallUtf8AndCharToHeap) {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = 'c' + a;
		EXPECT_EQ(b, "cÜbergrößenträgerasjhdgashh"_str);
		EXPECT_EQ(b, gk::String("cÜbergrößenträgerasjhdgashh"_str));
	}

	COMPTIME_TEST(String, InvertConcatSmallUtf8AndCharToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = 'c' + a;
		comptimeAssertEq(b, "cÜbergrößenträgerasjhdgashh"_str);
		comptimeAssertEq(b, gk::String("cÜbergrößenträgerasjhdgashh"_str));
	});

	TEST(String, InvertConcatHeapUtf8AndChar) {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = 'c' + a;
		EXPECT_EQ(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::String("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	}

	COMPTIME_TEST(String, InvertConcatHeapUtf8AndChar, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = 'c' + a;
		comptimeAssertEq(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::String("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	});

#pragma endregion

#pragma region Concat_Str

	TEST(String, ConcatEmptyAndStr) {
		const gk::String a;
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::String("cc"_str));
	}

	COMPTIME_TEST(String, ConcatEmptyAndStr, {
		const gk::String a;
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::String("cc"_str));
		});

	TEST(String, ConcatCharStringAndStr) {
		const gk::String a = 'c';
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "ccc"_str);
		EXPECT_EQ(b, gk::String("ccc"_str));
	}

	COMPTIME_TEST(String, ConcatCharStringAndStr, {
		const gk::String a = 'c';
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "ccc"_str);
		comptimeAssertEq(b, gk::String("ccc"_str));
		});

	TEST(String, ConcatSmallStringAndStrToHeap) {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
		});

	TEST(String, ConcatHeapStringAndStrToHeap) {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(String, ConcatHeapStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
		});

	TEST(String, ConcatSmallUtf8AndStr) {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "Übergrößenträgercc"_str);
		EXPECT_EQ(b, gk::String("Übergrößenträgercc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallUtf8AndStr, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "Übergrößenträgercc"_str);
		comptimeAssertEq(b, gk::String("Übergrößenträgercc"_str));
		});

	TEST(String, ConcatSmallUtf8AndStrToHeap) {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "Übergrößenträgerasjhdgashhcc"_str);
		EXPECT_EQ(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallUtf8AndStrToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "Übergrößenträgerasjhdgashhcc"_str);
		comptimeAssertEq(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
		});

	TEST(String, ConcatHeapUtf8AndStr) {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + "cc"_str;
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		EXPECT_EQ(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	}

	COMPTIME_TEST(String, ConcatHeapUtf8AndStr, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + "cc"_str;
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		comptimeAssertEq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
		});

#pragma endregion

#pragma region Concat_Str_Inverted

	TEST(String, InvertConcatEmptyAndStr) {
		const gk::String a;
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::String("cc"_str));
	}

	COMPTIME_TEST(String, InvertConcatEmptyAndStr, {
		const gk::String a;
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::String("cc"_str));
		});

	TEST(String, InvertConcatCharStringAndStr) {
		const gk::String a = 'c';
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "ccc"_str);
		EXPECT_EQ(b, gk::String("ccc"_str));
	}

	COMPTIME_TEST(String, InvertConcatCharStringAndStr, {
		const gk::String a = 'c';
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "ccc"_str);
		comptimeAssertEq(b, gk::String("ccc"_str));
		});

	TEST(String, InvertConcatSmallStringAndStrToHeap) {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::String("ccaslasdasddkjahldkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(String, InvertConcatSmallStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::String("ccaslasdasddkjahldkjahsldkjahsda"_str));
		});

	TEST(String, InvertConcatHeapStringAndStrToHeap) {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
		EXPECT_EQ(b, gk::String("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
	}

	COMPTIME_TEST(String, InvertConcatHeapStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
		comptimeAssertEq(b, gk::String("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
		});

	TEST(String, InvertConcatSmallUtf8AndStr) {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "ccÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::String("ccÜbergrößenträger"_str));
	}

	COMPTIME_TEST(String, InvertConcatSmallUtf8AndStr, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "ccÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::String("ccÜbergrößenträger"_str));
		});

	TEST(String, InvertConcatSmallUtf8AndStrToHeap) {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "ccÜbergrößenträgerasjhdgashh"_str);
		EXPECT_EQ(b, gk::String("ccÜbergrößenträgerasjhdgashh"_str));
	}

	COMPTIME_TEST(String, InvertConcatSmallUtf8AndStrToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "ccÜbergrößenträgerasjhdgashh"_str);
		comptimeAssertEq(b, gk::String("ccÜbergrößenträgerasjhdgashh"_str));
		});

	TEST(String, InvertConcatHeapUtf8AndStr) {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = "cc"_str + a;
		EXPECT_EQ(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		EXPECT_EQ(b, gk::String("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	}

	COMPTIME_TEST(String, InvertConcatHeapUtf8AndStr, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = "cc"_str + a;
		comptimeAssertEq(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		comptimeAssertEq(b, gk::String("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
		});

#pragma endregion

#pragma region Concat_Two_Strings

	TEST(String, ConcatEmptyAndOtherString) {
		const gk::String a;
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "cc"_str);
		EXPECT_EQ(b, gk::String("cc"_str));
	}

	COMPTIME_TEST(String, ConcatEmptyAndOtherString, {
		const gk::String a;
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "cc"_str);
		comptimeAssertEq(b, gk::String("cc"_str));
		});

	TEST(String, ConcatCharStringAndOtherString) {
		const gk::String a = 'c';
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "ccc"_str);
		EXPECT_EQ(b, gk::String("ccc"_str));
	}

	COMPTIME_TEST(String, ConcatCharStringAndOtherString, {
		const gk::String a = 'c';
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "ccc"_str);
		comptimeAssertEq(b, gk::String("ccc"_str));
		});

	TEST(String, ConcatSmallStringAndOtherStringToHeap) {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallStringAndOtherStringToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
		});

	TEST(String, ConcatHeapStringAndOtherStringToHeap) {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		EXPECT_EQ(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	}

	COMPTIME_TEST(String, ConcatHeapStringAndOtherStringToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		comptimeAssertEq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
		});

	TEST(String, ConcatSmallUtf8AndOtherString) {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "Übergrößenträgercc"_str);
		EXPECT_EQ(b, gk::String("Übergrößenträgercc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallUtf8AndOtherString, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "Übergrößenträgercc"_str);
		comptimeAssertEq(b, gk::String("Übergrößenträgercc"_str));
		});

	TEST(String, ConcatSmallUtf8AndOtherStringToHeap) {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "Übergrößenträgerasjhdgashhcc"_str);
		EXPECT_EQ(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
	}

	COMPTIME_TEST(String, ConcatSmallUtf8AndOtherStringToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "Übergrößenträgerasjhdgashhcc"_str);
		comptimeAssertEq(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
		});

	TEST(String, ConcatHeapUtf8AndOtherString) {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + gk::String("cc"_str);
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		EXPECT_EQ(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	}

	COMPTIME_TEST(String, ConcatHeapUtf8AndOtherString, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + gk::String("cc"_str);
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		comptimeAssertEq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
		});

#pragma endregion

#pragma region Concat_Multiple

	TEST(String, ChainConcat) {
		gk::String a = "hello world!"_str;
		gk::String b = a + ' ' + "hmm"_str + " t" + 'h' + gk::String("is is") + " a multi concat string thats quite large"_str;
		EXPECT_EQ(b, "hello world! hmm this is a multi concat string thats quite large"_str);
		EXPECT_EQ(b, gk::String("hello world! hmm this is a multi concat string thats quite large"_str));
	}

	COMPTIME_TEST(String, ChainConcat, {
		gk::String a = "hello world!"_str;
		gk::String b = a + ' ' + "hmm"_str + " t" + 'h' + gk::String("is is") + " a multi concat string thats quite large"_str;
		comptimeAssertEq(b, "hello world! hmm this is a multi concat string thats quite large"_str);
		comptimeAssertEq(b, gk::String("hello world! hmm this is a multi concat string thats quite large"_str));
	});

#pragma endregion

#pragma region From_Type

	TEST(String, FromBoolTrue) {
		gk::String a = gk::String::fromBool(true);
		EXPECT_EQ(a, "true"_str);
		EXPECT_EQ(a, gk::String("true"_str));
	}

	COMPTIME_TEST(String, FromBoolTrue, {
		gk::String a = gk::String::fromBool(true);
		comptimeAssertEq(a, "true"_str);
		comptimeAssertEq(a, gk::String("true"_str));
		});

	TEST(String, FromBoolFalse) {
		gk::String a = gk::String::fromBool(false);
		EXPECT_EQ(a, "false"_str);
		EXPECT_EQ(a, gk::String("false"_str));
	}

	COMPTIME_TEST(String, FromBoolFalse, {
		gk::String a = gk::String::fromBool(false);
		comptimeAssertEq(a, "false"_str);
		comptimeAssertEq(a, gk::String("false"_str));
	});

	TEST(String, FromSignedIntZero) {
		gk::String a = gk::String::fromInt(0);
		EXPECT_EQ(a, '0');
	}

	COMPTIME_TEST(String, FromSignedIntZero, {
		gk::String a = gk::String::fromInt(0);
		comptimeAssertEq(a, '0');
	});

	TEST(String, FromSignedIntSmallValue) {
		gk::String a = gk::String::fromInt(16);
		EXPECT_EQ(a, "16"_str);
	}

	COMPTIME_TEST(String, FromSignedIntSmallValue, {
		gk::String a = gk::String::fromInt(16);
		comptimeAssertEq(a, "16"_str);
	});

	TEST(String, FromSignedIntMaximumValue) {
		gk::String a = gk::String::fromInt(MAXINT64);
		EXPECT_EQ(a, "9223372036854775807"_str);
	}

	COMPTIME_TEST(String, FromSignedIntMaximumValue, {
		gk::String a = gk::String::fromInt(MAXINT64);
		comptimeAssertEq(a, "9223372036854775807"_str);
	});

	TEST(String, FromSignedIntSmallNegativeValue) {
		gk::String a = gk::String::fromInt(-3);
		EXPECT_EQ(a, "-3"_str);
	}

	COMPTIME_TEST(String, FromSignedIntSmallNegativeValue, {
		gk::String a = gk::String::fromInt(-3);
		comptimeAssertEq(a, "-3"_str);
	});

	TEST(String, FromSignedIntMinimumValue) {
		gk::String a = gk::String::fromInt(MININT64);
		EXPECT_EQ(a, "-9223372036854775808"_str);
	}

	COMPTIME_TEST(String, FromSignedIntMinimumValue, {
		gk::String a = gk::String::fromInt(MININT64);
		comptimeAssertEq(a, "-9223372036854775808"_str);
	});

	TEST(String, FromUnsignedIntZero) {
		gk::String a = gk::String::fromUint(0);
		EXPECT_EQ(a, '0');
	}

	COMPTIME_TEST(String, FromUnsignedIntZero, {
		gk::String a = gk::String::fromUint(0);
		comptimeAssertEq(a, '0');
	});

	TEST(String, FromUnsignedIntSmallValue) {
		gk::String a = gk::String::fromUint(23);
		EXPECT_EQ(a, "23"_str);
	}

	COMPTIME_TEST(String, FromUnsignedIntSmallValue, {
		gk::String a = gk::String::fromUint(23);
		comptimeAssertEq(a, "23"_str);
	});

	TEST(String, FromUnsignedIntMaximumValue) {
		gk::String a = gk::String::fromUint(MAXUINT64);
		EXPECT_EQ(a, "18446744073709551615"_str);
	}

	COMPTIME_TEST(String, FromUnsignedIntMaximumValue, {
		gk::String a = gk::String::fromUint(MAXUINT64);
		comptimeAssertEq(a, "18446744073709551615"_str);
	});

	TEST(String, FromFloatZero) {
		gk::String a = gk::String::fromFloat(0.0);
		EXPECT_EQ(a, "0.0"_str);
	}

	COMPTIME_TEST(String, FromFloatZero, {
		gk::String a = gk::String::fromFloat(0.0);
		comptimeAssertEq(a, "0.0"_str);
	});

	TEST(String, FromFloatPositiveInfinity) {
		gk::String a = gk::String::fromFloat(INFINITY);
		EXPECT_EQ(a, "inf"_str);
	}

	TEST(String, FromFloatNegativeInfinity) {
		gk::String a = gk::String::fromFloat(-1.0 * INFINITY);
		EXPECT_EQ(a, "-inf"_str);
	}

	TEST(String, FromFloatNaN) {
		gk::String a = gk::String::fromFloat(NAN);
		EXPECT_EQ(a, "nan"_str);
	}

	TEST(String, FromFloatWholeNumber) {
		gk::String a = gk::String::fromFloat(100.0);
		EXPECT_EQ(a, "100.0"_str);
	}

	COMPTIME_TEST(String, FromFloatWholeNumber, {
		gk::String a = gk::String::fromFloat(100.0);
		comptimeAssertEq(a, "100.0"_str);
	});

	TEST(String, FromFloatWholeNegativeNumber) {
		gk::String a = gk::String::fromFloat(-100.0);
		EXPECT_EQ(a, "-100.0"_str);
	}

	COMPTIME_TEST(String, FromFloatWholeNegativeNumber, {
		gk::String a = gk::String::fromFloat(-100.0);
		comptimeAssertEq(a, "-100.0"_str);
	});

	TEST(String, FromFloatDecimalNumber) {
		gk::String a = gk::String::fromFloat(100.09999);
		EXPECT_EQ(a, "100.09999"_str);
	}

	COMPTIME_TEST(String, FromFloatDecimalNumber, {
		gk::String a = gk::String::fromFloat(100.09999);
		comptimeAssertEq(a, "100.09999"_str);
	});

	TEST(String, FromFloatDecimalNegativeNumber) {
		gk::String a = gk::String::fromFloat(-100.09999);
		EXPECT_EQ(a, "-100.09999"_str);
	}

	COMPTIME_TEST(String, FromFloatDecimalNegativeNumber, {
		gk::String a = gk::String::fromFloat(-100.09999);
		comptimeAssertEq(a, "-100.09999"_str);
	});

	TEST(String, FromFloatDecimalNumberDefaultPrecision) {
		gk::String a = gk::String::fromFloat(100.12000005);
		EXPECT_EQ(a, "100.12"_str);
	}

	COMPTIME_TEST(String, FromFloatDecimalNumberDefaultPrecision, {
		gk::String a = gk::String::fromFloat(100.12000005);
		comptimeAssertEq(a, "100.12"_str);
	});

	TEST(String, FromFloatDecimalNegativeNumberDefaultPrecision) {
		gk::String a = gk::String::fromFloat(-100.12000005);
		EXPECT_EQ(a, "-100.12"_str);
	}

	COMPTIME_TEST(String, FromFloatDecimalNegativeNumberDefaultPrecision, {
		gk::String a = gk::String::fromFloat(-100.12000005);
		comptimeAssertEq(a, "-100.12"_str);
	});

	TEST(String, FromFloatDecimalNumberCustomPrecision) {
		gk::String a = gk::String::fromFloat(100.12000005, 10);
		EXPECT_EQ(a, "100.12000005"_str);
	}

	COMPTIME_TEST(String, FromFloatDecimalNumberCustomPrecision, {
		gk::String a = gk::String::fromFloat(100.12000005, 10);
		comptimeAssertEq(a, "100.12000005"_str);
	});

	TEST(String, FromFloatDecimalNegativeNumberCustomPrecision) {
		gk::String a = gk::String::fromFloat(-100.12000005, 10);
		EXPECT_EQ(a, "-100.12000005"_str);
	}

	COMPTIME_TEST(String, FromFloatDecimalNegativeNumberCustomPrecision, {
		gk::String a = gk::String::fromFloat(-100.12000005, 10);
		comptimeAssertEq(a, "-100.12000005"_str);
	});

	TEST(String, FromTemplateBool) {
		bool b = true;
		gk::String a = gk::String::from(b);
		EXPECT_EQ(a, "true"_str);
	}

	COMPTIME_TEST(String, FromTemplateBool, {
		bool b = true;
		gk::String a = gk::String::from(b);
		comptimeAssertEq(a, "true"_str);
	});

	TEST(String, FromTemplateInt8) {
		int8 num = -56;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "-56"_str);
	}

	COMPTIME_TEST(String, FromTemplateInt8, {
		int8 num = -56;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "-56"_str);
	});

	TEST(String, FromTemplateUint8) {
		uint8 num = 56;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "56"_str);
	}

	COMPTIME_TEST(String, FromTemplateUint8, {
		uint8 num = 56;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "56"_str);
	});

	TEST(String, FromTemplateInt16) {
		int16 num = -1000;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "-1000"_str);
	}

	COMPTIME_TEST(String, FromTemplateInt16, {
		int16 num = -1000;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "-1000"_str);
	});

	TEST(String, FromTemplateUint16) {
		uint16 num = 1000;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "1000"_str);
	}

	COMPTIME_TEST(String, FromTemplateUint16, {
		uint16 num = 1000;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "1000"_str);
	});

	TEST(String, FromTemplateInt32) {
		int32 num = -99999;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "-99999"_str);
	}

	COMPTIME_TEST(String, FromTemplateInt32, {
		int32 num = -99999;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "-99999"_str);
	});

	TEST(String, FromTemplateUint32) {
		uint32 num = 99999;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "99999"_str);
	}

	COMPTIME_TEST(String, FromTemplateUint32, {
		uint32 num = 99999;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "99999"_str);
	});

	TEST(String, FromTemplateInt64) {
		int64 num = -123456789012345;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "-123456789012345"_str);
	}

	COMPTIME_TEST(String, FromTemplateInt64, {
		int64 num = -123456789012345;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "-123456789012345"_str);
	});

	TEST(String, FromTemplateUint64) {
		uint64 num = 123456789012345;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "123456789012345"_str);
	}

	COMPTIME_TEST(String, FromTemplateUint64, {
		uint64 num = 123456789012345;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "123456789012345"_str);
	});

	TEST(String, FromTemplateFloat32) {
		float num = -123.45f;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "-123.44999"_str); // slightly imprecise
	}

	COMPTIME_TEST(String, FromTemplateFloat32, {
		float num = -123.45f;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "-123.44999"_str); // slightly imprecise
	});

	TEST(String, FromTemplateFloat64) {
		double num = -123.45;
		gk::String a = gk::String::from(num);
		EXPECT_EQ(a, "-123.45"_str);
	}

	COMPTIME_TEST(String, FromTemplateFloat64, {
		double num = -123.45;
		gk::String a = gk::String::from(num);
		comptimeAssertEq(a, "-123.45"_str);
	});

	TEST(String, FromTemplateCustomType) {
		StringTestExample e;
		e.a = 1.0;
		e.b = 1;
		gk::String a = gk::String::from(e);
		EXPECT_EQ(a, "1.0, 1"_str);
	}

	COMPTIME_TEST(String, FromTemplateCustomType, {
		StringTestExample e;
		e.a = 1.0;
		e.b = 1;
		gk::String a = gk::String::from(e);
		comptimeAssertEq(a, "1.0, 1"_str);
	});

#pragma endregion

#pragma region Format

	TEST(String, FormatOneArg) {
		int num = 4;
		gk::String a = gk::String::format<"num: {}">(num);
		EXPECT_EQ(a, "num: 4"_str);
	}

	COMPTIME_TEST(String, FormatOneArg, {
		int num = 4;
		gk::String a = gk::String::format<"num: {}">(num);
		comptimeAssertEq(a, "num: 4"_str);
	});

	TEST(String, FormatOneArgWithTextAfter) {
		float num = 4.f;
		gk::String a = gk::String::format<"num: {}... cool!">(num);
		EXPECT_EQ(a, "num: 4.0... cool!"_str);
	}

	COMPTIME_TEST(String, FormatOneArgWithTextAfter, {
		float num = 4.f;
		gk::String a = gk::String::format<"num: {}... cool!">(num);
		comptimeAssertEq(a, "num: 4.0... cool!"_str);
	});

	TEST(String, FormatTwoArgs) {
		int num1 = 5;
		float num2 = 5;
		gk::String a = gk::String::format<"num1: {}, num2: {}">(num1, num2);
		EXPECT_EQ(a, "num1: 5, num2: 5.0"_str);
	}

	COMPTIME_TEST(String, FormatTwoArgs, {
		int num1 = 5;
		float num2 = 5;
		gk::String a = gk::String::format<"num1: {}, num2: {}">(num1, num2);
		comptimeAssertEq(a, "num1: 5, num2: 5.0"_str);
	});

	TEST(String, FormatTwoArgsWithOperation) {
		int num1 = 5;
		float num2 = 5;
		gk::String a = gk::String::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
		EXPECT_EQ(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
	}

	COMPTIME_TEST(String, FormatTwoArgsWithOperation, {
		int num1 = 5;
		float num2 = 5;
		gk::String a = gk::String::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
		comptimeAssertEq(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
	});

	TEST(String, FormatFromCustomType) {
		StringTestExample e;
		e.a = -1.2;
		e.b = 5;
		int count = 2;
		gk::String a = gk::String::format<"the {} numbers are {}">(count, e);
		EXPECT_EQ(a, "the 2 numbers are -1.19999, 5"_str);
	}

	COMPTIME_TEST(String, FormatFromCustomType, {
		StringTestExample e;
		e.a = -1.2;
		e.b = 5;
		int count = 2;
		gk::String a = gk::String::format<"the {} numbers are {}">(count, e);
		comptimeAssertEq(a, "the 2 numbers are -1.19999, 5"_str);
	});

#pragma endregion

#pragma region Find_Char

	TEST(String, FindCharInSso) {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
	}

	COMPTIME_TEST(String, FindCharInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
	});

	TEST(String, FindCharInHeap) {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 81);
	}

	COMPTIME_TEST(String, FindCharInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('5');
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 81);
	});

	TEST(String, NotFindCharInSso) {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	}

	COMPTIME_TEST(String, NotFindCharInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	});

	TEST(String, NotFindCharInHeap) {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	}		

	COMPTIME_TEST(String, NotFindCharInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find('6');
		comptimeAssert(opt.none());
	});


#pragma endregion

#pragma region Find_Str

	TEST(String, FindStrInSso) {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 21);
	}

	COMPTIME_TEST(String, FindStrInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
		});

	TEST(String, FindStrInHeap) {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 83);
	}

	COMPTIME_TEST(String, FindStrInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find("5a"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 83);
		});

	TEST(String, FindUtf8StrInSso) {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßen"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 9);
	}

	COMPTIME_TEST(String, FindUtf8StrInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßen"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 9);
		});

	TEST(String, FindUtf8StrInHeap) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("6Übe"_str);
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 141);
	}

	COMPTIME_TEST(String, FindUtf8StrInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("6Übe"_str);
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 141);
	});

	TEST(String, NotFindStrInSso) {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("ya"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(String, NotFindStrInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find("ya"_str);
		comptimeAssert(opt.none());
		});

	TEST(String, NotFindStrInHeap) {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(";5"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(String, NotFindStrInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(";5"_str);
		comptimeAssert(opt.none());
		});

	TEST(String, NotFindUtf8StrInSso) {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßet"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(String, NotFindUtf8StrInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("ßet"_str);
		comptimeAssert(opt.none());
	});

	TEST(String, NotFindUtf8StrInHeap) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("5Üba"_str);
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(String, NotFindUtf8StrInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find("5Üba"_str);
		comptimeAssert(opt.none());
	});

#pragma endregion

#pragma region Find_Other_String

	TEST(String, FindOtherStringInSso) {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
	}

	COMPTIME_TEST(String, FindOtherStringInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 21);
		});

	TEST(String, FindOtherStringInHeap) {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 83);
	}

	COMPTIME_TEST(String, FindOtherStringInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("5a"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 83);
		});

	TEST(String, FindUtf8OtherStringInSso) {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("ßen"_str));
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 9);
	}

	COMPTIME_TEST(String, FindUtf8OtherStringInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("ßen"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 9);
		});

	TEST(String, FindUtf8OtherStringInHeap) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("6Übe"_str));
		ASSERT_FALSE(opt.none());
		ASSERT_EQ(opt.some(), 141);
	}

	COMPTIME_TEST(String, FindUtf8OtherStringInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("6Übe"_str));
		comptimeAssert(opt.none() == false);
		comptimeAssertEq(opt.some(), 141);
		});

	TEST(String, NotFindOtherStringInSso) {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("ya"_str));
		comptimeAssert(opt.none());
	}

	COMPTIME_TEST(String, NotFindOtherStringInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("ya"_str));
		comptimeAssert(opt.none());
		});

	TEST(String, NotFindOtherStringInHeap) {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String(";5"_str));
		comptimeAssert(opt.none());
	}

	COMPTIME_TEST(String, NotFindOtherStringInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String(";5"_str));
		comptimeAssert(opt.none());
		});

	TEST(String, NotFindUtf8OtherStringInSso) {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("ßet"_str));
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(String, NotFindUtf8OtherStringInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("ßet"_str));
		comptimeAssert(opt.none());
		});

	TEST(String, NotFindUtf8OtherStringInHeap) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("5Üba"_str));
		ASSERT_TRUE(opt.none());
	}

	COMPTIME_TEST(String, NotFindUtf8OtherStringInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::StringIndex> opt = a.find(gk::String("5Üba"_str));
		comptimeAssert(opt.none());
		});

#pragma endregion

#pragma region Substring

	TEST(String, SubstringSsoStartingFromBeginning) {
		gk::String a = "Übergrößenträger"_str;
		gk::String b = a.substring(0, 12);
		EXPECT_EQ(b, "Übergröße"_str);
	}

	COMPTIME_TEST(String, SubstringSsoStartingFromBeginning, {
		gk::String a = "Übergrößenträger"_str;
		gk::String b = a.substring(0, 12);
		comptimeAssertEq(b, "Übergröße"_str);
		});

	TEST(String, SubstringSsoStartingFromOffset) {
		gk::String a = "Übergrößenträger"_str;
		gk::String b = a.substring(2, 12);
		EXPECT_EQ(b, "bergröße"_str);
	}

	COMPTIME_TEST(String, SubstringSsoStartingFromOffset, {
		gk::String a = "Übergrößenträger"_str;
		gk::String b = a.substring(2, 12);
		comptimeAssertEq(b, "bergröße"_str);
	});

	TEST(String, SubstringHeapToSsoStartingFromBeginning) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(0, 20);
		EXPECT_EQ(b, "Übergrößenträger"_str);
	}

	COMPTIME_TEST(String, SubstringHeapToSsoStartingFromBeginning, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(0, 20);
		comptimeAssertEq(b, "Übergrößenträger"_str);
	});

	TEST(String, SubstringHeapToSsoStartingFromOffset) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(20, 40);
		EXPECT_EQ(b, "Übergrößenträger"_str);
	}

	COMPTIME_TEST(String, SubstringHeapToSsoStartingFromOffset, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(20, 40);
		comptimeAssertEq(b, "Übergrößenträger"_str);
	});

	TEST(String, SubstringHeapToHeapStartingFromBeginning) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(0, 40);
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträger"_str);
	}

	COMPTIME_TEST(String, SubstringHeapToHeapStartingFromBeginning, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(0, 40);
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträger"_str);
	});

	TEST(String, SubstringHeapToHeapStartingFromOffset) {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(20, 80);
		EXPECT_EQ(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	}

	COMPTIME_TEST(String, SubstringHeapToHeapStartingFromOffset, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(20, 80);
		comptimeAssertEq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	});

#pragma endregion
}