#include "utf8.h"

#if GK_TYPES_LIB_TEST

using gk::usize;

test_case("StrlenLengthAscii") {
	gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
	check_eq(metadata.length, 12);
}

consteval usize CompileTimeUtf8_StrlenLengthAscii() {
	gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
	return metadata.length;
}
static_assert(CompileTimeUtf8_StrlenLengthAscii() == 12);

test_case("StrlenTotalBytesAscii") {
	gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
	check_eq(metadata.totalBytes, 13);
}

consteval usize CompileTimeUtf8_StrlenTotalBytesAscii() {
	gk::Utf8Metadata metadata = gk::utf8::strlen("hello world!").ok();
	return metadata.totalBytes;
}
static_assert(CompileTimeUtf8_StrlenTotalBytesAscii() == 13);

test_case("StrlenLengthMultibyteCharacters") {
	gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
	check_eq(metadata.length, 16);
}

consteval usize CompileTimeUtf8_StrlenLengthMultibyteCharacters() {
	gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
	return metadata.length;
}
static_assert(CompileTimeUtf8_StrlenLengthMultibyteCharacters() == 16);

test_case("StrlenTotalBytesMultibyteCharacters") {
	gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
	check_eq(metadata.totalBytes, 21);
}

consteval usize CompileTimeUtf8_StrlenTotalBytesMultibyteCharacters() {
	gk::Utf8Metadata metadata = gk::utf8::strlen("Übergrößenträger").ok();
	return metadata.totalBytes;
}
static_assert(CompileTimeUtf8_StrlenTotalBytesMultibyteCharacters() == 21);

test_case("StrlenInvalidUtf8") {
	char buf[2];
	buf[0] = (char)255;
	buf[1] = '\0';
	auto result = gk::utf8::strlen(buf);
	check(result.isError());
}

#endif