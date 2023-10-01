#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include <Windows.h>
#include "../Error/Result.h"

// https://arxiv.org/pdf/2010.03090.pdf
// https://en.wikipedia.org/wiki/UTF-8

namespace gk
{
	struct Utf8Metadata {
		uint64 length;
		uint64 totalBytes;

		constexpr Utf8Metadata() : length(0), totalBytes(0) {}
		constexpr Utf8Metadata(uint64 inLength, uint64 inTotalBytes) : length(inLength), totalBytes(inTotalBytes) {}
	};

	class InvalidUtf8Error : public Error
	{
	public:

		InvalidUtf8Error(const String& errorCause) : Error(errorCause, nullptr) {}

		virtual const char* errorName() const {
			return "Invalid Utf8";
		}

		virtual const char* description() const {
			return "The parsed string is not valid utf8";
		}
	};

	template<>
	struct Result<Utf8Metadata>
	{
		Result() = delete;

		constexpr Result(const ResultOk<Utf8Metadata>& _ok) {
			_error = nullptr;
			_value = _ok._value;
		}

		Result(const ResultErr<InvalidUtf8Error>& _err) {
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_error = _err._value;
			_value.length = 0;
			_value.totalBytes = 0;
		}

		constexpr Result(Result<Utf8Metadata>&& other) noexcept {
			_error = other._error;
			_value = other._value;
			other._error = nullptr; 
#if GK_CHECK
			other._value.length = 0;
			other._value.totalBytes = 0;
#endif
		}

		constexpr ~Result() {
			if (isError()) { delete _error; }
		}

		constexpr Result& operator = (Result<Utf8Metadata>&& other) noexcept {
			_error = other._error;
			_value = other._value;
			other._error = nullptr;
#if GK_CHECK
			other._value.length = 0;
			other._value.totalBytes = 0;
#endif
			return *this;
		}

		constexpr [[nodiscard]] bool isError() const {
			return _error != nullptr;
		}

		constexpr [[nodiscard]] Utf8Metadata ok() {
			gk_assertm(!isError(), "Cannot get the value result that is an error\n" << _error->toString());
			return _value;
		}

		constexpr [[nodiscard]] const Utf8Metadata ok() const {
			gk_assertm(!isError(), "Cannot get the const value result that is an error\n" << _error->toString());
			return _value;
		}

		constexpr [[nodiscard]] InvalidUtf8Error* error() {
			gk_assertm(isError(), "Cannot get the error result that is a valid value");
			return _error;
		}

		constexpr [[nodiscard]] const InvalidUtf8Error* error() const {
			gk_assertm(isError(), "Cannot get the const error result that is a valid value");
			return _error;
		}

		constexpr [[nodiscard]] InvalidUtf8Error* errorMove() {
			gk_assertm(isError(), "Cannot move the error result that is a valid value");
			InvalidUtf8Error* ptr = _error;
			_error = nullptr;
			return ptr;
		}

	private:

		Utf8Metadata _value;
		InvalidUtf8Error* _error;
	};

	namespace utf8
	{
		constexpr gk::Result<Utf8Metadata> strlen(const char* str) {
			constexpr char asciiBitmask = (char)0b10000000;

			constexpr char utf8_TrailingBytesNotUsedBits = (char)0b00111111;
			constexpr char utf8_TrailingBytesCodePoint = (char)0b10000000;

			constexpr char utf8_2ByteCodePoint = (char)0b11000000;
			constexpr char utf8_2ByteBitmask = (char)0b11100000;

			constexpr char utf8_3ByteCodePoint = (char)0b11100000;
			constexpr char utf8_3ByteBitmask = (char)0b11110000;

			constexpr char utf8_4ByteCodePoint = (char)0b11110000;
			constexpr char utf8_4ByteBitmask = (char)0b11111000;

			uint64 index = 0;
			uint64 length = 0;

			if (std::is_constant_evaluated()) {

				while (str[index] != '\0') {
					const char leadingChar = str[index];
					if ((leadingChar & 0b10000000) == 0) { // ascii character, 1 byte
						index++;
					}
					else if ((leadingChar & utf8_2ByteBitmask) == utf8_2ByteCodePoint) {
						gk_assertm(str[index + 1] & utf8_TrailingBytesCodePoint, "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 1);
						index += 2;
					}
					else if ((leadingChar & utf8_3ByteBitmask) == utf8_3ByteCodePoint) {
						gk_assertm(str[index + 1] & utf8_TrailingBytesCodePoint, "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 1);
						gk_assertm(str[index + 2] & utf8_TrailingBytesCodePoint, "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 2);
						index += 3;
					}
					else if ((leadingChar & utf8_4ByteBitmask) == utf8_4ByteCodePoint) {
						gk_assertm(str[index + 1] & utf8_TrailingBytesCodePoint, "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 1);
						gk_assertm(str[index + 2] & utf8_TrailingBytesCodePoint, "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 2);
						gk_assertm(str[index + 3] & utf8_TrailingBytesCodePoint, "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 3);
						index += 4;
					}
					else {
						gk_assertm(false, "Invalid utf8 string. Does not have leading bits specifying one-four byte code points at index " << index);
					}

					length++;
				}

				Utf8Metadata metadata;
				metadata.length = length;
				metadata.totalBytes = index + 1;
				return ResultOk<Utf8Metadata>(metadata);
			}
			else {
				while (str[index] != '\0') {
					const char leadingChar = str[index];
					if ((leadingChar & 0b10000000) == 0) { // ascii character, 1 byte
						index++;
					}
					else if ((leadingChar & utf8_2ByteBitmask) == utf8_2ByteCodePoint) {
						if (!(str[index] + 1 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(new InvalidUtf8Error("Trailing byte of 2 byte utf8 character is not 0b10xxxxxx at index " + String::FromUint(index + 1))); }
						index += 2;
					}
					else if ((leadingChar & utf8_3ByteBitmask) == utf8_3ByteCodePoint) {
						if (!(str[index] + 1 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(new InvalidUtf8Error("First trailing byte of 3 byte utf8 character is not 0b10xxxxxx at index " + String::FromUint(index + 1))); }
						if (!(str[index] + 2 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(new InvalidUtf8Error("Second trailing byte of 3 byte utf8 character is not 0b10xxxxxx at index " + String::FromUint(index + 2))); }
						index += 3;
					}
					else if ((leadingChar & utf8_4ByteBitmask) == utf8_4ByteCodePoint) {
						if (!(str[index] + 1 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(new InvalidUtf8Error("First trailing byte of 4 byte utf8 character is not 0b10xxxxxx at index " + String::FromUint(index + 1))); }
						if (!(str[index] + 2 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(new InvalidUtf8Error("Second trailing byte of 4 byte utf8 character is not 0b10xxxxxx at index " + String::FromUint(index + 2))); }
						if (!(str[index] + 3 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(new InvalidUtf8Error("Third trailing byte of 5 byte utf8 character is not 0b10xxxxxx at index " + String::FromUint(index + 3))); }
						index += 4;
					}
					else [[unlikely]] {
						return ResultErr(new InvalidUtf8Error("Does not have leading bits specifying one-four byte code points at index " + String::FromUint(index)));
					}

					length++;
				}

				Utf8Metadata metadata;
				metadata.length = length;
				metadata.totalBytes = index + 1;
				return ResultOk<Utf8Metadata>(metadata);
			}

			
		}
	}
}