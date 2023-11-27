#include "str.h"

#if GK_TYPES_LIB_TEST

using gk::usize;

test_case("LengthCompileTimeAscii") {
	gk::Str str = "hello world!";
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAscii() {
	gk::Str str = "hello world!";
	return str.len;
}
static_assert(CompileTimeStr_LengthAscii() == 12);

test_case("TotalBytesCompileTimeAscii") {
	gk::Str str = "hello world!";
	check_eq(str.totalBytes, 13);
}

consteval usize CompileTimeStr_TotalBytesAscii() {
	gk::Str str = "hello world!";
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesAscii() == 13);

test_case("LengthCompileTimeMultibyteCharacters") {
	gk::Str str = "Übergrößenträger";
	check_eq(str.len, 16);
}

consteval usize CompileTimeStr_LengthMultibyteCharacters() {
	gk::Str str = "Übergrößenträger";
	return str.len;
}
static_assert(CompileTimeStr_LengthMultibyteCharacters() == 16);

test_case("TotalBytesCompileTimeMultibyteCharacters") {
	gk::Str str = "Übergrößenträger";
	check_eq(str.totalBytes, 21);
}

consteval usize CompileTimeStr_TotalBytesMultibyteCharacters() {
	gk::Str str = "Übergrößenträger";
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesMultibyteCharacters() == 21);

test_case("LengthCompileTimeAsciiSuffix") {
	auto str = "hello world!"_str;
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAsciiSuffix() {
	auto str = "hello world!"_str;
	return str.len;
}
static_assert(CompileTimeStr_LengthAsciiSuffix() == 12);

test_case("TotalBytesCompileTimeAsciiSuffix") {
	auto str = "hello world!"_str;
	check_eq(str.totalBytes, 13);
}

consteval usize CompileTimeStr_TotalBytesAsciiSuffix() {
	auto str = "hello world!"_str;
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesAsciiSuffix() == 13);

test_case("LengthCompileTimeMultibyteCharactersSuffix") {
	auto str = "Übergrößenträger"_str;
	check_eq(str.len, 16);
}

consteval usize CompileTimeStr_LengthMultibyteCharactersSuffix() {
	auto str = "Übergrößenträger"_str;
	return str.len;
}
static_assert(CompileTimeStr_LengthMultibyteCharactersSuffix() == 16);

test_case("TotalBytesCompileTimeMultibyteCharactersSuffix") {
	auto str = "Übergrößenträger"_str;
	check_eq(str.totalBytes, 21);
}

consteval usize CompileTimeStr_TotalBytesMultibyteCharactersSuffix() {
	auto str = "Übergrößenträger"_str;
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesMultibyteCharactersSuffix() == 21);

test_case("LengthRuntimeAscii") {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text);
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAsciiFrom() {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text);
	return str.len;
}
static_assert(CompileTimeStr_LengthAsciiFrom() == 12);

test_case("TotalBytesRuntimeAscii") {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text);
	check_eq(str.totalBytes, 13);
}

consteval usize CompileTimeStr_TotalBytesAsciiFrom() {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text);
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesAsciiFrom() == 13);

test_case("LengthRuntimeAsciiKnownLength") {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text, 12);
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAsciiFromKnownLength() {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text, 12);
	return str.len;
}
static_assert(CompileTimeStr_LengthAsciiFromKnownLength() == 12);

test_case("TotalBytesRuntimeAsciiKnownLength") {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text, 12);
	check_eq(str.totalBytes, 13);
}

consteval usize CompileTimeStr_TotalBytesAsciiFromKnownLength() {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromAscii(text, 12);
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesAsciiFromKnownLength() == 13);

test_case("LengthRuntimeMultibyteCharacters") {
	const char* text = "Übergrößenträger";
	gk::Str str = gk::Str::fromUtf8(text).ok();
	check_eq(str.len, 16);
}

consteval usize CompileTimeStr_LengthMultibyteCharactersFrom() {
	const char* text = "Übergrößenträger";
	gk::Str str = gk::Str::fromUtf8(text).ok();
	return str.len;
}
static_assert(CompileTimeStr_LengthMultibyteCharactersFrom() == 16);

test_case("TotalBytesRuntimeMultibyteCharacters") {
	const char* text = "Übergrößenträger";
	gk::Str str = gk::Str::fromUtf8(text).ok();
	check_eq(str.totalBytes, 21);
}

consteval usize CompileTimeStr_TotalBytesMultibyteCharactersFrom() {
	const char* text = "Übergrößenträger";
	gk::Str str = gk::Str::fromUtf8(text).ok();
	return str.totalBytes;
}
static_assert(CompileTimeStr_TotalBytesMultibyteCharactersFrom() == 21);

test_case("RuntimeMultibyteCharacterInvalid") {
	char buf[2];
	buf[0] = (char)255;
	buf[1] = '\0';
	auto res = gk::Str::fromUtf8(buf);
	check(res.isError());
}

test_case("CopyConstruct") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = str;
	check_eq(str2.len, 16);
	check_eq(str2.totalBytes, 21);
}

consteval bool CompileTimeStr_CopyConstruct() {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = str;
	if (str2.len != 16) return false;
	if (str2.totalBytes != 21) return false;
	return true;
}
static_assert(CompileTimeStr_CopyConstruct());

test_case("MoveConstruct") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = std::move(str);
	check_eq(str2.len, 16);
	check_eq(str2.totalBytes, 21);
}

consteval bool CompileTimeStr_MoveConstruct() {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = std::move(str);
	if (str2.len != 16) return false;
	if (str2.totalBytes != 21) return false;
	return true;
}
static_assert(CompileTimeStr_MoveConstruct());

test_case("CopyAssign") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = "lol";
	str2 = str;
	check_eq(str2.len, 16);
	check_eq(str2.totalBytes, 21);
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

test_case("MoveAssign") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = "lol";
	str2 = std::move(str);
	check_eq(str2.len, 16);
	check_eq(str2.totalBytes, 21);
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

#endif