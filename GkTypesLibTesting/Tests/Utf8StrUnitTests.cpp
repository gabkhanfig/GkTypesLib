#include "../pch.h"
#include "../../GkTypesLib/GkTypes/String/Utf8.h"
#include "../../GkTypesLib/GkTypes/String/Str.h"
#include "../GkTest.h"

namespace UnitTests
{
	TEST(Utf8Str, StrlenLengthAscii) {
		gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
		ASSERT_EQ(metadata.length, 12);
	}

	TEST(Utf8Str, StrlenTotalBytesAscii) {
		gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
		ASSERT_EQ(metadata.totalBytes, 13);
	}

	TEST(Utf8Str, StrlenInvalidUtf8) {
		char buf[2];
		buf[0] = 255;
		buf[1] = '\0';
		auto result = gk::utf8::strlen(buf);
		ASSERT_TRUE(result.isError());
	}


}