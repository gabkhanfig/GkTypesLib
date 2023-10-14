#include "../pch.h"
#include "../../GkTypesLib/GkTypes/String/Utf8.h"
#include "../../GkTypesLib/GkTypes/String/Str.h"
#include "../GkTest.h"

namespace UnitTests
{
	TEST(Utf8, StrlenLengthAscii) {
		gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
		ASSERT_EQ(metadata.length, 12);
	}

	consteval uint64 CompileTimeUtf8_StrlenLengthAscii() {
		gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
		return metadata.length;
	}
	static_assert(CompileTimeUtf8_StrlenLengthAscii() == 12);

	TEST(Utf8, StrlenTotalBytesAscii) {
		gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
		ASSERT_EQ(metadata.totalBytes, 13);
	}

	consteval uint64 CompileTimeUtf8_StrlenTotalBytesAscii() {
		gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
		return metadata.totalBytes;
	}
	static_assert(CompileTimeUtf8_StrlenTotalBytesAscii() == 13);

	TEST(Utf8, StrlenLengthMultibyteCharacters) {
		gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
		ASSERT_EQ(metadata.length, 16);
	}

	consteval uint64 CompileTimeUtf8_StrlenLengthMultibyteCharacters() {
		gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
		return metadata.length;
	}
	static_assert(CompileTimeUtf8_StrlenLengthMultibyteCharacters() == 16);

	TEST(Utf8, StrlenTotalBytesMultibyteCharacters) {
		gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
		ASSERT_EQ(metadata.totalBytes, 21);
	}

	consteval uint64 CompileTimeUtf8_StrlenTotalBytesMultibyteCharacters() {
		gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
		return metadata.totalBytes;
	}
	static_assert(CompileTimeUtf8_StrlenTotalBytesMultibyteCharacters() == 21);

	TEST(Utf8, StrlenInvalidUtf8) {
		char buf[2];
		buf[0] = 255;
		buf[1] = '\0';
		auto result = gk::utf8::strlen(buf);
		ASSERT_TRUE(result.isError());
	}

	TEST(Str, LengthCompileTimeAscii) {
		gk::Str str = "hello world!";
		ASSERT_EQ(str.len, 12);
	}

	consteval uint64 CompileTimeStr_LengthAscii() {
		gk::Str str = "hello world!";
		return str.len;
	}
	static_assert(CompileTimeStr_LengthAscii() == 12);

	TEST(Str, TotalBytesCompileTimeAscii) {
		gk::Str str = "hello world!";
		ASSERT_EQ(str.totalBytes, 13);
	}

	consteval uint64 CompileTimeStr_TotalBytesAscii() {
		gk::Str str = "hello world!";
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesAscii() == 13);

	TEST(Str, LengthCompileTimeMultibyteCharacters) {
		gk::Str str = "Übergrößenträger";
		ASSERT_EQ(str.len, 16);
	}

	consteval uint64 CompileTimeStr_LengthMultibyteCharacters() {
		gk::Str str = "Übergrößenträger";
		return str.len;
	}
	static_assert(CompileTimeStr_LengthMultibyteCharacters() == 16);

	TEST(Str, TotalBytesCompileTimeMultibyteCharacters) {
		gk::Str str = "Übergrößenträger";
		ASSERT_EQ(str.totalBytes, 21);
	}

	consteval uint64 CompileTimeStr_TotalBytesMultibyteCharacters() {
		gk::Str str = "Übergrößenträger";
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesMultibyteCharacters() == 21);

	TEST(Str, LengthCompileTimeAsciiSuffix) {
		auto str = "hello world!"_str;
		ASSERT_EQ(str.len, 12);
	}

	consteval uint64 CompileTimeStr_LengthAsciiSuffix() {
		auto str = "hello world!"_str;
		return str.len;
	}
	static_assert(CompileTimeStr_LengthAsciiSuffix() == 12);

	TEST(Str, TotalBytesCompileTimeAsciiSuffix) {
		auto str = "hello world!"_str;
		ASSERT_EQ(str.totalBytes, 13);
	}

	consteval uint64 CompileTimeStr_TotalBytesAsciiSuffix() {
		auto str = "hello world!"_str;
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesAsciiSuffix() == 13);

	TEST(Str, LengthCompileTimeMultibyteCharactersSuffix) {
		auto str = "Übergrößenträger"_str;
		ASSERT_EQ(str.len, 16);
	}

	consteval uint64 CompileTimeStr_LengthMultibyteCharactersSuffix() {
		auto str = "Übergrößenträger"_str;
		return str.len;
	}
	static_assert(CompileTimeStr_LengthMultibyteCharactersSuffix() == 16);

	TEST(Str, TotalBytesCompileTimeMultibyteCharactersSuffix) {
		auto str = "Übergrößenträger"_str;
		ASSERT_EQ(str.totalBytes, 21);
	}

	consteval uint64 CompileTimeStr_TotalBytesMultibyteCharactersSuffix() {
		auto str = "Übergrößenträger"_str;
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesMultibyteCharactersSuffix() == 21);

	TEST(Str, LengthRuntimeAscii) {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text);
		ASSERT_EQ(str.len, 12);
	}

	consteval uint64 CompileTimeStr_LengthAsciiFrom() {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text);
		return str.len;
	}
	static_assert(CompileTimeStr_LengthAsciiFrom() == 12);

	TEST(Str, TotalBytesRuntimeAscii) {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text);
		ASSERT_EQ(str.totalBytes, 13);
	}

	consteval uint64 CompileTimeStr_TotalBytesAsciiFrom() {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text);
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesAsciiFrom() == 13);

	TEST(Str, LengthRuntimeAsciiKnownLength) {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text, 12);
		ASSERT_EQ(str.len, 12);
	}

	consteval uint64 CompileTimeStr_LengthAsciiFromKnownLength() {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text, 12);
		return str.len;
	}
	static_assert(CompileTimeStr_LengthAsciiFromKnownLength() == 12);

	TEST(Str, TotalBytesRuntimeAsciiKnownLength) {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text, 12);
		ASSERT_EQ(str.totalBytes, 13);
	}

	consteval uint64 CompileTimeStr_TotalBytesAsciiFromKnownLength() {
		const char* text = "hello world!";
		gk::Str str = gk::Str::fromAscii(text, 12);
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesAsciiFromKnownLength() == 13);

	TEST(Str, LengthRuntimeMultibyteCharacters) {
		const char* text = "Übergrößenträger";
		gk::Str str = gk::Str::fromUtf8(text).ok();
		ASSERT_EQ(str.len, 16);
	}

	consteval uint64 CompileTimeStr_LengthMultibyteCharactersFrom() {
		const char* text = "Übergrößenträger";
		gk::Str str = gk::Str::fromUtf8(text).ok();
		return str.len;
	}
	static_assert(CompileTimeStr_LengthMultibyteCharactersFrom() == 16);

	TEST(Str, TotalBytesRuntimeMultibyteCharacters) {
		const char* text = "Übergrößenträger";
		gk::Str str = gk::Str::fromUtf8(text).ok();
		ASSERT_EQ(str.totalBytes, 21);
	}

	consteval uint64 CompileTimeStr_TotalBytesMultibyteCharactersFrom() {
		const char* text = "Übergrößenträger";
		gk::Str str = gk::Str::fromUtf8(text).ok();
		return str.totalBytes;
	}
	static_assert(CompileTimeStr_TotalBytesMultibyteCharactersFrom() == 21);

	TEST(Str, RuntimeMultibyteCharacterInvalid) {
		char buf[2];
		buf[0] = 255;
		buf[1] = '\0';
		auto res = gk::Str::fromUtf8(buf);
		ASSERT_TRUE(res.isError());
	}
	
	TEST(Str, CopyConstruct) {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = str;
		EXPECT_EQ(str2.len, 16);
		EXPECT_EQ(str2.totalBytes, 21);
	}

	consteval bool CompileTimeStr_CopyConstruct() {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = str;
		if (str2.len != 16) return false;
		if (str2.totalBytes != 21) return false;
		return true;
	}
	static_assert(CompileTimeStr_CopyConstruct());
	
	TEST(Str, MoveConstruct) {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = std::move(str);
		EXPECT_EQ(str2.len, 16);
		EXPECT_EQ(str2.totalBytes, 21);
	}

	consteval bool CompileTimeStr_MoveConstruct() {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = std::move(str);
		if (str2.len != 16) return false;
		if (str2.totalBytes != 21) return false;
		return true;
	}
	static_assert(CompileTimeStr_MoveConstruct());
	
	TEST(Str, CopyAssign) {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = "lol";
		str2 = str;
		EXPECT_EQ(str2.len, 16);
		EXPECT_EQ(str2.totalBytes, 21);
	}

	consteval bool CompileTimeStr_CopyAssign() {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = "lol";
		str2 = str;
		if (str2.len != 16) return false;
		if (str2.totalBytes != 21) return false;
		return true;
	}
	static_assert(CompileTimeStr_CopyAssign());
	
	TEST(Str, MoveAssign) {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = "lol";
		str2 = std::move(str);
		EXPECT_EQ(str2.len, 16);
		EXPECT_EQ(str2.totalBytes, 21);
	}

	consteval bool CompileTimeStr_MoveAssign() {
		gk::Str str = "Übergrößenträger";
		gk::Str str2 = "lol";
		str2 = std::move(str);
		if (str2.len != 16) return false;
		if (str2.totalBytes != 21) return false;
		return true;
	}
	static_assert(CompileTimeStr_MoveAssign());

}