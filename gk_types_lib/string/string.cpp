#include "string.h"
#include "../cpu_features/cpu_feature_detector.h"
#include <intrin.h>

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanForward)

using gk::String;
using gk::Str;
using gk::usize;
using gk::u32;
using gk::Option;

namespace gk
{
	constexpr bool SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS = false;

	typedef bool (*FuncCmpEqStringAndStr)(const char*, const Str&);
	typedef bool (*FuncCmpEqStringAndString)(const char*, const char*, usize);
	typedef Option<usize>(*FuncFindCharInString)(const char*, char, usize);

	namespace internal
	{
		[[nodiscard]] static bool avx512CompareEqualHeapBufferAndStr(const char* buffer, const Str& str) {
			constexpr usize equal64Bitmask = ~0;

			const __m512i* thisVec = reinterpret_cast<const __m512i*>(buffer);

			__m512i otherVec = _mm512_set1_epi8('\0');
			usize i = 0;
			const usize bytesToCheck = str.totalBytes - 1;
			if (bytesToCheck >= 64) {
				for (; i <= bytesToCheck - 64; i += 64) {
					memcpy(&otherVec, str.str + i, 64);
					if (_mm512_cmpeq_epi8_mask(*thisVec, otherVec) != equal64Bitmask) return false;
					thisVec++;
				}
			}

			for (; i < bytesToCheck; i++) {
				if (buffer[i] != str.str[i]) return false;
			}
			return true;
		}

		[[nodiscard]] static bool avx512CompareEqualHeapBufferAndOtherStringHeapBuffer(const char* buffer, const char* otherBuffer, usize bytesOfStringBufferUsed) {
			constexpr usize equal64Bitmask = ~0;
			const __m512i* thisVec = reinterpret_cast<const __m512i*>(buffer);
			const __m512i* otherVec = reinterpret_cast<const __m512i*>(otherBuffer);

			usize remainder = (bytesOfStringBufferUsed + 1) % 64;
			const usize bytesToCheck = remainder ? ((bytesOfStringBufferUsed + 1) + (64 - remainder)) : bytesOfStringBufferUsed + 1;
			for (usize i = 0; i < bytesToCheck; i += 64) {
				if (_mm512_cmpeq_epi8_mask(*thisVec, *otherVec) != equal64Bitmask) return false;
				thisVec++;
				otherVec++;
			}
			return true;
		}

		[[nodiscard]] static bool avx2CompareEqualHeapBufferAndStr(const char* buffer, const Str& str) {
			constexpr u32 equal32Bitmask = ~0;

			const __m256i* thisVec = reinterpret_cast<const __m256i*>(buffer);

			__m256i otherVec = _mm256_set1_epi8('\0');
			usize i = 0;
			const usize bytesToCheck = str.totalBytes - 1;
			for (; i <= bytesToCheck - 32; i += 32) {
				memcpy(&otherVec, str.str + i, 32);
				if (_mm256_cmpeq_epi8_mask(*thisVec, otherVec) != equal32Bitmask) return false;
				thisVec++;
			}

			for (; i < bytesToCheck; i++) {
				if (buffer[i] != str.str[i]) return false;
			}
			return true;
		}

		[[nodiscard]] static bool avx2CompareEqualHeapBufferAndOtherStringHeapBuffer(const char* buffer, const char* otherBuffer, usize bytesOfStringBufferUsed) {
			constexpr u32 equal32Bitmask = ~0;
			const __m256i* thisVec = reinterpret_cast<const __m256i*>(buffer);
			const __m256i* otherVec = reinterpret_cast<const __m256i*>(otherBuffer);

			usize remainder = (bytesOfStringBufferUsed + 1) % 32;
			const usize bytesToCheck = remainder ? ((bytesOfStringBufferUsed + 1) + (32 - remainder)) : bytesOfStringBufferUsed + 1;
			for (usize i = 0; i < bytesToCheck; i += 64) {
				if (_mm256_cmpeq_epi8_mask(*thisVec, *otherVec) != equal32Bitmask) return false;
				thisVec++;
				otherVec++;
			}
			return true;
		}

		[[nodiscard]] static Option<usize> avx512FindCharInString(const char* buffer, char c, usize bytesOfStringBufferUsed) {
			const __m512i vecChar = _mm512_set1_epi8(c);
			const __m512i* vecThis = reinterpret_cast<const __m512i*>(buffer);
			const usize iterationsToDo = ((bytesOfStringBufferUsed) % 64 == 0 ?
				bytesOfStringBufferUsed :
				bytesOfStringBufferUsed + (64 - (bytesOfStringBufferUsed % 64)))
				/ 64;

			for (usize i = 0; i < iterationsToDo; i++) {
				const usize bitmask = _mm512_cmpeq_epi8_mask(vecChar, *vecThis);
				if (bitmask == 0) { // nothing equal
					vecThis++;
					continue;
				}
				unsigned long index;
				_BitScanForward64(&index, bitmask);
				return Option<usize>(static_cast<usize>(index) + (i * 64));
			}
			return Option<usize>();
		}

		[[nodiscard]] static Option<usize> avx2FindCharInString(const char* buffer, char c, usize bytesOfStringBufferUsed) {
			const __m256i vecChar = _mm256_set1_epi8(c);
			const __m256i* vecThis = reinterpret_cast<const __m256i*>(buffer);
			const usize iterationsToDo = ((bytesOfStringBufferUsed) % 32 == 0 ?
				bytesOfStringBufferUsed :
				bytesOfStringBufferUsed + (32 - (bytesOfStringBufferUsed % 32)))
				/ 32;

			for (usize i = 0; i < iterationsToDo; i++) {
				const u32 bitmask = _mm256_cmpeq_epi8_mask(vecChar, *vecThis);
				if (bitmask == 0) { // nothing equal
					vecThis++;
					continue;
				}
				unsigned long index;
				_BitScanForward(&index, bitmask);
				return Option<usize>(static_cast<usize>(index) + (i * 32));
			}
			return Option<usize>();
		}
	}
}

bool gk::String::simdCompareStringAndStr(const gk::String& self, const Str& str)
{
	static FuncCmpEqStringAndStr func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::String function loader]: Using AVX-512 String-Str comparison\n";
			}
			
			return gk::internal::avx512CompareEqualHeapBufferAndStr;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::String function loader]: Using AVX-2 String-Str comparison\n";
			}
			return gk::internal::avx2CompareEqualHeapBufferAndStr;
		}
		else {
			std::cout << "[gk::String function loader]: ERROR\nCannot load string comparison functions if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(self._rep._heap._buffer, str);
}

bool gk::String::simdCompareStringAndString(const gk::String& self, const gk::String& other)
{
	static FuncCmpEqStringAndString func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::String function loader]: Using AVX-512 String-Str comparison\n";
			}
			return gk::internal::avx512CompareEqualHeapBufferAndOtherStringHeapBuffer;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::String function loader]: Using AVX-2 String-Str comparison\n";
			}
			return gk::internal::avx2CompareEqualHeapBufferAndOtherStringHeapBuffer;
		}
		else {
			std::cout << "[gk::String function loader]: ERROR\nCannot load string comparison functions if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(self._rep._heap._buffer, other._rep._heap._buffer, self._rep._heap._bytesOfBufferUsed);
}

Option<usize> gk::String::simdFindCharInString(const String& self, char c)
{
	static FuncFindCharInString func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::String function loader]: Using AVX-512 find char in string\n";
			}
			return gk::internal::avx512FindCharInString;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_STRING_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::String function loader]: Using AVX-2 find char in string\n";
			}
			return gk::internal::avx2FindCharInString;
		}
		else {
			std::cout << "[gk::String function loader]: ERROR\nCannot load find char in string function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(self._rep._heap._buffer, c, self._rep._heap._bytesOfBufferUsed);
}

#if GK_TYPES_LIB_TEST

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace gk
{
	namespace unitTests
	{
		struct StringTestExample {
			double a;
			i64 b;

			constexpr StringTestExample() :
				a(0.0), b(0) {}
		};
	}
}

using gk::unitTests::StringTestExample;

template<>
[[nodiscard]] static constexpr gk::String gk::String::from<gk::unitTests::StringTestExample>(const gk::unitTests::StringTestExample& value) {
	return gk::String::fromFloat(value.a) + gk::String(", ") + gk::String::fromUint(value.b);
}

test_case("DefaultConstruct") {
	gk::String a;
	check_eq(a.len(), 0);
}

comptime_test_case(String, DefaultConstruct, {
	gk::String a;
	check(a.len() == 0);
});

test_case("ConstructOneCharacter") {
	gk::String a = 'c';
	check_eq(a.len(), 1);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], '\0');
}

comptime_test_case(String, ConstructOneCharacter, {
		gk::String a = 'c';
		check(a.len() == 1);
		check(a.cstr()[0] == 'c');
		check(a.cstr()[1] == '\0');
	});

#pragma region Str_Construct

test_case("ConstructStrSmall") {
	gk::String a = "hi"_str;
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'h');
	check_eq(a.cstr()[1], 'i');
	check_eq(a.cstr()[2], '\0');
}

comptime_test_case(String, ConstructStrSmall, {
		gk::String a = "hi"_str;
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'h');
		check_eq(a.cstr()[1], 'i');
		check_eq(a.cstr()[2], '\0');
});

test_case("ConstructStrSmallUtf8") {
	gk::String a = "aÜ"_str;
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 3);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[1], "Ü"[0]);
	check_eq(a.cstr()[2], "Ü"[1]);
	check_eq(a.cstr()[4], '\0');
}

comptime_test_case(String, ConstructStrSmallUtf8, {
		gk::String a = "aÜ"_str;
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 3);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[1], "Ü"[0]);
		check_eq(a.cstr()[2], "Ü"[1]);
		check_eq(a.cstr()[4], '\0');
	});

test_case("ConstructStrLarge") {
	gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
	check_eq(a.len(), 39);
	check_eq(a.usedBytes(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, ConstructStrLarge, {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		check_eq(a.len(), 39);
		check_eq(a.usedBytes(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[39], '\0');
	});

test_case("ConstructStrLargeUtf8") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	check_eq(a.len(), 29);
	check_eq(a.usedBytes(), 37);
	check_eq(a.cstr()[0], "Ü"[0]);
	check_eq(a.cstr()[1], "Ü"[1]);
	check_ne(a.cstr()[36], '\0');
	check_eq(a.cstr()[37], '\0');
}

comptime_test_case(String, ConstructStrLargeUtf8, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		check_eq(a.len(), 29);
		check_eq(a.usedBytes(), 37);
		check_eq(a.cstr()[0], "Ü"[0]);
		check_eq(a.cstr()[1], "Ü"[1]);
		comptimeAssertNe(a.cstr()[36], '\0');
		check_eq(a.cstr()[37], '\0');
	});

#pragma endregion

#pragma region Copy_Construct

test_case("CopyDefaultConstruct") {
	gk::String a;
	gk::String b = a;
	check_eq(b.len(), 0);
}

comptime_test_case(String, CopyDefaultConstruct, {
		gk::String a;
		gk::String b = a;
		check_eq(b.len(), 0);
	});

test_case("CopyConstructOneCharacter") {
	gk::String a = 'c';
	gk::String b = a;
	check_eq(b.len(), 1);
	check_eq(b.cstr()[0], 'c');
	check_eq(b.cstr()[1], '\0');
}

comptime_test_case(String, CopyConstructOneCharacter, {
		gk::String a = 'c';
		gk::String b = a;
		check_eq(b.len(), 1);
		check_eq(b.cstr()[0], 'c');
		check_eq(b.cstr()[1], '\0');
	});

test_case("CopyConstructStrSmall") {
	gk::String a = "hi"_str;
	gk::String b = a;
	check_eq(b.len(), 2);
	check_eq(b.usedBytes(), 2);
	check_eq(b.cstr()[0], 'h');
	check_eq(b.cstr()[1], 'i');
	check_eq(b.cstr()[2], '\0');
}

comptime_test_case(String, CopyConstructStrSmall, {
		gk::String a = "hi"_str;
		gk::String b = a;
		check_eq(b.len(), 2);
		check_eq(b.usedBytes(), 2);
		check_eq(b.cstr()[0], 'h');
		check_eq(b.cstr()[1], 'i');
		check_eq(b.cstr()[2], '\0');
	});

test_case("CopyConstructStrSmallUtf8") {
	gk::String a = "aÜ"_str;
	gk::String b = a;
	check_eq(b.len(), 2);
	check_eq(b.usedBytes(), 3);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[1], "Ü"[0]);
	check_eq(b.cstr()[2], "Ü"[1]);
	check_eq(b.cstr()[4], '\0');
}

comptime_test_case(String, CopyConstructStrSmallUtf8, {
		gk::String a = "aÜ"_str;
		gk::String b = a;
		check_eq(b.len(), 2);
		check_eq(b.usedBytes(), 3);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[1], "Ü"[0]);
		check_eq(b.cstr()[2], "Ü"[1]);
		check_eq(b.cstr()[4], '\0');
	});

test_case("CopyConstructStrLarge") {
	gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
	gk::String b = a;
	check_eq(b.len(), 39);
	check_eq(b.usedBytes(), 39);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[39], '\0');
}

comptime_test_case(String, CopyConstructStrLarge, {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::String b = a;
		check_eq(b.len(), 39);
		check_eq(b.usedBytes(), 39);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[39], '\0');
	});

test_case("CopyConstructStrLargeUtf8") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	gk::String b = a;
	check_eq(b.len(), 29);
	check_eq(b.usedBytes(), 37);
	check_eq(b.cstr()[0], "Ü"[0]);
	check_eq(b.cstr()[1], "Ü"[1]);
	check_ne(b.cstr()[36], '\0');
	check_eq(b.cstr()[37], '\0');
}

comptime_test_case(String, CopyConstructStrLargeUtf8, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::String b = a;
		check_eq(b.len(), 29);
		check_eq(b.usedBytes(), 37);
		check_eq(b.cstr()[0], "Ü"[0]);
		check_eq(b.cstr()[1], "Ü"[1]);
		comptimeAssertNe(b.cstr()[36], '\0');
		check_eq(b.cstr()[37], '\0');
	});

#pragma endregion

#pragma region Move_Construct

test_case("MoveDefaultConstruct") {
	gk::String a;
	gk::String b = a;
	check_eq(b.len(), 0);
}

comptime_test_case(String, MoveDefaultConstruct, {
		gk::String a;
		gk::String b = a;
		check_eq(b.len(), 0);
	});

test_case("MoveConstructOneCharacter") {
	gk::String a = 'c';
	gk::String b = a;
	check_eq(b.len(), 1);
	check_eq(b.cstr()[0], 'c');
	check_eq(b.cstr()[1], '\0');
}

comptime_test_case(String, MoveConstructOneCharacter, {
		gk::String a = 'c';
		gk::String b = a;
		check_eq(b.len(), 1);
		check_eq(b.cstr()[0], 'c');
		check_eq(b.cstr()[1], '\0');
	});

test_case("MoveConstructStrSmall") {
	gk::String a = "hi"_str;
	gk::String b = a;
	check_eq(b.len(), 2);
	check_eq(b.usedBytes(), 2);
	check_eq(b.cstr()[0], 'h');
	check_eq(b.cstr()[1], 'i');
	check_eq(b.cstr()[2], '\0');
}

comptime_test_case(String, MoveConstructStrSmall, {
		gk::String a = "hi"_str;
		gk::String b = a;
		check_eq(b.len(), 2);
		check_eq(b.usedBytes(), 2);
		check_eq(b.cstr()[0], 'h');
		check_eq(b.cstr()[1], 'i');
		check_eq(b.cstr()[2], '\0');
	});

test_case("MoveConstructStrSmallUtf8") {
	gk::String a = "aÜ"_str;
	gk::String b = a;
	check_eq(b.len(), 2);
	check_eq(b.usedBytes(), 3);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[1], "Ü"[0]);
	check_eq(b.cstr()[2], "Ü"[1]);
	check_eq(b.cstr()[4], '\0');
}

comptime_test_case(String, MoveConstructStrSmallUtf8, {
		gk::String a = "aÜ"_str;
		gk::String b = a;
		check_eq(b.len(), 2);
		check_eq(b.usedBytes(), 3);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[1], "Ü"[0]);
		check_eq(b.cstr()[2], "Ü"[1]);
		check_eq(b.cstr()[4], '\0');
	});

test_case("MoveConstructStrLarge") {
	gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
	gk::String b = a;
	check_eq(b.len(), 39);
	check_eq(b.usedBytes(), 39);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[39], '\0');
}

comptime_test_case(String, MoveConstructStrLarge, {
		gk::String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		gk::String b = a;
		check_eq(b.len(), 39);
		check_eq(b.usedBytes(), 39);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[39], '\0');
	});

test_case("MoveConstructStrLargeUtf8") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	gk::String b = a;
	check_eq(b.len(), 29);
	check_eq(b.usedBytes(), 37);
	check_eq(b.cstr()[0], "Ü"[0]);
	check_eq(b.cstr()[1], "Ü"[1]);
	check_ne(b.cstr()[36], '\0');
	check_eq(b.cstr()[37], '\0');
}

comptime_test_case(String, MoveConstructStrLargeUtf8, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		gk::String b = a;
		check_eq(b.len(), 29);
		check_eq(b.usedBytes(), 37);
		check_eq(b.cstr()[0], "Ü"[0]);
		check_eq(b.cstr()[1], "Ü"[1]);
		comptimeAssertNe(b.cstr()[36], '\0');
		check_eq(b.cstr()[37], '\0');
	});

#pragma endregion

#pragma region Assign_Char

test_case("AssignFromChar") {
	gk::String a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
	a = 'c';
	check_eq(a.len(), 1);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], '\0');
}

comptime_test_case(String, AssignFromChar, {
		gk::String a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
		a = 'c';
		check_eq(a.len(), 1);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], '\0');
	});

test_case("AssignFromCharNullBytesSanityCheck") {
	gk::String a = "ha"_str;
	a = 'c';
	check_eq(a.len(), 1);
	check_eq(a.cstr()[0], 'c');
	for (gk::i32 i = 1; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromCharNullBytesSanityCheck, {
		gk::String a = "ha"_str;
		a = 'c';
		check_eq(a.len(), 1);
		check_eq(a.cstr()[0], 'c');
		for (int i = 1; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

#pragma endregion

#pragma region Assign_Str

test_case("AssignFromSmallStr") {
	gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
	a = "ca"_str;
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	check_eq(a.cstr()[2], '\0');
}

comptime_test_case(String, AssignFromSmallStr, {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		a = "ca"_str;
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		check_eq(a.cstr()[2], '\0');
	});

test_case("AssignFromLargeStr") {
	gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
	a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
	check_eq(a.len(), 39);
	check_eq(a.usedBytes(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[38], 'd');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, AssignFromLargeStr, {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		check_eq(a.len(), 39);
		check_eq(a.usedBytes(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[38], 'd');
		check_eq(a.cstr()[39], '\0');
	});

test_case("AssignFromStrNullBytesSanityCheck") {
	gk::String a = "hbb"_str;
	a = "ca"_str;
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	for (gk::i32 i = 2; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromStrNullBytesSanityCheck, {
		gk::String a = "hbb"_str;
		a = "ca"_str;
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

test_case("AssignFromStrReuseAllocation") {
	gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
	const char* oldBuffer = a.cstr();
	a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
	const char* newBuffer = a.cstr();
	check_eq(oldBuffer, newBuffer);
}

comptime_test_case(String, AssignFromStrReuseAllocation, {
		gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		const char* newBuffer = a.cstr();
		check_eq(oldBuffer, newBuffer);
	});

#pragma endregion

#pragma region Assign_Copy

test_case("AssignFromSmallCopy") {
	gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
	gk::String b = "ca"_str;
	a = b;
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
}

comptime_test_case(String, AssignFromSmallCopy, {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::String b = "ca"_str;
		a = b;
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		check_eq(a.cstr()[2], '\0');
	});

test_case("AssignFromLargeCopy") {
	gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
	gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
	a = b;
	check_eq(a.len(), 39);
	check_eq(a.usedBytes(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[38], 'd');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, AssignFromLargeCopy, {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = b;
		check_eq(a.len(), 39);
		check_eq(a.usedBytes(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[38], 'd');
		check_eq(a.cstr()[39], '\0');
	});

test_case("AssignFromCopyNullBytesSanityCheck") {
	gk::String a = "hbb"_str;
	gk::String b = "ca"_str;
	a = b;
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	for (gk::i32 i = 2; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromCopyNullBytesSanityCheck, {
		gk::String a = "hbb"_str;
		gk::String b = "ca"_str;
		a = b;
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

test_case("AssignFromCopyReuseAllocation") {
	gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
	const char* oldBuffer = a.cstr();
	gk::String b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
	a = b;
	const char* newBuffer = a.cstr();
	check_eq(oldBuffer, newBuffer);
}

comptime_test_case(String, AssignFromCopyReuseAllocation, {
		gk::String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
		const char* oldBuffer = a.cstr();
		gk::String b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
		a = b;
		const char* newBuffer = a.cstr();
		check_eq(oldBuffer, newBuffer);
	});

#pragma endregion

#pragma region Assign_Move

test_case("AssignFromSmallMove") {
	gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
	gk::String b = "ca"_str;
	a = std::move(b);
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	check_eq(a.cstr()[2], '\0');
}

comptime_test_case(String, AssignFromSmallMove, {
		gk::String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		gk::String b = "ca"_str;
		a = std::move(b);
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		check_eq(a.cstr()[2], '\0');
	});

test_case("AssignFromLargeMove") {
	gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
	gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
	a = std::move(b);
	check_eq(a.len(), 39);
	check_eq(a.usedBytes(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[38], 'd');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, AssignFromLargeMove, {
		gk::String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		gk::String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = std::move(b);
		check_eq(a.len(), 39);
		check_eq(a.usedBytes(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[38], 'd');
		check_eq(a.cstr()[39], '\0');
	});

test_case("AssignFromMoveNullBytesSanityCheck") {
	gk::String a = "hbb"_str;
	gk::String b = "ca"_str;
	a = std::move(b);
	check_eq(a.len(), 2);
	check_eq(a.usedBytes(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	for (gk::i32 i = 2; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromMoveNullBytesSanityCheck, {
		gk::String a = "hbb"_str;
		gk::String b = "ca"_str;
		a = std::move(b);
		check_eq(a.len(), 2);
		check_eq(a.usedBytes(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

#pragma endregion

#pragma region Equal_Char

test_case("EqualChar") {
	gk::String a = 'c';
	check_eq(a, 'c');
}

comptime_test_case(String, EqualChar, {
		gk::String a = 'c';
		check_eq(a, 'c');
	});

test_case("NotEqualChar") {
	gk::String a = 'b';
	check_ne(a, 'c');
}

comptime_test_case(String, NotEqualChar, {
		gk::String a = 'b';
		comptimeAssertNe(a, 'c');
	});

test_case("NotEqualCharSameFirst") {
	gk::String a = "ca"_str;
	check_ne(a, 'c');
}

comptime_test_case(String, NotEqualCharSameFirst, {
		gk::String a = "ca"_str;
		comptimeAssertNe(a, 'c');
	});

test_case("NotEqualCharAndLargeString") {
	gk::String a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
	check_ne(a, 'c');
}

comptime_test_case(String, NotEqualCharAndLargeString, {
		gk::String a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
		comptimeAssertNe(a, 'c');
	});

#pragma endregion

#pragma region Equal_Str

test_case("EqualSmallStr") {
	gk::String a = "hi"_str;
	check_eq(a, "hi"_str);
}

comptime_test_case(String, EqualSmallStr, {
		gk::String a = "hi"_str;
		check_eq(a, "hi"_str);
	});

test_case("EqualSsoMaxStr") {
	gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
	check_eq(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
}

comptime_test_case(String, EqualSsoMaxStr, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		check_eq(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

test_case("EqualLargeStr") {
	gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
	check_eq(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
}

comptime_test_case(String, EqualLargeStr, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		check_eq(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

test_case("EqualUtf8SmallStr") {
	gk::String a = "ßen"_str;
	check_eq(a, "ßen"_str);
}

comptime_test_case(String, EqualUtf8SmallStr, {
		gk::String a = "ßen"_str;
		check_eq(a, "ßen"_str);
	});

test_case("EqualUtf8LargeStr") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	check_eq(a, "ÜbergrößenträgerÜbergrößenträ"_str);
}

comptime_test_case(String, EqualUtf8LargeStr, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		check_eq(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

test_case("NotEqualSmallStr") {
	gk::String a = "hh"_str;
	check_ne(a, "hi"_str);
}

comptime_test_case(String, NotEqualSmallStr, {
		gk::String a = "hh"_str;
		comptimeAssertNe(a, "hi"_str);
	});

test_case("NotEqualSsoMaxStr") {
	gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
	check_ne(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
}

comptime_test_case(String, NotEqualSsoMaxStr, {
		gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertNe(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

test_case("NotEqualLargeStr") {
	gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
	check_ne(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
}

comptime_test_case(String, NotEqualLargeStr, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertNe(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

test_case("NotEqualUtf8Small") {
	gk::String a = "ßeb"_str;
	check_ne(a, "ßen"_str);
}

comptime_test_case(String, NotEqualUtf8Small, {
		gk::String a = "ßeb"_str;
		comptimeAssertNe(a, "ßen"_str);
	});

test_case("NotEqualUtf8Large") {
	gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
	check_ne(a, "ÜbergrößenträgerÜbergrößenträ"_str);
}

comptime_test_case(String, NotEqualUtf8Large, {
		gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		comptimeAssertNe(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

#pragma endregion

#pragma region Equal_Other_String

test_case("EqualCharOtherString") {
	gk::String a = 'c';
	check_eq(a, gk::String('c'));
}

comptime_test_case(String, EqualCharOtherString, {
		gk::String a = 'c';
		check_eq(a, gk::String('c'));
	});

test_case("EqualSmallOtherString") {
	gk::String a = "hi"_str;
	check_eq(a, gk::String("hi"_str));
}

comptime_test_case(String, EqualSmallOtherString, {
		gk::String a = "hi"_str;
		check_eq(a, gk::String("hi"_str));
	});

test_case("EqualSsoMaxOtherString") {
	gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
	check_eq(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
}

comptime_test_case(String, EqualSsoMaxOtherString, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		check_eq(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
	});

test_case("EqualLargeOtherString") {
	gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
	check_eq(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
}

comptime_test_case(String, EqualLargeOtherString, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		check_eq(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	});

test_case("EqualUtf8SmallOtherString") {
	gk::String a = "ßen"_str;
	check_eq(a, gk::String("ßen"_str));
}

comptime_test_case(String, EqualUtf8SmallOtherString, {
		gk::String a = "ßen"_str;
		check_eq(a, gk::String("ßen"_str));
	});

test_case("EqualUtf8LargeOtherString") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	check_eq(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
}

comptime_test_case(String, EqualUtf8LargeOtherString, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		check_eq(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
	});

test_case("NotEqualSmallStrOtherString") {
	gk::String a = "hh"_str;
	check_ne(a, gk::String("hi"_str));
}

comptime_test_case(String, NotEqualSmallStrOtherString, {
		gk::String a = "hh"_str;
		comptimeAssertNe(a, gk::String("hi"_str));
	});

test_case("NotEqualSsoMaxStrOtherString") {
	gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
	check_ne(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
}

comptime_test_case(String, NotEqualSsoMaxStrOtherString, {
		gk::String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		comptimeAssertNe(a, gk::String("ashdlakjshdlkajshdlkjasdasdddg"_str));
	});

test_case("NotEqualLargeStrOtherString") {
	gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
	check_ne(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
}

comptime_test_case(String, NotEqualLargeStrOtherString, {
		gk::String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		comptimeAssertNe(a, gk::String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	});

test_case("NotEqualUtf8SmallOtherString") {
	gk::String a = "ßeb"_str;
	check_ne(a, gk::String("ßen"_str));
}

comptime_test_case(String, NotEqualUtf8SmallOtherString, {
		gk::String a = "ßeb"_str;
		comptimeAssertNe(a, gk::String("ßen"_str));
	});

test_case("NotEqualUtf8LargeOtherString") {
	gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
	check_ne(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
}

comptime_test_case(String, NotEqualUtf8LargeOtherString, {
		gk::String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		comptimeAssertNe(a, gk::String("ÜbergrößenträgerÜbergrößenträ"_str));
	});

#pragma endregion

#pragma region Append

test_case("EmptyStringAppendChar") {
	gk::String a;
	a.append('c');
	check_eq(a, 'c');
	check_eq(a, gk::String('c')); // for sanity, same with following tests
}

comptime_test_case(String, EmptyStringAppendChar, {
		gk::String a;
		a.append('c');
		check_eq(a, 'c');
		check_eq(a, gk::String('c'));
	});

test_case("SmallStringAppendChar") {
	gk::String a = "hello"_str;
	a.append('!');
	check_eq(a, "hello!"_str);
	check_eq(a, gk::String("hello!"_str));
}

comptime_test_case(String, SmallStringAppendChar, {
		gk::String a = "hello"_str;
		a.append('!');
		check_eq(a, "hello!"_str);
		check_eq(a, gk::String("hello!"_str));
	});

test_case("SmallStringAppendCharMakeHeap") {
	gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
	a.append('!');
	check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
	check_eq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
}

comptime_test_case(String, SmallStringAppendCharMakeHeap, {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append('!');
		check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
		check_eq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
	});

test_case("LargeStringAppendChar") {
	gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
	a.append('a');
	check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
	check_eq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723a"_str));
}

comptime_test_case(String, LargeStringAppendChar, {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append('a');
		check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
		check_eq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723a"_str));
	});

test_case("SmallUtf8AppendChar") {
	gk::String a = "ßeb"_str;
	a.append('?');
	check_eq(a, "ßeb?"_str);
	check_eq(a, gk::String("ßeb?"_str));
}

comptime_test_case(String, SmallUtf8AppendChar, {
		gk::String a = "ßeb"_str;
		a.append('?');
		check_eq(a, "ßeb?"_str);
		check_eq(a, gk::String("ßeb?"_str));
	});

test_case("SmallUtf8AppendCharMakeHeap") {
	gk::String a = "ÜbergrößenträgerÜbergröa"_str;
	a.append('l');
	check_eq(a, "ÜbergrößenträgerÜbergröal"_str);
	check_eq(a, gk::String("ÜbergrößenträgerÜbergröal"_str));
}

comptime_test_case(String, SmallUtf8AppendCharMakeHeap, {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append('l');
		check_eq(a, "ÜbergrößenträgerÜbergröal"_str);
		check_eq(a, gk::String("ÜbergrößenträgerÜbergröal"_str));
	});

test_case("AppendCharHeapReallocate") {
	gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append('5');
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
	check_eq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
}

comptime_test_case(String, AppendCharHeapReallocate, {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append('5');
		check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
		check_eq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
	});

#pragma endregion

#pragma region Append_Str

test_case("EmptyStringAppendStr") {
	gk::String a;
	a.append("cc"_str);
	check_eq(a, "cc"_str);
	check_eq(a, gk::String("cc"_str)); // for sanity, same with following tests
}

comptime_test_case(String, EmptyStringAppendStr, {
		gk::String a;
		a.append("cc"_str);
		check_eq(a, "cc"_str);
		check_eq(a, gk::String("cc"_str));
	});

test_case("SmallStringAppendStr") {
	gk::String a = "hello"_str;
	a.append("!!"_str);
	check_eq(a, "hello!!"_str);
	check_eq(a, gk::String("hello!!"_str));
}

comptime_test_case(String, SmallStringAppendStr, {
		gk::String a = "hello"_str;
		a.append("!!"_str);
		check_eq(a, "hello!!"_str);
		check_eq(a, gk::String("hello!!"_str));
	});

test_case("SmallStringAppendStrMakeHeap") {
	gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
	a.append("!!"_str);
	check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
	check_eq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
}

comptime_test_case(String, SmallStringAppendStrMakeHeap, {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append("!!"_str);
		check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		check_eq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	});

test_case("LargeStringAppendStr") {
	gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
	a.append("aa"_str);
	check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
	check_eq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
}

comptime_test_case(String, LargeStringAppendStr, {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append("aa"_str);
		check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		check_eq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	});

test_case("SmallUtf8AppendStr") {
	gk::String a = "ßeb"_str;
	a.append("??"_str);
	check_eq(a, "ßeb??"_str);
	check_eq(a, gk::String("ßeb??"_str));
}

comptime_test_case(String, SmallUtf8AppendStr, {
		gk::String a = "ßeb"_str;
		a.append("??"_str);
		check_eq(a, "ßeb??"_str);
		check_eq(a, gk::String("ßeb??"_str));
	});

test_case("SmallUtf8AppendStrMakeHeap") {
	gk::String a = "ÜbergrößenträgerÜbergröa"_str;
	a.append("ll"_str);
	check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
	check_eq(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
}

comptime_test_case(String, SmallUtf8AppendStrMakeHeap, {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append("ll"_str);
		check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
		check_eq(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
	});

test_case("AppendStrHeapReallocate") {
	gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append("55"_str);
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
	check_eq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
}

comptime_test_case(String, AppendStrHeapReallocate, {
		gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append("55"_str);
		check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		check_eq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	});

#pragma endregion

#pragma region Append_Other_String

test_case("EmptyStringAppendOtherString") {
	gk::String a;
	a.append(gk::String("cc"_str));
	check_eq(a, "cc"_str);
	check_eq(a, gk::String("cc"_str)); // for sanity, same with following tests
}

comptime_test_case(String, EmptyStringAppendOtherString, {
		gk::String a;
		a.append(gk::String("cc"_str));
		check_eq(a, "cc"_str);
		check_eq(a, gk::String("cc"_str));
	});

test_case("SmallStringAppendOtherString") {
	gk::String a = "hello"_str;
	a.append(gk::String("!!"_str));
	check_eq(a, "hello!!"_str);
	check_eq(a, gk::String("hello!!"_str));
}

comptime_test_case(String, SmallStringAppendOtherString, {
		gk::String a = "hello"_str;
		a.append(gk::String("!!"_str));
		check_eq(a, "hello!!"_str);
		check_eq(a, gk::String("hello!!"_str));
	});

test_case("SmallStringAppendOtherStringMakeHeap") {
	gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
	a.append(gk::String("!!"_str));
	check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
	check_eq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
}

comptime_test_case(String, SmallStringAppendOtherStringMakeHeap, {
		gk::String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append(gk::String("!!"_str));
		check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		check_eq(a, gk::String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	});

test_case("LargeStringAppendOtherString") {
	gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
	a.append(gk::String("aa"_str));
	check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
	check_eq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
}

comptime_test_case(String, LargeStringAppendOtherString, {
		gk::String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append(gk::String("aa"_str));
		check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		check_eq(a, gk::String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	});

test_case("SmallUtf8AppendOtherString") {
	gk::String a = "ßeb"_str;
	a.append(gk::String("??"_str));
	check_eq(a, "ßeb??"_str);
	check_eq(a, gk::String("ßeb??"_str));
}

comptime_test_case(String, SmallUtf8AppendOtherString, {
		gk::String a = "ßeb"_str;
		a.append(gk::String("??"_str));
		check_eq(a, "ßeb??"_str);
		check_eq(a, gk::String("ßeb??"_str));
	});

test_case("SmallUtf8AppendOtherStringMakeHeap") {
	gk::String a = "ÜbergrößenträgerÜbergröa"_str;
	a.append(gk::String("ll"_str));
	check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
	check_eq(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
}

comptime_test_case(String, SmallUtf8AppendOtherStringMakeHeap, {
		gk::String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append(gk::String("ll"_str));
		check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
		check_eq(a, gk::String("ÜbergrößenträgerÜbergröall"_str));
	});

test_case("AppendOtherStringHeapReallocate") {
	gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append(gk::String("55"_str));
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
	check_eq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
}

comptime_test_case(String, AppendOtherStringHeapReallocate, {
	gk::String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append(gk::String("55"_str));
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
	check_eq(a, gk::String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	});

#pragma endregion

#pragma region Concat_Char

test_case("ConcatEmptyAndChar") {
	const gk::String a;
	gk::String b = a + 'c';
	check_eq(b, 'c');
	check_eq(b, gk::String('c'));
}

comptime_test_case(String, ConcatEmptyAndChar, {
		const gk::String a;
		gk::String b = a + 'c';
		check_eq(b, 'c');
		check_eq(b, gk::String('c'));
	});

test_case("ConcatCharStringAndChar") {
	const gk::String a = 'c';
	gk::String b = a + 'c';
	check_eq(b, "cc"_str);
	check_eq(b, gk::String("cc"_str));
}

comptime_test_case(String, ConcatCharStringAndChar, {
		const gk::String a = 'c';
		gk::String b = a + 'c';
		check_eq(b, "cc"_str);
		check_eq(b, gk::String("cc"_str));
	});

test_case("ConcatSmallStringAndCharToHeap") {
	const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	gk::String b = a + 'c';
	check_eq(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
	check_eq(b, gk::String("aslasdasddkjahldkjahsldkjahsdac"_str));
}

comptime_test_case(String, ConcatSmallStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + 'c';
		check_eq(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
		check_eq(b, gk::String("aslasdasddkjahldkjahsldkjahsdac"_str));
	});

test_case("ConcatHeapStringAndCharToHeap") {
	const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	gk::String b = a + 'c';
	check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
	check_eq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdac"_str));
}

comptime_test_case(String, ConcatHeapStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + 'c';
		check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
		check_eq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdac"_str));
	});

test_case("ConcatSmallUtf8AndChar") {
	const gk::String a = "Übergrößenträger"_str;
	gk::String b = a + 'c';
	check_eq(b, "Übergrößenträgerc"_str);
	check_eq(b, gk::String("Übergrößenträgerc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndChar, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + 'c';
		check_eq(b, "Übergrößenträgerc"_str);
		check_eq(b, gk::String("Übergrößenträgerc"_str));
	});

test_case("ConcatSmallUtf8AndCharToHeap") {
	const gk::String a = "Übergrößenträgerasjhdgashh"_str;
	gk::String b = a + 'c';
	check_eq(b, "Übergrößenträgerasjhdgashhc"_str);
	check_eq(b, gk::String("Übergrößenträgerasjhdgashhc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndCharToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + 'c';
		check_eq(b, "Übergrößenträgerasjhdgashhc"_str);
		check_eq(b, gk::String("Übergrößenträgerasjhdgashhc"_str));
	});

test_case("ConcatHeapUtf8AndChar") {
	const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a + 'c';
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
	check_eq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
}

comptime_test_case(String, ConcatHeapUtf8AndChar, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + 'c';
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
		check_eq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
	});

#pragma endregion

#pragma region Concat_Char_Inverted

test_case("InvertConcatEmptyAndChar") {
	const gk::String a;
	gk::String b = 'c' + a;
	check_eq(b, 'c');
	check_eq(b, gk::String('c'));
}

comptime_test_case(String, InvertConcatEmptyAndChar, {
		const gk::String a;
		gk::String b = 'c' + a;
		check_eq(b, 'c');
		check_eq(b, gk::String('c'));
	});

test_case("InvertConcatCharStringAndChar") {
	const gk::String a = 'c';
	gk::String b = 'c' + a;
	check_eq(b, "cc"_str);
	check_eq(b, gk::String("cc"_str));
}

comptime_test_case(String, InvertConcatCharStringAndChar, {
		const gk::String a = 'c';
		gk::String b = 'c' + a;
		check_eq(b, "cc"_str);
		check_eq(b, gk::String("cc"_str));
	});

test_case("InvertConcatSmallStringAndCharToHeap") {
	const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	gk::String b = 'c' + a;
	check_eq(b, "caslasdasddkjahldkjahsldkjahsda"_str);
	check_eq(b, gk::String("caslasdasddkjahldkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatSmallStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = 'c' + a;
		check_eq(b, "caslasdasddkjahldkjahsldkjahsda"_str);
		check_eq(b, gk::String("caslasdasddkjahldkjahsldkjahsda"_str));
	});

test_case("InvertConcatHeapStringAndCharToHeap") {
	const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	gk::String b = 'c' + a;
	check_eq(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
	check_eq(b, gk::String("caslasdasddkjahl55dkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatHeapStringAndCharToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = 'c' + a;
		check_eq(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
		check_eq(b, gk::String("caslasdasddkjahl55dkjahsldkjahsda"_str));
	});

test_case("InvertConcatSmallUtf8AndChar") {
	const gk::String a = "Übergrößenträger"_str;
	gk::String b = 'c' + a;
	check_eq(b, "cÜbergrößenträger"_str);
	check_eq(b, gk::String("cÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndChar, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = 'c' + a;
		check_eq(b, "cÜbergrößenträger"_str);
		check_eq(b, gk::String("cÜbergrößenträger"_str));
	});

test_case("InvertConcatSmallUtf8AndCharToHeap") {
	const gk::String a = "Übergrößenträgerasjhdgashh"_str;
	gk::String b = 'c' + a;
	check_eq(b, "cÜbergrößenträgerasjhdgashh"_str);
	check_eq(b, gk::String("cÜbergrößenträgerasjhdgashh"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndCharToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = 'c' + a;
		check_eq(b, "cÜbergrößenträgerasjhdgashh"_str);
		check_eq(b, gk::String("cÜbergrößenträgerasjhdgashh"_str));
	});

test_case("InvertConcatHeapUtf8AndChar") {
	const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = 'c' + a;
	check_eq(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	check_eq(b, gk::String("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatHeapUtf8AndChar, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = 'c' + a;
		check_eq(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		check_eq(b, gk::String("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	});

#pragma endregion

#pragma region Concat_Str

test_case("ConcatEmptyAndStr") {
	const gk::String a;
	gk::String b = a + "cc"_str;
	check_eq(b, "cc"_str);
	check_eq(b, gk::String("cc"_str));
}

comptime_test_case(String, ConcatEmptyAndStr, {
		const gk::String a;
		gk::String b = a + "cc"_str;
		check_eq(b, "cc"_str);
		check_eq(b, gk::String("cc"_str));
	});

test_case("ConcatCharStringAndStr") {
	const gk::String a = 'c';
	gk::String b = a + "cc"_str;
	check_eq(b, "ccc"_str);
	check_eq(b, gk::String("ccc"_str));
}

comptime_test_case(String, ConcatCharStringAndStr, {
		const gk::String a = 'c';
		gk::String b = a + "cc"_str;
		check_eq(b, "ccc"_str);
		check_eq(b, gk::String("ccc"_str));
	});

test_case("ConcatSmallStringAndStrToHeap") {
	const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	gk::String b = a + "cc"_str;
	check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
	check_eq(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatSmallStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + "cc"_str;
		check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		check_eq(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
	});

test_case("ConcatHeapStringAndStrToHeap") {
	const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	gk::String b = a + "cc"_str;
	check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
	check_eq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatHeapStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + "cc"_str;
		check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		check_eq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	});

test_case("ConcatSmallUtf8AndStr") {
	const gk::String a = "Übergrößenträger"_str;
	gk::String b = a + "cc"_str;
	check_eq(b, "Übergrößenträgercc"_str);
	check_eq(b, gk::String("Übergrößenträgercc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndStr, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + "cc"_str;
		check_eq(b, "Übergrößenträgercc"_str);
		check_eq(b, gk::String("Übergrößenträgercc"_str));
	});

test_case("ConcatSmallUtf8AndStrToHeap") {
	const gk::String a = "Übergrößenträgerasjhdgashh"_str;
	gk::String b = a + "cc"_str;
	check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
	check_eq(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndStrToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + "cc"_str;
		check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
		check_eq(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
	});

test_case("ConcatHeapUtf8AndStr") {
	const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a + "cc"_str;
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
	check_eq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
}

comptime_test_case(String, ConcatHeapUtf8AndStr, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + "cc"_str;
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		check_eq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	});

#pragma endregion

#pragma region Concat_Str_Inverted

test_case("InvertConcatEmptyAndStr") {
	const gk::String a;
	gk::String b = "cc"_str + a;
	check_eq(b, "cc"_str);
	check_eq(b, gk::String("cc"_str));
}

comptime_test_case(String, InvertConcatEmptyAndStr, {
		const gk::String a;
		gk::String b = "cc"_str + a;
		check_eq(b, "cc"_str);
		check_eq(b, gk::String("cc"_str));
	});

test_case("InvertConcatCharStringAndStr") {
	const gk::String a = 'c';
	gk::String b = "cc"_str + a;
	check_eq(b, "ccc"_str);
	check_eq(b, gk::String("ccc"_str));
}

comptime_test_case(String, InvertConcatCharStringAndStr, {
		const gk::String a = 'c';
		gk::String b = "cc"_str + a;
		check_eq(b, "ccc"_str);
		check_eq(b, gk::String("ccc"_str));
	});

test_case("InvertConcatSmallStringAndStrToHeap") {
	const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	gk::String b = "cc"_str + a;
	check_eq(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
	check_eq(b, gk::String("ccaslasdasddkjahldkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatSmallStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = "cc"_str + a;
		check_eq(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
		check_eq(b, gk::String("ccaslasdasddkjahldkjahsldkjahsda"_str));
	});

test_case("InvertConcatHeapStringAndStrToHeap") {
	const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	gk::String b = "cc"_str + a;
	check_eq(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
	check_eq(b, gk::String("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatHeapStringAndStrToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = "cc"_str + a;
		check_eq(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
		check_eq(b, gk::String("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
	});

test_case("InvertConcatSmallUtf8AndStr") {
	const gk::String a = "Übergrößenträger"_str;
	gk::String b = "cc"_str + a;
	check_eq(b, "ccÜbergrößenträger"_str);
	check_eq(b, gk::String("ccÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndStr, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = "cc"_str + a;
		check_eq(b, "ccÜbergrößenträger"_str);
		check_eq(b, gk::String("ccÜbergrößenträger"_str));
	});

test_case("InvertConcatSmallUtf8AndStrToHeap") {
	const gk::String a = "Übergrößenträgerasjhdgashh"_str;
	gk::String b = "cc"_str + a;
	check_eq(b, "ccÜbergrößenträgerasjhdgashh"_str);
	check_eq(b, gk::String("ccÜbergrößenträgerasjhdgashh"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndStrToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = "cc"_str + a;
		check_eq(b, "ccÜbergrößenträgerasjhdgashh"_str);
		check_eq(b, gk::String("ccÜbergrößenträgerasjhdgashh"_str));
	});

test_case("InvertConcatHeapUtf8AndStr") {
	const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = "cc"_str + a;
	check_eq(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	check_eq(b, gk::String("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatHeapUtf8AndStr, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = "cc"_str + a;
		check_eq(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		check_eq(b, gk::String("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	});

#pragma endregion

#pragma region Concat_Two_Strings

test_case("ConcatEmptyAndOtherString") {
	const gk::String a;
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "cc"_str);
	check_eq(b, gk::String("cc"_str));
}

comptime_test_case(String, ConcatEmptyAndOtherString, {
		const gk::String a;
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "cc"_str);
		check_eq(b, gk::String("cc"_str));
	});

test_case("ConcatCharStringAndOtherString") {
	const gk::String a = 'c';
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "ccc"_str);
	check_eq(b, gk::String("ccc"_str));
}

comptime_test_case(String, ConcatCharStringAndOtherString, {
		const gk::String a = 'c';
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "ccc"_str);
		check_eq(b, gk::String("ccc"_str));
	});

test_case("ConcatSmallStringAndOtherStringToHeap") {
	const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
	check_eq(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatSmallStringAndOtherStringToHeap, {
		const gk::String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		check_eq(b, gk::String("aslasdasddkjahldkjahsldkjahsdacc"_str));
	});

test_case("ConcatHeapStringAndOtherStringToHeap") {
	const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
	check_eq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatHeapStringAndOtherStringToHeap, {
		const gk::String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		check_eq(b, gk::String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	});

test_case("ConcatSmallUtf8AndOtherString") {
	const gk::String a = "Übergrößenträger"_str;
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "Übergrößenträgercc"_str);
	check_eq(b, gk::String("Übergrößenträgercc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndOtherString, {
		const gk::String a = "Übergrößenträger"_str;
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "Übergrößenträgercc"_str);
		check_eq(b, gk::String("Übergrößenträgercc"_str));
	});

test_case("ConcatSmallUtf8AndOtherStringToHeap") {
	const gk::String a = "Übergrößenträgerasjhdgashh"_str;
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
	check_eq(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndOtherStringToHeap, {
		const gk::String a = "Übergrößenträgerasjhdgashh"_str;
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
		check_eq(b, gk::String("Übergrößenträgerasjhdgashhcc"_str));
	});

test_case("ConcatHeapUtf8AndOtherString") {
	const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a + gk::String("cc"_str);
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
	check_eq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
}

comptime_test_case(String, ConcatHeapUtf8AndOtherString, {
		const gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a + gk::String("cc"_str);
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		check_eq(b, gk::String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	});

#pragma endregion

#pragma region Concat_Multiple

test_case("ChainConcat") {
	gk::String a = "hello world!"_str;
	gk::String b = a + ' ' + "hmm"_str + " t" + 'h' + gk::String("is is") + " a multi concat string thats quite large"_str;
	check_eq(b, "hello world! hmm this is a multi concat string thats quite large"_str);
	check_eq(b, gk::String("hello world! hmm this is a multi concat string thats quite large"_str));
}

comptime_test_case(String, ChainConcat, {
		gk::String a = "hello world!"_str;
		gk::String b = a + ' ' + "hmm"_str + " t" + 'h' + gk::String("is is") + " a multi concat string thats quite large"_str;
		check_eq(b, "hello world! hmm this is a multi concat string thats quite large"_str);
		check_eq(b, gk::String("hello world! hmm this is a multi concat string thats quite large"_str));
	});

#pragma endregion

#pragma region From_Type

test_case("FromBoolTrue") {
	gk::String a = gk::String::fromBool(true);
	check_eq(a, "true"_str);
	check_eq(a, gk::String("true"_str));
}

comptime_test_case(String, FromBoolTrue, {
		gk::String a = gk::String::fromBool(true);
		check_eq(a, "true"_str);
		check_eq(a, gk::String("true"_str));
	});

test_case("FromBoolFalse") {
	gk::String a = gk::String::fromBool(false);
	check_eq(a, "false"_str);
	check_eq(a, gk::String("false"_str));
}

comptime_test_case(String, FromBoolFalse, {
		gk::String a = gk::String::fromBool(false);
		check_eq(a, "false"_str);
		check_eq(a, gk::String("false"_str));
	});

test_case("FromSignedIntZero") {
	gk::String a = gk::String::fromInt(0);
	check_eq(a, '0');
}

comptime_test_case(String, FromSignedIntZero, {
	gk::String a = gk::String::fromInt(0);
	check_eq(a, '0');
	});

test_case("FromSignedIntSmallValue") {
	gk::String a = gk::String::fromInt(16);
	check_eq(a, "16"_str);
}

comptime_test_case(String, FromSignedIntSmallValue, {
		gk::String a = gk::String::fromInt(16);
		check_eq(a, "16"_str);
	});

test_case("FromSignedIntMaximumValue") {
	gk::String a = gk::String::fromInt(std::numeric_limits<gk::i64>::max());
	check_eq(a, "9223372036854775807"_str);
}

comptime_test_case(String, FromSignedIntMaximumValue, {
		gk::String a = gk::String::fromInt(MAXINT64);
		check_eq(a, "9223372036854775807"_str);
	});

test_case("FromSignedIntSmallNegativeValue") {
	gk::String a = gk::String::fromInt(-3);
	check_eq(a, "-3"_str);
}

comptime_test_case(String, FromSignedIntSmallNegativeValue, {
		gk::String a = gk::String::fromInt(-3);
		check_eq(a, "-3"_str);
	});

test_case("FromSignedIntMinimumValue") {
	gk::String a = gk::String::fromInt(std::numeric_limits<gk::i64>::min());
	check_eq(a, "-9223372036854775808"_str);
}

comptime_test_case(String, FromSignedIntMinimumValue, {
		gk::String a = gk::String::fromInt(MININT64);
		check_eq(a, "-9223372036854775808"_str);
	});

test_case("FromUnsignedIntZero") {
	gk::String a = gk::String::fromUint(0);
	check_eq(a, '0');
}

comptime_test_case(String, FromUnsignedIntZero, {
		gk::String a = gk::String::fromUint(0);
		check_eq(a, '0');
	});

test_case("FromUnsignedIntSmallValue") {
	gk::String a = gk::String::fromUint(23);
	check_eq(a, "23"_str);
}

comptime_test_case(String, FromUnsignedIntSmallValue, {
		gk::String a = gk::String::fromUint(23);
		check_eq(a, "23"_str);
	});

test_case("FromUnsignedIntMaximumValue") {
	gk::String a = gk::String::fromUint(std::numeric_limits<gk::u64>::max());
	check_eq(a, "18446744073709551615"_str);
}

comptime_test_case(String, FromUnsignedIntMaximumValue, {
		gk::String a = gk::String::fromUint(MAXUINT64);
		check_eq(a, "18446744073709551615"_str);
	});

test_case("FromFloatZero") {
	gk::String a = gk::String::fromFloat(0.0);
	check_eq(a, "0.0"_str);
}

comptime_test_case(String, FromFloatZero, {
		gk::String a = gk::String::fromFloat(0.0);
		check_eq(a, "0.0"_str);
	});

test_case("FromFloatPositiveInfinity") {
	gk::String a = gk::String::fromFloat(INFINITY);
	check_eq(a, "inf"_str);
}

test_case("FromFloatNegativeInfinity") {
	gk::String a = gk::String::fromFloat(-1.0 * INFINITY);
	check_eq(a, "-inf"_str);
}

test_case("FromFloatNaN") {
	gk::String a = gk::String::fromFloat(NAN);
	check_eq(a, "nan"_str);
}

test_case("FromFloatWholeNumber") {
	gk::String a = gk::String::fromFloat(100.0);
	check_eq(a, "100.0"_str);
}

comptime_test_case(String, FromFloatWholeNumber, {
		gk::String a = gk::String::fromFloat(100.0);
		check_eq(a, "100.0"_str);
	});

test_case("FromFloatWholeNegativeNumber") {
	gk::String a = gk::String::fromFloat(-100.0);
	check_eq(a, "-100.0"_str);
}

comptime_test_case(String, FromFloatWholeNegativeNumber, {
		gk::String a = gk::String::fromFloat(-100.0);
		check_eq(a, "-100.0"_str);
	});

test_case("FromFloatDecimalNumber") {
	gk::String a = gk::String::fromFloat(100.09999);
	check_eq(a, "100.09999"_str);
}

comptime_test_case(String, FromFloatDecimalNumber, {
		gk::String a = gk::String::fromFloat(100.09999);
		check_eq(a, "100.09999"_str);
	});

test_case("FromFloatDecimalNegativeNumber") {
	gk::String a = gk::String::fromFloat(-100.09999);
	check_eq(a, "-100.09999"_str);
}

comptime_test_case(String, FromFloatDecimalNegativeNumber, {
		gk::String a = gk::String::fromFloat(-100.09999);
		check_eq(a, "-100.09999"_str);
	});

test_case("FromFloatDecimalNumberDefaultPrecision") {
	gk::String a = gk::String::fromFloat(100.12000005);
	check_eq(a, "100.12"_str);
}

comptime_test_case(String, FromFloatDecimalNumberDefaultPrecision, {
		gk::String a = gk::String::fromFloat(100.12000005);
		check_eq(a, "100.12"_str);
	});

test_case("FromFloatDecimalNegativeNumberDefaultPrecision") {
	gk::String a = gk::String::fromFloat(-100.12000005);
	check_eq(a, "-100.12"_str);
}

comptime_test_case(String, FromFloatDecimalNegativeNumberDefaultPrecision, {
		gk::String a = gk::String::fromFloat(-100.12000005);
		check_eq(a, "-100.12"_str);
	});

test_case("FromFloatDecimalNumberCustomPrecision") {
	gk::String a = gk::String::fromFloat(100.12000005, 10);
	check_eq(a, "100.12000005"_str);
}

comptime_test_case(String, FromFloatDecimalNumberCustomPrecision, {
		gk::String a = gk::String::fromFloat(100.12000005, 10);
		check_eq(a, "100.12000005"_str);
	});

test_case("FromFloatDecimalNegativeNumberCustomPrecision") {
	gk::String a = gk::String::fromFloat(-100.12000005, 10);
	check_eq(a, "-100.12000005"_str);
}

comptime_test_case(String, FromFloatDecimalNegativeNumberCustomPrecision, {
		gk::String a = gk::String::fromFloat(-100.12000005, 10);
		check_eq(a, "-100.12000005"_str);
	});

test_case("FromTemplateBool") {
	bool b = true;
	gk::String a = gk::String::from(b);
	check_eq(a, "true"_str);
}

comptime_test_case(String, FromTemplateBool, {
		bool b = true;
		gk::String a = gk::String::from(b);
		check_eq(a, "true"_str);
	});

test_case("FromTemplategk::i8") {
	gk::i8 num = -56;
	gk::String a = gk::String::from(num);
	check_eq(a, "-56"_str);
}

comptime_test_case(String, FromTemplateInt8, {
		gk::i8 num = -56;
		gk::String a = gk::String::from(num);
		check_eq(a, "-56"_str);
	});

test_case("FromTemplategk::u8") {
	gk::u8 num = 56;
	gk::String a = gk::String::from(num);
	check_eq(a, "56"_str);
}

comptime_test_case(String, FromTemplateUint8, {
		gk::u8 num = 56;
		gk::String a = gk::String::from(num);
		check_eq(a, "56"_str);
	});

test_case("FromTemplategk::i16") {
	gk::i16 num = -1000;
	gk::String a = gk::String::from(num);
	check_eq(a, "-1000"_str);
}

comptime_test_case(String, FromTemplateInt16, {
		gk::i16 num = -1000;
		gk::String a = gk::String::from(num);
		check_eq(a, "-1000"_str);
	});

test_case("FromTemplategk::u16") {
	gk::u16 num = 1000;
	gk::String a = gk::String::from(num);
	check_eq(a, "1000"_str);
}

comptime_test_case(String, FromTemplateUint16, {
		gk::u16 num = 1000;
		gk::String a = gk::String::from(num);
		check_eq(a, "1000"_str);
	});

test_case("FromTemplategk::i32") {
	gk::i32 num = -99999;
	gk::String a = gk::String::from(num);
	check_eq(a, "-99999"_str);
}

comptime_test_case(String, FromTemplateInt32, {
		gk::i32 num = -99999;
		gk::String a = gk::String::from(num);
		check_eq(a, "-99999"_str);
	});

test_case("FromTemplategk::u32") {
	gk::u32 num = 99999;
	gk::String a = gk::String::from(num);
	check_eq(a, "99999"_str);
}

comptime_test_case(String, FromTemplateUint32, {
		gk::u32 num = 99999;
		gk::String a = gk::String::from(num);
		check_eq(a, "99999"_str);
	});

test_case("FromTemplategk::i64") {
	gk::i64 num = -123456789012345;
	gk::String a = gk::String::from(num);
	check_eq(a, "-123456789012345"_str);
}

comptime_test_case(String, FromTemplateInt64, {
		gk::i64 num = -123456789012345;
		gk::String a = gk::String::from(num);
		check_eq(a, "-123456789012345"_str);
	});

test_case("FromTemplategk::u64") {
	gk::u64 num = 123456789012345;
	gk::String a = gk::String::from(num);
	check_eq(a, "123456789012345"_str);
}

comptime_test_case(String, FromTemplateUint64, {
		gk::u64 num = 123456789012345;
		gk::String a = gk::String::from(num);
		check_eq(a, "123456789012345"_str);
	});

test_case("FromTemplateFloat32") {
	float num = -123.45f;
	gk::String a = gk::String::from(num);
	check_eq(a, "-123.44999"_str); // slightly imprecise
}

comptime_test_case(String, FromTemplateFloat32, {
		float num = -123.45f;
		gk::String a = gk::String::from(num);
		check_eq(a, "-123.44999"_str); // slightly imprecise
	});

test_case("FromTemplateFloat64") {
	double num = -123.45;
	gk::String a = gk::String::from(num);
	check_eq(a, "-123.45"_str);
}

comptime_test_case(String, FromTemplateFloat64, {
		double num = -123.45;
		gk::String a = gk::String::from(num);
		check_eq(a, "-123.45"_str);
	});

test_case("FromTemplateCustomType") {
	StringTestExample e;
	e.a = 1.0;
	e.b = 1;
	gk::String a = gk::String::from(e);
	check_eq(a, "1.0, 1"_str);
}

comptime_test_case(String, FromTemplateCustomType, {
		StringTestExample e;
		e.a = 1.0;
		e.b = 1;
		gk::String a = gk::String::from(e);
		check_eq(a, "1.0, 1"_str);
	});

#pragma endregion

#pragma region Format

test_case("FormatOneArg") {
	gk::i32 num = 4;
	gk::String a = gk::String::format<"num: {}">(num);
	check_eq(a, "num: 4"_str);
}

comptime_test_case(String, FormatOneArg, {
		gk::i32 num = 4;
		gk::String a = gk::String::format<"num: {}">(num);
		check_eq(a, "num: 4"_str);
	});

test_case("FormatOneArgWithTextAfter") {
	float num = 4.f;
	gk::String a = gk::String::format<"num: {}... cool!">(num);
	check_eq(a, "num: 4.0... cool!"_str);
}

comptime_test_case(String, FormatOneArgWithTextAfter, {
		float num = 4.f;
		gk::String a = gk::String::format<"num: {}... cool!">(num);
		check_eq(a, "num: 4.0... cool!"_str);
	});

test_case("FormatTwoArgs") {
	gk::i32 num1 = 5;
	float num2 = 5;
	gk::String a = gk::String::format<"num1: {}, num2: {}">(num1, num2);
	check_eq(a, "num1: 5, num2: 5.0"_str);
}

comptime_test_case(String, FormatTwoArgs, {
		int num1 = 5;
		float num2 = 5;
		gk::String a = gk::String::format<"num1: {}, num2: {}">(num1, num2);
		check_eq(a, "num1: 5, num2: 5.0"_str);
	});

test_case("FormatTwoArgsWithOperation") {
	gk::i32 num1 = 5;
	float num2 = 5;
	gk::String a = gk::String::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
	check_eq(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
}

comptime_test_case(String, FormatTwoArgsWithOperation, {
		int num1 = 5;
		float num2 = 5;
		gk::String a = gk::String::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
		check_eq(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
	});

test_case("FormatFromCustomType") {
	StringTestExample e;
	e.a = -1.2;
	e.b = 5;
	gk::i32 count = 2;
	gk::String a = gk::String::format<"the {} numbers are {}">(count, e);
	check_eq(a, "the 2 numbers are -1.19999, 5"_str);
}

comptime_test_case(String, FormatFromCustomType, {
		StringTestExample e;
		e.a = -1.2;
		e.b = 5;
		int count = 2;
		gk::String a = gk::String::format<"the {} numbers are {}">(count, e);
		check_eq(a, "the 2 numbers are -1.19999, 5"_str);
	});

#pragma endregion

#pragma region Find_Char

test_case("FindCharInSso") {
	gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find('5');
	check(opt.none() == false);
	check_eq(opt.some(), 21);
}

comptime_test_case(String, FindCharInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find('5');
		check(opt.none() == false);
		check_eq(opt.some(), 21);
	});

test_case("FindCharInHeap") {
	gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find('5');
	check(opt.none() == false);
	check_eq(opt.some(), 81);
}

comptime_test_case(String, FindCharInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find('5');
		check(opt.none() == false);
		check_eq(opt.some(), 81);
	});

test_case("NotFindCharInSso") {
	gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find('6');
	check(opt.none());
}

comptime_test_case(String, NotFindCharInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find('6');
		check(opt.none());
	});

test_case("NotFindCharInHeap") {
	gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find('6');
	check(opt.none());
}

comptime_test_case(String, NotFindCharInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find('6');
		check(opt.none());
	});

#pragma endregion

#pragma region Find_Str

test_case("FindStrInSso") {
	gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find("5a"_str);
	check_not(opt.none());
	check_eq(opt.some(), 21);
}

comptime_test_case(String, FindStrInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find("5a"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 21);
	});

test_case("FindStrInHeap") {
	gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find("5a"_str);
	check_not(opt.none());
	check_eq(opt.some(), 83);
}

comptime_test_case(String, FindStrInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find("5a"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 83);
	});

test_case("FindUtf8StrInSso") {
	gk::String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("ßen"_str);
	check_not(opt.none());
	check_eq(opt.some(), 9);
}

comptime_test_case(String, FindUtf8StrInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("ßen"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 9);
	});

test_case("FindUtf8StrInHeap") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("6Übe"_str);
	check_not(opt.none());
	check_eq(opt.some(), 141);
}

comptime_test_case(String, FindUtf8StrInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("6Übe"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 141);
	});

test_case("NotFindStrInSso") {
	gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find("ya"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindStrInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find("ya"_str);
		check(opt.none());
	});

test_case("NotFindStrInHeap") {
	gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find(";5"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindStrInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find(";5"_str);
		check(opt.none());
	});

test_case("NotFindUtf8StrInSso") {
	gk::String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("ßet"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8StrInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("ßet"_str);
		check(opt.none());
	});

test_case("NotFindUtf8StrInHeap") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("5Üba"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8StrInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("5Üba"_str);
		check(opt.none());
	});

#pragma endregion

#pragma region Find_Other_String

test_case("FindOtherStringInSso") {
	gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("5a"_str));
	check(opt.none() == false);
	check_eq(opt.some(), 21);
}

comptime_test_case(String, FindOtherStringInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("5a"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 21);
	});

test_case("FindOtherStringInHeap") {
	gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("5a"_str));
	check(opt.none() == false);
	check_eq(opt.some(), 83);
}

comptime_test_case(String, FindOtherStringInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("5a"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 83);
	});

test_case("FindUtf8OtherStringInSso") {
	gk::String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("ßen"_str));
	check_not(opt.none());
	check_eq(opt.some(), 9);
}

comptime_test_case(String, FindUtf8OtherStringInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("ßen"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 9);
	});

test_case("FindUtf8OtherStringInHeap") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("6Übe"_str));
	check_not(opt.none());
	check_eq(opt.some(), 141);
}

comptime_test_case(String, FindUtf8OtherStringInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("6Übe"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 141);
	});

test_case("NotFindOtherStringInSso") {
	gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("ya"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindOtherStringInSso, {
		gk::String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("ya"_str));
		check(opt.none());
	});

test_case("NotFindOtherStringInHeap") {
	gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find(gk::String(";5"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindOtherStringInHeap, {
		gk::String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find(gk::String(";5"_str));
		check(opt.none());
	});

test_case("NotFindUtf8OtherStringInSso") {
	gk::String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("ßet"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8OtherStringInSso, {
		gk::String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("ßet"_str));
		check(opt.none());
	});

test_case("NotFindUtf8OtherStringInHeap") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(gk::String("5Üba"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8OtherStringInHeap, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(gk::String("5Üba"_str));
		check(opt.none());
	});

#pragma endregion

#pragma region Substring

test_case("SubstringSsoStartingFromBeginning") {
	gk::String a = "Übergrößenträger"_str;
	gk::String b = a.substring(0, 12);
	check_eq(b, "Übergröße"_str);
}

comptime_test_case(String, SubstringSsoStartingFromBeginning, {
		gk::String a = "Übergrößenträger"_str;
		gk::String b = a.substring(0, 12);
		check_eq(b, "Übergröße"_str);
	});

test_case("SubstringSsoStartingFromOffset") {
	gk::String a = "Übergrößenträger"_str;
	gk::String b = a.substring(2, 12);
	check_eq(b, "bergröße"_str);
}

comptime_test_case(String, SubstringSsoStartingFromOffset, {
		gk::String a = "Übergrößenträger"_str;
		gk::String b = a.substring(2, 12);
		check_eq(b, "bergröße"_str);
	});

test_case("SubstringHeapToSsoStartingFromBeginning") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a.substring(0, 20);
	check_eq(b, "Übergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToSsoStartingFromBeginning, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(0, 20);
		check_eq(b, "Übergrößenträger"_str);
	});

test_case("SubstringHeapToSsoStartingFromOffset") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a.substring(20, 40);
	check_eq(b, "Übergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToSsoStartingFromOffset, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(20, 40);
		check_eq(b, "Übergrößenträger"_str);
	});

test_case("SubstringHeapToHeapStartingFromBeginning") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a.substring(0, 40);
	check_eq(b, "ÜbergrößenträgerÜbergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToHeapStartingFromBeginning, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(0, 40);
		check_eq(b, "ÜbergrößenträgerÜbergrößenträger"_str);
	});

test_case("SubstringHeapToHeapStartingFromOffset") {
	gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	gk::String b = a.substring(20, 80);
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToHeapStartingFromOffset, {
		gk::String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		gk::String b = a.substring(20, 80);
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	});

#pragma endregion

#endif