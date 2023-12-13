﻿#include "str.h"
#include "../cpu_features/cpu_feature_detector.h"
#include <intrin.h>
#include "../utility.h"

namespace gk {
	namespace internal {
		static usize calculateAvx512IterationsCount(usize length) {
			return ((length) % 64 == 0 ?
				length :
				length + (64 - (length % 64)))
				/ 64;
		}

		static usize calculateAvx2IterationsCount(usize length) {
			return ((length) % 32 == 0 ?
				length :
				length + (32 - (length % 32)))
				/ 32;
		}

		typedef bool (*CmpEqStrAndStrFunc)(const gk::Str&, const Str&);
		typedef Option<usize> (*FindCharInStrFunc)(const gk::Str&, char);

		static bool avx512cmpEqStrAndStr(const gk::Str& a, const gk::Str& b) {
			constexpr usize equal64Bitmask = ~0;
			const usize len = a.len;

			// dont need to intialize
			__m512i aVec;
			__m512i bVec;

			const usize iters = calculateAvx512IterationsCount(len);

			for (usize i = 0; i < iters; i++) {
				memset(&aVec, 0, sizeof(__m512i));
				memset(&bVec, 0, sizeof(__m512i));

				memcpy(&aVec, a.buffer + (i * 64), (len - (i * 64)) % 64);
				memcpy(&bVec, b.buffer + (i * 64), (len - (i  * 64)) % 64);

				if (_mm512_cmpeq_epi8_mask(aVec, bVec) != equal64Bitmask) return false;
			}
			return true;
		}

		static bool avx2cmpEqStrAndStr(const gk::Str& a, const gk::Str& b) {
			constexpr u32 equal32Bitmask = ~0;
			const usize len = a.len;

			// dont need to intialize
			__m256i aVec;
			__m256i bVec;

			const usize iters = calculateAvx512IterationsCount(len);

			for (usize i = 0; i < iters; i++) {
				memset(&aVec, 0, sizeof(__m256i));
				memset(&bVec, 0, sizeof(__m256i));

				memcpy(&aVec, a.buffer + (i * 32), (len - (i * 32)) % 32);
				memcpy(&bVec, b.buffer + (i * 32), (len - (i * 32)) % 32);

				if (_mm256_cmpeq_epi8_mask(aVec, bVec) != equal32Bitmask) return false;
			}
			return true;
		}

		static bool cmpEqStrAndStrSimd(const gk::Str& a, const gk::Str& b) {
			static CmpEqStrAndStrFunc func = []() {
				if (gk::x86::isAvx512Supported()) {
					if (true) {
						std::cout << "[String Slice function loader]: Using AVX-512 Str-Str comparison\n";
					}
					return gk::internal::avx512cmpEqStrAndStr;
				}
				else if (gk::x86::isAvx2Supported()) {
					if (true) {
						std::cout << "[String Slice function loader]: Using AVX-2 Str-Str comparison\n";
					}
					return gk::internal::avx2cmpEqStrAndStr;
				}
				else {
					std::cout << "[String Slice function loader]: ERROR\nCannot load Str comparison functions if AVX-512 or AVX-2 aren't supported\n";
					abort();
				}
			}();

			return func(a, b);
		}

		static gk::Option<gk::usize> avx512FindCharInStr(const gk::Str& str, char c) {
			const usize len = str.len;
			const usize iters = calculateAvx512IterationsCount(len);
			const __m512i charVec = _mm512_set1_epi8(c);
			__m512i strVec;

			for (usize i = 0; i < iters; i++) {
				memset(&strVec, 0, sizeof(__m512i));
				memcpy(&strVec, str.buffer + (i * 64), (len - (i * 64)) % 64);

				const u64 bitmask = _mm512_cmpeq_epi8_mask(strVec, charVec);

				unsigned long index;
				if (_BitScanForward64(&index, bitmask) == 0) continue;
				return Option<usize>(static_cast<usize>(index) + (i * 64));
			}
			return Option<usize>();
		}

		static gk::Option<gk::usize> avx2FindCharInStr(const gk::Str& str, char c) {
			const usize len = str.len;
			const usize iters = calculateAvx2IterationsCount(len);
			const __m256i charVec = _mm256_set1_epi8(c);
			__m256i strVec;

			for (usize i = 0; i < iters; i++) {
				memset(&strVec, 0, sizeof(__m256i));
				memcpy(&strVec, str.buffer + (i * 32), (len - (i * 32)) % 32);

				const u32 bitmask = _mm256_cmpeq_epi8_mask(strVec, charVec);

				unsigned long index;
				if (_BitScanForward(&index, bitmask) == 0) continue;
				return Option<usize>(static_cast<usize>(index) + (i * 32));
			}
			return Option<usize>();
		}

		static gk::Option<usize> findCharSimd(const gk::Str& str, char c) {
			static FindCharInStrFunc func = []() {
				if (gk::x86::isAvx512Supported()) {
					if (true) {
						std::cout << "[String Slice function loader]: Using AVX-512 find char in Str\n";
					}
					return gk::internal::avx512FindCharInStr;
				}
				else if (gk::x86::isAvx2Supported()) {
					if (true) {
						std::cout << "[String Slice function loader]: Using AVX-2 find char in Str\n";
					}
					return gk::internal::avx2FindCharInStr;
				}
				else {
					std::cout << "[String Slice function loader]: ERROR\nCannot load find char in Str functions if AVX-512 or AVX-2 aren't supported\n";
					abort();
				}
			}();

			return func(str, c);
		}
	}
}

bool gk::Str::equalStr(const gk::Str& str) const
{
	// at this point, can be assumed that the lengths are equal.
	constexpr usize SIMD_EQUAL_CHECK_THRESHOLD_LENGTH = 16;
	if (len > SIMD_EQUAL_CHECK_THRESHOLD_LENGTH) {
		internal::cmpEqStrAndStrSimd(*this, str);
	}
	else {
		for (usize i = 0; i < len; i++) {
			if (buffer[i] != str.buffer[i]) {
				return false;
			}
		}
		return true;
	}
}

gk::Option<gk::usize> gk::Str::findChar(char c) const
{
	constexpr usize SIMD_FIND_THRESHOLD_LENGTH = 16;
	if (len > SIMD_FIND_THRESHOLD_LENGTH) {
		return internal::findCharSimd(*this, c);
	}
	else {
		for (usize i = 0; i < len; i++) {
			if (buffer[i] == c) {
				return Option<usize>(i);
			}
		}
		return Option<usize>();
	}
}

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

test_case("Str equal char") {
	gk::Str str = "a";
	check_eq(str, 'a');
}

test_case("Str not equal char") {
	gk::Str str = "a";
	check_ne(str, 'b');
}

test_case("Str equal small string slice") {
	char temp1[3];
	char temp2[3];
	memcpy(temp1, "hi", 3);
	memcpy(temp2, "hi", 3);

	gk::Str str1 = temp1;
	gk::Str str2 = temp2;
	check_eq(str1, str2);
}

test_case("Str not equal small string slice") {
	char temp1[3];
	char temp2[3];
	memcpy(temp1, "hi", 3);
	memcpy(temp2, "wo", 3);

	gk::Str str1 = temp1;
	gk::Str str2 = temp2;
	check_ne(str1, str2);
}

test_case("Str equal large string slice") {
	const char* large = "akusydhliauaysdoiuaysdoiauysdpoiuaysdpiuaysd";
	char temp1[45];
	char temp2[45];
	memcpy(temp1, large, 45);
	memcpy(temp2, large, 45);

	gk::Str str1 = temp1;
	gk::Str str2 = temp2;
	check_eq(str1, str2);
}

test_case("Str not equal large string slice") {
	char temp1[45];
	char temp2[45];
	memcpy(temp1, "akusydhliauaysgoiuaysdoiauysdpoiuaysdpiuaysd", 45);
	memcpy(temp2, "akusydhliauaysdoiuaysdoiauysdpoiuaysdpiuaysd", 45);

	gk::Str str1 = temp1;
	gk::Str str2 = temp2;
	check_ne(str1, str2);
}

#endif