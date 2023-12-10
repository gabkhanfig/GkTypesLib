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

test_case("LengthCompileTimeMultibyteCharacters") {
	gk::Str str = "Übergrößenträger";
	check_eq(str.len, 20);
}

consteval usize CompileTimeStr_LengthMultibyteCharacters() {
	gk::Str str = "Übergrößenträger";
	return str.len;
}
static_assert(CompileTimeStr_LengthMultibyteCharacters() == 20);

test_case("LengthCompileTimeAsciiSuffix") {
	auto str = "hello world!"_str;
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAsciiSuffix() {
	auto str = "hello world!"_str;
	return str.len;
}
static_assert(CompileTimeStr_LengthAsciiSuffix() == 12);

test_case("LengthCompileTimeMultibyteCharactersSuffix") {
	auto str = "Übergrößenträger"_str;
	check_eq(str.len, 20);
}

consteval usize CompileTimeStr_LengthMultibyteCharactersSuffix() {
	auto str = "Übergrößenträger"_str;
	return str.len;
}
static_assert(CompileTimeStr_LengthMultibyteCharactersSuffix() == 20);

test_case("LengthRuntimeAscii") {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromNullTerminated(text);
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAsciiFrom() {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromNullTerminated(text);
	return str.len;
}
static_assert(CompileTimeStr_LengthAsciiFrom() == 12);

test_case("LengthRuntimeAsciiKnownLength") {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromSlice(text, 12);
	check_eq(str.len, 12);
}

consteval usize CompileTimeStr_LengthAsciiFromKnownLength() {
	const char* text = "hello world!";
	gk::Str str = gk::Str::fromSlice(text, 12);
	return str.len;
}
static_assert(CompileTimeStr_LengthAsciiFromKnownLength() == 12);

test_case("LengthRuntimeMultibyteCharacters") {
	const char* text = "Übergrößenträger";
	gk::Str str = gk::Str::fromNullTerminated(text);
	check_eq(str.len, 20);
}

consteval usize CompileTimeStr_LengthMultibyteCharactersFrom() {
	const char* text = "Übergrößenträger";
	gk::Str str = gk::Str::fromNullTerminated(text);
	return str.len;
}
static_assert(CompileTimeStr_LengthMultibyteCharactersFrom() == 20);

test_case("CopyConstruct") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = str;
	check_eq(str2.len, 20);
}

consteval bool CompileTimeStr_CopyConstruct() {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = str;
	if (str2.len != 20) return false;
	return true;
}
static_assert(CompileTimeStr_CopyConstruct());

test_case("MoveConstruct") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = std::move(str);
	check_eq(str2.len, 20);
}

consteval bool CompileTimeStr_MoveConstruct() {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = std::move(str);
	if (str2.len != 20) return false;
	return true;
}
static_assert(CompileTimeStr_MoveConstruct());

test_case("CopyAssign") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = "lol";
	str2 = str;
	check_eq(str2.len, 20);
}

consteval bool CompileTimeStr_CopyAssign() {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = "lol";
	str2 = str;
	if (str2.len != 20) return false;
	return true;
}
static_assert(CompileTimeStr_CopyAssign());

test_case("MoveAssign") {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = "lol";
	str2 = std::move(str);
	check_eq(str2.len, 20);
}

consteval bool CompileTimeStr_MoveAssign() {
	gk::Str str = "Übergrößenträger";
	gk::Str str2 = "lol";
	str2 = std::move(str);
	if (str2.len != 20) return false;
	return true;
}
static_assert(CompileTimeStr_MoveAssign());

#endif