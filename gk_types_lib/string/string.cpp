#include "string.h"
#include "../allocator/heap_allocator.h"
#include "../cpu_features/cpu_feature_detector.h"
#include <intrin.h>
#include "../utility.h"

char* gk::mallocCharBufferAligned(usize* capacity)
{
	constexpr usize alignment = 64;
	const usize remainder = *capacity % alignment;
	if (remainder) {
		*capacity = *capacity + (alignment - remainder);
	}

	const usize mallocCapacity = *capacity;

	char* buffer = globalHeapAllocator()->mallocAlignedBuffer<char>(mallocCapacity, alignment).ok();
	memset(buffer, '\0', mallocCapacity);
	return buffer;
}

void gk::freeCharBufferAligned(char*& buffer, usize capacity)
{
	constexpr usize alignment = 64;
	globalHeapAllocator()->freeAlignedBuffer(buffer, capacity, alignment);
}

namespace gk {
	static __m256i stringHashIteration(const __m256i* vec, gk::i8 num) {
		// in the case of SSO, will ignore the 
		const __m256i seed = _mm256_set1_epi64x(0);
		const __m256i indices = _mm256_set_epi8(31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		const __m256i numVec = _mm256_set1_epi8(num);

		// Checks if num is greater than each value of indices.
		// Mask is 0xFF if greater than, and 0x00 otherwise. 
		const __m256i mask = _mm256_cmpgt_epi8(numVec, indices);
		const __m256i partial = _mm256_and_si256(*vec, mask);
		return _mm256_add_epi8(partial, numVec);
	}
}

/*
AVX-512 would require only updating the h value
4 at a time, rather than the full 8 of the AVX-512 buffer.
This is because the hash XOR, multiplication, and Xorshift
will cause a difference when doing 4 vs 8, EVEN if the
last 4 of the 8 are unused.
Supporting it will require some bookkeeping.
TODO investigate if that's a good performance tradeoff.
*/

gk::usize gk::String::hash() const
{
	static_assert(sizeof(String) == sizeof(__m256i));

	usize h = 0;

	if (isSso()) {
		h = 0 ^ (ssoLen() * HASH_MODIFIER);
		const __m256i thisVec = _mm256_loadu_epi8((const void*)this);
		const __m256i hashIter = stringHashIteration(&thisVec, static_cast<i8>(ssoLen()));

		for (usize i = 0; i < 4; i++) {
			h ^= hashIter.m256i_u64[i];
			h *= HASH_MODIFIER;
			h ^= h >> HASH_SHIFT;
		}
	}
	else {
		const usize length = rep.heap.length;
		h = 0 ^ (length * HASH_MODIFIER);

		const usize iterationsToDo = ((length) % 32 == 0 ?
			length :
			length + (32 - (length % 32)))
			/ 32;

		for (usize i = 0; i < iterationsToDo; i++) {
			const __m256i* thisVec = reinterpret_cast<const __m256i*>(rep.heap.buffer);
			const i8 num = i != (iterationsToDo - 1) ? static_cast<i8>(32) : static_cast<i8>((iterationsToDo * i) - length);
			check_le(num, 32);
			const __m256i hashIter = stringHashIteration(thisVec + i, num);

			for (usize j = 0; j < 4; j++) {
				h ^= hashIter.m256i_u64[j];
				h *= HASH_MODIFIER;
				h ^= h >> HASH_SHIFT;
			}
		}
	}

	h ^= h >> HASH_SHIFT;
	h *= HASH_MODIFIER;
	h ^= h >> HASH_SHIFT;
	return h;
}

namespace gk
{
	namespace internal
	{
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


		typedef bool (*CmpEqStringAndStrFunc)(const char*, const Str&);
		typedef bool (*CmpEqStringAndStringFunc)(const char*, const char*, usize);
		typedef Option<usize>(*FindCharInStringFunc)(const char*, char, usize);
		typedef Option<usize>(*FindStrSliceInStringFunc)(const char*, usize, const Str&);

		static bool avx512CompareEqualStringAndStr(const char* buffer, const Str& str) {
			constexpr usize equal64Bitmask = ~0;

			const __m512i* thisVec = reinterpret_cast<const __m512i*>(buffer);
			__m512i otherVec = _mm512_set1_epi8('\0');

			usize i = 0;
			const usize bytesToCheck = str.len;
			if (bytesToCheck >= 64) {
				for (; i <= bytesToCheck - 64; i += 64) {
					memcpy(&otherVec, str.buffer + i, 64);
					if (_mm512_cmpeq_epi8_mask(*thisVec, otherVec) != equal64Bitmask) return false;
					thisVec++;
				}
			}

			for (; i < bytesToCheck; i++) {
				if (buffer[i] != str.buffer[i]) return false;
			}
			return true;
		}

		static bool avx2CompareEqualStringAndStr(const char* buffer, const Str& str) {
			constexpr u32 equal32Bitmask = ~0;

			const __m256i* thisVec = reinterpret_cast<const __m256i*>(buffer);

			__m256i otherVec = _mm256_set1_epi8('\0');
			usize i = 0;
			const usize bytesToCheck = str.len;
			for (; i <= bytesToCheck - 32; i += 32) {
				memcpy(&otherVec, str.buffer + i, 32);
				if (_mm256_cmpeq_epi8_mask(*thisVec, otherVec) != equal32Bitmask) return false;
				thisVec++;
			}

			for (; i < bytesToCheck; i++) {
				if (buffer[i] != str.buffer[i]) return false;
			}
			return true;
		}

		static bool avx512CompareEqualStringAndString(const char* buffer, const char* otherBuffer, usize len) {
			constexpr usize equal64Bitmask = ~0;
			const __m512i* thisVec = reinterpret_cast<const __m512i*>(buffer);
			const __m512i* otherVec = reinterpret_cast<const __m512i*>(otherBuffer);
			// null terminator
			usize remainder = (len + 1) % 64;
			const usize bytesToCheck = remainder ? ((len + 1) + (64 - remainder)) : len + 1;
			for (usize i = 0; i < bytesToCheck; i += 64) {
				if (_mm512_cmpeq_epi8_mask(*thisVec, *otherVec) != equal64Bitmask) return false;
				thisVec++;
				otherVec++;
			}
			return true;
		}

		static bool avx2CompareEqualStringAndString(const char* buffer, const char* otherBuffer, usize len) {
			constexpr u32 equal32Bitmask = ~0;
			const __m256i* thisVec = reinterpret_cast<const __m256i*>(buffer);
			const __m256i* otherVec = reinterpret_cast<const __m256i*>(otherBuffer);
			// null terminator
			usize remainder = (len + 1) % 32;
			const usize bytesToCheck = remainder ? ((len + 1) + (32 - remainder)) : len + 1;
			for (usize i = 0; i < bytesToCheck; i += 64) {
				if (_mm256_cmpeq_epi8_mask(*thisVec, *otherVec) != equal32Bitmask) return false;
				thisVec++;
				otherVec++;
			}
			return true;
		}

		static Option<usize> avx512FindCharInString(const char* buffer, char c, usize len) {
			const __m512i vecChar = _mm512_set1_epi8(c);
			const __m512i* vecThis = reinterpret_cast<const __m512i*>(buffer);
			const usize iterationsToDo = ((len) % 64 == 0 ?
				len :
				len + (64 - (len % 64)))
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

		static Option<usize> avx2FindCharInString(const char* buffer, char c, usize len) {
			const __m256i vecChar = _mm256_set1_epi8(c);
			const __m256i* vecThis = reinterpret_cast<const __m256i*>(buffer);
			const usize iterationsToDo = ((len) % 32 == 0 ?
				len :
				len + (32 - (len % 32)))
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

		static Option<usize> avx512FindStrSliceInString(const char* buffer, usize length, const Str& str) {
			const __m512i firstChar = _mm512_set1_epi8(str.buffer[0]);
			const __m512i* vecThis = reinterpret_cast<const __m512i*>(buffer);

			__m512i findCmpA = _mm512_set1_epi8(0);
			__m512i findCmpB = _mm512_set1_epi8(0);

			const usize iterationsToDo = internal::calculateAvx512IterationsCount(length);
			const usize findIterations = internal::calculateAvx512IterationsCount(str.len);

			for (usize i = 0; i < iterationsToDo; i++) {
				// First, try to find the first character within the string buffer.
				u64 bitmask = _mm512_cmpeq_epi8_mask(firstChar, *vecThis);

				while (true) {
					Option<usize> next = bitscanForwardNext(&bitmask);
					if (next.none()) break;

					const usize index = next.some() + (i * 64);

					// the str is too long for the rest of the string.
					if ((length - index) < str.len) return Option<usize>();

					bool found = true;
					for (usize i = 0; i < findIterations; i++) {
						constexpr usize equal64Bitmask = ~0;

						memset(&findCmpA, 0, sizeof(__m512i));
						memset(&findCmpB, 0, sizeof(__m512i));

						memcpy(&findCmpA, buffer + index, str.len - (i * 64));
						memcpy(&findCmpB, str.buffer, str.len - (i * 64));

						if (_mm512_cmpeq_epi8_mask(findCmpA, findCmpB) != equal64Bitmask) {
							found = false;
							break;
						}
					}

					if (found) {
						return Option<usize>(index);
					}
				}

				vecThis++;
			}
			return Option<usize>();
		}

		static Option<usize> avx2FindStrSliceInString(const char* buffer, usize length, const Str& str) {
			const __m256i firstChar = _mm256_set1_epi8(str.buffer[0]);
			const __m256i* vecThis = reinterpret_cast<const __m256i*>(buffer);

			__m256i findCmpA = _mm256_set1_epi8(0);
			__m256i findCmpB = _mm256_set1_epi8(0);

			const usize iterationsToDo = internal::calculateAvx2IterationsCount(length);
			const usize findIterations = internal::calculateAvx2IterationsCount(str.len);

			for (usize i = 0; i < iterationsToDo; i++) {
				// First, try to find the first character within the string buffer.
				u64 bitmask = _mm256_cmpeq_epi8_mask(firstChar, *vecThis);

				while (true) {
					Option<usize> next = bitscanForwardNext(&bitmask);
					if (next.none()) break;

					const usize index = next.some() + (i * 64);

					// the str is too long for the rest of the string.
					if ((length - index) < str.len) return Option<usize>();

					bool found = true;
					for (usize i = 0; i < findIterations; i++) {
						constexpr usize equal64Bitmask = ~0;

						memset(&findCmpA, 0, sizeof(__m256i));
						memset(&findCmpB, 0, sizeof(__m256i));

						memcpy(&findCmpA, buffer + index, str.len - (i * 64));
						memcpy(&findCmpB, str.buffer, str.len - (i * 64));

						if (_mm256_cmpeq_epi8_mask(findCmpA, findCmpB) != equal64Bitmask) {
							found = false;
							break;
						}
					}

					if (found) {
						return Option<usize>(index);
					}
				}

				vecThis++;
			}
			return Option<usize>();
		}
	}
}

bool gk::String::equalToStrSimd(const gk::Str& str) const
{
	if (isSso()) {
		if (ssoLen() != str.len) return false;

		const usize* charsAsMachineWord = reinterpret_cast<const usize*>(rep.sso.chars);
		usize buf[4] = { 0 };
		memcpy(buf, str.buffer, str.len);
		((char*)buf)[31] = flag; // must copy last byte from string for the equality check.

		return (charsAsMachineWord[0] == buf[0])
			&& (charsAsMachineWord[1] == buf[1])
			&& (charsAsMachineWord[2] == buf[2])
			&& (charsAsMachineWord[3] == buf[3]);
	}
	else {
		if (rep.heap.length != str.len) return false;

		static internal::CmpEqStringAndStrFunc cmpFunc = []() {
			if (gk::x86::isAvx512Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-512 String-Str comparison\n";
				}

				return gk::internal::avx512CompareEqualStringAndStr;
			}
			else if (gk::x86::isAvx2Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-2 String-Str comparison\n";
				}
				return gk::internal::avx2CompareEqualStringAndStr;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load string comparison functions if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}();

		return cmpFunc(rep.heap.buffer, str);
	}


	return false;
}

bool gk::String::equalToStringSimd(const gk::String& other) const
{
	if (isSso()) {
		const usize* thisSso = reinterpret_cast<const usize*>(rep.sso.chars);
		const usize* otherSso = reinterpret_cast<const usize*>(other.rep.sso.chars);
		return (thisSso[0] == otherSso[0])
			&& (thisSso[1] == otherSso[1])
			&& (thisSso[2] == otherSso[2])
			&& (thisSso[3] == otherSso[3]);
	}
	else {
		if (rep.heap.length != other.len()) return false;
		check_false_message(other.isSso(), "at this point, the other string being sso should have been weeded out");

		static internal::CmpEqStringAndStringFunc cmpFunc = []() {
			if (gk::x86::isAvx512Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-512 String-Str comparison\n";
				}
				return gk::internal::avx512CompareEqualStringAndString;
			}
			else if (gk::x86::isAvx2Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-2 String-Str comparison\n";
				}
				return gk::internal::avx2CompareEqualStringAndString;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load string comparison functions if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}();

		return cmpFunc(rep.heap.buffer, other.rep.heap.buffer, rep.heap.length);
	}

}

gk::Option<gk::usize> gk::String::findCharInStringSimd(char c) const
{
	if (isSso()) {
		for (usize i = 0; i < MAX_SSO_LEN; i++) {
			if (rep.sso.chars[i] == c) return Option<usize>(i);
		}
		return Option<usize>();
	}
	else {
		static internal::FindCharInStringFunc findFunc = []() {
			if (gk::x86::isAvx512Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-512 find character in String\n";
				}
				return gk::internal::avx512FindCharInString;
			}
			else if (gk::x86::isAvx2Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-2 find character in String\n";
				}
				return gk::internal::avx2FindCharInString;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load string find character functions if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}();

		return findFunc(rep.heap.buffer, c, rep.heap.length);
	}
}

gk::Option<gk::usize> gk::String::findStrInStringSimd(const gk::Str& str) const
{
	if (isSso()) {
		const usize length = ssoLen();
		const char firstChar = str.buffer[0];
		for (u64 i = 0; i < length; i++) {
			if (rep.sso.chars[i] == firstChar) {
				const char* thisCompareStart = rep.sso.chars + i;

				bool found = true;
				for (u64 compareIndex = 1; compareIndex < str.len; compareIndex++) { // dont need to check the first character
					if (thisCompareStart[compareIndex] != str.buffer[compareIndex]) {
						found = false;
						break;
					}
				}
				if (found) {
					return Option<usize>(i); // All has been checked.
				}
			}
		}
		return Option<usize>();
	}
	else {
		static internal::FindStrSliceInStringFunc findFunc = []() {
			if (gk::x86::isAvx512Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-512 find string slice in String\n";
				}
				return gk::internal::avx512FindStrSliceInString;
			}
			else if (gk::x86::isAvx2Supported()) {
				if (true) {
					std::cout << "[String function loader]: Using AVX-2 find string slice in String\n";
				}
				return gk::internal::avx2FindStrSliceInString;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load string find string slice functions if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}();

		return findFunc(rep.heap.buffer, rep.heap.length, str);
	}
}

#if GK_TYPES_LIB_TEST

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

using String = gk::String;
using gk::unitTests::StringTestExample;

template<>
[[nodiscard]] static constexpr gk::String gk::String::from<gk::unitTests::StringTestExample>(const gk::unitTests::StringTestExample& value) {
	return String::fromFloat(value.a) + String(", ") + String::fromUint(value.b);
}

test_case("DefaultConstruct") {
	String a;
	check_eq(a.len(), 0);
}

comptime_test_case(String, DefaultConstruct, {
	String a;
	check(a.len() == 0);
	});

test_case("ConstructOneCharacter") {
	String a = 'c';
	check_eq(a.len(), 1);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], '\0');
}

comptime_test_case(String, ConstructOneCharacter, {
		String a = 'c';
		check(a.len() == 1);
		check(a.cstr()[0] == 'c');
		check(a.cstr()[1] == '\0');
	});

#pragma region Str_Construct

test_case("ConstructStrSmall") {
	String a = "hi"_str;
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'h');
	check_eq(a.cstr()[1], 'i');
	check_eq(a.cstr()[2], '\0');
}

comptime_test_case(String, ConstructStrSmall, {
		String a = "hi"_str;
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'h');
		check_eq(a.cstr()[1], 'i');
		check_eq(a.cstr()[2], '\0');
	});

test_case("ConstructStrSmallUtf8") {
	String a = "aÜ"_str;
	check_eq(a.len(), 3);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[1], "Ü"[0]);
	check_eq(a.cstr()[2], "Ü"[1]);
	check_eq(a.cstr()[4], '\0');
}

comptime_test_case(String, ConstructStrSmallUtf8, {
		String a = "aÜ"_str;
		check_eq(a.len(), 3);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[1], "Ü"[0]);
		check_eq(a.cstr()[2], "Ü"[1]);
		check_eq(a.cstr()[4], '\0');
	});

test_case("ConstructStrLarge") {
	String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
	check_eq(a.len(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, ConstructStrLarge, {
		String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		check_eq(a.len(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[39], '\0');
	});

test_case("ConstructStrLargeUtf8") {
	String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	check_eq(a.len(), 37);
	check_eq(a.cstr()[0], "Ü"[0]);
	check_eq(a.cstr()[1], "Ü"[1]);
	check_ne(a.cstr()[36], '\0');
	check_eq(a.cstr()[37], '\0');
}

comptime_test_case(String, ConstructStrLargeUtf8, {
		String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		check_eq(a.len(), 37);
		check_eq(a.cstr()[0], "Ü"[0]);
		check_eq(a.cstr()[1], "Ü"[1]);
		check_ne(a.cstr()[36], '\0');
		check_eq(a.cstr()[37], '\0');
	});

#pragma endregion

#pragma region Copy_Construct

test_case("CopyDefaultConstruct") {
	String a;
	String b = a;
	check_eq(b.len(), 0);
}

comptime_test_case(String, CopyDefaultConstruct, {
		String a;
		String b = a;
		check_eq(b.len(), 0);
	});

test_case("CopyConstructOneCharacter") {
	String a = 'c';
	String b = a;
	check_eq(b.len(), 1);
	check_eq(b.cstr()[0], 'c');
	check_eq(b.cstr()[1], '\0');
}

comptime_test_case(String, CopyConstructOneCharacter, {
		String a = 'c';
		String b = a;
		check_eq(b.len(), 1);
		check_eq(b.cstr()[0], 'c');
		check_eq(b.cstr()[1], '\0');
	});

test_case("CopyConstructStrSmall") {
	String a = "hi"_str;
	String b = a;
	check_eq(b.len(), 2);
	check_eq(b.cstr()[0], 'h');
	check_eq(b.cstr()[1], 'i');
	check_eq(b.cstr()[2], '\0');
}

comptime_test_case(String, CopyConstructStrSmall, {
		String a = "hi"_str;
		String b = a;
		check_eq(b.len(), 2);
		check_eq(b.cstr()[0], 'h');
		check_eq(b.cstr()[1], 'i');
		check_eq(b.cstr()[2], '\0');
	});

test_case("CopyConstructStrSmallUtf8") {
	String a = "aÜ"_str;
	String b = a;
	check_eq(b.len(), 3);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[1], "Ü"[0]);
	check_eq(b.cstr()[2], "Ü"[1]);
	check_eq(b.cstr()[4], '\0');
}

comptime_test_case(String, CopyConstructStrSmallUtf8, {
		String a = "aÜ"_str;
		String b = a;
		check_eq(b.len(), 3);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[1], "Ü"[0]);
		check_eq(b.cstr()[2], "Ü"[1]);
		check_eq(b.cstr()[4], '\0');
	});

test_case("CopyConstructStrLarge") {
	String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
	String b = a;
	check_eq(b.len(), 39);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[39], '\0');
}

comptime_test_case(String, CopyConstructStrLarge, {
		String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		String b = a;
		check_eq(b.len(), 39);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[39], '\0');
	});

test_case("CopyConstructStrLargeUtf8") {
	String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	String b = a;
	check_eq(b.len(), 37);
	check_eq(b.cstr()[0], "Ü"[0]);
	check_eq(b.cstr()[1], "Ü"[1]);
	check_ne(b.cstr()[36], '\0');
	check_eq(b.cstr()[37], '\0');
}

comptime_test_case(String, CopyConstructStrLargeUtf8, {
		String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		String b = a;
		check_eq(b.len(), 37);
		check_eq(b.cstr()[0], "Ü"[0]);
		check_eq(b.cstr()[1], "Ü"[1]);
		check_ne(b.cstr()[36], '\0');
		check_eq(b.cstr()[37], '\0');
	});

#pragma endregion

#pragma region Move_Construct

test_case("MoveDefaultConstruct") {
	String a;
	String b = a;
	check_eq(b.len(), 0);
}

comptime_test_case(String, MoveDefaultConstruct, {
		String a;
		String b = a;
		check_eq(b.len(), 0);
	});

test_case("MoveConstructOneCharacter") {
	String a = 'c';
	String b = a;
	check_eq(b.len(), 1);
	check_eq(b.cstr()[0], 'c');
	check_eq(b.cstr()[1], '\0');
}

comptime_test_case(String, MoveConstructOneCharacter, {
		String a = 'c';
		String b = a;
		check_eq(b.len(), 1);
		check_eq(b.cstr()[0], 'c');
		check_eq(b.cstr()[1], '\0');
	});

test_case("MoveConstructStrSmall") {
	String a = "hi"_str;
	String b = a;
	check_eq(b.len(), 2);
	check_eq(b.cstr()[0], 'h');
	check_eq(b.cstr()[1], 'i');
	check_eq(b.cstr()[2], '\0');
}

comptime_test_case(String, MoveConstructStrSmall, {
		String a = "hi"_str;
		String b = a;
		check_eq(b.len(), 2);
		check_eq(b.cstr()[0], 'h');
		check_eq(b.cstr()[1], 'i');
		check_eq(b.cstr()[2], '\0');
	});

test_case("MoveConstructStrSmallUtf8") {
	String a = "aÜ"_str;
	String b = a;
	check_eq(b.len(), 3);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[1], "Ü"[0]);
	check_eq(b.cstr()[2], "Ü"[1]);
	check_eq(b.cstr()[4], '\0');
}

comptime_test_case(String, MoveConstructStrSmallUtf8, {
		String a = "aÜ"_str;
		String b = a;
		check_eq(b.len(), 3);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[1], "Ü"[0]);
		check_eq(b.cstr()[2], "Ü"[1]);
		check_eq(b.cstr()[4], '\0');
	});

test_case("MoveConstructStrLarge") {
	String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
	String b = a;
	check_eq(b.len(), 39);
	check_eq(b.cstr()[0], 'a');
	check_eq(b.cstr()[39], '\0');
}

comptime_test_case(String, MoveConstructStrLarge, {
		String a = "asdglkjahsldkjahsldkjahsldkjahsdlkajshd"_str;
		String b = a;
		check_eq(b.len(), 39);
		check_eq(b.cstr()[0], 'a');
		check_eq(b.cstr()[39], '\0');
	});

test_case("MoveConstructStrLargeUtf8") {
	String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	String b = a;
	check_eq(b.len(), 37);
	check_eq(b.cstr()[0], "Ü"[0]);
	check_eq(b.cstr()[1], "Ü"[1]);
	check_ne(b.cstr()[36], '\0');
	check_eq(b.cstr()[37], '\0');
}

comptime_test_case(String, MoveConstructStrLargeUtf8, {
		String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		String b = a;
		check_eq(b.len(), 37);
		check_eq(b.cstr()[0], "Ü"[0]);
		check_eq(b.cstr()[1], "Ü"[1]);
		check_ne(b.cstr()[36], '\0');
		check_eq(b.cstr()[37], '\0');
	});

#pragma endregion

#pragma region Assign_Char

test_case("AssignFromChar") {
	String a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
	a = 'c';
	check_eq(a.len(), 1);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], '\0');
}

comptime_test_case(String, AssignFromChar, {
		String a = "ahosiduyapisudypaiusdypaiusdypaiusydpaiusd"_str;
		a = 'c';
		check_eq(a.len(), 1);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], '\0');
	});

test_case("AssignFromCharNullBytesSanityCheck") {
	String a = "ha"_str;
	a = 'c';
	check_eq(a.len(), 1);
	check_eq(a.cstr()[0], 'c');
	for (gk::i32 i = 1; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromCharNullBytesSanityCheck, {
		String a = "ha"_str;
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
	String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
	a = "ca"_str;
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	check_eq(a.cstr()[2], '\0');
}

comptime_test_case(String, AssignFromSmallStr, {
		String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		a = "ca"_str;
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		check_eq(a.cstr()[2], '\0');
	});

test_case("AssignFromLargeStr") {
	String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
	a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
	check_eq(a.len(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[38], 'd');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, AssignFromLargeStr, {
		String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		a = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		check_eq(a.len(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[38], 'd');
		check_eq(a.cstr()[39], '\0');
	});

test_case("AssignFromStrNullBytesSanityCheck") {
	String a = "hbb"_str;
	a = "ca"_str;
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	for (gk::i32 i = 2; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromStrNullBytesSanityCheck, {
		String a = "hbb"_str;
		a = "ca"_str;
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

test_case("AssignFromStrReuseAllocation") {
	String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
	const char* oldBuffer = a.cstr();
	a = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
	const char* newBuffer = a.cstr();
	check_eq(oldBuffer, newBuffer);
}

#pragma endregion

#pragma region Assign_Copy

test_case("AssignFromSmallCopy") {
	String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
	String b = "ca"_str;
	a = b;
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
}

comptime_test_case(String, AssignFromSmallCopy, {
		String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		String b = "ca"_str;
		a = b;
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		check_eq(a.cstr()[2], '\0');
	});

test_case("AssignFromLargeCopy") {
	String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
	String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
	a = b;
	check_eq(a.len(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[38], 'd');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, AssignFromLargeCopy, {
		String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = b;
		check_eq(a.len(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[38], 'd');
		check_eq(a.cstr()[39], '\0');
	});

test_case("AssignFromCopyNullBytesSanityCheck") {
	String a = "hbb"_str;
	String b = "ca"_str;
	a = b;
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	for (gk::i32 i = 2; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromCopyNullBytesSanityCheck, {
		String a = "hbb"_str;
		String b = "ca"_str;
		a = b;
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

test_case("AssignFromCopyReuseAllocation") {
	String a = "asjkhdglakjshdlakjshdlakjshdasadasd"_str;
	const char* oldBuffer = a.cstr();
	String b = "shsldkjahsldkjahlsdkjhp398ury08970897-98"_str;
	a = b;
	const char* newBuffer = a.cstr();
	check_eq(oldBuffer, newBuffer);
}

#pragma endregion

#pragma region Assign_Move

test_case("AssignFromSmallMove") {
	String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
	String b = "ca"_str;
	a = std::move(b);
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	check_eq(a.cstr()[2], '\0');
}

comptime_test_case(String, AssignFromSmallMove, {
		String a = "haaiusydp8iauysdoliuaqyweoiuqywepoiuaqyspediausd"_str;
		String b = "ca"_str;
		a = std::move(b);
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		check_eq(a.cstr()[2], '\0');
	});

test_case("AssignFromLargeMove") {
	String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
	String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
	a = std::move(b);
	check_eq(a.len(), 39);
	check_eq(a.cstr()[0], 'a');
	check_eq(a.cstr()[38], 'd');
	check_eq(a.cstr()[39], '\0');
}

comptime_test_case(String, AssignFromLargeMove, {
		String a = "hagsldihaglsdhalsiudhasduia;sikjdhlakjsdhl;akjsdh;akjsdh;akjshdoiuaysdo8q76wye08uyatsd"_str;
		String b = "aijshdliajshdlkajshdlkjashdlkajshdlaasd"_str;
		a = std::move(b);
		check_eq(a.len(), 39);
		check_eq(a.cstr()[0], 'a');
		check_eq(a.cstr()[38], 'd');
		check_eq(a.cstr()[39], '\0');
	});

test_case("AssignFromMoveNullBytesSanityCheck") {
	String a = "hbb"_str;
	String b = "ca"_str;
	a = std::move(b);
	check_eq(a.len(), 2);
	check_eq(a.cstr()[0], 'c');
	check_eq(a.cstr()[1], 'a');
	for (gk::i32 i = 2; i < 30; i++) {
		check_eq(a.cstr()[i], '\0');
	}
}

comptime_test_case(String, AssignFromMoveNullBytesSanityCheck, {
		String a = "hbb"_str;
		String b = "ca"_str;
		a = std::move(b);
		check_eq(a.len(), 2);
		check_eq(a.cstr()[0], 'c');
		check_eq(a.cstr()[1], 'a');
		for (int i = 2; i < 30; i++) {
			check_eq(a.cstr()[i], '\0');
		}
	});

#pragma endregion

#pragma region Equal_Char

test_case("EqualChar") {
	String a = 'c';
	check_eq(a, 'c');
}

comptime_test_case(String, EqualChar, {
		String a = 'c';
		check_eq(a, 'c');
	});

test_case("NotEqualChar") {
	String a = 'b';
	check_ne(a, 'c');
}

comptime_test_case(String, NotEqualChar, {
		String a = 'b';
		check_ne(a, 'c');
	});

test_case("NotEqualCharSameFirst") {
	String a = "ca"_str;
	check_ne(a, 'c');
}

comptime_test_case(String, NotEqualCharSameFirst, {
		String a = "ca"_str;
		check_ne(a, 'c');
	});

test_case("NotEqualCharAndLargeString") {
	String a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
	check_ne(a, 'c');
}

comptime_test_case(String, NotEqualCharAndLargeString, {
		String a = "calsjkhdglajhsgdlajhsgdoauiysgdoauyisgdoauhsgdlajhsgdlajhsgdlajhsd"_str;
		check_ne(a, 'c');
	});

#pragma endregion

#pragma region Equal_Str

test_case("EqualSmallStr") {
	String a = "hi"_str;
	check_eq(a, "hi"_str);
}

comptime_test_case(String, EqualSmallStr, {
		String a = "hi"_str;
		check_eq(a, "hi"_str);
	});

test_case("EqualSsoMaxStr") {
	String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
	check_eq(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
}

comptime_test_case(String, EqualSsoMaxStr, {
		String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		check_eq(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

test_case("EqualLargeStr") {
	String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
	check_eq(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
}

comptime_test_case(String, EqualLargeStr, {
		String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		check_eq(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

test_case("EqualUtf8SmallStr") {
	String a = "ßen"_str;
	check_eq(a, "ßen"_str);
}

comptime_test_case(String, EqualUtf8SmallStr, {
		String a = "ßen"_str;
		check_eq(a, "ßen"_str);
	});

test_case("EqualUtf8LargeStr") {
	String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	check_eq(a, "ÜbergrößenträgerÜbergrößenträ"_str);
}

comptime_test_case(String, EqualUtf8LargeStr, {
		String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		check_eq(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

test_case("NotEqualSmallStr") {
	String a = "hh"_str;
	check_ne(a, "hi"_str);
}

comptime_test_case(String, NotEqualSmallStr, {
		String a = "hh"_str;
		check_ne(a, "hi"_str);
	});

test_case("NotEqualSsoMaxStr") {
	String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
	check_ne(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
}

comptime_test_case(String, NotEqualSsoMaxStr, {
		String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		check_ne(a, "ashdlakjshdlkajshdlkjasdasdddg"_str);
	});

test_case("NotEqualLargeStr") {
	String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
	check_ne(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
}

comptime_test_case(String, NotEqualLargeStr, {
		String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		check_ne(a, "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str);
	});

test_case("NotEqualUtf8Small") {
	String a = "ßeb"_str;
	check_ne(a, "ßen"_str);
}

comptime_test_case(String, NotEqualUtf8Small, {
		String a = "ßeb"_str;
		check_ne(a, "ßen"_str);
	});

test_case("NotEqualUtf8Large") {
	String a = "ÜbergrößenträgerÜbargrößenträ"_str;
	check_ne(a, "ÜbergrößenträgerÜbergrößenträ"_str);
}

comptime_test_case(String, NotEqualUtf8Large, {
		String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		check_ne(a, "ÜbergrößenträgerÜbergrößenträ"_str);
	});

#pragma endregion

#pragma region Equal_Other_String

test_case("EqualCharOtherString") {
	String a = 'c';
	check_eq(a, String('c'));
}

comptime_test_case(String, EqualCharOtherString, {
		String a = 'c';
		check_eq(a, String('c'));
	});

test_case("EqualSmallOtherString") {
	String a = "hi"_str;
	check_eq(a, String("hi"_str));
}

comptime_test_case(String, EqualSmallOtherString, {
		String a = "hi"_str;
		check_eq(a, String("hi"_str));
	});

test_case("EqualSsoMaxOtherString") {
	String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
	check_eq(a, String("ashdlakjshdlkajshdlkjasdasdddg"_str));
}

comptime_test_case(String, EqualSsoMaxOtherString, {
		String a = "ashdlakjshdlkajshdlkjasdasdddg"_str;
		check_eq(a, String("ashdlakjshdlkajshdlkjasdasdddg"_str));
	});

test_case("EqualLargeOtherString") {
	String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
	check_eq(a, String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
}

comptime_test_case(String, EqualLargeOtherString, {
		String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str;
		check_eq(a, String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	});

test_case("EqualUtf8SmallOtherString") {
	String a = "ßen"_str;
	check_eq(a, String("ßen"_str));
}

comptime_test_case(String, EqualUtf8SmallOtherString, {
		String a = "ßen"_str;
		check_eq(a, String("ßen"_str));
	});

test_case("EqualUtf8LargeOtherString") {
	String a = "ÜbergrößenträgerÜbergrößenträ"_str;
	check_eq(a, String("ÜbergrößenträgerÜbergrößenträ"_str));
}

comptime_test_case(String, EqualUtf8LargeOtherString, {
		String a = "ÜbergrößenträgerÜbergrößenträ"_str;
		check_eq(a, String("ÜbergrößenträgerÜbergrößenträ"_str));
	});

test_case("NotEqualSmallStrOtherString") {
	String a = "hh"_str;
	check_ne(a, String("hi"_str));
}

comptime_test_case(String, NotEqualSmallStrOtherString, {
		String a = "hh"_str;
		check_ne(a, String("hi"_str));
	});

test_case("NotEqualSsoMaxStrOtherString") {
	String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
	check_ne(a, String("ashdlakjshdlkajshdlkjasdasdddg"_str));
}

comptime_test_case(String, NotEqualSsoMaxStrOtherString, {
		String a = "bshdlakjshdlkajshdlkjasdasdddg"_str;
		check_ne(a, String("ashdlakjshdlkajshdlkjasdasdddg"_str));
	});

test_case("NotEqualLargeStrOtherString") {
	String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
	check_ne(a, String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
}

comptime_test_case(String, NotEqualLargeStrOtherString, {
		String a = "ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwsteoiuywgoiuy6203871602837610238761023"_str;
		check_ne(a, String("ashdlakjshdlkajshdlkjasdasdddgaksjhdgaljshdglajshdglaiuwyteoiuywgoiuy6203871602837610238761023"_str));
	});

test_case("NotEqualUtf8SmallOtherString") {
	String a = "ßeb"_str;
	check_ne(a, String("ßen"_str));
}

comptime_test_case(String, NotEqualUtf8SmallOtherString, {
		String a = "ßeb"_str;
		check_ne(a, String("ßen"_str));
	});

test_case("NotEqualUtf8LargeOtherString") {
	String a = "ÜbergrößenträgerÜbargrößenträ"_str;
	check_ne(a, String("ÜbergrößenträgerÜbergrößenträ"_str));
}

comptime_test_case(String, NotEqualUtf8LargeOtherString, {
		String a = "ÜbergrößenträgerÜbargrößenträ"_str;
		check_ne(a, String("ÜbergrößenträgerÜbergrößenträ"_str));
	});

#pragma endregion

#pragma region Append

test_case("EmptyStringAppendChar") {
	String a;
	a.append('c');
	check_eq(a, 'c');
	check_eq(a, String('c')); // for sanity, same with following tests
}

comptime_test_case(String, EmptyStringAppendChar, {
		String a;
		a.append('c');
		check_eq(a, 'c');
		check_eq(a, String('c'));
	});

test_case("SmallStringAppendChar") {
	String a = "hello"_str;
	a.append('!');
	check_eq(a, "hello!"_str);
	check_eq(a, String("hello!"_str));
}

comptime_test_case(String, SmallStringAppendChar, {
		String a = "hello"_str;
		a.append('!');
		check_eq(a, "hello!"_str);
		check_eq(a, String("hello!"_str));
	});

test_case("SmallStringAppendCharMakeHeap") {
	String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
	a.append('!');
	check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
	check_eq(a, String("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
}

comptime_test_case(String, SmallStringAppendCharMakeHeap, {
		String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append('!');
		check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!"_str);
		check_eq(a, String("ahlskdjhalskjdhlaskjdhlakjsgga!"_str));
	});

test_case("LargeStringAppendChar") {
	String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
	a.append('a');
	check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
	check_eq(a, String("1672038761203876102873601287630187263018723601872630187263018723a"_str));
}

comptime_test_case(String, LargeStringAppendChar, {
		String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append('a');
		check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723a"_str);
		check_eq(a, String("1672038761203876102873601287630187263018723601872630187263018723a"_str));
	});

test_case("SmallUtf8AppendChar") {
	String a = "ßeb"_str;
	a.append('?');
	check_eq(a, "ßeb?"_str);
	check_eq(a, String("ßeb?"_str));
}

comptime_test_case(String, SmallUtf8AppendChar, {
		String a = "ßeb"_str;
		a.append('?');
		check_eq(a, "ßeb?"_str);
		check_eq(a, String("ßeb?"_str));
	});

test_case("SmallUtf8AppendCharMakeHeap") {
	String a = "ÜbergrößenträgerÜbergröa"_str;
	a.append('l');
	check_eq(a, "ÜbergrößenträgerÜbergröal"_str);
	check_eq(a, String("ÜbergrößenträgerÜbergröal"_str));
}

comptime_test_case(String, SmallUtf8AppendCharMakeHeap, {
		String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append('l');
		check_eq(a, "ÜbergrößenträgerÜbergröal"_str);
		check_eq(a, String("ÜbergrößenträgerÜbergröal"_str));
	});

test_case("AppendCharHeapReallocate") {
	String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append('5');
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
	check_eq(a, String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
}

comptime_test_case(String, AppendCharHeapReallocate, {
		String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append('5');
		check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str);
		check_eq(a, String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl5"_str));
	});

#pragma endregion

#pragma region Append_Str

test_case("EmptyStringAppendStr") {
	String a;
	a.append("cc"_str);
	check_eq(a, "cc"_str);
	check_eq(a, String("cc"_str)); // for sanity, same with following tests
}

comptime_test_case(String, EmptyStringAppendStr, {
		String a;
		a.append("cc"_str);
		check_eq(a, "cc"_str);
		check_eq(a, String("cc"_str));
	});

test_case("SmallStringAppendStr") {
	String a = "hello"_str;
	a.append("!!"_str);
	check_eq(a, "hello!!"_str);
	check_eq(a, String("hello!!"_str));
}

comptime_test_case(String, SmallStringAppendStr, {
		String a = "hello"_str;
		a.append("!!"_str);
		check_eq(a, "hello!!"_str);
		check_eq(a, String("hello!!"_str));
	});

test_case("SmallStringAppendStrMakeHeap") {
	String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
	a.append("!!"_str);
	check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
	check_eq(a, String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
}

comptime_test_case(String, SmallStringAppendStrMakeHeap, {
		String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append("!!"_str);
		check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		check_eq(a, String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	});

test_case("LargeStringAppendStr") {
	String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
	a.append("aa"_str);
	check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
	check_eq(a, String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
}

comptime_test_case(String, LargeStringAppendStr, {
		String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append("aa"_str);
		check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		check_eq(a, String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	});

test_case("SmallUtf8AppendStr") {
	String a = "ßeb"_str;
	a.append("??"_str);
	check_eq(a, "ßeb??"_str);
	check_eq(a, String("ßeb??"_str));
}

comptime_test_case(String, SmallUtf8AppendStr, {
		String a = "ßeb"_str;
		a.append("??"_str);
		check_eq(a, "ßeb??"_str);
		check_eq(a, String("ßeb??"_str));
	});

test_case("SmallUtf8AppendStrMakeHeap") {
	String a = "ÜbergrößenträgerÜbergröa"_str;
	a.append("ll"_str);
	check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
	check_eq(a, String("ÜbergrößenträgerÜbergröall"_str));
}

comptime_test_case(String, SmallUtf8AppendStrMakeHeap, {
		String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append("ll"_str);
		check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
		check_eq(a, String("ÜbergrößenträgerÜbergröall"_str));
	});

test_case("AppendStrHeapReallocate") {
	String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append("55"_str);
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
	check_eq(a, String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
}

comptime_test_case(String, AppendStrHeapReallocate, {
		String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
		a.append("55"_str);
		check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
		check_eq(a, String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	});

#pragma endregion

#pragma region Append_Other_String

test_case("EmptyStringAppendOtherString") {
	String a;
	a.append(String("cc"_str));
	check_eq(a, "cc"_str);
	check_eq(a, String("cc"_str)); // for sanity, same with following tests
}

comptime_test_case(String, EmptyStringAppendOtherString, {
		String a;
		a.append(String("cc"_str));
		check_eq(a, "cc"_str);
		check_eq(a, String("cc"_str));
	});

test_case("SmallStringAppendOtherString") {
	String a = "hello"_str;
	a.append(String("!!"_str));
	check_eq(a, "hello!!"_str);
	check_eq(a, String("hello!!"_str));
}

comptime_test_case(String, SmallStringAppendOtherString, {
		String a = "hello"_str;
		a.append(String("!!"_str));
		check_eq(a, "hello!!"_str);
		check_eq(a, String("hello!!"_str));
	});

test_case("SmallStringAppendOtherStringMakeHeap") {
	String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
	a.append(String("!!"_str));
	check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
	check_eq(a, String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
}

comptime_test_case(String, SmallStringAppendOtherStringMakeHeap, {
		String a = "ahlskdjhalskjdhlaskjdhlakjsgga"_str;
		a.append(String("!!"_str));
		check_eq(a, "ahlskdjhalskjdhlaskjdhlakjsgga!!"_str);
		check_eq(a, String("ahlskdjhalskjdhlaskjdhlakjsgga!!"_str));
	});

test_case("LargeStringAppendOtherString") {
	String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
	a.append(String("aa"_str));
	check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
	check_eq(a, String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
}

comptime_test_case(String, LargeStringAppendOtherString, {
		String a = "1672038761203876102873601287630187263018723601872630187263018723"_str;
		a.append(String("aa"_str));
		check_eq(a, "1672038761203876102873601287630187263018723601872630187263018723aa"_str);
		check_eq(a, String("1672038761203876102873601287630187263018723601872630187263018723aa"_str));
	});

test_case("SmallUtf8AppendOtherString") {
	String a = "ßeb"_str;
	a.append(String("??"_str));
	check_eq(a, "ßeb??"_str);
	check_eq(a, String("ßeb??"_str));
}

comptime_test_case(String, SmallUtf8AppendOtherString, {
		String a = "ßeb"_str;
		a.append(String("??"_str));
		check_eq(a, "ßeb??"_str);
		check_eq(a, String("ßeb??"_str));
	});

test_case("SmallUtf8AppendOtherStringMakeHeap") {
	String a = "ÜbergrößenträgerÜbergröa"_str;
	a.append(String("ll"_str));
	check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
	check_eq(a, String("ÜbergrößenträgerÜbergröall"_str));
}

comptime_test_case(String, SmallUtf8AppendOtherStringMakeHeap, {
		String a = "ÜbergrößenträgerÜbergröa"_str;
		a.append(String("ll"_str));
		check_eq(a, "ÜbergrößenträgerÜbergröall"_str);
		check_eq(a, String("ÜbergrößenträgerÜbergröall"_str));
	});

test_case("AppendOtherStringHeapReallocate") {
	String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append(String("55"_str));
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
	check_eq(a, String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
}

comptime_test_case(String, AppendOtherStringHeapReallocate, {
	String a = "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl"_str;
	a.append(String("55"_str));
	check_eq(a, "askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str);
	check_eq(a, String("askjdhlakjshl;iuhgl;isudhvlisuhdfoliuaysdoiauhsfoaajhgblkajdhfl55"_str));
	});

#pragma endregion

#pragma region Concat_Char

test_case("ConcatEmptyAndChar") {
	const String a;
	String b = a + 'c';
	check_eq(b, 'c');
	check_eq(b, String('c'));
}

comptime_test_case(String, ConcatEmptyAndChar, {
		const String a;
		String b = a + 'c';
		check_eq(b, 'c');
		check_eq(b, String('c'));
	});

test_case("ConcatCharStringAndChar") {
	const String a = 'c';
	String b = a + 'c';
	check_eq(b, "cc"_str);
	check_eq(b, String("cc"_str));
}

comptime_test_case(String, ConcatCharStringAndChar, {
		const String a = 'c';
		String b = a + 'c';
		check_eq(b, "cc"_str);
		check_eq(b, String("cc"_str));
	});

test_case("ConcatSmallStringAndCharToHeap") {
	const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	String b = a + 'c';
	check_eq(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
	check_eq(b, String("aslasdasddkjahldkjahsldkjahsdac"_str));
}

comptime_test_case(String, ConcatSmallStringAndCharToHeap, {
		const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		String b = a + 'c';
		check_eq(b, "aslasdasddkjahldkjahsldkjahsdac"_str);
		check_eq(b, String("aslasdasddkjahldkjahsldkjahsdac"_str));
	});

test_case("ConcatHeapStringAndCharToHeap") {
	const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	String b = a + 'c';
	check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
	check_eq(b, String("aslasdasddkjahl55dkjahsldkjahsdac"_str));
}

comptime_test_case(String, ConcatHeapStringAndCharToHeap, {
		const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		String b = a + 'c';
		check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdac"_str);
		check_eq(b, String("aslasdasddkjahl55dkjahsldkjahsdac"_str));
	});

test_case("ConcatSmallUtf8AndChar") {
	const String a = "Übergrößenträger"_str;
	String b = a + 'c';
	check_eq(b, "Übergrößenträgerc"_str);
	check_eq(b, String("Übergrößenträgerc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndChar, {
		const String a = "Übergrößenträger"_str;
		String b = a + 'c';
		check_eq(b, "Übergrößenträgerc"_str);
		check_eq(b, String("Übergrößenträgerc"_str));
	});

test_case("ConcatSmallUtf8AndCharToHeap") {
	const String a = "Übergrößenträgerasjhdgashh"_str;
	String b = a + 'c';
	check_eq(b, "Übergrößenträgerasjhdgashhc"_str);
	check_eq(b, String("Übergrößenträgerasjhdgashhc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndCharToHeap, {
		const String a = "Übergrößenträgerasjhdgashh"_str;
		String b = a + 'c';
		check_eq(b, "Übergrößenträgerasjhdgashhc"_str);
		check_eq(b, String("Übergrößenträgerasjhdgashhc"_str));
	});

test_case("ConcatHeapUtf8AndChar") {
	const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = a + 'c';
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
	check_eq(b, String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
}

comptime_test_case(String, ConcatHeapUtf8AndChar, {
		const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = a + 'c';
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str);
		check_eq(b, String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerc"_str));
	});

#pragma endregion

#pragma region Concat_Char_Inverted

test_case("InvertConcatEmptyAndChar") {
	const String a;
	String b = 'c' + a;
	check_eq(b, 'c');
	check_eq(b, String('c'));
}

comptime_test_case(String, InvertConcatEmptyAndChar, {
		const String a;
		String b = 'c' + a;
		check_eq(b, 'c');
		check_eq(b, String('c'));
	});

test_case("InvertConcatCharStringAndChar") {
	const String a = 'c';
	String b = 'c' + a;
	check_eq(b, "cc"_str);
	check_eq(b, String("cc"_str));
}

comptime_test_case(String, InvertConcatCharStringAndChar, {
		const String a = 'c';
		String b = 'c' + a;
		check_eq(b, "cc"_str);
		check_eq(b, String("cc"_str));
	});

test_case("InvertConcatSmallStringAndCharToHeap") {
	const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	String b = 'c' + a;
	check_eq(b, "caslasdasddkjahldkjahsldkjahsda"_str);
	check_eq(b, String("caslasdasddkjahldkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatSmallStringAndCharToHeap, {
		const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		String b = 'c' + a;
		check_eq(b, "caslasdasddkjahldkjahsldkjahsda"_str);
		check_eq(b, String("caslasdasddkjahldkjahsldkjahsda"_str));
	});

test_case("InvertConcatHeapStringAndCharToHeap") {
	const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	String b = 'c' + a;
	check_eq(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
	check_eq(b, String("caslasdasddkjahl55dkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatHeapStringAndCharToHeap, {
		const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		String b = 'c' + a;
		check_eq(b, "caslasdasddkjahl55dkjahsldkjahsda"_str);
		check_eq(b, String("caslasdasddkjahl55dkjahsldkjahsda"_str));
	});

test_case("InvertConcatSmallUtf8AndChar") {
	const String a = "Übergrößenträger"_str;
	String b = 'c' + a;
	check_eq(b, "cÜbergrößenträger"_str);
	check_eq(b, String("cÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndChar, {
		const String a = "Übergrößenträger"_str;
		String b = 'c' + a;
		check_eq(b, "cÜbergrößenträger"_str);
		check_eq(b, String("cÜbergrößenträger"_str));
	});

test_case("InvertConcatSmallUtf8AndCharToHeap") {
	const String a = "Übergrößenträgerasjhdgashh"_str;
	String b = 'c' + a;
	check_eq(b, "cÜbergrößenträgerasjhdgashh"_str);
	check_eq(b, String("cÜbergrößenträgerasjhdgashh"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndCharToHeap, {
		const String a = "Übergrößenträgerasjhdgashh"_str;
		String b = 'c' + a;
		check_eq(b, "cÜbergrößenträgerasjhdgashh"_str);
		check_eq(b, String("cÜbergrößenträgerasjhdgashh"_str));
	});

test_case("InvertConcatHeapUtf8AndChar") {
	const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = 'c' + a;
	check_eq(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	check_eq(b, String("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatHeapUtf8AndChar, {
		const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = 'c' + a;
		check_eq(b, "cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		check_eq(b, String("cÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	});

#pragma endregion

#pragma region Concat_Str

test_case("ConcatEmptyAndStr") {
	const String a;
	String b = a + "cc"_str;
	check_eq(b, "cc"_str);
	check_eq(b, String("cc"_str));
}

comptime_test_case(String, ConcatEmptyAndStr, {
		const String a;
		String b = a + "cc"_str;
		check_eq(b, "cc"_str);
		check_eq(b, String("cc"_str));
	});

test_case("ConcatCharStringAndStr") {
	const String a = 'c';
	String b = a + "cc"_str;
	check_eq(b, "ccc"_str);
	check_eq(b, String("ccc"_str));
}

comptime_test_case(String, ConcatCharStringAndStr, {
		const String a = 'c';
		String b = a + "cc"_str;
		check_eq(b, "ccc"_str);
		check_eq(b, String("ccc"_str));
	});

test_case("ConcatSmallStringAndStrToHeap") {
	const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	String b = a + "cc"_str;
	check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
	check_eq(b, String("aslasdasddkjahldkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatSmallStringAndStrToHeap, {
		const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		String b = a + "cc"_str;
		check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		check_eq(b, String("aslasdasddkjahldkjahsldkjahsdacc"_str));
	});

test_case("ConcatHeapStringAndStrToHeap") {
	const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	String b = a + "cc"_str;
	check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
	check_eq(b, String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatHeapStringAndStrToHeap, {
		const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		String b = a + "cc"_str;
		check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		check_eq(b, String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	});

test_case("ConcatSmallUtf8AndStr") {
	const String a = "Übergrößenträger"_str;
	String b = a + "cc"_str;
	check_eq(b, "Übergrößenträgercc"_str);
	check_eq(b, String("Übergrößenträgercc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndStr, {
		const String a = "Übergrößenträger"_str;
		String b = a + "cc"_str;
		check_eq(b, "Übergrößenträgercc"_str);
		check_eq(b, String("Übergrößenträgercc"_str));
	});

test_case("ConcatSmallUtf8AndStrToHeap") {
	const String a = "Übergrößenträgerasjhdgashh"_str;
	String b = a + "cc"_str;
	check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
	check_eq(b, String("Übergrößenträgerasjhdgashhcc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndStrToHeap, {
		const String a = "Übergrößenträgerasjhdgashh"_str;
		String b = a + "cc"_str;
		check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
		check_eq(b, String("Übergrößenträgerasjhdgashhcc"_str));
	});

test_case("ConcatHeapUtf8AndStr") {
	const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = a + "cc"_str;
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
	check_eq(b, String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
}

comptime_test_case(String, ConcatHeapUtf8AndStr, {
		const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = a + "cc"_str;
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		check_eq(b, String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	});

#pragma endregion

#pragma region Concat_Str_Inverted

test_case("InvertConcatEmptyAndStr") {
	const String a;
	String b = "cc"_str + a;
	check_eq(b, "cc"_str);
	check_eq(b, String("cc"_str));
}

comptime_test_case(String, InvertConcatEmptyAndStr, {
		const String a;
		String b = "cc"_str + a;
		check_eq(b, "cc"_str);
		check_eq(b, String("cc"_str));
	});

test_case("InvertConcatCharStringAndStr") {
	const String a = 'c';
	String b = "cc"_str + a;
	check_eq(b, "ccc"_str);
	check_eq(b, String("ccc"_str));
}

comptime_test_case(String, InvertConcatCharStringAndStr, {
		const String a = 'c';
		String b = "cc"_str + a;
		check_eq(b, "ccc"_str);
		check_eq(b, String("ccc"_str));
	});

test_case("InvertConcatSmallStringAndStrToHeap") {
	const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	String b = "cc"_str + a;
	check_eq(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
	check_eq(b, String("ccaslasdasddkjahldkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatSmallStringAndStrToHeap, {
		const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		String b = "cc"_str + a;
		check_eq(b, "ccaslasdasddkjahldkjahsldkjahsda"_str);
		check_eq(b, String("ccaslasdasddkjahldkjahsldkjahsda"_str));
	});

test_case("InvertConcatHeapStringAndStrToHeap") {
	const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	String b = "cc"_str + a;
	check_eq(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
	check_eq(b, String("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
}

comptime_test_case(String, InvertConcatHeapStringAndStrToHeap, {
		const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		String b = "cc"_str + a;
		check_eq(b, "ccaslasdasddkjahl55dkjahsldkjahsda"_str);
		check_eq(b, String("ccaslasdasddkjahl55dkjahsldkjahsda"_str));
	});

test_case("InvertConcatSmallUtf8AndStr") {
	const String a = "Übergrößenträger"_str;
	String b = "cc"_str + a;
	check_eq(b, "ccÜbergrößenträger"_str);
	check_eq(b, String("ccÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndStr, {
		const String a = "Übergrößenträger"_str;
		String b = "cc"_str + a;
		check_eq(b, "ccÜbergrößenträger"_str);
		check_eq(b, String("ccÜbergrößenträger"_str));
	});

test_case("InvertConcatSmallUtf8AndStrToHeap") {
	const String a = "Übergrößenträgerasjhdgashh"_str;
	String b = "cc"_str + a;
	check_eq(b, "ccÜbergrößenträgerasjhdgashh"_str);
	check_eq(b, String("ccÜbergrößenträgerasjhdgashh"_str));
}

comptime_test_case(String, InvertConcatSmallUtf8AndStrToHeap, {
		const String a = "Übergrößenträgerasjhdgashh"_str;
		String b = "cc"_str + a;
		check_eq(b, "ccÜbergrößenträgerasjhdgashh"_str);
		check_eq(b, String("ccÜbergrößenträgerasjhdgashh"_str));
	});

test_case("InvertConcatHeapUtf8AndStr") {
	const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = "cc"_str + a;
	check_eq(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	check_eq(b, String("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
}

comptime_test_case(String, InvertConcatHeapUtf8AndStr, {
		const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = "cc"_str + a;
		check_eq(b, "ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
		check_eq(b, String("ccÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str));
	});

#pragma endregion

#pragma region Concat_Two_Strings

test_case("ConcatEmptyAndOtherString") {
	const String a;
	String b = a + String("cc"_str);
	check_eq(b, "cc"_str);
	check_eq(b, String("cc"_str));
}

comptime_test_case(String, ConcatEmptyAndOtherString, {
		const String a;
		String b = a + String("cc"_str);
		check_eq(b, "cc"_str);
		check_eq(b, String("cc"_str));
	});

test_case("ConcatCharStringAndOtherString") {
	const String a = 'c';
	String b = a + String("cc"_str);
	check_eq(b, "ccc"_str);
	check_eq(b, String("ccc"_str));
}

comptime_test_case(String, ConcatCharStringAndOtherString, {
		const String a = 'c';
		String b = a + String("cc"_str);
		check_eq(b, "ccc"_str);
		check_eq(b, String("ccc"_str));
	});

test_case("ConcatSmallStringAndOtherStringToHeap") {
	const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
	String b = a + String("cc"_str);
	check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
	check_eq(b, String("aslasdasddkjahldkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatSmallStringAndOtherStringToHeap, {
		const String a = "aslasdasddkjahldkjahsldkjahsda"_str;
		String b = a + String("cc"_str);
		check_eq(b, "aslasdasddkjahldkjahsldkjahsdacc"_str);
		check_eq(b, String("aslasdasddkjahldkjahsldkjahsdacc"_str));
	});

test_case("ConcatHeapStringAndOtherStringToHeap") {
	const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
	String b = a + String("cc"_str);
	check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
	check_eq(b, String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
}

comptime_test_case(String, ConcatHeapStringAndOtherStringToHeap, {
		const String a = "aslasdasddkjahl55dkjahsldkjahsda"_str;
		String b = a + String("cc"_str);
		check_eq(b, "aslasdasddkjahl55dkjahsldkjahsdacc"_str);
		check_eq(b, String("aslasdasddkjahl55dkjahsldkjahsdacc"_str));
	});

test_case("ConcatSmallUtf8AndOtherString") {
	const String a = "Übergrößenträger"_str;
	String b = a + String("cc"_str);
	check_eq(b, "Übergrößenträgercc"_str);
	check_eq(b, String("Übergrößenträgercc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndOtherString, {
		const String a = "Übergrößenträger"_str;
		String b = a + String("cc"_str);
		check_eq(b, "Übergrößenträgercc"_str);
		check_eq(b, String("Übergrößenträgercc"_str));
	});

test_case("ConcatSmallUtf8AndOtherStringToHeap") {
	const String a = "Übergrößenträgerasjhdgashh"_str;
	String b = a + String("cc"_str);
	check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
	check_eq(b, String("Übergrößenträgerasjhdgashhcc"_str));
}

comptime_test_case(String, ConcatSmallUtf8AndOtherStringToHeap, {
		const String a = "Übergrößenträgerasjhdgashh"_str;
		String b = a + String("cc"_str);
		check_eq(b, "Übergrößenträgerasjhdgashhcc"_str);
		check_eq(b, String("Übergrößenträgerasjhdgashhcc"_str));
	});

test_case("ConcatHeapUtf8AndOtherString") {
	const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = a + String("cc"_str);
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
	check_eq(b, String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
}

comptime_test_case(String, ConcatHeapUtf8AndOtherString, {
		const String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = a + String("cc"_str);
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str);
		check_eq(b, String("ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgercc"_str));
	});

#pragma endregion

#pragma region Concat_Multiple

test_case("ChainConcat") {
	String a = "hello world!"_str;
	String b = a + ' ' + "hmm"_str + " t" + 'h' + String("is is") + " a multi concat string thats quite large"_str;
	check_eq(b, "hello world! hmm this is a multi concat string thats quite large"_str);
	check_eq(b, String("hello world! hmm this is a multi concat string thats quite large"_str));
}

comptime_test_case(String, ChainConcat, {
		String a = "hello world!"_str;
		String b = a + ' ' + "hmm"_str + " t" + 'h' + String("is is") + " a multi concat string thats quite large"_str;
		check_eq(b, "hello world! hmm this is a multi concat string thats quite large"_str);
		check_eq(b, String("hello world! hmm this is a multi concat string thats quite large"_str));
	});

#pragma endregion

#pragma region From_Type

test_case("FromBoolTrue") {
	String a = String::fromBool(true);
	check_eq(a, "true"_str);
	check_eq(a, String("true"_str));
}

comptime_test_case(String, FromBoolTrue, {
		String a = String::fromBool(true);
		check_eq(a, "true"_str);
		check_eq(a, String("true"_str));
	});

test_case("FromBoolFalse") {
	String a = String::fromBool(false);
	check_eq(a, "false"_str);
	check_eq(a, String("false"_str));
}

comptime_test_case(String, FromBoolFalse, {
		String a = String::fromBool(false);
		check_eq(a, "false"_str);
		check_eq(a, String("false"_str));
	});

test_case("FromSignedIntZero") {
	String a = String::fromInt(0);
	check_eq(a, '0');
}

comptime_test_case(String, FromSignedIntZero, {
	String a = String::fromInt(0);
	check_eq(a, '0');
	});

test_case("FromSignedIntSmallValue") {
	String a = String::fromInt(16);
	check_eq(a, "16"_str);
}

comptime_test_case(String, FromSignedIntSmallValue, {
		String a = String::fromInt(16);
		check_eq(a, "16"_str);
	});

test_case("FromSignedIntMaximumValue") {
	String a = String::fromInt(std::numeric_limits<gk::i64>::max());
	check_eq(a, "9223372036854775807"_str);
}

comptime_test_case(String, FromSignedIntMaximumValue, {
		String a = String::fromInt(std::numeric_limits<gk::i64>::max());
		check_eq(a, "9223372036854775807"_str);
	});

test_case("FromSignedIntSmallNegativeValue") {
	String a = String::fromInt(-3);
	check_eq(a, "-3"_str);
}

comptime_test_case(String, FromSignedIntSmallNegativeValue, {
		String a = String::fromInt(-3);
		check_eq(a, "-3"_str);
	});

test_case("FromSignedIntMinimumValue") {
	String a = String::fromInt(std::numeric_limits<gk::i64>::min());
	check_eq(a, "-9223372036854775808"_str);
}

comptime_test_case(String, FromSignedIntMinimumValue, {
		String a = String::fromInt(std::numeric_limits<gk::i64>::min());
		check_eq(a, "-9223372036854775808"_str);
	});

test_case("FromUnsignedIntZero") {
	String a = String::fromUint(0);
	check_eq(a, '0');
}

comptime_test_case(String, FromUnsignedIntZero, {
		String a = String::fromUint(0);
		check_eq(a, '0');
	});

test_case("FromUnsignedIntSmallValue") {
	String a = String::fromUint(23);
	check_eq(a, "23"_str);
}

comptime_test_case(String, FromUnsignedIntSmallValue, {
		String a = String::fromUint(23);
		check_eq(a, "23"_str);
	});

test_case("FromUnsignedIntMaximumValue") {
	String a = String::fromUint(std::numeric_limits<gk::u64>::max());
	check_eq(a, "18446744073709551615"_str);
}

comptime_test_case(String, FromUnsignedIntMaximumValue, {
		String a = String::fromUint(std::numeric_limits<gk::u64>::max());
		check_eq(a, "18446744073709551615"_str);
	});

test_case("FromFloatZero") {
	String a = String::fromFloat(0.0);
	check_eq(a, "0.0"_str);
}

comptime_test_case(String, FromFloatZero, {
		String a = String::fromFloat(0.0);
		check_eq(a, "0.0"_str);
	});

test_case("FromFloatPositiveInfinity") {
	String a = String::fromFloat(INFINITY);
	check_eq(a, "inf"_str);
}

test_case("FromFloatNegativeInfinity") {
	String a = String::fromFloat(-1.0 * INFINITY);
	check_eq(a, "-inf"_str);
}

test_case("FromFloatNaN") {
	String a = String::fromFloat(NAN);
	check_eq(a, "nan"_str);
}

test_case("FromFloatWholeNumber") {
	String a = String::fromFloat(100.0);
	check_eq(a, "100.0"_str);
}

comptime_test_case(String, FromFloatWholeNumber, {
		String a = String::fromFloat(100.0);
		check_eq(a, "100.0"_str);
	});

test_case("FromFloatWholeNegativeNumber") {
	String a = String::fromFloat(-100.0);
	check_eq(a, "-100.0"_str);
}

comptime_test_case(String, FromFloatWholeNegativeNumber, {
		String a = String::fromFloat(-100.0);
		check_eq(a, "-100.0"_str);
	});

test_case("FromFloatDecimalNumber") {
	String a = String::fromFloat(100.09999);
	check_eq(a, "100.09999"_str);
}

comptime_test_case(String, FromFloatDecimalNumber, {
		String a = String::fromFloat(100.09999);
		check_eq(a, "100.09999"_str);
	});

test_case("FromFloatDecimalNegativeNumber") {
	String a = String::fromFloat(-100.09999);
	check_eq(a, "-100.09999"_str);
}

comptime_test_case(String, FromFloatDecimalNegativeNumber, {
		String a = String::fromFloat(-100.09999);
		check_eq(a, "-100.09999"_str);
	});

test_case("FromFloatDecimalNumberDefaultPrecision") {
	String a = String::fromFloat(100.12000005);
	check_eq(a, "100.12"_str);
}

comptime_test_case(String, FromFloatDecimalNumberDefaultPrecision, {
		String a = String::fromFloat(100.12000005);
		check_eq(a, "100.12"_str);
	});

test_case("FromFloatDecimalNegativeNumberDefaultPrecision") {
	String a = String::fromFloat(-100.12000005);
	check_eq(a, "-100.12"_str);
}

comptime_test_case(String, FromFloatDecimalNegativeNumberDefaultPrecision, {
		String a = String::fromFloat(-100.12000005);
		check_eq(a, "-100.12"_str);
	});

test_case("FromFloatDecimalNumberCustomPrecision") {
	String a = String::fromFloat(100.12000005, 10);
	check_eq(a, "100.12000005"_str);
}

comptime_test_case(String, FromFloatDecimalNumberCustomPrecision, {
		String a = String::fromFloat(100.12000005, 10);
		check_eq(a, "100.12000005"_str);
	});

test_case("FromFloatDecimalNegativeNumberCustomPrecision") {
	String a = String::fromFloat(-100.12000005, 10);
	check_eq(a, "-100.12000005"_str);
}

comptime_test_case(String, FromFloatDecimalNegativeNumberCustomPrecision, {
		String a = String::fromFloat(-100.12000005, 10);
		check_eq(a, "-100.12000005"_str);
	});

test_case("FromTemplateBool") {
	bool b = true;
	String a = String::from(b);
	check_eq(a, "true"_str);
}

comptime_test_case(String, FromTemplateBool, {
		bool b = true;
		String a = String::from(b);
		check_eq(a, "true"_str);
	});

test_case("FromTemplategk::i8") {
	gk::i8 num = -56;
	String a = String::from(num);
	check_eq(a, "-56"_str);
}

comptime_test_case(String, FromTemplateInt8, {
		gk::i8 num = -56;
		String a = String::from(num);
		check_eq(a, "-56"_str);
	});

test_case("FromTemplategk::u8") {
	gk::u8 num = 56;
	String a = String::from(num);
	check_eq(a, "56"_str);
}

comptime_test_case(String, FromTemplateUint8, {
		gk::u8 num = 56;
		String a = String::from(num);
		check_eq(a, "56"_str);
	});

test_case("FromTemplategk::i16") {
	gk::i16 num = -1000;
	String a = String::from(num);
	check_eq(a, "-1000"_str);
}

comptime_test_case(String, FromTemplateInt16, {
		gk::i16 num = -1000;
		String a = String::from(num);
		check_eq(a, "-1000"_str);
	});

test_case("FromTemplategk::u16") {
	gk::u16 num = 1000;
	String a = String::from(num);
	check_eq(a, "1000"_str);
}

comptime_test_case(String, FromTemplateUint16, {
		gk::u16 num = 1000;
		String a = String::from(num);
		check_eq(a, "1000"_str);
	});

test_case("FromTemplategk::i32") {
	gk::i32 num = -99999;
	String a = String::from(num);
	check_eq(a, "-99999"_str);
}

comptime_test_case(String, FromTemplateInt32, {
		gk::i32 num = -99999;
		String a = String::from(num);
		check_eq(a, "-99999"_str);
	});

test_case("FromTemplategk::u32") {
	gk::u32 num = 99999;
	String a = String::from(num);
	check_eq(a, "99999"_str);
}

comptime_test_case(String, FromTemplateUint32, {
		gk::u32 num = 99999;
		String a = String::from(num);
		check_eq(a, "99999"_str);
	});

test_case("FromTemplategk::i64") {
	gk::i64 num = -123456789012345;
	String a = String::from(num);
	check_eq(a, "-123456789012345"_str);
}

comptime_test_case(String, FromTemplateInt64, {
		gk::i64 num = -123456789012345;
		String a = String::from(num);
		check_eq(a, "-123456789012345"_str);
	});

test_case("FromTemplategk::u64") {
	gk::u64 num = 123456789012345;
	String a = String::from(num);
	check_eq(a, "123456789012345"_str);
}

comptime_test_case(String, FromTemplateUint64, {
		gk::u64 num = 123456789012345;
		String a = String::from(num);
		check_eq(a, "123456789012345"_str);
	});

test_case("FromTemplateFloat32") {
	float num = -123.45f;
	String a = String::from(num);
	check_eq(a, "-123.44999"_str); // slightly imprecise
}

comptime_test_case(String, FromTemplateFloat32, {
		float num = -123.45f;
		String a = String::from(num);
		check_eq(a, "-123.44999"_str); // slightly imprecise
	});

test_case("FromTemplateFloat64") {
	double num = -123.45;
	String a = String::from(num);
	check_eq(a, "-123.45"_str);
}

comptime_test_case(String, FromTemplateFloat64, {
		double num = -123.45;
		String a = String::from(num);
		check_eq(a, "-123.45"_str);
	});

test_case("FromTemplateCustomType") {
	StringTestExample e;
	e.a = 1.0;
	e.b = 1;
	String a = String::from(e);
	check_eq(a, "1.0, 1"_str);
}

comptime_test_case(String, FromTemplateCustomType, {
		StringTestExample e;
		e.a = 1.0;
		e.b = 1;
		String a = String::from(e);
		check_eq(a, "1.0, 1"_str);
	});

#pragma endregion

#pragma region Format

test_case("FormatOneArg") {
	gk::i32 num = 4;
	String a = String::format<"num: {}">(num);
	check_eq(a, "num: 4"_str);
}

comptime_test_case(String, FormatOneArg, {
		gk::i32 num = 4;
		String a = String::format<"num: {}">(num);
		check_eq(a, "num: 4"_str);
	});

test_case("FormatOneArgWithTextAfter") {
	float num = 4.f;
	String a = String::format<"num: {}... cool!">(num);
	check_eq(a, "num: 4.0... cool!"_str);
}

comptime_test_case(String, FormatOneArgWithTextAfter, {
		float num = 4.f;
		String a = String::format<"num: {}... cool!">(num);
		check_eq(a, "num: 4.0... cool!"_str);
	});

test_case("FormatTwoArgs") {
	gk::i32 num1 = 5;
	float num2 = 5;
	String a = String::format<"num1: {}, num2: {}">(num1, num2);
	check_eq(a, "num1: 5, num2: 5.0"_str);
}

comptime_test_case(String, FormatTwoArgs, {
		int num1 = 5;
		float num2 = 5;
		String a = String::format<"num1: {}, num2: {}">(num1, num2);
		check_eq(a, "num1: 5, num2: 5.0"_str);
	});

test_case("FormatTwoArgsWithOperation") {
	gk::i32 num1 = 5;
	float num2 = 5;
	String a = String::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
	check_eq(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
}

comptime_test_case(String, FormatTwoArgsWithOperation, {
		int num1 = 5;
		float num2 = 5;
		String a = String::format<"num1: {}, num2: {}, multiplied: {}">(num1, num2, num1 * num2);
		check_eq(a, "num1: 5, num2: 5.0, multiplied: 25.0"_str);
	});

test_case("FormatFromCustomType") {
	StringTestExample e;
	e.a = -1.2;
	e.b = 5;
	gk::i32 count = 2;
	String a = String::format<"the {} numbers are {}">(count, e);
	check_eq(a, "the 2 numbers are -1.19999, 5"_str);
}

comptime_test_case(String, FormatFromCustomType, {
		StringTestExample e;
		e.a = -1.2;
		e.b = 5;
		int count = 2;
		String a = String::format<"the {} numbers are {}">(count, e);
		check_eq(a, "the 2 numbers are -1.19999, 5"_str);
	});

#pragma endregion

#pragma region Find_Char

test_case("FindCharInSso") {
	String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find('5');
	check(opt.none() == false);
	check_eq(opt.some(), 21);
}

comptime_test_case(String, FindCharInSso, {
		String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find('5');
		check(opt.none() == false);
		check_eq(opt.some(), 21);
	});

test_case("FindCharInHeap") {
	String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find('5');
	check(opt.none() == false);
	check_eq(opt.some(), 81);
}

comptime_test_case(String, FindCharInHeap, {
		String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find('5');
		check(opt.none() == false);
		check_eq(opt.some(), 81);
	});

test_case("NotFindCharInSso") {
	String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find('6');
	check(opt.none());
}

comptime_test_case(String, NotFindCharInSso, {
		String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find('6');
		check(opt.none());
	});

test_case("NotFindCharInHeap") {
	String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find('6');
	check(opt.none());
}

comptime_test_case(String, NotFindCharInHeap, {
		String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajwheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find('6');
		check(opt.none());
	});

#pragma endregion

#pragma region Find_Str

test_case("FindStrInSso") {
	String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find("5a"_str);
	check_not(opt.none());
	check_eq(opt.some(), 21);
}

comptime_test_case(String, FindStrInSso, {
		String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find("5a"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 21);
	});

test_case("FindStrInHeap") {
	String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find("5a"_str);
	check_not(opt.none());
	check_eq(opt.some(), 83);
}

comptime_test_case(String, FindStrInHeap, {
		String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find("5a"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 83);
	});

test_case("FindUtf8StrInSso") {
	String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("ßen"_str);
	check_not(opt.none());
	check_eq(opt.some(), 9);
}

comptime_test_case(String, FindUtf8StrInSso, {
		String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("ßen"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 9);
	});

test_case("FindUtf8StrInHeap") {
	String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("6Übe"_str);
	check_not(opt.none());
	check_eq(opt.some(), 141);
}

comptime_test_case(String, FindUtf8StrInHeap, {
		String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("6Übe"_str);
		check(opt.none() == false);
		check_eq(opt.some(), 141);
	});

test_case("NotFindStrInSso") {
	String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find("ya"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindStrInSso, {
		String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find("ya"_str);
		check(opt.none());
	});

test_case("NotFindStrInHeap") {
	String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find(";5"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindStrInHeap, {
		String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find(";5"_str);
		check(opt.none());
	});

test_case("NotFindUtf8StrInSso") {
	String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("ßet"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8StrInSso, {
		String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("ßet"_str);
		check(opt.none());
	});

test_case("NotFindUtf8StrInHeap") {
	String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find("5Üba"_str);
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8StrInHeap, {
		String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find("5Üba"_str);
		check(opt.none());
	});

#pragma endregion

#pragma region Find_Other_String

test_case("FindOtherStringInSso") {
	String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find(String("5a"_str));
	check(opt.none() == false);
	check_eq(opt.some(), 21);
}

comptime_test_case(String, FindOtherStringInSso, {
		String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find(String("5a"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 21);
	});

test_case("FindOtherStringInHeap") {
	String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find(String("5a"_str));
	check(opt.none() == false);
	check_eq(opt.some(), 83);
}

comptime_test_case(String, FindOtherStringInHeap, {
		String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find(String("5a"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 83);
	});

test_case("FindUtf8OtherStringInSso") {
	String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(String("ßen"_str));
	check_not(opt.none());
	check_eq(opt.some(), 9);
}

comptime_test_case(String, FindUtf8OtherStringInSso, {
		String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(String("ßen"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 9);
	});

test_case("FindUtf8OtherStringInHeap") {
	String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(String("6Übe"_str));
	check_not(opt.none());
	check_eq(opt.some(), 141);
}

comptime_test_case(String, FindUtf8OtherStringInHeap, {
		String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(String("6Übe"_str));
		check(opt.none() == false);
		check_eq(opt.some(), 141);
	});

test_case("NotFindOtherStringInSso") {
	String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
	gk::Option<gk::usize> opt = a.find(String("ya"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindOtherStringInSso, {
		String a = "iuhlgiuhpiuyupaiusdyp5a"_str;
		gk::Option<gk::usize> opt = a.find(String("ya"_str));
		check(opt.none());
	});

test_case("NotFindOtherStringInHeap") {
	String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
	gk::Option<gk::usize> opt = a.find(String(";5"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindOtherStringInHeap, {
		String a = "woieufypaiuwdypaijsfnl;kajwhrpiauysdpiaujshd;lkajhsdl;kajw5bheoiuaywodiuaysodiuayso5asuidjyhoasiudya"_str;
		gk::Option<gk::usize> opt = a.find(String(";5"_str));
		check(opt.none());
	});

test_case("NotFindUtf8OtherStringInSso") {
	String a = "Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(String("ßet"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8OtherStringInSso, {
		String a = "Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(String("ßet"_str));
		check(opt.none());
	});

test_case("NotFindUtf8OtherStringInHeap") {
	String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
	gk::Option<gk::usize> opt = a.find(String("5Üba"_str));
	check(opt.none());
}

comptime_test_case(String, NotFindUtf8OtherStringInHeap, {
		String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger5ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger6Übergrößenträger"_str;
		gk::Option<gk::usize> opt = a.find(String("5Üba"_str));
		check(opt.none());
	});

#pragma endregion

#pragma region Substring

test_case("SubstringSsoStartingFromBeginning") {
	String a = "Übergrößenträger"_str;
	String b = a.substring(0, 12);
	check_eq(b, "Übergröße"_str);
}

comptime_test_case(String, SubstringSsoStartingFromBeginning, {
		String a = "Übergrößenträger"_str;
		String b = a.substring(0, 12);
		check_eq(b, "Übergröße"_str);
	});

test_case("SubstringSsoStartingFromOffset") {
	String a = "Übergrößenträger"_str;
	String b = a.substring(2, 12);
	check_eq(b, "bergröße"_str);
}

comptime_test_case(String, SubstringSsoStartingFromOffset, {
		String a = "Übergrößenträger"_str;
		String b = a.substring(2, 12);
		check_eq(b, "bergröße"_str);
	});

test_case("SubstringHeapToSsoStartingFromBeginning") {
	String a = "ÜbergrößenträgerÜbergrößenträger"_str;
	String b = a.substring(0, 20);
	check_eq(b, "Übergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToSsoStartingFromBeginning, {
		String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		String b = a.substring(0, 20);
		check_eq(b, "Übergrößenträger"_str);
	});

test_case("SubstringHeapToSsoStartingFromOffset") {
	String a = "ÜbergrößenträgerÜbergrößenträger"_str;
	String b = a.substring(20, 40);
	check_eq(b, "Übergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToSsoStartingFromOffset, {
		String a = "ÜbergrößenträgerÜbergrößenträger"_str;
		String b = a.substring(20, 40);
		check_eq(b, "Übergrößenträger"_str);
	});

test_case("SubstringHeapToHeapStartingFromBeginning") {
	String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = a.substring(0, 40);
	check_eq(b, "ÜbergrößenträgerÜbergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToHeapStartingFromBeginning, {
		String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = a.substring(0, 40);
		check_eq(b, "ÜbergrößenträgerÜbergrößenträger"_str);
	});

test_case("SubstringHeapToHeapStartingFromOffset") {
	String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
	String b = a.substring(20, 80);
	check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
}

comptime_test_case(String, SubstringHeapToHeapStartingFromOffset, {
		String a = "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str;
		String b = a.substring(20, 80);
		check_eq(b, "ÜbergrößenträgerÜbergrößenträgerÜbergrößenträger"_str);
	});

#pragma endregion

#endif