#pragma once

#include "str.h"
#include "../option/option.h"
#include "../error/result.h"
#include "../hash/hash.h"

namespace gk
{
	/**
	* Allocate a char buffer that's 64 byte aligned, with
	* capacity increments of 64, to allow for simple AVX-512 usage.
	* See `freeCharBufferAligned()` for freeing the allocated memory.
	*
	* @param capacity: Non-null pointer to mutable variable holding the required capacity.
	* Will be mutated to be a multiple of 64.
	*
	* @return AVX-512 aligned, zeroed char buffer on the heap.
	*/
	char* mallocCharBufferAligned(usize* capacity);


	/**
	* Free's the memory of a buffer that was allocating with
	* `mallocCharBufferAligned()`.
	*
	* @param buffer: Sets to nullptr. The non-null pointer to free. MUST have been allocated with
	* `mallocCharBufferAligned` or the globalHeapAllocator's mallocAlignedBuffer() function.
	*/
	void freeCharBufferAligned(char*& buffer, usize capacity);

	/**
	*/
	struct alignas(8) String
	{
	private:

		static constexpr char FLAG_BIT = static_cast<char>(0b10000000);
		static constexpr usize MAX_SSO_LEN = 31;
		static constexpr usize HASH_MODIFIER = 0xc6a4a7935bd1e995ULL;
		static constexpr usize HASH_SHIFT = 47;

#pragma pack(push, 1)
		struct HeapRep {
			char* buffer;
			usize length;
			usize capacity;
			char _unused[7];

			constexpr HeapRep() : buffer(nullptr), length(0), capacity(0), _unused{ 0 } {}
		};
#pragma pack(pop)

		struct SsoRep {
			char chars[31];

			constexpr SsoRep() : chars{ 0 } {}
		};

		union StringRep
		{
			SsoRep sso;
			HeapRep heap;

			constexpr StringRep() : sso(SsoRep()) {}
		};

		StringRep rep;
		char flag;

	public:

		/**
		* Default constructed string is just an empty SSO string.
		*/
		constexpr String() : rep(StringRep()), flag((char)0) {
			setSsoLen(0);
		}

		/***/
		constexpr String(char c) : rep(StringRep()), flag((char)0) {
			rep.sso.chars[0] = c;
			setSsoLen(1);
		}

		/**
		* Construct from utf8 string slice.
		*/
		constexpr String(const Str& str);

		/**
		* Copy constructor. For sso strings, it's just a memcpy.
		*/
		constexpr String(const String& other);

		/**
		* Move constructor. Is just a memcpy.
		*/
		constexpr String(String&& other) noexcept;

		/***/
		constexpr ~String() {
			freeHeapBufferIfNotSso();
		}

		/***/
		constexpr String& operator = (char c);

		/**
		* Asign from utf8 string slice.
		* Tries to reuse existing allocation if possible at runtime.
		*/
		constexpr String& operator = (const Str& str);

		/**
		* Copy assignment. For sso strings, it's just a memcpy.
		* Tries to reuse existing allocation if possible at runtime.
		*/
		constexpr String& operator = (const String& other);

		/**
		* Move constructor. Is just a memcpy.
		*/
		constexpr String& operator = (String&& other) noexcept;

		/**
		* @return Length of this String in bytes, NOT chars or utf8 code points.
		* For non-ascii text, this is not what a human would consider the length of the string.
		*/
		[[nodiscard]] constexpr usize len() const;

		/**
		* @return `len() == 0`
		*/
		[[nodiscard]] constexpr forceinline bool isEmpty() const { return len() == 0; }

		/**
		* @return The beginning of this string's buffer data, which is null terminated.
		*/
		[[nodiscard]] constexpr const char* cstr() const {
			if (isSso()) {
				return rep.sso.chars;
			}
			return rep.heap.buffer;
		}

		/**
		* @return This string as a string slice. Useful for non-simd operations.
		*/
		[[nodiscard]] constexpr gk::Str asStr() const { return gk::Str::fromSlice(cstr(), len()); }

		friend std::ostream& operator << (std::ostream& os, const String& inString) {
			return os << inString.cstr();
		}

		/**
		*	AVX-2 optimized hash function.
		* The hash will be the same in the event that the SSO version and heap version of the string have equal data.
		*
		* @return the hash code of this String.
		*/
		[[nodiscard]] usize hash() const;

		/**
		* @return If this string is of length 1, and the only char is equal to the argument `c`
		*/
		[[nodiscard]] constexpr bool operator == (char c) const;

		/**
		* SIMD optimized equality comparison between this string and a string slice.
		*
		* @return If this string equals the slice.
		*/
		[[nodiscard]] constexpr bool operator == (const Str& str) const;

		/**
		* SIMD optimized equality comparison between this string and another string.
		*
		* @return If this string equals the other string.
		*/
		[[nodiscard]] constexpr bool operator == (const String& other) const;

		/**
		* Append a char to the end of this string, mutating this string.
		*
		* @return self reference. Allows chaining appends.
		*/
		constexpr String& append(char c);

		/**
		* Append a string slice to the end of this string, mutating this string.
		*
		* @return self reference. Allows chaining appends.
		*/
		constexpr String& append(const Str& str);

		/**
		* Append another string to the end of this string, mutating this string.
		*
		* @return self reference. Allows chaining appends.
		*/
		constexpr String& append(const String& other) { return append(other.asStr()); }

		/**
		* Operator version of char append.
		*/
		constexpr String& operator += (char c) { return append(c); }

		/**
		* Operator version of string slice append.
		*/
		constexpr String& operator += (const Str& str) { return append(str); }

		/**
		* Operator version of other string append.
		*/
		constexpr String& operator += (const String& other) { return append(other); }

		/**
		* Concat a copy of a string with a char.
		*
		* @return a new string with the char concatenated.
		*/
		constexpr friend String operator + (const String& lhs, char rhs);

		/**
		* Concat a moved string with a char.
		*
		* @return `lhs` with the char appended.
		*/
		constexpr friend String operator + (String&& lhs, char rhs) { return std::move(lhs.append(rhs)); }

		/**
		* Concat a copy of a string with a string slice.
		*
		* @return a new string with the string slice concatenated.
		*/
		constexpr friend String operator + (const String& lhs, const Str& rhs);

		/**
		* Concat a moved string with a string slice.
		*
		* @return `lhs` with the string slice appended.
		*/
		constexpr friend String operator + (String&& lhs, const Str& rhs) { return std::move(lhs.append(rhs)); }

		/**
		* Concat a copy of a string with another string.
		*
		* @return a new string with the other string concatenated.
		*/
		constexpr friend String operator + (const String& lhs, const String& rhs) { return lhs + rhs.asStr(); }

		/**
		* Concat a moved string with a copy of a string.
		*
		* @return `lhs` with the other string appended.
		*/
		constexpr friend String operator + (String&& lhs, const String& rhs) { return std::move(lhs.append(rhs)); }

		/**
		* Concat a char with a string after it.
		*
		* @return a new string with the char concatenated with the other string.
		*/
		constexpr friend String operator + (char lhs, const String& rhs);

		/**
		* Concat a string slice with a string after it.
		*
		* @return a new string with the string slice concatenated with the other string.
		*/
		constexpr friend String operator + (const Str& lhs, const String& rhs);

		/**
		* Create a string from a bool
		*/
		[[nodiscard]] constexpr static String fromBool(bool b);

		/**
		* Create a string from a signed integer.
		*/
		[[nodiscard]] constexpr static String fromInt(i64 num);

		/**
		* Create a string from an unsigned integer.
		*/
		[[nodiscard]] constexpr static String fromUint(u64 num);

		/**e
		* Creates a string from a float (double) with variable precision.
		* Due to rounding errors of floats, some string representations will be SLIGHTLY off.
		* All numbers have decimals after, including whole numbers (eg. "0.0").
		* Positive and negative infinity are "inf" and "-inf" respectively. Not a Number is "nan".
		*
		* @param num: Number to convert to a string.
		* @param precision: Maximum decimal precision. Must be 19 or less.
		*/
		[[nodiscard]] constexpr static String fromFloat(double num, const int precision = 5);

		/**
		* Allow converting types to a String. Specialization is required.
		* Specializing gk::String::from<T>() will allow use in gk::String::format()
		*/
		template<typename T>
		[[nodiscard]] constexpr static String from(const T& value) = delete;

		template<>
		[[nodiscard]] constexpr static String from<bool>(const bool& b) { return fromBool(b); }

		template<>
		[[nodiscard]] constexpr static String from<i8>(const i8& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<i16>(const i16& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<int>(const int& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<i64>(const i64& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<u8>(const u8& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<u16>(const u16& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<u32>(const u32& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<u64>(const u64& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<float>(const float& num) { return fromFloat(num); }

		template<>
		[[nodiscard]] constexpr static String from<double>(const double& num) { return fromFloat(num); }

		/**
		* Construct a formatted rust-like string with variadic argument converted into strings.
		* All variadic arguments must be specialized from gk::String::from<T>(), or be a gk::String/gk::Str.
		* Example of usage `gk::String s = gk::String::format<"numbers are {} and {} :D">(num1, num2);`
		*
		* @param formatStr: Template argument defining the layout of the format string. Is validated at compile time.
		* @param inputs: Variadic argument that must have gk::String::from<T>() specialized, or be a String/Str.
		*
		* @return Formatted string
		*/
		template<gk::Str formatStr, typename... Types>
		constexpr static String format(const Types&... inputs);

		/**
		* Find the first occurrence of a character within this String.
		* Is SIMD optimized.
		*/
		constexpr Option<usize> find(char c) const;

		/**
		* Find the starting index of a string slice within this String.
		* Is SIMD optimzed.
		*/
		constexpr Option<usize> find(const Str& str) const;

		/**
		* Find the starting index of a substring within this String.
		* Is SIMD optimzed.
		*/
		constexpr Option<usize> find(const String& other) const { return find(other.asStr()); }

		/**
		* Creates a substring from `startIndexInclusive` to `endIndexExclusive`.
		* Checks that the substring is valid UTF8.
		* Uses indices into the string, not UTF8 codepoints.
		* Doing `substring(start, start + 10)` will make a substring of `len() == 10`.
		*
		* @param startIndexInclusive: Beginning of the substring. cstr()[startIndexInclusive] is included.
		* @param endIndexInclusive: End of the substring. cstr()[endIndexExclusive] is NOT included.
		* @return Valid UTF8 substring.
		*/
		constexpr String substring(usize startIndexInclusive, usize endIndexExclusive) const;

		/**
		* Parses a bool from the string.
		* If the string is "true", returns an Ok variant of `true`.
		* If the string is "false", returns an Ok variant of `false`.
		* All other string values will be an Error variant.
		* 
		* @return The parsed boolean, or an error.
		*/
		constexpr Result<bool> parseBool() const;

		/**
		* Parses a signed 64 bit integer from the string.
		* For example, the string of "-1234" returns an Ok variant of `-1234`.
		* 
		* Errors:
		* 
		* - Decimals will return an Error variant (eg. 12.5).
		* - Anything out of the signed 64 bit range will return an Error (eg. "9223372036854775808" / "-9223372036854775809").
		* 
		* @return The parsed signed 64 bit integer, or an error.
		*/
		constexpr Result<i64> parseInt() const;

		/**
		* Parses an unsigned 64 bit integer from the string.
		* For example, the string of "1234" returns an Ok variant of `-1234`.
		* 
		* Errors:
		* 
		* - Negatives will return an Error variant.
		* - Decimals will return an Error variant (eg. 12.5).
		* - Anything out of the signed 64 bit range will return an Error (eg. "18446744073709551616").
		* 
		* @return The parsed unsigned 64 bit integer, or an error.
		*/
		constexpr Result<u64> parseUint() const;

	private:

		constexpr void setHeapFlag() {
			flag = FLAG_BIT;
		}

		constexpr bool isSso() const {
			return !(flag & FLAG_BIT);
		}

		constexpr void setSsoLen(usize newLen) {
			check_le(newLen, MAX_SSO_LEN);
			flag = static_cast<char>(MAX_SSO_LEN) - static_cast<char>(newLen);
		}

		constexpr usize ssoLen() const {
			return MAX_SSO_LEN - static_cast<usize>(flag);
		}

		static constexpr char* mallocHeapBuffer(usize* inCapacity) {
			if (std::is_constant_evaluated()) {
				const usize capacity = *inCapacity;
				char* constexprBuffer = new char[capacity];
				for (usize i = 0; i < capacity; i++) {
					constexprBuffer[i] = '\0';
				}
				return constexprBuffer;
			}
			else {
				return mallocCharBufferAligned(inCapacity);
			}
		}

		static constexpr void freeHeapBuffer(char*& inBuffer, usize inCapacity) {
			if (std::is_constant_evaluated()) {
				delete[] inBuffer;
				inBuffer = nullptr;
			}
			else {
				freeCharBufferAligned(inBuffer, inCapacity);
			}
		}

		constexpr void copyOtherSsoStringData(const String& other) {
			if (std::is_constant_evaluated()) {
				for (usize i = 0; i < 31; i++) {
					rep.sso.chars[i] = other.rep.sso.chars[i];
				}
				flag = other.flag;
			}
			else {
				// The layout of String ensures this is safe.
				memcpy(this, &other, sizeof(String));
			}
		}

		static constexpr void copyChars(char* destination, const char* source, usize num) {
			if (std::is_constant_evaluated()) {
				for (usize i = 0; i < num; i++) {
					destination[i] = source[i];
				}
			}
			else {
				memcpy(destination, source, num);
			}
		}

		constexpr void freeHeapBufferIfNotSso() {
			if (!isSso()) {
				if (rep.heap.buffer != nullptr) {
					freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
				}
			}
		}

		bool equalToStrSimd(const gk::Str& str) const;

		bool equalToStringSimd(const gk::String& other) const;

		Option<usize> findCharInStringSimd(char c) const;

		Option<usize> findStrInStringSimd(const gk::Str& str) const;

	};// struct String
} // namespace gk

inline constexpr gk::String::String(const Str& str)
	: flag(0)
{
	if (str.len <= MAX_SSO_LEN) {
		copyChars(rep.sso.chars, str.buffer, str.len);
		setSsoLen(str.len);
		return;
	}
	rep.heap = HeapRep();

	usize capacity = str.len + 1; // null terminator
	char* buffer = mallocHeapBuffer(&capacity);
	copyChars(buffer, str.buffer, str.len);
	setHeapFlag();
	rep.heap.buffer = buffer;
	rep.heap.length = str.len;
	rep.heap.capacity = capacity;
}

inline constexpr gk::String::String(const String& other)
	: flag(0)
{
	if (other.isSso()) {
		copyOtherSsoStringData(other);
		return;
	}
	rep.heap = HeapRep();

	usize capacity = other.rep.heap.length + 1; // null terminator
	char* buffer = mallocHeapBuffer(&capacity);
	if (other.rep.heap.buffer) {
		copyChars(buffer, other.rep.heap.buffer, other.rep.heap.length);
	}
	setHeapFlag();
	rep.heap.buffer = buffer;
	rep.heap.length = other.rep.heap.length;
	rep.heap.capacity = capacity;
}

inline constexpr gk::String::String(String&& other) noexcept
	: flag(0)
{
	if (std::is_constant_evaluated()) {
		if (other.isSso()) {
			copyOtherSsoStringData(other);
			return;
		}
		rep.heap = HeapRep();
		rep.heap.buffer = other.rep.heap.buffer;
		rep.heap.length = other.rep.heap.length;
		rep.heap.capacity = other.rep.heap.capacity;
		flag = other.flag;

		other.rep.heap.buffer = nullptr;
		other.rep.heap.length = 0;
		other.rep.heap.capacity = 0;

		return;
	}
	else {
		// The layout of String ensures this is safe.
		memcpy(this, &other, sizeof(String));
		memset(&other, 0, sizeof(String));
	}
}

inline constexpr gk::String& gk::String::operator=(char c)
{
	freeHeapBufferIfNotSso();
	rep.sso = SsoRep();
	rep.sso.chars[0] = c;
	setSsoLen(1);
	return *this;
}

inline constexpr gk::String& gk::String::operator=(const Str& str)
{
	const bool shouldFreeHeap = !isSso() && (rep.heap.buffer != nullptr);

	// Sso only
	if (str.len <= MAX_SSO_LEN) {
		if (shouldFreeHeap) {
			freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
		}
		rep.sso = SsoRep();
		copyChars(rep.sso.chars, str.buffer, str.len);
		setSsoLen(str.len);
		return *this;
	}

	// Check if can reuse existing allocation
	if (!std::is_constant_evaluated() && !isSso() && (rep.heap.capacity >= str.len)) {
		check_ne(str.buffer, nullptr);
		memcpy(rep.heap.buffer, str.buffer, str.len);
		memset(rep.heap.buffer + str.len, '\0', rep.heap.capacity - str.len);

		rep.heap.length = str.len;
		return *this;
	}

	// Allocate buffer and copy
	if (shouldFreeHeap) {
		freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
	}
	rep.heap = HeapRep();

	usize capacity = str.len + 1; // null terminator
	char* buffer = mallocHeapBuffer(&capacity);
	copyChars(buffer, str.buffer, str.len);
	setHeapFlag();
	rep.heap.buffer = buffer;
	rep.heap.length = str.len;
	rep.heap.capacity = capacity;
	return *this;
}

inline constexpr gk::String& gk::String::operator=(const String& other)
{
	const bool shouldFreeHeap = !isSso() && (rep.heap.buffer != nullptr);

	// Sso only
	if (other.isSso()) {
		if (shouldFreeHeap) {
			freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
		}
		rep.sso = SsoRep();
		copyOtherSsoStringData(other);
		return *this;
	}

	// Check if can reuse existing allocation
	// The other string is guaranteed to be heap at this point
	if (!isSso() && (rep.heap.capacity >= other.rep.heap.length)) {
		if (std::is_constant_evaluated()) {
			usize i = 0;
			for (; i < other.rep.heap.length; i++) {
				rep.heap.buffer[i] = other.rep.heap.buffer[i];
			}
			for (; i < rep.heap.capacity; i++) {
				rep.heap.buffer[i] = '\0';
			}
		}
		else {
			if (other.rep.heap.buffer) {
				memcpy(rep.heap.buffer, other.rep.heap.buffer, other.rep.heap.length);
			}
			memset(rep.heap.buffer + other.rep.heap.length, '\0', rep.heap.capacity - other.rep.heap.length);
		}

		rep.heap.length = other.rep.heap.length;
		return *this;
	}

	// Allocate buffer and copy
	if (shouldFreeHeap) {
		freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
	}
	rep.heap = HeapRep();

	usize capacity = other.rep.heap.length + 1; // null terminator
	char* buffer = mallocHeapBuffer(&capacity);
	copyChars(buffer, other.rep.heap.buffer, other.rep.heap.length);
	setHeapFlag();
	rep.heap.buffer = buffer;
	rep.heap.length = other.rep.heap.length;
	rep.heap.capacity = capacity;
	return *this;
}

inline constexpr gk::String& gk::String::operator=(String&& other) noexcept
{
	freeHeapBufferIfNotSso();
	if (std::is_constant_evaluated()) {
		if (other.isSso()) {
			copyOtherSsoStringData(other);
			return *this;
		}
		rep.heap = HeapRep();
		rep.heap.buffer = other.rep.heap.buffer;
		rep.heap.length = other.rep.heap.length;
		rep.heap.capacity = other.rep.heap.capacity;
		flag = other.flag;

		other.rep.heap.buffer = nullptr;
		other.rep.heap.length = 0;
		other.rep.heap.capacity = 0;

		return *this;
	}
	else {
		// The layout of String ensures this is safe.
		memcpy(this, &other, sizeof(String));
		memset(&other, 0, sizeof(String));
		return *this;
	}
}

inline constexpr gk::usize gk::String::len() const
{
	if (std::is_constant_evaluated()) {
		if (isSso()) {
			return ssoLen();
		}
		else {
			return rep.heap.length;
		}
	}

	const usize isSsoRep = isSso();
	const usize isHeapRep = !isSso();
	return (isSsoRep * ssoLen()) + (isHeapRep * rep.heap.length);
}

//namespace gk {
//	namespace internal {
//		constexpr usize charToUsize(const char* charPtr) {
//			usize out = 0;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[0]));
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 8;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 16;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 24;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 32;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 40;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 48;
//			out |= static_cast<usize>(static_cast<u8>(charPtr[1])) << 56;
//			return out;
//		}
//	}
//}

inline constexpr bool gk::String::operator==(char c) const
{
	constexpr char FLAG_SSO_AND_LENGTH_1 = 31 - 1;
	if (flag == FLAG_SSO_AND_LENGTH_1) {
		return rep.sso.chars[0] == c;
	}
	return false;
}

inline constexpr bool gk::String::operator==(const Str& str) const
{
	if (std::is_constant_evaluated()) {
		if (len() != str.len) return false;
		const char* thisStr = cstr();
		for (usize i = 0; i < str.len; i++) {
			if (thisStr[i] != str.buffer[i]) return false;
		}
		return true;
	}
	else {
		return equalToStrSimd(str);
	}
}

inline constexpr bool gk::String::operator==(const String& other) const
{
	if (std::is_constant_evaluated()) {
		const usize length = len();
		if (length != other.len()) return false;
		const char* thisStr = cstr();
		const char* otherStr = other.cstr();
		for (u64 i = 0; i < length; i++) {
			if (thisStr[i] != otherStr[i]) return false;
		}
		return true;
	}
	else {
		return equalToStringSimd(other);
	}
}

inline constexpr gk::String& gk::String::append(char c)
{
	const usize length = len();
	const usize newLength = length + 1;
	const usize minCapacity = newLength + 1;

	if (newLength <= MAX_SSO_LEN) {
		check_message(isSso(), "String representation should already be SSO");
		rep.sso.chars[length] = c;
		setSsoLen(newLength);
		//check_eq(rep.sso.chars[newLength], '\0');
		return *this;
	}

	if (isSso()) { // If it is an SSO, it must reallocate.
		char temp[31]; // does not include null terminator
		copyChars(temp, rep.sso.chars, 31);

		rep.heap = HeapRep();
		usize mallocCapacity = 64;
		char* buffer = mallocHeapBuffer(&mallocCapacity);
		copyChars(buffer, temp, 31);
		buffer[31] = c;
		rep.heap.buffer = buffer;
		rep.heap.length = newLength;
		rep.heap.capacity = mallocCapacity;
		setHeapFlag();
		return *this;
	}

	if (minCapacity > rep.heap.capacity) { // must reallocate
		usize mallocCapacity = minCapacity;
		char* buffer = mallocHeapBuffer(&mallocCapacity);
		copyChars(buffer, rep.heap.buffer, length);
		freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
		rep.heap.buffer = buffer;
		rep.heap.capacity = mallocCapacity;
	}

	rep.heap.buffer[length] = c;
	rep.heap.length = newLength;
	return *this;
}

inline constexpr gk::String& gk::String::append(const Str& str)
{
	const usize length = len();
	const usize newLength = length + str.len;
	const usize minCapacity = newLength + 1;

	if (newLength <= MAX_SSO_LEN) {
		check_message(isSso(), "String representation should already be SSO");
		copyChars(rep.sso.chars + length, str.buffer, str.len);
		setSsoLen(newLength);
		//check_eq(rep.sso.chars[newLength], '\0');
		return *this;
	}

	usize mallocCapacity = minCapacity;

	if (isSso()) { // will need to reallocate
		char temp[31]; // does not include null terminator
		copyChars(temp, rep.sso.chars, 31);

		rep.heap = HeapRep();
		char* buffer = mallocHeapBuffer(&mallocCapacity);
		copyChars(buffer, temp, length);
		copyChars(buffer + length, str.buffer, str.len);

		rep.heap.buffer = buffer;
		rep.heap.length = newLength;
		rep.heap.capacity = mallocCapacity;
		setHeapFlag();
		return *this;
	}

	if (minCapacity > rep.heap.capacity) { // must reallocate
		char* buffer = mallocHeapBuffer(&mallocCapacity);
		copyChars(buffer, rep.heap.buffer, length);
		freeHeapBuffer(rep.heap.buffer, rep.heap.capacity);
		rep.heap.buffer = buffer;
		rep.heap.capacity = mallocCapacity;
	}

	copyChars(rep.heap.buffer + length, str.buffer, str.len);
	rep.heap.length = newLength;
	return *this;
}

constexpr gk::String gk::operator+(const String& lhs, char rhs)
{
	const usize length = lhs.len();
	const usize newLength = length + 1;
	const usize minCapacity = newLength + 1;

	String newString;

	if (newLength <= String::MAX_SSO_LEN) {
		//check_message(lhs.isSso(), "String representation should already be SSO");
		String::copyChars(newString.rep.sso.chars, lhs.rep.sso.chars, length);
		newString.rep.sso.chars[length] = rhs;
		newString.setSsoLen(newLength);
		return newString;
	}

	newString.rep.heap = String::HeapRep();

	usize mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
	char* buffer = String::mallocHeapBuffer(&mallocCapacity);
	String::copyChars(buffer, lhs.cstr(), length);
	buffer[length] = rhs;

	newString.rep.heap.buffer = buffer;
	newString.rep.heap.length = newLength;
	newString.rep.heap.capacity = mallocCapacity;
	newString.setHeapFlag();
	return newString;
}

constexpr gk::String gk::operator+(const String& lhs, const Str& rhs)
{
	const usize length = lhs.len();
	const usize newLength = length + rhs.len;
	const usize minCapacity = newLength + 1;

	String newString;

	if (newLength <= String::MAX_SSO_LEN) {
		check_message(lhs.isSso(), "String representation should already be SSO");
		String::copyChars(newString.rep.sso.chars, lhs.rep.sso.chars, length);
		String::copyChars(newString.rep.sso.chars + length, rhs.buffer, rhs.len);
		newString.setSsoLen(newLength);
		return newString;
	}

	newString.rep.heap = String::HeapRep();

	usize mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
	char* buffer = String::mallocHeapBuffer(&mallocCapacity);
	String::copyChars(buffer, lhs.cstr(), length);
	String::copyChars(buffer + length, rhs.buffer, rhs.len);

	newString.rep.heap.buffer = buffer;
	newString.rep.heap.length = newLength;
	newString.rep.heap.capacity = mallocCapacity;
	newString.setHeapFlag();
	return newString;
}

constexpr gk::String gk::operator+(char lhs, const String& rhs)
{
	const usize length = rhs.len();
	const usize newLength = 1 + length;
	const usize minCapacity = newLength + 1;

	String newString;

	if (newLength <= String::MAX_SSO_LEN) {
		check_message(rhs.isSso(), "String representation should already be SSO");
		newString.rep.sso.chars[0] = lhs;
		String::copyChars(newString.rep.sso.chars + 1, rhs.rep.sso.chars, length);
		newString.setSsoLen(newLength);
		return newString;
	}

	newString.rep.heap = String::HeapRep();

	usize mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
	char* buffer = String::mallocHeapBuffer(&mallocCapacity);
	buffer[0] = lhs;
	String::copyChars(buffer + 1, rhs.cstr(), length);

	newString.rep.heap.buffer = buffer;
	newString.rep.heap.length = newLength;
	newString.rep.heap.capacity = mallocCapacity;
	newString.setHeapFlag();
	return newString;
}

constexpr gk::String gk::operator+(const Str& lhs, const String& rhs)
{
	const usize length = rhs.len();
	const usize newLength = lhs.len + length;
	const usize minCapacity = newLength + 1;

	String newString;

	if (newLength <= String::MAX_SSO_LEN) {
		check_message(rhs.isSso(), "String representation should already be SSO");
		String::copyChars(newString.rep.sso.chars, lhs.buffer, lhs.len);
		String::copyChars(newString.rep.sso.chars + lhs.len, rhs.rep.sso.chars, length);
		newString.setSsoLen(newLength);
		return newString;
	}

	newString.rep.heap = String::HeapRep();

	usize mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
	char* buffer = String::mallocHeapBuffer(&mallocCapacity);
	String::copyChars(buffer, lhs.buffer, lhs.len);
	String::copyChars(buffer + lhs.len, rhs.cstr(), length);

	newString.rep.heap.buffer = buffer;
	newString.rep.heap.length = newLength;
	newString.rep.heap.capacity = mallocCapacity;
	newString.setHeapFlag();
	return newString;
}

inline constexpr gk::String gk::String::fromBool(bool b) {
	String newString;
	if (b) {
		copyChars(newString.rep.sso.chars, "true", 4);
		newString.setSsoLen(4);
		return newString;
	}
	copyChars(newString.rep.sso.chars, "false", 5);
	newString.setSsoLen(5);
	return newString;
}

inline constexpr gk::String gk::String::fromInt(i64 num)
{
	if (num == 0) return String('0');

	constexpr const char* digits = "9876543210123456789";
	constexpr usize zeroDigit = 9;
	constexpr usize maxChars = 20;

	const bool isNegative = num < 0;

	char tempNums[maxChars];
	usize tempAt = maxChars;

	while (num) {
		tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
		num /= 10LL;
	}
	if (isNegative) {
		tempNums[--tempAt] = '-';
	}

	const usize length = maxChars - tempAt;

	String newString;
	copyChars(newString.rep.sso.chars, tempNums + tempAt, length);
	newString.setSsoLen(length);
	return newString;
}

inline constexpr gk::String gk::String::fromUint(u64 num)
{
	if (num == 0) return String('0');

	constexpr const char* digits = "9876543210123456789";
	constexpr usize zeroDigit = 9;
	constexpr usize maxChars = 20;

	char tempNums[maxChars];
	usize tempAt = maxChars;

	while (num) {
		tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
		num /= 10LL;
	}

	const usize length = maxChars - tempAt;

	String newString;
	copyChars(newString.rep.sso.chars, tempNums + tempAt, length);
	newString.setSsoLen(length);
	return newString;
}

namespace gk {
	namespace internal {
		__pragma(optimize("", off))
			static constexpr void stripNegativeZero(double& inFloat)
		{
			// Taken from Unreal Engine
			// This works for translating a negative zero into a positive zero,
			// but if optimizations are enabled when compiling with -ffast-math
			// or /fp:fast, the compiler can strip it out.
			inFloat += 0.0;
		}
		__pragma(optimize("", on))
	} // namespace internal
} // namespace gk

inline constexpr gk::String gk::String::fromFloat(double num, const int precision)
{
	check_message(precision < 20, "gk::String::FromFloat precision must be 19 or less. The precision is set to ", precision);

	if (num == 0.0)							return String("0.0"_str);			// Zero
	else if (num > DBL_MAX)		return String("inf"_str);			// Positive Infinity
	else if (num < -DBL_MAX)	return String("-inf"_str);		// Negative Infinity
	else if (num != num)			return String("nan"_str);			// Not a Number

	internal::stripNegativeZero(num);
	const bool isNegative = num < 0;

	const i64 whole = static_cast<i64>(num);
	const String wholeString = (isNegative && whole == 0) ? gk::Str("-0") : String::fromInt(whole);

	num -= static_cast<double>(whole);
	if (num == 0) {
		return wholeString + gk::Str(".0"); // quick return if there's nothing after the decimal
	}

	if (num < 0) num *= -1; // Make fractional part positive
	int zeroesInFractionBeforeFirstNonZero = 0;
	for (int i = 0; i < precision; i++) {
		num *= 10;
		if (num < 1) zeroesInFractionBeforeFirstNonZero += 1;
	}

	const u64 fraction = static_cast<u64>(num);
	if (fraction == 0) {
		return wholeString + gk::Str(".0"); // quick return if after the precision check, there's nothing
	}

	auto MakeFractionZeroesString = [](const int zeroCount) {
		String str;
		for (int i = 0; i < zeroCount; i++) {
			str.rep.sso.chars[i] = '0';
		}
		str.setSsoLen(zeroCount);
		return str;
	};

	auto MakeStringWithIntWithoutEndingZeroes = [](const u64 value, const int availableDigits) {
		String str = String::fromUint(value);
		const u64 lastIndex = (availableDigits < str.len() - 1) ? availableDigits : str.len() - 1;

		for (u64 i = lastIndex; i > 0; i--) {
			if (str.rep.sso.chars[i] != '0') {
				for (u64 c = i + 1; c < 30; c++) {
					str.rep.sso.chars[c] = '\0';
				}
				str.setSsoLen(i + 1);
				return str;
			}
			if (i == 1) { // was on last iteration
				for (u64 c = i; c < 30; c++) {
					str.rep.sso.chars[i] = '\0';
				}
				str.setSsoLen(1);
				return str;
			}
		}
		return str; // completely not necessary
	};

	const String fractionZeroesString = MakeFractionZeroesString(zeroesInFractionBeforeFirstNonZero);
	const String fractionalString = MakeStringWithIntWithoutEndingZeroes(fraction, precision - zeroesInFractionBeforeFirstNonZero);

	return wholeString + '.' + fractionZeroesString + fractionalString;
}

namespace gk {
	namespace internal {

		struct FormatStringStrOffset {
			const char* start;
			usize count;
		};

		template<typename T>
		constexpr static String formatStringConvertArg(const T& arg) {
			auto convertCheck = [](auto& t) -> decltype(String::from<T>(t)) {
				return String::from<T>(t);
			};
			constexpr bool canMakeStringFromType =
				std::is_invocable_v < decltype(convertCheck), T&>;
			static_assert(canMakeStringFromType, "String::from<T>() is not specialized for type T");
			return String::from<T>(arg);
		}

		consteval Result<usize> formatStrCountArgs(const gk::Str& formatStr) {
			usize count = 0;
			for (usize i = 0; i < formatStr.len; i++) {
				if (formatStr.buffer[i] == '{') {
					count++;
					if (formatStr.buffer[i + 1] != '}') {
						return gk::ResultErr();
					}
				}
			}
			return gk::ResultOk<usize>(count);
		}

		//consteval static bool formatStringIsValid(const gk::AltStr& formatStr) {
		//	usize count = 0;
		//	for (usize i = 0; i < formatStr.len; i++) {
		//		if (formatStr.buffer[i] == '{') {
		//			if (formatStr.buffer[i + 1] != '}') {
		//				return false;
		//			}
		//			count++;
		//		}
		//	}
		//	return true;
		//}

		//consteval static usize formatStrCountArgs(const gk::AltStr& formatStr) {
		//	usize count = 0;
		//	for (usize i = 0; i < formatStr.len; i++) {
		//		if (formatStr.buffer[i] == '{') count++;
		//	}
		//	return count;
		//}

		template<gk::Str formatStr>
		constexpr static void formatStrFindOffsets(FormatStringStrOffset* offsetsArr, usize bufferSize) {
			usize currentOffset = 0;
			usize offsetIndex = 0;
			for (usize i = 0; i < formatStr.len; i++) {
				if (formatStr.buffer[i] == '{') {
					offsetsArr[offsetIndex].start = formatStr.buffer + currentOffset;
					offsetsArr[offsetIndex].count = i - currentOffset;
					currentOffset = i + 2;
					offsetIndex++;
				}
			}
			if (formatStr.buffer[formatStr.len - 1] != '}') {
				offsetsArr[bufferSize - 1].start = formatStr.buffer + currentOffset;
				offsetsArr[bufferSize - 1].count = (formatStr.len) - currentOffset;
			}
			else {
				offsetsArr[bufferSize - 1].start = nullptr;
				offsetsArr[bufferSize - 1].count = 0;
			}
		}

	} // namespace internal
} // namespace gk

template<gk::Str formatStr, typename ...Types>
inline constexpr gk::String gk::String::format(const Types & ...inputs)
{
	constexpr Result<usize> formatSlotsCount = internal::formatStrCountArgs(formatStr);
	static_assert(formatSlotsCount.isOk(), "String::format format str is not valid. Required format is \"some value: {}\"");

	constexpr usize argumentCount = sizeof...(Types);
	constexpr usize formatSlots = formatSlotsCount.okCopy();
	static_assert(formatSlots == argumentCount, "Arguments passed into String::format do not match the amount of format slots specified");
	if constexpr (argumentCount == 0) {
		return gk::String(formatStr);
	}

	String argsAsStrings[argumentCount == 0 ? 1 : argumentCount];

	internal::FormatStringStrOffset offsets[argumentCount + 1];
	internal::formatStrFindOffsets<formatStr>(offsets, argumentCount + 1);

	usize mallocCapacity = 0;
	usize i = 0;

	([&] { // loop through all elements, checking the types
		if constexpr (std::is_same_v<Types, gk::String>) {
			mallocCapacity += inputs.len();
		}
		else if constexpr (std::is_same_v<Types, gk::Str>) {
			mallocCapacity += inputs.len;
		}
		else {
			argsAsStrings[i] = internal::formatStringConvertArg<Types>(inputs);
			mallocCapacity += argsAsStrings[i].len();
		}
		i++;
		} (), ...);

	for (i = 0; i < argumentCount; i++) {
		mallocCapacity += offsets[i].count;
	}
	if (offsets[argumentCount].start != nullptr) { // there is some format string after the last argument
		mallocCapacity += offsets[argumentCount].count;
	}

	mallocCapacity += 1; // null terminator
	//usize currentStringIndex = 0;
	usize outputLength = 0;
	i = 0;

	String outString;

	if ((mallocCapacity - 1) <= MAX_SSO_LEN) { // All of the strings can fit within a singular sso buffer
		([&] {
			internal::FormatStringStrOffset offset = offsets[i];
			copyChars(outString.rep.sso.chars + outputLength, offset.start, offset.count);
			outputLength += offset.count;

			if constexpr (std::is_same_v<Types, gk::String>) {
				copyChars(outString.rep.sso.chars + outputLength, inputs.rep.sso.chars, inputs.ssoLen()); // guaranteed to be sso
				outputLength += inputs.ssoLen();
			}
			else if constexpr (std::is_same_v<Types, gk::Str>) {
				copyChars(outString.rep.sso.chars + outputLength, inputs.buffer, inputs.len);
				outputLength += inputs.len;
			}
			else {
				copyChars(outString.rep.sso.chars + outputLength, argsAsStrings[i].rep.sso.chars, argsAsStrings[i].ssoLen());
				outputLength += argsAsStrings[i].ssoLen();
			}
			i++;
			} (), ...);

		if (offsets[argumentCount].start != nullptr) { // need to use the last offset
			internal::FormatStringStrOffset offset = offsets[argumentCount];
			copyChars(outString.rep.sso.chars + outputLength, offset.start, offset.count);
			outputLength += offset.count;
		}

		outString.setSsoLen(outputLength);
		return outString;
	}

	char* buffer = mallocHeapBuffer(&mallocCapacity);
	([&]
		{
			internal::FormatStringStrOffset offset = offsets[i];
			copyChars(buffer + outputLength, offset.start, offset.count);
			outputLength += offset.count;

			if constexpr (std::is_same_v<Types, gk::String>) {
				copyChars(buffer + outputLength, inputs.cstr(), inputs.len()); // guaranteed to be sso
				outputLength += inputs.len();
			}
			else if constexpr (std::is_same_v<Types, gk::Str>) {
				copyChars(buffer + outputLength, inputs.buffer, inputs.len);
				outputLength += inputs.len;
			}
			else {
				copyChars(buffer + outputLength, argsAsStrings[i].cstr(), argsAsStrings[i].len());
				outputLength += argsAsStrings[i].len();
			}

			i++;
		} (), ...);

	if (offsets[argumentCount].start != nullptr) { // need to use the last offset
		internal::FormatStringStrOffset offset = offsets[argumentCount];
		copyChars(buffer + outputLength, offset.start, offset.count);
		outputLength += offset.count;
	}

	outString.rep.heap.buffer = buffer;
	outString.rep.heap.length = outputLength;
	outString.rep.heap.capacity = mallocCapacity;
	outString.setHeapFlag();
	return outString;
}

inline constexpr gk::Option<gk::usize> gk::String::find(char c) const
{
	if (std::is_constant_evaluated()) {
		const char* str = cstr();
		const usize bytesToCheck = len();
		for (usize i = 0; i < bytesToCheck; i++) {
			if (str[i] == c) return Option<usize>(i);
		}
		return Option<usize>();
	}
	return findCharInStringSimd(c);
}

inline constexpr gk::Option<gk::usize> gk::String::find(const Str& str) const
{
	const usize length = len();
	if (str.len > length) {
		return Option<usize>();
	}
	else if (str.len == length) {
		if (*this == str) {
			return Option<usize>(0);
		}
		return Option<usize>();
	}
	if (str.len == 1) {
		return find(str.buffer[0]);
	}

	if (std::is_constant_evaluated()) {
		const char firstChar = str.buffer[0];
		const char* thisStr = cstr();
		for (u64 i = 0; i < length; i++) {
			if (thisStr[i] == firstChar) {
				const char* thisCompareStart = thisStr + i;

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
		return findStrInStringSimd(str);
	}
}

inline constexpr gk::String gk::String::substring(usize startIndexInclusive, usize endIndexExclusive) const
{
	check_message(startIndexInclusive <= len(), "Substring start index must be within string used utf8 bytes count. start index is ", startIndexInclusive, " and used bytes is ", len());
	check_message(endIndexExclusive <= len(), "Substring end index must be less than or equal to the string used utf8 bytes count. end index is ", endIndexExclusive, " and used bytes is ", len());
	check_message(startIndexInclusive < endIndexExclusive, "Substring start index must be less than end index. start index is ", startIndexInclusive, " and end index is ", endIndexExclusive);

	String outStr = String();

	const usize substrLen = endIndexExclusive - startIndexInclusive;
	const usize minCapacity = substrLen + 1; // include null terminator

	if (minCapacity <= MAX_SSO_LEN) {
		const char* copyStart = cstr() + startIndexInclusive;
		copyChars(outStr.rep.sso.chars, copyStart, substrLen);
		outStr.setSsoLen(substrLen);
		return outStr;
	}

	outStr.rep.heap = HeapRep();
	usize mallocCapacity = minCapacity;

	char* newBuffer = mallocHeapBuffer(&mallocCapacity);
	const char* copyStart = cstr() + startIndexInclusive;
	copyChars(newBuffer, copyStart, substrLen);

	outStr.rep.heap.buffer = newBuffer;
	outStr.rep.heap.length = substrLen;
	outStr.rep.heap.capacity = mallocCapacity;
	outStr.setHeapFlag();
	return outStr;
}

inline constexpr gk::Result<bool> gk::String::parseBool() const
{
	constexpr usize TRUE_STRING_BUFFER = []() {
		usize out = 0;
		out |= static_cast<usize>('t');
		out |= static_cast<usize>('r') << 8;
		out |= static_cast<usize>('u') << 16;
		out |= static_cast<usize>('e') << 24;
		return out;
	}();

	constexpr usize FALSE_STRING_BUFFER = []() {
		usize out = 0;
		out |= static_cast<usize>('f');
		out |= static_cast<usize>('a') << 8;
		out |= static_cast<usize>('l') << 16;
		out |= static_cast<usize>('s') << 24;
		out |= static_cast<usize>('e') << 32;
		return out;
	}();

	if (!isSso()) {
		return ResultErr();
	}

	const usize ssoBufferAsMachineWord = [&]() {
		if (std::is_constant_evaluated()) {
			usize out = 0;
			out |= static_cast<usize>(rep.sso.chars[0]);
			out |= static_cast<usize>(rep.sso.chars[1]) << 8;
			out |= static_cast<usize>(rep.sso.chars[2]) << 16;
			out |= static_cast<usize>(rep.sso.chars[3]) << 24;
			out |= static_cast<usize>(rep.sso.chars[4]) << 32;
			out |= static_cast<usize>(rep.sso.chars[5]) << 40;
			out |= static_cast<usize>(rep.sso.chars[6]) << 48;
			out |= static_cast<usize>(rep.sso.chars[7]) << 56;
			return out;
		}
		else {
			return *reinterpret_cast<const usize*>(rep.sso.chars);
		}
	}();

	if (ssoBufferAsMachineWord == TRUE_STRING_BUFFER) {
		return ResultOk<bool>(true);
	}
	else if (ssoBufferAsMachineWord == FALSE_STRING_BUFFER) {
		return ResultOk<bool>(false);
	}
	else {
		return ResultErr();
	}
}

namespace gk {
	namespace internal {
		constexpr u64 convertCharToInt(char c) {
			return static_cast<u64>(c - '0');
		}
	}
}

inline constexpr gk::Result<gk::i64> gk::String::parseInt() const
{
	// All ints will fit within the 31 char SSO buffer.
	if (!isSso()) {
		return ResultErr();
	}

	const usize length = ssoLen();
	const char* buffer = rep.sso.chars;

	if (length == 0) return ResultErr();

	const bool isNegative = buffer[0] == '-';

	if (length == 1) { // fast return
		if (buffer[0] >= '0' && buffer[0] <= '9') {
			return ResultOk<i64>(static_cast<i64>(internal::convertCharToInt(buffer[0])));
		}
	}
	else if (length == 2 && isNegative) {
		if (buffer[1] >= '0' && buffer[1] <= '9') {
			return ResultOk<i64>(static_cast<i64>(internal::convertCharToInt(buffer[1])) * -1LL);
		}
	}

	// validate
	do {
		// max/min signed int64.
		constexpr usize MAX_NUMBER_LENGTH = 19;
		bool isLengthMax = false;
		if (isNegative) {
			if (length > (MAX_NUMBER_LENGTH + 1)) return ResultErr();
			if (length == (MAX_NUMBER_LENGTH + 1)) isLengthMax = true;
		}
		else {
			if (length > MAX_NUMBER_LENGTH) return ResultErr();
			if (length == MAX_NUMBER_LENGTH) isLengthMax = true;
		}

		usize i = static_cast<usize>(isNegative); // start at 0 for positive, or 1 for negative;
		for (; i < length; i++) {
			const char c = buffer[i];
			if (c >= '0' && c <= '9') {
				continue;
			}
			return ResultErr();
		}

		// MUST ensure string is within 64 bit signed int bounds.
		if (isLengthMax) {
			i = static_cast<usize>(isNegative);

			// + 9,223,372,036,854,775,807
			// - 9,223,372,036,854,775,808
			if (buffer[i] != '9') {
				break;
			}
			if (buffer[i + 1] > '2') {
				return ResultErr();
			}
			if (buffer[i + 2] > '2') {
				return ResultErr();
			}
			if (buffer[i + 3] > '3') {
				return ResultErr();
			}
			if (buffer[i + 4] > '3') {
				return ResultErr();
			}
			if (buffer[i + 5] > '7') {
				return ResultErr();
			}
			if (buffer[i + 6] > '2') {
				return ResultErr();
			}
			if (buffer[i + 7] > '0') {
				return ResultErr();
			}
			if (buffer[i + 8] > '3') {
				return ResultErr();
			}
			if (buffer[i + 9] > '6') {
				return ResultErr();
			}
			if (buffer[i + 10] > '8') {
				return ResultErr();
			}
			if (buffer[i + 11] > '5') {
				return ResultErr();
			}
			if (buffer[i + 12] > '4') {
				return ResultErr();
			}
			if (buffer[i + 13] > '7') {
				return ResultErr();
			}
			if (buffer[i + 14] > '7') {
				return ResultErr();
			}
			if (buffer[i + 15] > '5') {
				return ResultErr();
			}
			if (buffer[i + 16] > '8') {
				return ResultErr();
			}
			if (buffer[i + 17] > '0') {
				return ResultErr();
			}

			if (isNegative) {
				if (buffer[i + 18] > '8') {
					return ResultErr();
				}
			}
			else {
				if (buffer[i + 18] > '7') {
					return ResultErr();
				}
			}
		} 
	} while (false); // allow breaking

	const char* end = buffer + length - 1;

	i64 out = 0;

	{
		const i64 lengthToCheck = static_cast<i64>(isNegative ? length - 1 : length);
		for (i64 i = 0; i < lengthToCheck; i++) {
			const i64 tens = [](i64 index) {
				i64 ret = 1;
				for (i64 _i = 0; _i < index; _i++) {
					ret *= 10;
				}
				return ret;
			}(i);

			const char c = *(end - i); // decrement
			out += static_cast<i64>(internal::convertCharToInt(c)) * tens;
		}
	}

	if (isNegative) {
		out *= -1LL;
	}

	return ResultOk<i64>(out);
}

inline constexpr gk::Result<gk::u64> gk::String::parseUint() const
{
	// All ints will fit within the 31 char SSO buffer.
	if (!isSso()) {
		return ResultErr();
	}

	const usize length = ssoLen();
	const char* buffer = rep.sso.chars;

	if (length == 0) return ResultErr();

	if (length == 1) { // fast return
		if (buffer[0] >= '0' && buffer[0] <= '9') {
			return ResultOk<u64>(internal::convertCharToInt(buffer[0]));
		}
	}

	// validate
	do {
		// max chars in an unsigned 64 bit integer
		constexpr usize MAX_NUMBER_LENGTH = 20;
		const bool isLengthMax = length == MAX_NUMBER_LENGTH;

		if (length > MAX_NUMBER_LENGTH) return ResultErr();

		for (usize i = 0; i < length; i++) {
			const char c = buffer[i];
			if (c >= '0' && c <= '9') {
				continue;
			}
			return ResultErr();
		}

		if (!isLengthMax) {
			break;
		}

		// 18,446,744,073,709,551,615
		if (buffer[0] > '1') {
			return ResultErr();
		}
		if (buffer[1] > '8') {
			return ResultErr();
		}
		if (buffer[2] > '4') {
			return ResultErr();
		}
		if (buffer[3] > '4') {
			return ResultErr();
		}
		if (buffer[4] > '6') {
			return ResultErr();
		}
		if (buffer[5] > '7') {
			return ResultErr();
		}
		if (buffer[6] > '4') {
			return ResultErr();
		}
		if (buffer[7] > '4') {
			return ResultErr();
		}
		if (buffer[8] > '0') {
			return ResultErr();
		}
		if (buffer[9] > '7') {
			return ResultErr();
		}
		if (buffer[10] > '3') {
			return ResultErr();
		}
		if (buffer[11] > '7') {
			return ResultErr();
		}
		if (buffer[12] > '0') {
			return ResultErr();
		}
		if (buffer[13] > '9') {
			return ResultErr();
		}
		if (buffer[14] > '5') {
			return ResultErr();
		}
		if (buffer[15] > '5') {
			return ResultErr();
		}
		if (buffer[16] > '1') {
			return ResultErr();
		}
		if (buffer[17] > '6') {
			return ResultErr();
		}
		if (buffer[18] > '1') {
			return ResultErr();
		}
		if (buffer[19] > '5') {
			return ResultErr();
		}
	} while (false);

	const char* end = buffer + length - 1;

	u64 out = 0;

	{
		for (u64 i = 0; i < length; i++) {
			const u64 tens = [](u64 index) {
				u64 ret = 1;
				for (u64 _i = 0; _i < index; _i++) {
					ret *= 10;
				}
				return ret;
			}(i);

			const char c = *(end - i); // decrement
			out += internal::convertCharToInt(c) * tens;
		}
	}

	return ResultOk<u64>(out);
}

template<>
inline size_t gk::hash<gk::String>(const gk::String& key) { // Why would you ever use this??
	return key.hash();
}

