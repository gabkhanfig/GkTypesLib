#pragma once

#include "../Core.h"
#include "../Utility.h"
#include "../BasicTypes.h"
//#include "../Array/DynamicArray.h"
#include <iostream>

#define GK_STRING_ALIGNMENT 16
#define GK_STRING_HEAP_ALIGNMENT 32
#define GK_STRING_SSO_STRLEN 15
#define GK_STRING_MAX_LEN (1ULL << 63)
#define GK_STRING_DEFAULT_FLOAT_PRECISION 5
#ifndef GK_STRING_MAX_CONST_CHAR_SIZE
// 4GB max. #define this macro before #include to change
#define GK_STRING_MAX_CONST_CHAR_SIZE (1ULL << 32)
#endif

#ifndef GK_STRING_USE_X86_SIMD
#define GK_STRING_USE_X86_SIMD true
#endif 

namespace gk
{

	struct
		ALIGN_AS(GK_STRING_ALIGNMENT)
		/*
		32 byte long string designed for highly optimized string to string equality checking using AVX2.
		For const char* constructor or =, see GK_STRING_MAX_CONST_CHAR_SIZE macro.
		By default, a string of 4GB long is the maximum that can be parsed through const char*, but this can be overridden.
		*/
		String
	{
	public:

		struct OptionalIndex {

			OptionalIndex(uint64 index) : _index(index) {}

			bool IsValidIndex() const { return _index != MAXUINT64; }

			/* Will throw an assertion if IsValidIndex() returns false. */
			uint64 Get() const {
				gk_assertm(IsValidIndex(), "Cannot get the an invalid optional index");
				return _index;
			}

			bool operator == (OptionalIndex rhs) const { return _index == rhs._index; }
			bool operator == (uint64 rhs) const { return _index == rhs; }

		private:

			const uint64 _index;
		};

		[[nodiscard]] bool IsSmallString() const { return _ssoChars[15] == StringRepresentation::Small; }

		[[nodiscard]] bool IsHeapString() const { return _ssoChars[15] == StringRepresentation::Heap; }

		[[nodiscard]] bool IsConstSegmentString() const { return _ssoChars[15] == StringRepresentation::ConstSeg; }

		[[nodiscard]] uint64 Len() const { return _length; }

		[[nodiscard]] bool IsEmpty() const { return Len() == 0; }

		[[nodiscard]] const char* CStr() const { return _constSegData; }

		/* Calling this function on a non-heap string is useless. It will be garbage data. */
		[[nodiscard]] uint64 HeapCapacity() const { return _heapCapacity; }

		String() { Internal_ConstructDefault(); }

		String(char c) { Internal_ConstructCharacter(c); }

		String(const char* str, uint64 maxSize = GK_STRING_MAX_CONST_CHAR_SIZE) { Internal_ConstructConstChar(str, maxSize); }

		String(const String & other) { Internal_ConstructCopy(other); }

		String(String && other) noexcept { Internal_ConstructMove(std::move(other)); }

		String(const char* _begin, const char* _end) { Internal_ConstructRange(_begin, _end); }

		~String() {
			if (IsHeapString()) {
				Internal_DeleteLongData();
			}
		}

		/* Sets this string to be an empty string "", and frees any allocated memory. */
		void Empty() {
			if (IsHeapString()) {
				Internal_DeleteLongData();
			}
			Internal_ConstructDefault();
		}

		void operator = (char c) {
			if (IsHeapString()) {
				Internal_DeleteLongData();
			}
			Internal_ConstructCharacter(c);
		}

		void operator = (const char* str) {
			// TODO investigate if possible to reuse existing heap allocation
			if (IsHeapString()) {
				Internal_DeleteLongData();
			}
			Internal_ConstructConstChar(str, GK_STRING_MAX_CONST_CHAR_SIZE);
		}

		void operator = (const String & other) {
			// TODO investigate if possible to reuse existing heap allocation
			if (IsHeapString()) {
				Internal_DeleteLongData();
			}
			Internal_ConstructCopy(other);
		}

		void operator = (String && other) noexcept {
			if (IsHeapString()) {
				Internal_DeleteLongData();
			}
			Internal_ConstructMove(std::move(other));
		}

		[[nodiscard]] bool operator == (char c) const {
			if (Len() != 1) return false;
			return _ssoChars[0] == c;
		}

		[[nodiscard]] bool operator == (const char* str) const {
			gk_assertNotNull(str);
			const char* cstr = CStr();
			if (cstr == str) return true;
			return strcmp(cstr, str) == 0; // strcmp because the string in this string is guaranteed to be null terminated.
		}

		/* Optimized for x86 through the use of SIMD. See macro GK_STRING_USE_X86_SIMD. */
		[[nodiscard]] bool operator == (const String & other) const {
			const uint64 length = Len();
			if (length != other.Len()) return false;

#if GK_STRING_USE_X86_SIMD
			if (IsSmallString()) {
				// sso chars are 16 byte aligned already
				return _mm_cmpeq_epi8_mask(*(__m128i*)_ssoChars, *(__m128i*)other._ssoChars) == 65535; // 16 equal bits
			}

			const char* cstr = CStr();
			const char* otherCstr = other.CStr();

			if (IsConstSegmentString() || other.IsConstSegmentString()) {
				if (cstr == otherCstr) return true;
				return strcmp(cstr, otherCstr) == 0;
			}

			gk_assert(!gk::IsDataInConstSegment(cstr));
			gk_assert(!gk::IsDataInConstSegment(otherCstr));

			uint64 i = 0;
			uint64 collect = 0;
			for (i = 0; i < length - 15; i += 32) {
				// Heap allocated string is 32 byte aligned.
				const int equal32Bitmask = ~0;
				collect += _mm256_cmpeq_epi8_mask(*(__m256i*) & cstr[i], *(__m256i*) & otherCstr[i]) != equal32Bitmask;
			}
			if (collect != 0) return false;

			return strcmp(cstr + i, otherCstr + i) == 0;
#else
			const char* cstr = CStr();
			const char* otherCstr = other.CStr();
			return strncmp(cstr, otherCstr, GK_STRING_MAX_LEN) == 0;
#endif
		}

		/* std::cout << string */
		friend std::ostream& operator << (std::ostream & os, const String & _string) {
			return os << _string.CStr();
		}

		String& Append(char c) {
			const uint64 minCapacity = gk::UpperPowerOfTwo(_length + 2);
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				gk_assert(IsSmallString());
				_ssoChars[_length] = c;
				gk_assertm(_ssoChars[_length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length++;
				return *this;
			}

			if (IsHeapString()) {
				if (_heapCapacity < minCapacity) {
					Internal_ReallocateHeapString(minCapacity);
				}
				_heapData[_length] = c;
				gk_assertm(_heapData[_length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length++;
				return *this;
			}

			else if (IsSmallString()) { // From min capacity less than sso strlen check, an allocation is required here.
				Internal_SsoToHeap(minCapacity);
				_heapData[_length] = c;
				gk_assertm(_heapData[_length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length++;
				return *this;
			}

			// The only other possibility is if const segment
			Internal_ConstSegToHeap(minCapacity);
			_heapData[_length] = c;
			gk_assertm(_heapData[_length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
			_length++;
			return *this;
		}

		String& Append(const char* str) {
			const uint64 len = gk::strnlen(str, GK_STRING_MAX_CONST_CHAR_SIZE);
			const uint64 minCapacity = gk::UpperPowerOfTwo(_length + len + 1);
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				gk_assert(IsSmallString());
				memcpy(&_ssoChars[_length], str, len);
				gk_assertm(_ssoChars[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length += len;
				return *this;
			}

			if (IsHeapString()) {
				if (_heapCapacity < minCapacity) {
					Internal_ReallocateHeapString(minCapacity);
				}
				memcpy(&_heapData[_length], str, len);
				gk_assertm(_heapData[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length += len;
				return *this;
			}

			else if (IsSmallString()) { // From min capacity less than sso strlen check, an allocation is required here.
				Internal_SsoToHeap(minCapacity);
				memcpy(&_heapData[_length], str, len);
				gk_assertm(_heapData[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length += len;
				return *this;
			}

			// The only other possibility is if const segment
			Internal_ConstSegToHeap(minCapacity);
			memcpy(&_heapData[_length], str, len);
			gk_assertm(_heapData[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
			_length += len;
			return *this;
		}

		String& Append(const String & other) {
			const uint64 len = other.Len();
			const uint64 minCapacity = gk::UpperPowerOfTwo(_length + len + 1);
			const char* str = other.CStr();
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				gk_assert(IsSmallString());
				memcpy(&_ssoChars[_length], str, len);
				gk_assertm(_ssoChars[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length += len;
				return *this;
			}

			if (IsHeapString()) {
				if (_heapCapacity < minCapacity) {
					Internal_ReallocateHeapString(minCapacity);
				}
				memcpy(&_heapData[_length], str, len);
				gk_assertm(_heapData[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length += len;
				return *this;
			}

			else if (IsSmallString()) { // From min capacity less than sso strlen check, an allocation is required here.
				Internal_SsoToHeap(minCapacity);
				memcpy(&_heapData[_length], str, len);
				gk_assertm(_heapData[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				_length += len;
				return *this;
			}

			// The only other possibility is if const segment
			Internal_ConstSegToHeap(minCapacity);
			memcpy(&_heapData[_length], str, len);
			gk_assertm(_heapData[_length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
			_length += len;
			return *this;
		}

		// TODO Add append with template for String::From

		String& operator += (char c) { return Append(c); }

		String& operator += (const char* str) { return Append(str); }

		String& operator += (const String & other) { return Append(other); }

		[[nodiscard]] friend String operator + (const String & lhs, char rhs) { return Internal_ConcatStringAndChar(lhs, rhs); }

		[[nodiscard]] friend String operator + (const String & lhs, const char* rhs) { return Internal_ConcatStringAndConstChar(lhs, rhs); }

		[[nodiscard]] friend String operator + (const String & lhs, const String & rhs) { return Internal_ConcatStringAndString(lhs, rhs); }

		[[nodiscard]] friend String operator + (char lhs, const String & rhs) { return Internal_ConcatCharAndString(lhs, rhs); }

		[[nodiscard]] friend String operator + (const char* lhs, const String & rhs) { return Internal_ConcatConstCharAndString(lhs, rhs); }

		/* Returns a substring from the start index to the end of the string. */
		[[nodiscard]] String Substring(uint64 startIndexInclusive) const {
			return Substring(startIndexInclusive, _length);
		}

		/* If endIndexExclusive equal to the length of the string, the substring will be startIndexInclusive to the end of the string.
		For doing a length, you can do Substring(startIndexInclusive, startIndexInclusive + length). */
		[[nodiscard]] String Substring(uint64 startIndexInclusive, uint64 endIndexExclusive) const {
			gk_assertm(startIndexInclusive <= _length, "Substring start index must be within string length");
			gk_assertm(endIndexExclusive <= _length, "Substring end index must be within string length");
			gk_assertm(startIndexInclusive < endIndexExclusive, "Substring start index must be less than end index");

			const uint64 len = (endIndexExclusive - startIndexInclusive);
			const char* copyFrom = CStr() + startIndexInclusive;

			gk::String str;
			if (len == 0) return str;

			str._length = len;
			if (len <= GK_STRING_SSO_STRLEN) {
				memcpy(str._ssoChars, copyFrom, len);
				gk_assertm(str._ssoChars[len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				return str;
			}

			const bool isSubstringToEnd = endIndexExclusive == _length;
			if (IsConstSegmentString() && isSubstringToEnd) {
				str._constSegData = copyFrom;
				str.Internal_SetFlagConstSegmentString();
				gk_assertm(str._constSegData[len] == '\0', "Const segment should already have null terminator... it's currently " << str._constSegData[len + 1]);
				return str;
			}

			const uint64 allocateCapacity = gk::UpperPowerOfTwo(len + 1);
			str.Internal_AllocateLongStringHeap(allocateCapacity);
			String::Internal_CopyToBufferAndSetRemainingCapacityTo0(str._heapData, str._heapCapacity, copyFrom, len);
			str.Internal_SetFlagHeapString();
			return str;
		}

		/* Find the first occurrence of a character in this string. */
		[[nodiscard]] OptionalIndex Find(char c) const {
			const __m256i avxChar = Internal_CharToAVX2(c);
			return Internal_AVX2FindFirstChar(CStr(), _length, c, avxChar);
		}

		/* Find the first occurrence of a substring in this string. */
		[[nodiscard]] OptionalIndex Find(const char* str) const {
			gk_assertNotNull(str);
			const uint64 len = gk::strnlen(str, GK_STRING_MAX_CONST_CHAR_SIZE);
			if (len > _length || len == 0) return MAXUINT64;

			char firstChar = str[0];
			const __m256i avxFirstChar = Internal_CharToAVX2(firstChar);

			uint64 foundStartOffset = 0;
			uint64 stepIntoLength = 0;
			const char* start = CStr();


			while (true) {
				foundStartOffset = Internal_AVX2FindFirstChar(start, _length - stepIntoLength, firstChar, avxFirstChar);
				uint64 strIndex = 0;
				uint64 thisIndex = stepIntoLength + foundStartOffset;
				if (foundStartOffset == MAXUINT64 || thisIndex >= _length) return MAXUINT64;

				bool didFindThisIteration = true;
				if (len >= 32) {
					uint64 collect = 0;
					while (strIndex < (len - 31)) {
						const __m256i v1 = _mm256_loadu_epi8(CStr() + thisIndex);
						const __m256i v2 = _mm256_loadu_epi8(str + strIndex);
						collect += _mm256_cmpeq_epi8_mask(v1, v2) != -1; // 32 equal bits
						strIndex += 32;
						thisIndex += 32;
					}
					if (collect != 0) {
						didFindThisIteration = false;
						start += foundStartOffset + 1;
						stepIntoLength += foundStartOffset + 1;
						continue;
					}
				}

				while (strIndex < len) {
					if (CStr()[thisIndex] != str[strIndex]) {
						didFindThisIteration = false;
						start += foundStartOffset + 1;
						stepIntoLength += foundStartOffset + 1;
						break;
					}
					strIndex++;
					thisIndex++;
				}
				if (!didFindThisIteration) {
					continue;
				}
				return stepIntoLength + foundStartOffset;
			}
			return MAXUINT64;
		}

		/* Find the first occurrence of a substring in this string. */
		[[nodiscard]] OptionalIndex Find(const String & other) const {
			const uint64 len = other.Len();
			const char* str = other.CStr();
			if (len > _length || len == 0) return MAXUINT64;

			char firstChar = str[0];
			const __m256i avxFirstChar = Internal_CharToAVX2(firstChar);

			uint64 foundStartOffset = 0;
			uint64 stepIntoLength = 0;
			const char* start = CStr();


			while (true) {
				foundStartOffset = Internal_AVX2FindFirstChar(start, _length - stepIntoLength, firstChar, avxFirstChar);
				uint64 strIndex = 0;
				uint64 thisIndex = stepIntoLength + foundStartOffset;
				if (foundStartOffset == MAXUINT64 || thisIndex >= _length) return MAXUINT64;

				bool didFindThisIteration = true;
				if (len >= 32) {
					uint64 collect = 0;
					while (strIndex < (len - 31)) {
						const __m256i v1 = _mm256_loadu_epi8(CStr() + thisIndex);
						const __m256i v2 = _mm256_loadu_epi8(str + strIndex);
						collect += _mm256_cmpeq_epi8_mask(v1, v2) != -1; // 32 equal bits
						strIndex += 32;
						thisIndex += 32;
					}
					if (collect != 0) {
						didFindThisIteration = false;
						start += foundStartOffset + 1;
						stepIntoLength += foundStartOffset + 1;
						continue;
					}
				}

				while (strIndex < len) {
					if (CStr()[thisIndex] != str[strIndex]) {
						didFindThisIteration = false;
						start += foundStartOffset + 1;
						stepIntoLength += foundStartOffset + 1;
						break;
					}
					strIndex++;
					thisIndex++;
				}
				if (!didFindThisIteration) {
					continue;
				}
				return stepIntoLength + foundStartOffset;
			}
			return MAXUINT64;
		}

		/* true -> "true", false -> "false" */
		[[nodiscard]] static String FromBool(bool b) {
			String str;
			if (b) {
				memcpy(str._ssoChars, "true", 4);
				str._length = 4;
				return str;
			}
			memcpy(str._ssoChars, "false", 5);
			str._length = 5;
			return str;
		}

		[[nodiscard]] static String FromInt(int64 num) {
			if (num == 0) return '0';
			constexpr const char* digits = "9876543210123456789";
			constexpr uint64 zeroDigit = 9;
			constexpr uint64 maxChars = 21;
			const bool isNegative = num < 0;

			char tempNums[maxChars];
			uint64 tempAt = maxChars;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
				num /= 10LL;
			}
			if (isNegative) {
				tempNums[--tempAt] = '-';
			}

			const char* start = tempNums + tempAt;
			const char* end = &tempNums[maxChars - 1];
			return String(start, end);
		}

		[[nodiscard]] static String FromUint(uint64 num) {
			if (num == 0) return '0';
			constexpr const char* digits = "9876543210123456789";
			constexpr uint64 zeroDigit = 9;
			constexpr uint64 maxChars = 21;

			char tempNums[maxChars];
			uint64 tempAt = maxChars;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10ULL)];
				num /= 10ULL;
			}

			const char* start = tempNums + tempAt;
			const char* end = &tempNums[maxChars - 1];
			return String(start, end);
		}

		/* Precision must be 19 or less due. */
		[[nodiscard]] static String FromFloat(double num, const int precision = GK_STRING_DEFAULT_FLOAT_PRECISION) {
			gk_assertm(precision < 20, "gk::string::FromFloat precision must be 19 or less. The precision is set to " << precision);

			if (num == 0) return "0.0";								// Zero
			else if (num > DBL_MAX) return "inf";			// Positive Infinity
			else if (num < -DBL_MAX) return "-inf";		// Negative Infinity
			else if (num != num) return "nan";				// Not a Number

			Internal_StripNegativeZero(num);
			const bool isNegative = num < 0;

			const int64 whole = static_cast<int64>(num);
			const String wholeString = (isNegative && whole == 0) ? "-0" : String::FromInt(whole);

			num -= whole;
			if (num == 0) return wholeString + ".0"; // Quick return if no fractional part
			if (num < 0) num *= -1; // Make positive for fractional part
			int zeroesInFractionBeforeFirstNonZero = 0;
			for (int i = 0; i < precision; i++) {
				num *= 10;
				if (num < 1) zeroesInFractionBeforeFirstNonZero += 1;
			}
			const uint64 fraction = static_cast<uint64>(num);
			if (fraction == 0) return wholeString + ".0";

			auto MakeFractionZeroesString = [](uint64 zeroCount) {
				String str;
				for (uint64 i = 0; i < zeroCount; i++) {
					str.Append('0');
				}
				return str;
			};

			auto MakeStringWithIntWithoutEndingZeroes = [](uint64 value, int availableDigits) {
				const String str = String::FromUint(value);
				const uint64 last = availableDigits > str.Len() - 1 ? str.Len() - 1 : availableDigits;
				for (uint64 i = last; i > 0; i--) {
					if (str.CStr()[i] != '0') {
						return str.Substring(0, i + 1);
					}
				}
				if (str.CStr()[0] != '0') {
					return str.Substring(0, 1);
				}
				return str;
			};

			const String fractionZeroesString = MakeFractionZeroesString(zeroesInFractionBeforeFirstNonZero);
			const String fractionalString = MakeStringWithIntWithoutEndingZeroes(fraction, precision - zeroesInFractionBeforeFirstNonZero);

			return wholeString + '.' + fractionZeroesString + fractionalString;
		}

		/* Creates a string from a custom data type. Example:
		struct Example {
			int a;
			int b;
		};
		template<>
		[[nodiscard]] static gk::string gk::string::From<Example>(const Example& value) {
			return string::FromInt(value.a) + gk::string(", ") + gk::string::FromInt(value.b);
		}
		*/
		template<typename T>
		[[nodiscard]] static String From(const T & value) = delete;

		template<>
		[[nodiscard]] static String From<bool>(const bool& b) { return FromBool(b); }

		template<>
		[[nodiscard]] static String From<int8>(const int8 & num) { return FromInt(num); }

		template<>
		[[nodiscard]] static String From<int16>(const int16 & num) { return FromInt(num); }

		template<>
		[[nodiscard]] static String From<int>(const int& num) { return FromInt(num); }

		template<>
		[[nodiscard]] static String From<int64>(const int64 & num) { return FromInt(num); }

		template<>
		[[nodiscard]] static String From<uint8>(const uint8 & num) { return FromUint(num); }

		template<>
		[[nodiscard]] static String From<uint16>(const uint16 & num) { return FromUint(num); }

		template<>
		[[nodiscard]] static String From<uint32>(const uint32 & num) { return FromUint(num); }

		template<>
		[[nodiscard]] static String From<uint64>(const uint64 & num) { return FromUint(num); }

		template<>
		[[nodiscard]] static String From<float>(const float& num) { return FromFloat(num); }

		template<>
		[[nodiscard]] static String From<double>(const double& num) { return FromFloat(num); }

		/* All strings that aren't "true" will be treated as false. */
		[[nodiscard]] bool ToBool() const {
			static String strTrue = "true";
			return *this == strTrue;
		}

		/* Murmur hash */
		[[nodiscard]] size_t ComputeHash() const {
			const char* str = CStr();
			const int seed = 0;
			const uint64 len = Len();
			const uint64 m = 0xc6a4a7935bd1e995ULL;
			const int r = 47;

			uint64 h = seed ^ (len * m);

			const uint64* data = (const uint64*)str;
			const uint64* end = data + (len / 8);

			while (data != end)
			{
				uint64 k = *data++;

				k *= m;
				k ^= k >> r;
				k *= m;

				h ^= k;
				h *= m;
			}

			const unsigned char* data2 = (const unsigned char*)data;

			switch (len & 7)
			{
			case 7: h ^= uint64(data2[6]) << 48;
			case 6: h ^= uint64(data2[5]) << 40;
			case 5: h ^= uint64(data2[4]) << 32;
			case 4: h ^= uint64(data2[3]) << 24;
			case 3: h ^= uint64(data2[2]) << 16;
			case 2: h ^= uint64(data2[1]) << 8;
			case 1: h ^= uint64(data2[0]);
				h *= m;
			};

			h ^= h >> r;
			h *= m;
			h ^= h >> r;

			return h;
		}

	private:

		void Internal_SetAllToZero() {
			memset(this, 0, sizeof(String));
		}

		void Internal_DeleteLongData() {
			if (_heapData) {
				gk_assertm(!gk::IsDataInConstSegment(_heapData), "Not allowed to delete data from the application's const data segment");
				// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170
				_aligned_free(_heapData);
			}
		}

		void Internal_SetFlagHeapString() {
			_ssoChars[15] = StringRepresentation::Heap;
			//_flags = StringRepresentation::Heap;
		}

		void Internal_SetFlagConstSegmentString() {
			_ssoChars[15] = StringRepresentation::ConstSeg;
			//_flags = StringRepresentation::ConstSeg;
		}

		void Internal_AllocateLongStringHeap(uint64 capacity) {
			gk_assertm(capacity == gk::UpperPowerOfTwo(capacity), "String must allocate a capacity of a power of 2. Requested allocation capacity is " << capacity);
			gk_assertm(capacity >= 32, "String must allocate a capacity of 32 or more. Requested allocation capacity is " << capacity);
			// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170
			_heapData = (char*)_aligned_malloc(capacity, GK_STRING_HEAP_ALIGNMENT);
			gk_assertNotNull(_heapData);
			_heapCapacity = capacity;
			memset(_heapData, '\0', _heapCapacity);
			gk_assertm(gk::isAligned(_heapData, GK_STRING_HEAP_ALIGNMENT), "Heap allocated char buffer for string must have an alignment of " << GK_STRING_HEAP_ALIGNMENT << " bytes");
		}

		void Internal_ReallocateHeapString(uint64 capacity) {
			gk_assertm(capacity == gk::UpperPowerOfTwo(capacity), "String must allocate a capacity of a power of 2. Requested allocation capacity is " << capacity);
			gk_assertm(capacity >= 32, "String must allocate a capacity of 32 or more. Requested allocation capacity is " << capacity);
			// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170
			char* oldData = _heapData;
			_heapData = (char*)_aligned_malloc(capacity, GK_STRING_HEAP_ALIGNMENT);
			gk_assertNotNull(_heapData);
			_heapCapacity = capacity;
			memcpy(_heapData, oldData, _length);
			memset(_heapData + _length, '\0', _heapCapacity - _length);
			gk_assertm(!gk::IsDataInConstSegment(_heapData), "Not allowed to delete data from the application's const data segment");
			_aligned_free(oldData);
			gk_assertm(gk::isAligned(_heapData, GK_STRING_HEAP_ALIGNMENT), "Heap allocated char buffer for string must have an alignment of " << GK_STRING_HEAP_ALIGNMENT << " bytes");
		}

		static void Internal_CopyToBufferAndSetRemainingCapacityTo0(char* buffer, uint64 bufferCapacity, const char* copy, uint64 copyLength) {
			const uint64 lenAddOne = copyLength + 1;
			memcpy(buffer, copy, copyLength);
			char* startZero = buffer + copyLength;
			const uint64 numToZero = (uint64)((buffer + bufferCapacity) - startZero);
			memset(startZero, '\0', numToZero);
		}

		void Internal_ConstructDefault() {
			memset(this, 0, sizeof(String));
			_heapData = _ssoChars;
		}

		void Internal_ConstructCharacter(char c) {
			memset(this, 0, sizeof(String));
			_heapData = _ssoChars;
			_length = 1;
			_ssoChars[0] = c;
		}

		void Internal_ConstructConstChar(const char* str, uint64 maxSize) {
			gk_assertNotNull(str);
			memset(this, 0, sizeof(String));
			const bool isConstSegment = gk::IsDataInConstSegment(str);
			_length = gk::strnlen(str, maxSize);

			if (_length <= GK_STRING_SSO_STRLEN) {
				Internal_SetSsoCharsAndLength(str, _length);
				return;
			}

			if (isConstSegment) {
				_constSegData = str;
				Internal_SetFlagConstSegmentString();
				gk_assertm(_constSegData == _heapData, "const segment data string pointer must be the same as long string pointer due to them occupying the same union space");
				return;
			}

			const uint64 allocateCapacity = gk::UpperPowerOfTwo(_length + 1);
			Internal_AllocateLongStringHeap(allocateCapacity);
			String::Internal_CopyToBufferAndSetRemainingCapacityTo0(_heapData, _heapCapacity, str, _length);
			Internal_SetFlagHeapString();
		}

		void Internal_ConstructCopy(const String & other) {
			memset(this, 0, sizeof(String));
			_length = other._length;
			memcpy(_ssoChars, other._ssoChars, sizeof(_ssoChars)); // also copies heap capacity and flags

			if (other.IsHeapString()) {
				//std::cout << "copying heap string now..." << std::endl;
				//std::cout << "copy:" << other << std::endl;
			}
			if (other.IsSmallString()) {
				_heapData = _ssoChars;
				return;
			}
			else if (other.IsConstSegmentString()) {
				_constSegData = other._constSegData;
				return;
			}

			Internal_AllocateLongStringHeap(_heapCapacity);
			String::Internal_CopyToBufferAndSetRemainingCapacityTo0(_heapData, _heapCapacity, other._heapData, _length);
			Internal_SetFlagHeapString();
			//std::cout << "this: " << *this << std::endl;
		}

		void Internal_ConstructMove(String && other) noexcept {
			memset(this, 0, sizeof(String));
			_length = other._length;
			_heapCapacity = other._heapCapacity;
			_flags = other._flags;
			memcpy(_ssoChars, other._ssoChars, sizeof(_ssoChars));
			if (other.IsHeapString()) {
				_heapData = other._heapData;
				other._heapData = nullptr;
				return;
			}
			else if (other.IsConstSegmentString()) {
				_constSegData = other._constSegData;
				return;
			}

			_heapData = _ssoChars;
		}

		void Internal_ConstructRange(const char* _begin, const char* _end) {
			gk_assertNotNull(_begin);
			gk_assertNotNull(_end);

			memset(this, 0, sizeof(String));
			const bool isNullTerminated = *_end == '\0';
			_length = (_end - _begin) + (!isNullTerminated);

			if (_length <= GK_STRING_SSO_STRLEN) {
				_constSegData = _ssoChars;
				memcpy(_ssoChars, _begin, _length);
				return;
			}

			if (gk::IsDataInConstSegment(_begin) && isNullTerminated) {
				_constSegData = _begin;
				Internal_SetFlagConstSegmentString();
				return;
			}

			const uint64 allocateCapacity = gk::UpperPowerOfTwo(_length + 1);
			Internal_AllocateLongStringHeap(allocateCapacity);
			String::Internal_CopyToBufferAndSetRemainingCapacityTo0(_heapData, _heapCapacity, _begin, _length);
			Internal_SetFlagHeapString();
		}

		void Internal_SetSsoCharsAndLength(const char* str, uint64 length) {
			_heapData = _ssoChars;
			_length = length;
			memset(_ssoChars, '\0', sizeof(_ssoChars));
			memcpy(_ssoChars, str, _length); // null terminator already set via memset.
		}

		void Internal_SsoToHeap(uint64 bufferCapacity) {
			char temp[16];
			memcpy(temp, _ssoChars, 16);
			Internal_AllocateLongStringHeap(bufferCapacity);
			_flags = 0;
			Internal_SetFlagHeapString();
			memcpy(_heapData, temp, 16);
		}

		void Internal_ConstSegToHeap(uint64 bufferCapacity) {
			const char* constData = _constSegData;
			Internal_AllocateLongStringHeap(bufferCapacity);
			_flags = 0;
			Internal_SetFlagHeapString();
			memcpy(_heapData, constData, _length);
		}

		static gk::String Internal_ConcatStringAndChar(const String & lhs, char c) {
			const uint64 minCapacity = gk::UpperPowerOfTwo(lhs._length + 2);

			gk::String str;
			str._length = lhs._length + 1;
			const char* cstr = lhs.CStr();
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				memcpy(str._ssoChars, cstr, lhs._length);
				str._ssoChars[lhs._length] = c;
				gk_assertm(str._ssoChars[lhs._length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
				gk_assert(str.IsSmallString());
				return str;
			}

			str.Internal_AllocateLongStringHeap(minCapacity);
			memcpy(str._heapData, cstr, lhs._length);
			str._heapData[lhs._length] = c;
			gk_assertm(str._heapData[lhs._length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
			str.Internal_SetFlagHeapString();
			return str;
		}

		static gk::String Internal_ConcatStringAndConstChar(const String & lhs, const char* str) {
			gk_assertNotNull(str);
			const uint64 len = gk::strnlen(str, GK_STRING_MAX_CONST_CHAR_SIZE);
			const uint64 minCapacity = gk::UpperPowerOfTwo(lhs._length + len + 1);

			gk::String concatString;
			concatString._length = lhs._length + len;
			const char* cstr = lhs.CStr();
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				memcpy(concatString._ssoChars, cstr, lhs._length);
				memcpy(concatString._ssoChars + lhs._length, str, len);
				gk_assertm(concatString._ssoChars[lhs._length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				gk_assert(concatString.IsSmallString());
				return concatString;
			}

			concatString.Internal_AllocateLongStringHeap(minCapacity);
			memcpy(concatString._heapData, cstr, lhs._length);
			memcpy(concatString._heapData + lhs._length, str, len);
			gk_assertm(concatString._heapData[lhs._length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
			concatString.Internal_SetFlagHeapString();
			return concatString;
		}

		static gk::String Internal_ConcatStringAndString(const String & lhs, const String & rhs) {
			const uint64 len = rhs.Len();
			const char* str = rhs.CStr();
			const uint64 minCapacity = gk::UpperPowerOfTwo(lhs._length + len + 1);

			gk::String concatString;
			concatString._length = lhs._length + len;
			const char* cstr = lhs.CStr();
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				memcpy(concatString._ssoChars, cstr, lhs._length);
				memcpy(concatString._ssoChars + lhs._length, str, len);
				gk_assertm(concatString._ssoChars[lhs._length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				gk_assert(concatString.IsSmallString());
				return concatString;
			}

			concatString.Internal_AllocateLongStringHeap(minCapacity);
			memcpy(concatString._heapData, cstr, lhs._length);
			memcpy(concatString._heapData + lhs._length, str, len);
			gk_assertm(concatString._heapData[lhs._length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
			concatString.Internal_SetFlagHeapString();
			return concatString;
		}

		static gk::String Internal_ConcatCharAndString(char c, const String & rhs) {
			const uint64 minCapacity = gk::UpperPowerOfTwo(rhs._length + 2);

			gk::String str;
			str._length = rhs._length + 1;
			const char* cstr = rhs.CStr();
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				str._ssoChars[0] = c;
				memcpy(str._ssoChars + 1, cstr, rhs._length);
				gk_assertm(str._ssoChars[rhs._length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
				gk_assert(str.IsSmallString());
				return str;
			}

			str.Internal_AllocateLongStringHeap(minCapacity);
			str._heapData[0] = c;
			memcpy(str._heapData + 1, cstr, rhs._length);
			gk_assertm(str._heapData[rhs._length + 1] == '\0', "Sso chars should already have unused bytes set to null terminator");
			str.Internal_SetFlagHeapString();
			return str;
		}

		static gk::String Internal_ConcatConstCharAndString(const char* str, const String & rhs) {
			gk_assertNotNull(str);
			const uint64 len = gk::strnlen(str, GK_STRING_MAX_CONST_CHAR_SIZE);
			const uint64 minCapacity = gk::UpperPowerOfTwo(rhs._length + len + 1);

			gk::String concatString;
			concatString._length = rhs._length + len;
			const char* cstr = rhs.CStr();
			if (minCapacity <= (GK_STRING_SSO_STRLEN + 1)) {
				memcpy(concatString._ssoChars, str, len);
				memcpy(concatString._ssoChars + len, cstr, rhs._length);
				gk_assertm(concatString._ssoChars[rhs._length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
				gk_assert(concatString.IsSmallString());
				return concatString;
			}

			concatString.Internal_AllocateLongStringHeap(minCapacity);
			memcpy(concatString._heapData, str, len);
			memcpy(concatString._heapData + len, cstr, rhs._length);
			gk_assertm(concatString._heapData[rhs._length + len] == '\0', "Sso chars should already have unused bytes set to null terminator");
			concatString.Internal_SetFlagHeapString();
			return concatString;
		}

#if GK_STRING_USE_X86_SIMD

		static __m256i Internal_CharToAVX2(char c) {
			__m256i v;
			memset(&v, c, 32);
			return v;
		}

#pragma intrinsic(_BitScanForward)

		/* Returns MAXUINT64 if cannot find */
		static uint64 Internal_AVX2FindFirstChar(const char* str, const uint64 len, const char c, const __m256i avxChar) {
			uint64 i = 0;
			for (; i < (len - 31); i += 32) {
				const __m256i vc = _mm256_loadu_epi8(str + i);
				static_assert(sizeof(unsigned long) == 4);
				const unsigned long bitmask = static_cast<unsigned long>(_mm256_cmpeq_epi8_mask(vc, avxChar));
				if (bitmask == 0) continue;
				unsigned long index;
				_BitScanForward(&index, bitmask);
				return index + i;
			}

			for (; i < len; i++) {
				if (str[i] == c) return i;
			}
			return MAXUINT64;
		}

#endif

		static String Internal_RemoveZeroesFromFractional(int64 num, int availableDigits) {
			// should always have at least 1 available digit if it gets to this function.
			String str = String::FromInt(num);
			const char* data = str.CStr();
			const size_t last = availableDigits > str.Len() - 1 ? str.Len() - 1 : availableDigits;
			for (size_t i = last; i > 0; i--) {
				if (data[i] != '0') {
					return str.Substring(0, i + 1);
				}
			}
			return str.Substring(0, last + 1);
		}

		__pragma(optimize("", off))
			static void Internal_StripNegativeZero(double& inFloat)
		{
			// Taken from Unreal Engine
			// This works for translating a negative zero into a positive zero,
			// but if optimizations are enabled when compiling with -ffast-math
			// or /fp:fast, the compiler can strip it out.
			inFloat += 0.0;
		}
		__pragma(optimize("", on))

	private:

		uint64 _length;
		union {
			char* _heapData;
			const char* _constSegData;
		};
		union {
			struct {
				char _ssoChars[16];
			};
			struct {
				uint64 _heapCapacity;
				uint64 _flags;
			};
		};

		enum StringRepresentation : char
		{
			Small = 0,
			Heap = 1,
			ConstSeg = 2
		};
	};

	static_assert(sizeof(String) == 32, "gk::string must occupy 32 bytes only");
}

namespace std
{
	template<>
	struct hash<gk::String>
	{
		size_t operator()(const gk::String& str) const {
			return str.ComputeHash();
		}
	};
}
