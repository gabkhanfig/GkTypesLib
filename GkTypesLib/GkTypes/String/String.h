#pragma once

#include "../Utility.h"
#include <iostream>
#include <stdexcept>
#include "../BasicTypes.h"

#define _STRING_SSO_ALIGNMENT 32
#define SSO_STRLEN 23
#define SSO_SIZE SSO_STRLEN + 1

/* Should this string implement all of the std::string functions? Set to false if not. */
#ifndef STD_STRING_SUPPORT
#define STD_STRING_SUPPORT true
#endif

namespace gk 
{
	constexpr const char* _emptyString = "";

	/*  */
	struct
		ALIGN_AS(_STRING_SSO_ALIGNMENT)
		string
	{

	private:

		union representation
		{
			struct ssoString {
				char chars[SSO_SIZE];

				constexpr ssoString() : chars{ '\0' } {}
			};
			struct longString {
				char* data;
				size_t capacity;

				constexpr longString() : data(nullptr), capacity(0) {}
			};

			ssoString sso;
			longString longStr;

			constexpr representation() : sso(ssoString()) {}
		};

		representation rep;
		size_t length : 63;
		size_t isLong : 1;

	public:

		/* The length of the string (does not include null terminator). */
		[[nodiscard]] constexpr size_t Len() const { return length; }

		/* Is the string current using the SSO buffer? */
		[[nodiscard]] constexpr bool IsSSO() const { return !isLong; }

		/* Is the string empty? */
		[[nodiscard]] constexpr bool IsEmpty() const { return length == 0; }

		/* Allocated capacity excluding null terminator. */
		[[nodiscard]] constexpr size_t Capacity() const { return isLong ? rep.longStr.capacity - 1 : SSO_STRLEN; }

		constexpr string() : length(0), isLong(0) {}

		constexpr string(char c) { Internal_ConstructSingleChar(c); }

		constexpr string(const char* str) { Internal_ConstructConstChar(str); }

		constexpr string(const string & other) { Internal_ConstructCopy(other); }

		constexpr string(string && other) noexcept { Internal_ConstructMove(std::move(other)); }

		constexpr string(const char* _begin, const char* _end) { Internal_ConstructRange(_begin, _end); }

		constexpr ~string() {
			if (isLong) Internal_DeleteLongData();
		}

		/* Comparison against a single character. */
		[[nodiscard]] constexpr bool operator == (char c) {
			if (length != 1) return false;
			return rep.sso.chars[0] == c;
		}

		/* Comparison against a character array. */
		[[nodiscard]] constexpr bool operator == (const char* str) {
			if (length != Strlen(str)) {
				return false;
			}
			return StrEqual(CStr(), str, length);
		}

		/* Comparison against another string. */
		[[nodiscard]] constexpr bool operator == (const string & other) const {
			if (length != other.length) {
				return false;
			}
			return StrEqual(CStr(), other.CStr(), length);
		}

		/* Sets this string equal to a single character. Overwrites / deletes pre-existing string data. */
		constexpr string& operator = (char c) {
			if (isLong) {
				Internal_DeleteLongData();
			}
			rep.sso = representation::ssoString();
			Internal_ConstructSingleChar(c);
			return *this;
		}

		/* Sets this string equal to a character array. Overwrites / deletes pre-existing string data. */
		constexpr string& operator = (const char* str) {
			if (isLong) {
				Internal_DeleteLongData();
			}
			rep.sso = representation::ssoString(); 
			Internal_ConstructConstChar(str);
			return *this;
		}

		/* Sets this string equal to another string by copy. Overwrites / deletes pre-existing string data. */
		constexpr string& operator = (const string & other) {
			if (isLong) {
				Internal_DeleteLongData();
			}
			rep.sso = representation::ssoString();
			Internal_ConstructCopy(other);
			return *this;
		}

		/* Sets this string equal to another string by move. Overwrites / deletes pre-existing string data, and potentially makes the other string unusable (swap data pointer). */
		constexpr string& operator = (string&& other) noexcept {
			if (isLong) {
				Internal_DeleteLongData();
			}
			rep.sso = representation::ssoString();
			Internal_ConstructMove(std::move(other));
			return *this;
		}

		/* Gets this string as a c-style const char* */
		[[nodiscard]] constexpr const char* CStr() const {
			return isLong ? rep.longStr.data : rep.sso.chars;
		}

		/* std::cout << string */
		friend std::ostream& operator << (std::ostream& os, const string& _string) {
			return os << _string.CStr();
		}

		/* Computes the hash for this string. */
		[[nodiscard]] constexpr size_t ComputeHash() const {
			// http://www.cse.yorku.ca/~oz/hash.html
			const char* str = CStr();
			size_t h = 5381;
			int c;
			while ((c = *str++))
				h = ((h << 5) + h) + c;
			return h;
		}

		/* Append a single character to this string. Potentially reallocates the string into the long string representation. */
		constexpr string& Append(char c) {
			const size_t minCapacity = length + 2;
			if (!isLong) {
				if ((length + 1) <= SSO_STRLEN) {
					rep.sso.chars[length] = c;
					rep.sso.chars[length + 1] = '\0';
					length++;
					return *this;
				}
				Internal_SSOToLongData(minCapacity);
			}
			rep.longStr.data[length] = c;
			rep.longStr.data[length + 1] = '\0';
			return *this;
		}

		/* Append a character array to this string. Potentially reallocates the string into the long string representation. */
		constexpr string& Append(const char* str) {
			const size_t len = Strlen(str);
			const size_t minCapacity = length + len + 1;
			if (!isLong) {
				if ((minCapacity - 1) <= SSO_STRLEN) {
					Internal_CharCopy(&rep.sso.chars[length], str, len + 1);
					length += len;
					return *this;
				}
				Internal_SSOToLongData(minCapacity);
			}
			if (rep.longStr.capacity < minCapacity) {
				Internal_ReallocateLongString(minCapacity);
			}
			Internal_CharCopy(&rep.longStr.data[length], str, len);
			length += len;
			return *this;
		}

		/* Append another string to this string, without modifying the other string. Potentially reallocates the string into the long string representation.  */
		constexpr string& Append(const string& other) {
			const size_t len = other.Len();
			const size_t minCapacity = length + len + 1;
			const char* str = other.CStr();
			if (!isLong) {
				if ((minCapacity - 1) <= SSO_STRLEN) {
					Internal_CharCopy(&rep.sso.chars[length], str, len + 1);
					length += len;
					return *this;
				}
				Internal_SSOToLongData(minCapacity);
			}
			if (rep.longStr.capacity < minCapacity) {
				Internal_ReallocateLongString(minCapacity);
			}
			Internal_CharCopy(&rep.longStr.data[length], str, len);
			length += len;
			return *this;
		}

		/* Append a single character to this string. Potentially reallocates the string into the long string representation. */
		constexpr string& operator += (char c) { return Append(c); }

		/* Append a character array to this string. Potentially reallocates the string into the long string representation. */
		constexpr string& operator += (const char* str) { return Append(str); }

		/* Append another string to this string, without modifying the other string. Potentially reallocates the string into the long string representation.  */
		constexpr string& operator += (const string& other) { return Append(other); }

		/* Create a new string with a character appended to this one. Does not modify this string. */
		[[nodiscard]] constexpr string Concatenate(char c) const {
			string str = *this;
			str.Append(c);
			return str;
		}

		/* Create a new string with a character array appended to this one. Does not modify this string. */
		[[nodiscard]] constexpr string Concatenate(const char* str) const {
			string _str = *this;
			_str.Append(str);
			return _str;
		}

		/* Create a new string with another string appended to this one. Does not modify this string. */
		[[nodiscard]] constexpr string Concatenate(const string& other) const {
			string str = *this;
			str.Append(other);
			return str;
		}

		/* Create a new string with a character appended to this one. Does not modify this string. */
		[[nodiscard]] constexpr string operator + (char c) const { return Concatenate(c); }

		/* Create a new string with a character array appended to this one. Does not modify this string. */
		[[nodiscard]] constexpr string operator + (const char* str) const { return Concatenate(str); }

		/* Create a new string with a character appended to this one. Does not modify this string. */
		[[nodiscard]] constexpr string operator + (const string& other) const { return Concatenate(other); }

		/* Get character in the string by reference. */
		[[nodiscard]] constexpr char& At(size_t index) {
			if (index >= length) {
				throw std::out_of_range("string character index is out of bounds!");
			}
			char* cstr = isLong ? rep.longStr.data : rep.sso.chars;
			return cstr[index];
		}

		/* Get character in the string by const reference. */
		[[nodiscard]] constexpr const char& At(size_t index) const {
			if (index >= length) {
				throw std::out_of_range("string character index is out of bounds!");
			}
			return CStr()[index];
		}

		/* Get character in the string by reference. */
		[[nodiscard]] constexpr char& operator [] (size_t index) { return At(index); }

		/* Get character in the string by const reference. */
		[[nodiscard]] constexpr const char& operator [] (size_t index) const { return At(index); }

		/* Get the first character in the string by reference. */
		[[nodiscard]] constexpr char& Front() {
			char* cstr = isLong ? rep.longStr.data : rep.sso.chars;
			return cstr[0];
		}

		/* Get the first character in the string by const reference. */
		[[nodiscard]] constexpr const char& Front() const {
			return CStr()[0];
		}

		/* Get the last character in the string by reference. */
		[[nodiscard]] constexpr char& Back() {
			char* cstr = isLong ? rep.longStr.data : rep.sso.chars;
			return cstr[length - 1];
		}

		/* Get the last character in the string by const reference. */
		[[nodiscard]] constexpr const char& Back() const {
			return CStr()[length - 1];
		}

		/* Get a substring of this string.
		@param startInclusive. The start index, included in the string. 
		@param endExclusive: The end index, which is not included. */
		[[nodiscard]] constexpr string Substring(size_t startInclusive, size_t endExclusive) const {
			const char* cstr = CStr();
			return string(&cstr[startInclusive], &cstr[endExclusive - 1]);
		}

		/* Creates a string from a boolean. If (bool)true, the string is "true", and if (bool)false, the string is "false". */
		[[nodiscard]] constexpr static string FromBool(bool b) {
			string str;
			if (b) {
				Internal_CharCopy(str.rep.sso.chars, "true", 4);
				str.length = 4;
				return str;
			}
			else {
				Internal_CharCopy(str.rep.sso.chars, "false", 5);
				str.length = 5;
				return str;
			}
		}

		/* Converts this strings contents to a boolean value. Only "true" will yield (bool)true, all other values will yield (bool)false. */
		[[nodiscard]] constexpr bool ToBool() const {
			return *this == "true";
		}

		/* Creates a string from a signed integer, including negative numbers which will have a '-' in front of the text.
		Due to the size of the SSO_STRLEN buffer, the entire number can fit within it. */
		[[nodiscard]] constexpr static string FromInt(long long num) {
			constexpr const char* digits	= "9876543210123456789";
			constexpr size_t zeroDigit		= 9;
			constexpr size_t maxChars			= 21;
			const bool isNegative					= num < 0;

			char tempNums[maxChars];
			size_t tempAt = maxChars;

			string str;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
				num /= 10LL;
			}
			if (isNegative) {
				tempNums[--tempAt] = '-';
			}

			const char* start = tempNums + tempAt;
			const char* end = &tempNums[maxChars];
			std::copy(start, end, str.rep.sso.chars); // It is guaranteed to fit within the SSO_STRLEN buffer.
			str.length = end - start;
			return str;
		}

		/* Creates a string from an unsigned integer. Due to the size of the SSO_STRLEN buffer, the entire number can fit within it. */
		[[nodiscard]] constexpr static string FromUInt(size_t num) {
			constexpr const char* digits = "9876543210123456789";
			constexpr size_t zeroDigit = 9;
			constexpr size_t maxChars = 21;

			char tempNums[maxChars];
			size_t tempAt = maxChars;

			string str;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10ULL)];
				num /= 10ULL;
			}

			const char* start = tempNums + tempAt;
			const char* end = &tempNums[maxChars];
			std::copy(start, end, str.rep.sso.chars); // It is guaranteed to fit within the SSO_STRLEN buffer.
			str.length = end - start;
			return str;
		}

		/* 
			std::string support 
		*/
#if STD_STRING_SUPPORT == true

		/**/
		[[nodiscard]] constexpr const char* c_str() const { return CStr(); }

#endif

	private:

		/* numToCopy is strlen. Does not include null terminator. */
		constexpr static void Internal_CharCopy(char* destination, const char* source, size_t numToCopy) {
			std::copy(source, &source[numToCopy + 1], destination);
		}

		constexpr void Internal_ReallocateLongString(size_t newCapacity) {
			if (newCapacity <= rep.longStr.capacity) return;

			if (rep.longStr.data == nullptr) {
				rep.longStr.data = new char[newCapacity];
				rep.longStr.capacity = newCapacity;
				return;
			}

			char* newData = new char[newCapacity];
			Internal_CharCopy(newData, rep.longStr.data, length);
			delete[] rep.longStr.data;
			rep.longStr.data = newData;
		}

		constexpr void Internal_ConstructSingleChar(char c) {
			length = 1;
			isLong = false;
			rep.sso.chars[0] = c;
			rep.sso.chars[1] = '\0';
		}

		constexpr void Internal_ConstructConstChar(const char* str) {
			length = gk::Strlen(str);
			isLong = length > SSO_STRLEN;
			if (isLong) {
				rep.longStr = representation::longString();
				Internal_ReallocateLongString(length + 1);
				Internal_CharCopy(rep.longStr.data, str, length);
			}
			else {
				Internal_CharCopy(rep.sso.chars, str, length);
			}
		}

		constexpr void Internal_ConstructCopy(const string& other) {
			length = other.length;
			isLong = other.isLong;
			const char* cstr = other.CStr();
			if (isLong) {
				rep.longStr = representation::longString();
				Internal_ReallocateLongString(length + 1);
				Internal_CharCopy(rep.longStr.data, cstr, length);
			}
			else {
				Internal_CharCopy(rep.sso.chars, cstr, length);
			}
		}

		constexpr void Internal_ConstructMove(string&& other) noexcept {
			length = other.length;
			isLong = other.isLong;
			if (isLong) {
				rep.longStr = representation::longString();
				rep.longStr.data = other.rep.longStr.data;
				other.rep.longStr.data = nullptr;
			}
			else {
				Internal_CharCopy(rep.sso.chars, other.CStr(), length);
			}
		}

		constexpr void Internal_ConstructRange(const char* _begin, const char* _end) {
			const bool notNullTerminated = *_end != '\0';
			length = (_end -_begin);
			isLong = length > SSO_STRLEN ? true : false;
			if (isLong) {
				rep.longStr = representation::longString();
				Internal_ReallocateLongString(length + 1);
				Internal_CharCopy(rep.longStr.data, _begin, length - 1);
				rep.longStr.data[length] = '\0';
			}
			else {
				Internal_CharCopy(rep.sso.chars, _begin, length);
			}
		}

		constexpr void Internal_DeleteLongData() {
			if (rep.longStr.data) {
				delete[] rep.longStr.data;
			}
		}

		constexpr static size_t Internal_GetMinCapacityForIncrease(size_t currentCapacity) {
			return 2 * currentCapacity;
		}

		/* @param minCapacity: includes null terminator */
		constexpr void Internal_SSOToLongData(size_t minCapacity) {
			constexpr size_t increaseCap = Internal_GetMinCapacityForIncrease(SSO_STRLEN);
			const size_t newCapacity = (increaseCap > minCapacity ? increaseCap : minCapacity);
			char* newData = new char[newCapacity];
			Internal_CharCopy(newData, rep.sso.chars, SSO_STRLEN);
			rep.longStr = representation::longString();
			rep.longStr.data = newData;
			rep.longStr.capacity = newCapacity;
			isLong = true;
		}

	};

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
//#undef SSO_STRLEN
#undef SSO_SIZE
#undef STD_STRING_SUPPORT
