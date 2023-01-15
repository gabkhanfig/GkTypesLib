#pragma once

#include "../Utility.h"
#include <iostream>
#include <stdexcept>
#include "../BasicTypes.h"

#define _STRING_SSO_ALIGNMENT 32

namespace gk 
{
	constexpr const char* _emptyString = "";

	/* A constexpr, sso, and const segment enabled string class. Can check if const char*'s passed in exist within the application const data segment,
	and avoid doing unnecessary copying with this.
	@param BUFFER_SIZE: Size of the sso character buffer. Must be a multiple of 32. */
	template<uint64 BUFFER_SIZE = 32>
	struct
		ALIGN_AS(_STRING_SSO_ALIGNMENT)
		buffer_string
	{
		static_assert(BUFFER_SIZE % _STRING_SSO_ALIGNMENT == 0, "The size of the buffer characters must be a multiple of _STRING_SSO_ALIGNMENT (32)");

	private:

		/* Small String Optimization buffer. */
		char sso[BUFFER_SIZE];

		/* Length of the string, whether in the buffer or the heap data. */
		uint64 length;

		/* Heap data or constexpr string. Also points to the beginning of the sso buffer if it's being used. */
		char* data;

		/* Capacity of the heap data string. */
		uint64 capacity;

		/* Is this string currently using the sso buffer? */
		uint8 flagSSOBuffer : 1;

		/* Is this string's data pointer held within the const segment? */
		uint8 flagConstSegment : 1;

	private:

		constexpr static uint64 GetMaxSSOLength() {
			return BUFFER_SIZE - 1;
		}

		/* Sets the data pointer to a const segment. Assumes the pointer passed in is within the const segment. */
		constexpr void SetStringToConstSegment(const char* segment) {
			data = (char*)segment;
			flagSSOBuffer = false;
			flagConstSegment = true;
		}

		/* Yeah :) */
		constexpr void SetLength(uint64 newLength) {
			length = newLength;
		}

		/* Sets the sso buffer to a copy of whatever the characters in chars are.
		Uses len + 1 to include null terminator. Sets the relevant flags. */
		constexpr void SetSSOBufferChars(const char* chars, uint64 len) {
			flagSSOBuffer = true;
			flagConstSegment = false;
			CopyCharsIntoBuffer(chars, len);
			sso[BUFFER_SIZE - 1] = '\0';
			data = &sso[0];
		}

		/* Sets the data pointer to new chars by copying them. Assumes passed in chars is not within the const segment.
		Uses len + 1 to include null terminator. Sets the relevant flags.*/
		constexpr void SetDataChars(const char* chars, uint64 len) {
			flagSSOBuffer = false;
			flagConstSegment = false;
			CopyCharsIntoData(chars, len);
		}

		/* Copies numToCopy + 1, primarily being the null terminator. */
		constexpr void CopyCharsIntoBuffer(const char* chars, uint64 numToCopy) {
			std::copy(chars, &chars[numToCopy + 1], sso);
		}

		/* Copies numToCopy + 1, primarily being the null terminator. */
		constexpr void CopyCharsIntoData(const char* chars, uint64 numToCopy) {
			std::copy(chars, &chars[numToCopy + 1], data);
		}

		/* Attempts to delete the data string. Will not delete under the following conditions.
		1. The string is using the SSO buffer.
		2. The data pointer is nullptr.
		3. If the data pointer is in the const segment and not constexpr. */
		constexpr void TryDeleteDataString() {
			const bool SSOBufferNotUsed = !flagSSOBuffer;
			const bool DataNotNull = data != nullptr;
			const bool DataNotEmptyString = data != gk::_emptyString;

			if (SSOBufferNotUsed && DataNotNull && DataNotEmptyString) {
				const bool IsNotUsingConstSegment = !flagConstSegment;

				if (!std::is_constant_evaluated() && IsNotUsingConstSegment) {
					delete[] data;
					return;
				}

				if (std::is_constant_evaluated()) {
					delete[] data;
				}
				
			}
		}

		/* Performs necessary construction for this string's values from a const char*. */
		constexpr void ConstructConstChar(const char* str)
		{
			const uint64 len = StrLen(str);
			SetLength(len);
			if (!std::is_constant_evaluated() && gk::IsDataInConstSegment(str)) {
				SetStringToConstSegment(str);
				return;
			}

			if (len > GetMaxSSOLength()) {
				capacity = len + 1;
				data = new char[capacity];
				SetDataChars(str, len);
			}
			else {
				SetSSOBufferChars(str, len);
			}
		}

		/* Performs necessary construction for this string's values from another string through copying. */
		constexpr void ConstructCopy(const buffer_string<BUFFER_SIZE>&other)
		{
			const uint64 len = other.Len();
			const char* str = other.CStr();
			SetLength(len);
			if (!std::is_constant_evaluated() && gk::IsDataInConstSegment(str)) {
				SetStringToConstSegment(str);
				return;
			}

			if (len > GetMaxSSOLength()) {
				capacity = len + 1;
				data = new char[capacity];
				SetDataChars(str, len);
			}
			else {
				SetSSOBufferChars(str, len);
			}
		}

		/* Performs necessary construction for this string's values from moving another string. Sets the other string data to by nullptr if it's using it. */
		constexpr void ConstructMove(buffer_string<BUFFER_SIZE> && other) noexcept
		{
			const uint64 len = other.Len();
			const char* str = other.CStr();
			SetLength(len);

			if (!std::is_constant_evaluated() && gk::IsDataInConstSegment(str)) {
				data = (char*)str;
				flagSSOBuffer = false;
				capacity = 0;
				flagConstSegment = true;
				return;
			}

			if (len > GetMaxSSOLength()) {
				capacity = len + 1;
				data = (char*)str;
				other.data = nullptr;
				flagSSOBuffer = false;

			}
			else {
				SetSSOBufferChars(str, len);
			}
		}

	public:

		[[nodiscard]] constexpr static size_t HashConstChar(const char* str)
		{
			// http://www.cse.yorku.ca/~oz/hash.html
			size_t h = 5381;
			int c;
			while ((c = *str++))
				h = ((h << 5) + h) + c;
			return h;
		}

		/* Check the length of a const char*
		TODO SIMD / SSE / AVX optimizations. */
		[[nodiscard]] constexpr static uint64 StrLen(const char* str) {
			return std::char_traits<char>::length(str);
		}

		/* Check if two character arrays are equal.
		TODO SIMD / SSE / AVX optimizations. */
		[[nodiscard]] constexpr static bool StrEqual(const char* str1, const char* str2, uint64 num) {
			if (str1 == str2) {
				return true;
			}

			for (int i = 0; i < num; i++) {
				if (str1[i] != str2[i]) {
					return false;
				}
			}
			return true;
		}

		/**/
		constexpr buffer_string()
			: sso{ '\0' }, data{ &sso[0] }, length{ 0 }, capacity{ 0 }
		{
			SetStringToConstSegment(gk::_emptyString);
		}

		/**/
		constexpr buffer_string(const char* str)
			: sso{ '\0' }, data{ &sso[0] }, length{ 0 }, capacity{ 0 }
		{
			ConstructConstChar(str);
		}

		/**/
		constexpr buffer_string(const buffer_string<BUFFER_SIZE>&other)
			: sso{ '\0' }, data{ &sso[0] }, length{ 0 }, capacity{ 0 }
		{
			ConstructCopy(other);
		}

		/**/
		constexpr buffer_string(buffer_string<BUFFER_SIZE> && other) noexcept
			: sso{ '\0' }, data{ &sso[0] }, length{ 0 }, capacity{ 0 }
		{
			ConstructMove(std::move(other));
		}

		/**/
		constexpr ~buffer_string()
		{
			TryDeleteDataString();
		}

		/**/
		constexpr buffer_string<BUFFER_SIZE>& operator = (const char* str)
		{
			TryDeleteDataString();
			ConstructConstChar(str);
			return *this;
		}

		/**/
		constexpr buffer_string<BUFFER_SIZE>& operator = (const buffer_string<BUFFER_SIZE>&other)
		{
			TryDeleteDataString();
			ConstructCopy(other);
			return *this;
		}

		/**/
		constexpr buffer_string<BUFFER_SIZE>& operator = (buffer_string<BUFFER_SIZE> && other) noexcept
		{
			TryDeleteDataString();
			ConstructMove(std::move(other));
			return *this;
		}

		/* Whether this string is currently using the sso buffer. */
		[[nodiscard]] constexpr bool IsSSO() const { return flagSSOBuffer; }

		/* Whether this string is currently pointing to data in the const data segment. */
		[[nodiscard]] constexpr bool IsConstSegment() const { return flagConstSegment; }

		/* Get the length of this string. */
		[[nodiscard]] constexpr uint64 Len() const { return length; }

		/* Get the const char* string version of this string. Pulls either the sso buffer or the data pointer. */
		[[nodiscard]] constexpr const char* CStr() const {
			return data;
		}

		/**/
		[[nodiscard]] constexpr bool IsEmpty() const { Len() == 0; }

		/* Get a character at a specified index. Not a reference to the character though. */
		[[nodiscard]] constexpr char& At(uint64 index) {
			if (index >= length) {
				throw std::out_of_range("buffer_string character index is out of bounds!");
			}
			return CStr()[index];
		}

		/* Get a character at a specified index. Not a reference to the character though. */
		[[nodiscard]] constexpr char& operator[](uint64 index) {
			return At(index);
		}

		/* Check if this string is equal to a const char*. Can check if they are within the const data segment and bypass most string checks. */
		[[nodiscard]] constexpr bool operator == (const char* str) const
		{
			if (data == str) {
				return true;
			}
			const uint64 len = StrLen(str);
			if (Len() != len) {
				return false;
			}
			return StrEqual(CStr(), str, len);
		}

		/* Check if this string is equal to another string. Can check if they both use the same const data segment pointer and bypass most string checks. */
		[[nodiscard]] constexpr bool operator == (const buffer_string<BUFFER_SIZE>&other) const
		{
			if (data == other.data) {
				return true;
			}
			const uint64 len = other.Len();
			if (Len() != len) {
				return false;
			}
			return StrEqual(CStr(), other.CStr(), len);
		}

		[[nodiscard]] constexpr bool operator != (const char* str) const {
			return !(*this == str);
		}

		[[nodiscard]] constexpr bool operator != (const buffer_string<BUFFER_SIZE>& other) const {
			return !(*this == other);
		}

		/* std::cout << string */
		friend std::ostream& operator << (std::ostream & os, const buffer_string<BUFFER_SIZE>&_string) {
			return os << _string.CStr();
		}

		/* Computes the hash for this string. See buffer_string<T>::HashConstChar(). */
		[[nodiscard]] constexpr size_t ComputeHash() const {
			return HashConstChar(data);
		}

	};

	/* A constexpr, sso, and const segment enabled string class. Can check if const char*'s passed in exist within the application const data segment,
	and avoid doing unnecessary copying with this. Has an internal sso buffer size of 32. */
	typedef buffer_string<32> string;

}

namespace std
{
	template<>
	struct hash<gk::string>
	{
		size_t operator()(const gk::string& str) const {
			return str.ComputeHash();
		}

	};
}

#undef _STRING_SSO_ALIGNMENT
