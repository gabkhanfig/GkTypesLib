#pragma once

#include "../basic_types.h"
#include "../doctest/doctest_proxy.h"
#include "../error/result.h"

// https://arxiv.org/pdf/2010.03090.pdf
// https://en.wikipedia.org/wiki/UTF-8

namespace gk
{
	struct Utf8Metadata {
		/* Length of the string as utf8. */
		usize length;
		/* Total bytes used by the string including null terminator. */
		usize totalBytes;

		constexpr Utf8Metadata() : length(0), totalBytes(0) {}
		constexpr Utf8Metadata(usize inLength, usize inTotalBytes) : length(inLength), totalBytes(inTotalBytes) {}
	};

	namespace utf8
	{
		/* Will fail to compile if error in constexpr. */
		constexpr gk::Result<Utf8Metadata, void> strlen(const char* str) {
			constexpr char asciiBitmask = (char)0b10000000;

			constexpr char utf8_TrailingBytesNotUsedBits = (char)0b00111111;
			constexpr char utf8_TrailingBytesCodePoint = (char)0b10000000;

			constexpr char utf8_2ByteCodePoint = (char)0b11000000;
			constexpr char utf8_2ByteBitmask = (char)0b11100000;

			constexpr char utf8_3ByteCodePoint = (char)0b11100000;
			constexpr char utf8_3ByteBitmask = (char)0b11110000;

			constexpr char utf8_4ByteCodePoint = (char)0b11110000;
			constexpr char utf8_4ByteBitmask = (char)0b11111000;

			usize index = 0;
			usize length = 0;

			if (std::is_constant_evaluated()) {

				while (str[index] != '\0') {
					const char leadingChar = str[index];
					if ((leadingChar & 0b10000000) == 0) { // ascii character, 1 byte
						index++;
					}
					else if ((leadingChar & utf8_2ByteBitmask) == utf8_2ByteCodePoint) {
						if (!(str[index + 1] & utf8_TrailingBytesCodePoint)) {
							throw;
						}
						//check_message((str[index + 1] & utf8_TrailingBytesCodePoint), "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 1);
						index += 2;
					}
					else if ((leadingChar & utf8_3ByteBitmask) == utf8_3ByteCodePoint) {
						if (!(str[index + 1] & utf8_TrailingBytesCodePoint)) {
							throw;
						}
						if (!(str[index + 2] & utf8_TrailingBytesCodePoint)) {
							throw;
						}
						//check_message((str[index + 1] & utf8_TrailingBytesCodePoint), "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 1);
						//check_message((str[index + 2] & utf8_TrailingBytesCodePoint), "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 2);
						index += 3;
					}
					else if ((leadingChar & utf8_4ByteBitmask) == utf8_4ByteCodePoint) {
						if (!(str[index + 1] & utf8_TrailingBytesCodePoint)) {
							throw;
						}
						if (!(str[index + 2] & utf8_TrailingBytesCodePoint)) {
							throw;
						}
						if (!(str[index + 3] & utf8_TrailingBytesCodePoint)) {
							throw;
						}
						//check_message((str[index + 1] & utf8_TrailingBytesCodePoint), "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 1);
						//check_message((str[index + 2] & utf8_TrailingBytesCodePoint), "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 2);
						//check_message((str[index + 3] & utf8_TrailingBytesCodePoint), "Invalid utf8 string. Trailing bytes are not 0b10xxxxxx at index " << index + 3);
						index += 4;
					}
					else {
						throw;
						//check_message(false, "Invalid utf8 string. Does not have leading bits specifying one-four byte code points at index " << index);
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
						if (!(str[index] + 1 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(); }
						index += 2;
					}
					else if ((leadingChar & utf8_3ByteBitmask) == utf8_3ByteCodePoint) {
						if (!(str[index] + 1 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(); }
						if (!(str[index] + 2 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(); }
						index += 3;
					}
					else if ((leadingChar & utf8_4ByteBitmask) == utf8_4ByteCodePoint) {
						if (!(str[index] + 1 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(); }
						if (!(str[index] + 2 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(); }
						if (!(str[index] + 3 & utf8_TrailingBytesCodePoint)) [[unlikely]] { return ResultErr(); }
						index += 4;
					}
					else [[unlikely]] {
						return ResultErr();
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



	/**
	* @param str: Beginning of the utf8 buffer range
	* @param length: The amount of bytes long the utf8 range is
	*/
	constexpr bool isValidUtf8(const char* str, usize length) {
		constexpr char asciiBitmask = (char)0b10000000;

		constexpr char utf8_TrailingBytesNotUsedBits = (char)0b00111111;
		constexpr char utf8_TrailingBytesCodePoint = (char)0b10000000;

		constexpr char utf8_2ByteCodePoint = (char)0b11000000;
		constexpr char utf8_2ByteBitmask = (char)0b11100000;

		constexpr char utf8_3ByteCodePoint = (char)0b11100000;
		constexpr char utf8_3ByteBitmask = (char)0b11110000;

		constexpr char utf8_4ByteCodePoint = (char)0b11110000;
		constexpr char utf8_4ByteBitmask = (char)0b11111000;

		usize i = 0;
		while (i < length) {
			char c = str[i];
			if ((c & asciiBitmask) == 0) { // ascii character, 1 byte
				i += 1;
			}
			else if ((c & utf8_2ByteBitmask) == utf8_2ByteCodePoint) { // 2 byte utf8
				if (!(str[i + 1] & utf8_TrailingBytesCodePoint)) {
					return false;
				}
				i += 2;
			}
			else if ((c & utf8_3ByteBitmask) == utf8_3ByteCodePoint) { // 3 byte utf8
				if (!(str[i + 1] & utf8_TrailingBytesCodePoint)) {
					return false;
				}
				if (!(str[i + 2] & utf8_TrailingBytesCodePoint)) {
					return false;
				}
				i += 3;
			}
			else if ((c & utf8_4ByteBitmask) == utf8_4ByteCodePoint) { // 4 bye utf8
				if (!(str[i + 1] & utf8_TrailingBytesCodePoint)) {
					return false;
				}
				if (!(str[i + 2] & utf8_TrailingBytesCodePoint)) {
					return false;
				}
				if (!(str[i + 3] & utf8_TrailingBytesCodePoint)) {
					return false;
				}
				i += 4;
			}
			else if (c == '\0') { // null terminator within buffer
				return false;
			}
			else { // not 1-4 bytes
				return false;
			}

		}
		return true;
	}

}