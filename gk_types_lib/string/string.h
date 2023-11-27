#pragma once

#include "str.h"
#include "../option/option.h"
#include "../hash/hash.h"
#include "../allocator/heap_allocator.h"

namespace gk
{
	struct ALIGN_AS(8) String
	{
	private:

#pragma pack(push, 1)
		struct HeapRep
		{
			// length as utf8 characters
			u64 _length;
			// 64 byte aligned buffer
			char* _buffer;
			// does not include null terminator
			u64 _bytesOfBufferUsed;
			// total byte capacity of the string, used along with the _flag byte to make a u64.
			char _totalByteCapacityBitmask[7];

			constexpr HeapRep() : _length(0), _buffer(nullptr), _bytesOfBufferUsed(0), _totalByteCapacityBitmask{ 0 } {}
		};
#pragma pack(pop)

		struct SsoRep
		{
			char _chars[31];

			constexpr SsoRep() : _chars{ 0 } {}
		};

		union StringRep
		{
			SsoRep _sso;
			HeapRep _heap;

			constexpr StringRep() : _sso(SsoRep()) {}
		};

		static constexpr char FLAG_BIT = static_cast<char>(0b10000000); 
		static constexpr u64 MAX_SSO_UTF8_BYTES = 31;
		static constexpr u64 MAX_SSO_LEN = 30;
		static constexpr u64 HEAP_CAPACITY_BITMASK = ~(1ULL << 63);

		// union representation
		StringRep _rep;
		// is also sso length
		char _flag;

	public:

#pragma region Construct_Destruct

		constexpr String() : _rep(StringRep()), _flag(0) {
			setSsoUsedBytes(0);
		}

		constexpr String(const char c) : _rep(StringRep()), _flag(1) {
			_rep._sso._chars[0] = c;
			setSsoUsedBytes(1);		
		}

		constexpr String(const Str& str) {
			const u64 stringBytes = str.totalBytes - 1;
			if (str.totalBytes <= MAX_SSO_UTF8_BYTES) {
				if (std::is_constant_evaluated()) {
					for (u64 i = 0; i < stringBytes; i++) {
						_rep._sso._chars[i] = str.str[i];
					}
				}
				else {
					memcpy(_rep._sso._chars, str.str, stringBytes);
				}
				setSsoUsedBytes(stringBytes);
				setSsoLen(str.len);
				return;
			}
			// heap construction
			_rep._heap = HeapRep();

			u64 capacity = str.totalBytes;
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (u64 i = 0; i < stringBytes; i++) {
					buffer[i] = str.str[i];
				}
			}
			else {
				memcpy(buffer, str.str, stringBytes);
			}

			_rep._heap._length = str.len;
			_rep._heap._buffer = buffer;
			_rep._heap._bytesOfBufferUsed = stringBytes;
			setHeapCapacity(capacity);
		}

		constexpr String(const String& other) {
			if (other.isSso()) {
				if (std::is_constant_evaluated()) {
					for (int i = 0; i < 31; i++) {
						_rep._sso._chars[i] = other._rep._sso._chars[i];
					}
					_flag = other._flag;
					return;
				}
				memcpy(this, &other, sizeof(String));
				return;
			}

			_rep._heap = HeapRep();
			u64 capacity = other.heapCapacity(); 
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (u64 i = 0; i < other._rep._heap._bytesOfBufferUsed; i++) {
					buffer[i] = other._rep._heap._buffer[i];
				}
			}
			else {
				memcpy(buffer, other._rep._heap._buffer, other._rep._heap._bytesOfBufferUsed);
			}

			_rep._heap._length = other._rep._heap._length;
			_rep._heap._buffer = buffer;
			_rep._heap._bytesOfBufferUsed = other._rep._heap._bytesOfBufferUsed;
			setHeapCapacity(capacity);
		}

		constexpr String(String&& other) noexcept {
			if (other.isSso()) {
				if (std::is_constant_evaluated()) {
					for (int i = 0; i < 31; i++) {
						_rep._sso._chars[i] = other._rep._sso._chars[i];
					}
					_flag = other._flag;
				}
				else{
					memcpy(this, &other, sizeof(String));
				}
#if GK_CHECK // Ensure not use after move
				for (int i = 0; i < 31; i++) {
					other._rep._sso._chars[i] = '\0';
				}
				other._flag = 0;
#endif
				return;
			}

			_rep._heap = HeapRep();
			_rep._heap._length = other._rep._heap._length;
			_rep._heap._buffer = other._rep._heap._buffer;
			_rep._heap._bytesOfBufferUsed = other._rep._heap._bytesOfBufferUsed;
			setHeapCapacity(other.heapCapacity());

			other._rep._heap._buffer = nullptr;

#if GK_CHECK // Ensure not use after move
			other._rep._heap._length = 0;
			other._rep._heap._bytesOfBufferUsed = 0;
			other.setHeapCapacity(0);
			other._flag = 0;
#endif
		}

		constexpr ~String() {
			freeHeapBufferIfNotSso();
		}

#pragma endregion

#pragma region Assign

		constexpr String& operator = (const char c) {
			freeHeapBufferIfNotSso();
			_rep._sso = SsoRep();
			_rep._sso._chars[0] = c;
			_flag = 1;
			setSsoUsedBytes(1);
			return *this;
		}

		constexpr String& operator = (const Str& str) {
			const u64 stringBytes = str.totalBytes - 1;
			if (str.totalBytes <= MAX_SSO_UTF8_BYTES) { // should be sso.
				freeHeapBufferIfNotSso();
				_rep._sso = SsoRep();
				if (std::is_constant_evaluated()) {
					for (u64 i = 0; i < stringBytes; i++) {
						_rep._sso._chars[i] = str.str[i];
					}
				}
				else {
					memcpy(_rep._sso._chars, str.str, stringBytes);
				}
				setSsoUsedBytes(stringBytes);
				setSsoLen(str.len);
				return *this;
			}

			const u64 heapCap = heapCapacity();
			if (heapCap > stringBytes) { // reuse existing allocation
				if (std::is_constant_evaluated()) {
					u64 i = 0;
					for (; i < stringBytes; i++) {
						_rep._heap._buffer[i] = str.str[i];
					}
					for (; i < heapCap; i++) {
						_rep._heap._buffer[i] = '\0';
					}
				}
				else {
					memcpy(_rep._heap._buffer, str.str, stringBytes);
					memset(_rep._heap._buffer + stringBytes, '\0', heapCapacity() - stringBytes);
				}

				_rep._heap._length = str.len;
				_rep._heap._bytesOfBufferUsed = stringBytes;
				return *this;
			}

			// heap construction
			freeHeapBufferIfNotSso();
			_rep._heap = HeapRep();

			u64 capacity = str.totalBytes;
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (u64 i = 0; i < stringBytes; i++) {
					buffer[i] = str.str[i];
				}
			}
			else {
				memcpy(buffer, str.str, stringBytes);
			}

			_rep._heap._length = str.len;
			_rep._heap._buffer = buffer;
			_rep._heap._bytesOfBufferUsed = str.totalBytes;
			setHeapCapacity(capacity);
			return *this;
		}

		constexpr String& operator = (const String& other) {
			const u64 stringBytes = other.usedBytes();
			if (other.isSso()) { // should be sso.
				freeHeapBufferIfNotSso();
				_rep._sso = SsoRep();
				if (std::is_constant_evaluated()) {
					for (u64 i = 0; i < stringBytes; i++) {
						_rep._sso._chars[i] = other._rep._sso._chars[i];
					}
				}
				else {
					memcpy(_rep._sso._chars, other._rep._sso._chars, stringBytes);
				}
				setSsoUsedBytes(stringBytes);
				setSsoLen(other.ssoLen());
				return *this;
			}

			if (!isSso()) {
				if (heapCapacity() > stringBytes) { // reuse existing allocation
					if (std::is_constant_evaluated()) {
						u64 i = 0;
						for (; i < stringBytes; i++) {
							_rep._heap._buffer[i] = other._rep._heap._buffer[i];
						}
						for (; i < heapCapacity(); i++) {
							_rep._heap._buffer[i] = '\0';
						}
					}
					else {
						memcpy(_rep._heap._buffer, other._rep._heap._buffer, stringBytes);
						memset(_rep._heap._buffer + stringBytes, '\0', heapCapacity() - stringBytes);
					}

					_rep._heap._length = other._rep._heap._length;
					_rep._heap._bytesOfBufferUsed = stringBytes;
					return *this;
				}
			}

			// heap construction
			freeHeapBufferIfNotSso();
			_rep._heap = HeapRep();

			u64 capacity = stringBytes + 1;
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (u64 i = 0; i < stringBytes; i++) {
					buffer[i] = other._rep._heap._buffer[i];
				}
			}
			else {
				memcpy(buffer, other._rep._heap._buffer, stringBytes);
			}

			_rep._heap._length = other._rep._heap._length;
			_rep._heap._buffer = buffer;
			_rep._heap._bytesOfBufferUsed = other._rep._heap._bytesOfBufferUsed;
			setHeapCapacity(capacity);
			return *this;
		}

		constexpr String& operator = (String&& other) noexcept {
			freeHeapBufferIfNotSso(); 
			if (other.isSso()) {
				if (std::is_constant_evaluated()) {
					for (int i = 0; i < 31; i++) {
						_rep._sso._chars[i] = other._rep._sso._chars[i];
					}
					_flag = other._flag;
				}
				else {
					memcpy(this, &other, sizeof(String));
				}
#if GK_CHECK // Ensure not use after move
				for (int i = 0; i < 31; i++) {
					other._rep._sso._chars[i] = '\0';
				}
				other._flag = 0;
#endif
				return *this;
			}

			_rep._heap = HeapRep();
			_rep._heap._length = other._rep._heap._length;
			_rep._heap._buffer = other._rep._heap._buffer;
			_rep._heap._bytesOfBufferUsed = other._rep._heap._bytesOfBufferUsed;
			setHeapCapacity(other.heapCapacity());

			other._rep._heap._buffer = nullptr;

#if GK_CHECK // Ensure not use after move
			other._rep._heap._length = 0;
			other._rep._heap._bytesOfBufferUsed = 0;
			other.setHeapCapacity(0);
			other._flag = 0;
#endif
			return *this;
		}
 
#pragma endregion

		[[nodiscard]] constexpr forceinline u64 len() const {
			if (std::is_constant_evaluated()) {
				if (isSso()) {
					return ssoLen();
				}
				return heapLen();
			}
			const u64 isSsoRep = isSso();
			const u64 isHeapRep = !isSso();
			return (isSsoRep * ssoLen()) + (isHeapRep * heapLen());
		}

		/* Amount of utf8 bytes used by this string (same as length for ascii only string) */
		[[nodiscard]] constexpr forceinline u64 usedBytes() const {
			if (isSso()) {
				return ssoUsedBytes();
			}
			return _rep._heap._bytesOfBufferUsed;
		}

		[[nodiscard]] constexpr forceinline bool isEmpty() const { return len() == 0; }

		[[nodiscard]] constexpr const char* cstr() const {
			if (isSso()) {
				return _rep._sso._chars;
			}
			return _rep._heap._buffer;
		}

		/* std::cout << string */
		friend std::ostream& operator << (std::ostream& os, const String& _string) {
			return os << _string.cstr();
		}

		/* Murmur hash */
		[[nodiscard]] constexpr size_t hash() const {
			const char* str = cstr();
			const int seed = 0;
			const u64 length = len();
			const u64 m = 0xc6a4a7935bd1e995ULL;
			const int r = 47;

			u64 h = seed ^ (length * m);

			const u64* data = (const u64*)str;
			const u64* end = data + (length / 8);

			while (data != end)
			{
				u64 k = *data++;

				k *= m;
				k ^= k >> r;
				k *= m;

				h ^= k;
				h *= m;
			}

			const unsigned char* data2 = (const unsigned char*)data;

			switch (length & 7)
			{
			case 7: h ^= u64(data2[6]) << 48;
			case 6: h ^= u64(data2[5]) << 40;
			case 5: h ^= u64(data2[4]) << 32;
			case 4: h ^= u64(data2[3]) << 24;
			case 3: h ^= u64(data2[2]) << 16;
			case 2: h ^= u64(data2[1]) << 8;
			case 1: h ^= u64(data2[0]);
				h *= m;
			};

			h ^= h >> r;
			h *= m;
			h ^= h >> r;

			return h;
		}

#pragma region Compare

		[[nodiscard]] constexpr bool operator == (const char c) const {
			if (_flag == 0b00000001) { // Is sso and length is 1
				return (_rep._sso._chars[0] == c);
			}
			return false;
		}

		[[nodiscard]] constexpr bool operator == (const Str& str) const {
			if (std::is_constant_evaluated()) {
				if (len() != str.len) return false;
				const char* thisStr = cstr();
				for (u64 i = 0; i < str.totalBytes; i++) {
					if (thisStr[i] != str.str[i]) return false;
				}
				return true;
			}

			if (isSso()) {
				if (ssoUsedBytes() != str.totalBytes - 1) return false;

				const u64* ssoCharsAsMachineWord = reinterpret_cast<const u64*>(_rep._sso._chars);
				u64 buf[4] = { 0, 0, 0, 0 };
				//gk_assertm(str.totalBytes <= 32, "shut up compiler");
				memcpy(buf, str.str, str.totalBytes - 1);

				((char*)buf)[30] = _rep._sso._chars[30];	// must copy sso capacity byte
				((char*)buf)[31] = _flag;									// must copy last byte for length

				return (ssoCharsAsMachineWord[0] == buf[0])
					&& (ssoCharsAsMachineWord[1] == buf[1])
					&& (ssoCharsAsMachineWord[2] == buf[2])
					&& (ssoCharsAsMachineWord[3] == buf[3]);
			}

			if (_rep._heap._bytesOfBufferUsed != str.totalBytes - 1) return false;

			return simdCompareStringAndStr(*this, str);
		}

		[[nodiscard]] constexpr bool operator == (const String& other) const {
			if (std::is_constant_evaluated()) {
				if (len() != other.len()) return false;
				const char* thisStr = cstr();
				const char* otherStr = other.cstr();
				for (u64 i = 0; i < usedBytes(); i++) {
					if (thisStr[i] != otherStr[i]) return false;
				}
				return true;
			}

			if (isSso()) {
				const u64* thisSso = reinterpret_cast<const u64*>(_rep._sso._chars);
				const u64* otherSso = reinterpret_cast<const u64*>(other._rep._sso._chars);
				return (thisSso[0] == otherSso[0])
					&& (thisSso[1] == otherSso[1])
					&& (thisSso[2] == otherSso[2])
					&& (thisSso[3] == otherSso[3]);
			}

			if (_rep._heap._bytesOfBufferUsed != other._rep._heap._bytesOfBufferUsed) return false;

			return simdCompareStringAndString(*this, other);
		}

#pragma endregion

#pragma region Append

		constexpr String& append(const char c) {
			const u64 length = len();
			const u64 newLength = length + 1;
			const u64 capacityUsedNoTerminator = usedBytes();
			const u64 minCapacity = capacityUsedNoTerminator + 2;

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				check_message(isSso(), "String representation should be sso");
				_rep._sso._chars[capacityUsedNoTerminator] = c;
				setSsoLen(newLength);
				setSsoUsedBytes(capacityUsedNoTerminator + 1);
				check_message(_rep._sso._chars[capacityUsedNoTerminator + 1] == '\0', "String representation should be sso");
				return *this;
			}

			if (isSso()) { // If it is an sso, it must reallocate.
				char temp[30]; // does not include null terminator
				copyChars(temp, _rep._sso._chars, 30);
		
				_rep._heap = HeapRep();
				u64 mallocCapacity = 64;
				char* buffer = mallocHeapBuffer(&mallocCapacity);
				copyChars(buffer, temp, 30);
				buffer[30] = c;
				_rep._heap._buffer = buffer;
				setHeapCapacity(mallocCapacity);
				setHeapLen(newLength);
				setHeapUsedBytes(31);
				return *this;
			}

			if (minCapacity > heapCapacity()) { // must reallocate.
				u64 mallocCapacity = minCapacity;
				char* newBuffer = mallocHeapBuffer(&mallocCapacity);
				copyChars(newBuffer, _rep._heap._buffer, capacityUsedNoTerminator);
				freeHeapBuffer(_rep._heap._buffer);
				_rep._heap._buffer = newBuffer;
				setHeapCapacity(mallocCapacity);
			}

			_rep._heap._buffer[capacityUsedNoTerminator] = c;
			setHeapLen(newLength);
			setHeapUsedBytes(capacityUsedNoTerminator + 1);
			return *this;
		}

		constexpr String& append(const Str& str) {
			const u64 length = len();
			const u64 newLength = length + str.len;
			const u64 capacityUsedNoTerminator = usedBytes();
			const u64 strBytes = str.totalBytes - 1;
			const u64 minCapacity = capacityUsedNoTerminator + str.totalBytes; // includes null terminator

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				check_message(isSso(), "String representation should be sso");
				copyChars(_rep._sso._chars + capacityUsedNoTerminator, str.str, strBytes);
				setSsoLen(newLength);
				setSsoUsedBytes(minCapacity - 1);
				check_message(_rep._sso._chars[capacityUsedNoTerminator + strBytes] == '\0', "Sso chars should already have null terminator");
				return *this;
			}

			if (isSso()) { // If it is an sso, it must reallocate.
				char temp[30]; // does not include null terminator
				copyChars(temp, _rep._sso._chars, 30);

				_rep._heap = HeapRep();
				u64 mallocCapacity = minCapacity + str.totalBytes - 1; // overallocate
				char* buffer = mallocHeapBuffer(&mallocCapacity);

				copyChars(buffer, temp, capacityUsedNoTerminator);
				copyChars(buffer + capacityUsedNoTerminator, str.str, str.totalBytes - 1);

				_rep._heap._buffer = buffer;
				setHeapCapacity(mallocCapacity);
				setHeapLen(newLength);
				setHeapUsedBytes(minCapacity - 1);
				return *this;
			}

			if (minCapacity > heapCapacity()) { // must reallocate.
				u64 mallocCapacity = minCapacity + str.len; // overallocate
				char* newBuffer = mallocHeapBuffer(&mallocCapacity);
				copyChars(newBuffer, _rep._heap._buffer, capacityUsedNoTerminator);
				freeHeapBuffer(_rep._heap._buffer);
				_rep._heap._buffer = newBuffer;
				setHeapCapacity(mallocCapacity);
			}

			copyChars(_rep._heap._buffer + length, str.str, str.len);
			setHeapLen(newLength);
			setHeapUsedBytes(minCapacity - 1);
			return *this;
		}

		constexpr String& append(const String& other) {
			const u64 length = len();
			const u64 otherLen = other.len();
			const u64 newLength = length + otherLen;
			const u64 capacityUsedNoTerminator = usedBytes();
			const u64 stringBytes = other.usedBytes();
			const u64 minCapacity = capacityUsedNoTerminator + stringBytes + 1; // includes null terminator

			const char* otherStr = other.cstr();

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				check_message(isSso(), "String representation should be sso");
				copyChars(_rep._sso._chars + capacityUsedNoTerminator, otherStr, stringBytes);
				setSsoLen(newLength);
				setSsoUsedBytes(minCapacity - 1);
				check_message(_rep._sso._chars[capacityUsedNoTerminator + stringBytes] == '\0', "Sso chars should already have null terminator");
				return *this;
			}

			if (isSso()) { // If it is an sso, it must reallocate.
				char temp[30]; // does not include null terminator
				copyChars(temp, _rep._sso._chars, 30);

				_rep._heap = HeapRep();
				u64 mallocCapacity = minCapacity + stringBytes; // overallocate
				char* buffer = mallocHeapBuffer(&mallocCapacity);

				copyChars(buffer, temp, capacityUsedNoTerminator);
				copyChars(buffer + capacityUsedNoTerminator, otherStr, stringBytes);

				_rep._heap._buffer = buffer;
				setHeapCapacity(mallocCapacity);
				setHeapLen(newLength);
				setHeapUsedBytes(minCapacity - 1);
				return *this;
			}

			if (minCapacity > heapCapacity()) { // must reallocate.
				u64 mallocCapacity = minCapacity + stringBytes; // overallocate
				char* newBuffer = mallocHeapBuffer(&mallocCapacity);
				copyChars(newBuffer, _rep._heap._buffer, capacityUsedNoTerminator);
				freeHeapBuffer(_rep._heap._buffer);
				_rep._heap._buffer = newBuffer;
				setHeapCapacity(mallocCapacity);
			}

			copyChars(_rep._heap._buffer + capacityUsedNoTerminator, otherStr, stringBytes);
			setHeapLen(newLength);
			setHeapUsedBytes(minCapacity - 1);
			return *this;
		}

		constexpr forceinline String& operator += (const char c)						{ return append(c); }
		constexpr forceinline String& operator += (const Str& str)					{ return append(str); }
		constexpr forceinline String& operator += (const String& other) { return append(other); }

#pragma endregion

#pragma region Concat

		[[nodiscard]] constexpr friend String operator + (const String& lhs, const char rhs) {
			const u64 length = lhs.len();
			const u64 newLength = length + 1;
			const u64 capacityUsedNoTerminator = lhs.usedBytes();
			const u64 minCapacity = capacityUsedNoTerminator + 2;

			String newString;

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs._rep._sso._chars, capacityUsedNoTerminator);
				newString._rep._sso._chars[capacityUsedNoTerminator] = rhs;
				newString.setSsoUsedBytes(capacityUsedNoTerminator + 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			u64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
			char* buffer = String::mallocHeapBuffer(&mallocCapacity);
			copyChars(buffer, lhs.cstr(), capacityUsedNoTerminator);
			buffer[capacityUsedNoTerminator] = rhs;

			newString._rep._heap._buffer = buffer;
			newString.setHeapLen(newLength);
			newString.setHeapUsedBytes(capacityUsedNoTerminator + 1);
			newString.setHeapCapacity(mallocCapacity);
			return newString;
		}

		[[nodiscard]] constexpr friend String operator + (String&& lhs, const char rhs) { return lhs.append(rhs); }

		[[nodiscard]] constexpr friend String operator + (const String& lhs, const Str& rhs) {
			const u64 length = lhs.len();
			const u64 newLength = length + rhs.len;
			const u64 capacityUsedNoTerminator = lhs.usedBytes();
			const u64 strBytes = rhs.totalBytes - 1;
			const u64 minCapacity = capacityUsedNoTerminator + rhs.totalBytes; // includes null terminator

			String newString; 
			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs._rep._sso._chars, capacityUsedNoTerminator);
				copyChars(newString._rep._sso._chars + capacityUsedNoTerminator, rhs.str, strBytes);
				newString.setSsoUsedBytes(minCapacity - 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			u64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
			char* buffer = String::mallocHeapBuffer(&mallocCapacity);
			copyChars(buffer, lhs.cstr(), capacityUsedNoTerminator);
			copyChars(buffer + capacityUsedNoTerminator, rhs.str, strBytes);

			newString._rep._heap._buffer = buffer;
			newString.setHeapLen(newLength);
			newString.setHeapUsedBytes(minCapacity - 1);
			newString.setHeapCapacity(mallocCapacity);
			return newString;
		}

		[[nodiscard]] constexpr friend String operator + (String&& lhs, const Str& rhs) { return lhs.append(rhs); }

		[[nodiscard]] constexpr friend String operator + (const String& lhs, const String& rhs) {
			const u64 length = lhs.len();
			const u64 newLength = length + rhs.len();
			const u64 capacityUsedNoTerminator = lhs.usedBytes();
			const u64 stringBytes = rhs.usedBytes();
			const u64 minCapacity = capacityUsedNoTerminator + stringBytes + 1; // includes null terminator

			String newString;
			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs._rep._sso._chars, capacityUsedNoTerminator);
				copyChars(newString._rep._sso._chars + capacityUsedNoTerminator, rhs._rep._sso._chars, stringBytes);
				newString.setSsoUsedBytes(minCapacity - 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			u64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
			char* buffer = String::mallocHeapBuffer(&mallocCapacity);
			copyChars(buffer, lhs.cstr(), capacityUsedNoTerminator);
			copyChars(buffer + capacityUsedNoTerminator, rhs.cstr(), stringBytes);

			newString._rep._heap._buffer = buffer;
			newString.setHeapLen(newLength);
			newString.setHeapUsedBytes(minCapacity - 1);
			newString.setHeapCapacity(mallocCapacity);
			return newString;
		}

		[[nodiscard]] constexpr friend String operator + (String&& lhs, const String& rhs) { return lhs.append(rhs); }

		[[nodiscard]] constexpr friend String operator + (const char lhs, const String& rhs) {
			const u64 length = rhs.len();
			const u64 newLength = length + 1;
			const u64 capacityUsedNoTerminator = rhs.usedBytes();
			const u64 minCapacity = capacityUsedNoTerminator + 2;

			String newString;

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				newString._rep._sso._chars[0] = lhs;
				copyChars(newString._rep._sso._chars + 1, rhs._rep._sso._chars, capacityUsedNoTerminator);
				newString.setSsoUsedBytes(capacityUsedNoTerminator + 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			u64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
			char* buffer = String::mallocHeapBuffer(&mallocCapacity);
			buffer[0] = lhs;
			copyChars(buffer + 1, rhs.cstr(), capacityUsedNoTerminator);

			newString._rep._heap._buffer = buffer;
			newString.setHeapLen(newLength);
			newString.setHeapUsedBytes(capacityUsedNoTerminator + 1);
			newString.setHeapCapacity(mallocCapacity);
			return newString;
		}

		[[nodiscard]] constexpr friend String operator + (const Str& lhs, const String& rhs) {
			const u64 length = rhs.len();
			const u64 newLength = length + lhs.len;
			const u64 capacityUsedNoTerminator = rhs.usedBytes();
			const u64 strBytes = lhs.totalBytes - 1;
			const u64 minCapacity = capacityUsedNoTerminator + lhs.totalBytes; // includes null terminator

			String newString;
			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs.str, strBytes);
				copyChars(newString._rep._sso._chars + strBytes, rhs._rep._sso._chars, capacityUsedNoTerminator);
				newString.setSsoUsedBytes(minCapacity - 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			u64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
			char* buffer = String::mallocHeapBuffer(&mallocCapacity);
			copyChars(buffer, lhs.str, strBytes);
			copyChars(buffer + strBytes, rhs.cstr(), capacityUsedNoTerminator);

			newString._rep._heap._buffer = buffer;
			newString.setHeapLen(newLength);
			newString.setHeapUsedBytes(minCapacity - 1);
			newString.setHeapCapacity(mallocCapacity);
			return newString;
		}

#pragma endregion

#pragma region From_Other_Type

		[[nodiscard]] constexpr static String fromBool(const bool b) {
			String newString;
			if (b) {
				copyChars(newString._rep._sso._chars, "true", 4);
				newString.setSsoLen(4);
				newString.setSsoUsedBytes(4);
				return newString;
			}
			copyChars(newString._rep._sso._chars, "false", 5);
			newString.setSsoLen(5);
			newString.setSsoUsedBytes(5);
			return newString;
		}

		[[nodiscard]] constexpr static String fromInt(i64 num) {
			if (num == 0) return String('0');

			constexpr const char* digits = "9876543210123456789";
			constexpr u64 zeroDigit = 9;
			constexpr u64 maxChars = 20;

			const bool isNegative = num < 0;

			char tempNums[maxChars];
			u64 tempAt = maxChars;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
				num /= 10LL;
			}
			if (isNegative) {
				tempNums[--tempAt] = '-';
			}

			const u64 length = maxChars - tempAt;

			String newString; // Only needs 20 + 1 characters max, so is guaranteed to fit in sso buffer
			copyChars(newString._rep._sso._chars, tempNums + tempAt, length);
			newString.setSsoLen(length);
			newString.setSsoUsedBytes(length);
			return newString;
		}

		[[nodiscard]] constexpr static String fromUint(u64 num) {
			if (num == 0) return String('0');

			constexpr const char* digits = "9876543210123456789";
			constexpr u64 zeroDigit = 9;
			constexpr u64 maxChars = 20;

			char tempNums[maxChars];
			u64 tempAt = maxChars;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
				num /= 10LL;
			}

			const u64 length = maxChars - tempAt;

			String newString; // Only needs 20 + 1 characters max, so is guaranteed to fit in sso buffer
			copyChars(newString._rep._sso._chars, tempNums + tempAt, length);
			newString.setSsoLen(length);
			newString.setSsoUsedBytes(length);
			return newString;
		}

		/* Max precision is 19. All numbers have decimals after, including whole numbers (eg. "0.0").
		Float positive/negative infinity are "inf" and "-inf" respectively. Not a Number is "nan". */ 
		[[nodiscard]] constexpr static String fromFloat(double num, const int precision = 5) {
			check_message(precision < 20, "gk::String::FromFloat precision must be 19 or less. The precision is set to ", precision);

			if (num == 0)							return String("0.0");			// Zero
			else if (num > DBL_MAX)		return String("inf");			// Positive Infinity
			else if (num < -DBL_MAX)	return String("-inf");		// Negative Infinity
			else if (num != num)			return String("nan");			// Not a Number

			_StripNegativeZero(num);
			const bool isNegative = num < 0;

			const i64 whole = static_cast<i64>(num);
			String wholeString = (isNegative && whole == 0) ? gk::Str("-0") : String::fromInt(whole);

			num -= static_cast<double>(whole);
			if (num == 0) return wholeString + gk::Str(".0"); // quick return if there's nothing after the decimal

			if (num < 0) num *= -1; // Make fractional part positive

			int zeroesInFractionBeforeFirstNonZero = 0;
			for (int i = 0; i < precision; i++) {
				num *= 10;
				if (num < 1) zeroesInFractionBeforeFirstNonZero += 1;
			}

			const u64 fraction = static_cast<u64>(num);
			if (fraction == 0) return wholeString + gk::Str(".0"); // quick return if after the precision check, there's nothing

			auto MakeFractionZeroesString = [](const u64 zeroCount) {
				String str;
				for (u64 i = 0; i < zeroCount; i++) {
					str._rep._sso._chars[i] = '0';
				}
				str.setSsoLen(zeroCount);
				str.setSsoUsedBytes(zeroCount);
				return str;
			};

			auto MakeStringWithIntWithoutEndingZeroes = [](const u64 value, const int availableDigits) {
				String str = String::fromUint(value);
				const u64 lastIndex = (availableDigits < str.usedBytes() - 1) ? availableDigits : str.usedBytes() - 1;

				for (u64 i = lastIndex; i > 0; i--) {
					if (str._rep._sso._chars[i] != '0') {
						for (u64 c = i + 1; c < 30; c++) {
							str._rep._sso._chars[c] = '\0';	
						}
						str.setSsoLen(i + 1);
						str.setSsoUsedBytes(i + 1);
						return str;
					}
					if (i == 1) { // was on last iteration
						for (u64 c = i; c < 30; c++) {
							str._rep._sso._chars[i] = '\0';				
						}
						str.setSsoLen(1);
						str.setSsoUsedBytes(1);
						return str;
					}
				}
				return str; // completely not necessary
			};

			const String fractionZeroesString = MakeFractionZeroesString(zeroesInFractionBeforeFirstNonZero);
			const String fractionalString = MakeStringWithIntWithoutEndingZeroes(fraction, precision - zeroesInFractionBeforeFirstNonZero);

			return wholeString + '.' + fractionZeroesString + fractionalString;
		}

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

#pragma endregion

#pragma region Format		
		
		template<gk::Str formatStr, typename ... Types>
		/** 
		* Construct a formatted string with arguments converted into strings.
		* All variadic arguments must be convertible to gk::String through specializing gk::String::from<T>.
		* Usage:
		* int num = 10;
		* int othernum = 20;
		* // With one argument
		* gk::String s1 = gk::String::format<"number is: {}">(num);
		* // With multiple arguments
		* gk::String s2 = gk::String::format<"number is: {}\nother number is: {}">(num, othernum);
		* // Any number of arguments with text following
		* gk::String s3 = gk::String::format<"multiplying the numbers is: {}! That's very cool :D">(num * othernum);
		*/
		constexpr static String format(const Types&... inputs) {
			static_assert(formatStrIsValid(formatStr), "String::format format str is not valid. Required format is \"some value: {}\"");
			constexpr u64 formatStrLen = formatStr.len;
			constexpr u64 argumentCount = sizeof...(Types);
			constexpr u64 formatSlots = formatStrCountArgs(formatStr);
			static_assert(formatSlots == argumentCount, "Arguments passed into String::format do not match the amount of format slots specified");

			String argsAsStrings[argumentCount];

			FormatStringStrOffset offsets[argumentCount + 1];
			formatStrFindOffsets<formatStr>(offsets, argumentCount + 1);

			u64 mallocCapacity = 0;
			u64 i = 0;

			([&] // loop through all elements, checking the types
				{
					if constexpr (std::is_same_v<Types, gk::String>) {
						mallocCapacity += inputs.usedBytes();
					}
					else if constexpr (std::is_same_v<Types, gk::Str>) {
						mallocCapacity += inputs.totalBytes - 1;
					}
					else {
						argsAsStrings[i] = String::formatStrConvertArg<Types>(inputs);
						mallocCapacity += argsAsStrings[i].usedBytes();
					}
					i++;

				} (), ...);

			for (i = 0; i < argumentCount; i++) {
				mallocCapacity += offsets[i].count;
			}
			if (offsets[argumentCount].start != nullptr) {
				mallocCapacity += offsets[argumentCount].count;
			}

			mallocCapacity += 1;
			String output;
			u64 currentStringIndex = 0;
			u64 outputLength = 0;
			i = 0;

			// Is valid sso
			if (mallocCapacity <= MAX_SSO_UTF8_BYTES) {
				([&]
					{
						FormatStringStrOffset offset = offsets[i];
						copyChars(output._rep._sso._chars + currentStringIndex, offset.start, offset.count);
						currentStringIndex += offset.count;
						outputLength += offset.count;

						if constexpr (std::is_same_v<Types, gk::String>) {
							copyChars(output._rep._sso._chars + currentStringIndex, inputs._rep._sso._chars, inputs.ssoUsedBytes()); // guaranteed to be sso
							currentStringIndex += inputs.ssoUsedBytes();
							outputLength += inputs.ssoLen();
						}
						else if constexpr (std::is_same_v<Types, gk::Str>) {
							copyChars(output._rep._sso._chars + currentStringIndex, inputs.str, inputs.totalBytes - 1);
							currentStringIndex += inputs.totalBytes - 1;
							outputLength += inputs.len;
						}
						else {
							copyChars(output._rep._sso._chars + currentStringIndex, argsAsStrings[i]._rep._sso._chars, argsAsStrings[i].ssoUsedBytes());
							currentStringIndex += argsAsStrings[i].ssoUsedBytes();
							outputLength += argsAsStrings[i].ssoLen();
						}
						i++;
					} (), ...);

				if (offsets[argumentCount].start != nullptr) { // need to use the last offset
					FormatStringStrOffset offset = offsets[argumentCount];
					copyChars(output._rep._sso._chars + currentStringIndex, offset.start, offset.count);
					outputLength += offset.count;
				}

				const u64 utf8Length = gk::utf8::strlen(output._rep._sso._chars).ok().length;
				output.setSsoLen(utf8Length);
				output.setSsoUsedBytes(outputLength);
				return output;
			}

			// Is heap
			char* buffer = String::mallocHeapBuffer(&mallocCapacity);
			([&]
				{
					FormatStringStrOffset offset = offsets[i];
					copyChars(buffer + currentStringIndex, offset.start, offset.count);
					currentStringIndex += offset.count;
					outputLength += offset.count;

					if constexpr (std::is_same_v<Types, gk::String>) {
						copyChars(buffer + currentStringIndex, inputs.cstr(), inputs.usedBytes()); // guaranteed to be sso
						currentStringIndex += inputs.usedBytes();
						outputLength += inputs.len();
					}
					else if constexpr (std::is_same_v<Types, gk::Str>) {
						copyChars(buffer + currentStringIndex, inputs.str, inputs.totalBytes - 1);
						currentStringIndex += inputs.totalBytes - 1;
						outputLength += inputs.len;
					}
					else {
						copyChars(buffer + currentStringIndex, argsAsStrings[i].cstr(), argsAsStrings[i].usedBytes());
						currentStringIndex += argsAsStrings[i].usedBytes();
						outputLength += argsAsStrings[i].len();
					}

					i++;
				} (), ...);


			if (offsets[argumentCount].start != nullptr) { // need to use the last offset
				FormatStringStrOffset offset = offsets[argumentCount];
				copyChars(buffer + currentStringIndex, offset.start, offset.count);
				currentStringIndex += offset.count;
				outputLength += offset.count;
			}

			const u64 utf8Length = gk::utf8::strlen(buffer).ok().length;
			output._rep._heap._buffer = buffer;
			output.setHeapLen(utf8Length);
			output.setHeapUsedBytes(outputLength);
			output.setHeapCapacity(mallocCapacity);
			return output;
		}

#pragma endregion

#pragma region Find

		/* Find the index of a specified character in the cstr array. */
		constexpr Option<usize> find(const char c) const {
			if (std::is_constant_evaluated()) {
				const char* str = cstr();
				const usize bytesToCheck = isSso() ? 30 : _rep._heap._bytesOfBufferUsed;
				for (usize i = 0; i < bytesToCheck; i++) {
					if(str[i] == c) return Option<usize>(i);
				}
				return Option<usize>();
			}

			if (isSso()) {
				for (int i = 0; i < 30; i++) {
					if (_rep._sso._chars[i] == c) return Option<usize>(i);
				}
				return Option<usize>();
			}
			//return findCharInStringFunc(*this, c);
			return simdFindCharInString(*this, c);
		}

		/* Find the index of a specified substring in the cstr array. */
		constexpr Option<usize> find(const Str& str) const {
			const u64 length = len();
			if(str.len > length) return Option<usize>(); // If the substring is larger than this string, it cannot possibly contain it.
			if (str.len == length) {
				if (*this == str) { // If the substring is the same as this string, the found index is just the start of the string.
					return Option<usize>(0);
				}
				Option<usize>();
			}
			if (str.len == 1 && str.totalBytes == 2) {
				return find(str.str[0]);
			}

			const char firstChar = str.str[0];
			if (true) { // TODO constant evaluated
				const char* thisStr = cstr();
				const u64 bytesToCheck = (isSso() ? 30 : _rep._heap._bytesOfBufferUsed) - (str.totalBytes - 1);
				for (u64 i = 0; i < bytesToCheck; i++) {
					if (thisStr[i] == firstChar) {
						const char* thisCompareStart = thisStr + i;

						bool found = true;
						for (u64 compareIndex = 1; compareIndex < (str.totalBytes - 1); compareIndex++) { // dont need to check the first character
							if (thisCompareStart[compareIndex] != str.str[compareIndex]) {
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
			
			
		}

		/* Find the index of a specified substring in the cstr array. */
		constexpr Option<usize> find(const String& other) const {
			const u64 length = len();
			const u64 otherLen = other.len();
			if (otherLen > length) return Option<usize>(); // If the substring is larger than this string, it cannot possibly contain it.
			if (otherLen == length) {
				if (*this == other) { // If the substring is the same as this string, the found index is just the start of the string.
					return Option<usize>(0);
				}
				Option<usize>();
			}

			const u64 otherUsedBytes = other.usedBytes();
			if (otherLen == 1 && otherUsedBytes == 2) {
				return find(other._rep._sso._chars[0]);
			}

			if (true) { // TODO constant evaluated
				const char* thisStr = cstr();
				const char* otherCstr = other.cstr();
				const char firstChar = otherCstr[0];
				const u64 bytesToCheck = (isSso() ? 30 : _rep._heap._bytesOfBufferUsed) - (otherUsedBytes);
				for (u64 i = 0; i < bytesToCheck; i++) {
					if (thisStr[i] == firstChar) {
						const char* thisCompareStart = thisStr + i;

						bool found = true;
						for (u64 compareIndex = 1; compareIndex < (otherUsedBytes); compareIndex++) { // dont need to check the first character
							if (thisCompareStart[compareIndex] != otherCstr[compareIndex]) {
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
		}

#pragma endregion

#pragma region Substring

		/* Uses indices into the cstr, not length offsets. With ascii, adding length will work, with multi byte characters, it will not. */
		[[nodiscard]] constexpr String substring(const u64 startIndexInclusive, const u64 endIndexExclusive) const {
			if (std::is_constant_evaluated()) {
				if (startIndexInclusive > usedBytes()) throw;
				if (endIndexExclusive > usedBytes()) throw;
				if (startIndexInclusive >= endIndexExclusive) throw;
			}
			else {
				check_message(startIndexInclusive <= usedBytes(), "Substring start index must be within string used utf8 bytes count. start index is ", startIndexInclusive, " and used bytes is ", usedBytes());
				check_message(endIndexExclusive <= usedBytes(), "Substring end index must be less than or equal to the string used utf8 bytes count. end index is ", endIndexExclusive, " and used bytes is ", usedBytes());
				check_message(startIndexInclusive < endIndexExclusive, "Substring start index must be less than end index. start index is ", startIndexInclusive, " and end index is ", usedBytes());
			}

			String outStr;

			const u64 substrUsedBytes = endIndexExclusive - startIndexInclusive;
			u64 capacity = substrUsedBytes + 1; // include null terminator
			if (capacity <= MAX_SSO_UTF8_BYTES) {
				const char* copyStart = cstr() + startIndexInclusive;
				copyChars(outStr._rep._sso._chars, copyStart, substrUsedBytes);
				const u64 substrLength = gk::utf8::strlen(outStr._rep._sso._chars).ok().length;

				outStr.setSsoLen(substrLength);
				outStr.setSsoUsedBytes(substrUsedBytes);
				return outStr;
			}

			char* newBuffer = mallocHeapBuffer(&capacity);
			const char* copyStart = cstr() + startIndexInclusive;
			copyChars(newBuffer, copyStart, substrUsedBytes);
			outStr._rep._heap = HeapRep();
			outStr._rep._heap._buffer = newBuffer;
			const u64 substrLength = gk::utf8::strlen(newBuffer).ok().length;

			outStr.setHeapLen(substrLength);
			outStr.setHeapUsedBytes(substrUsedBytes);
			outStr.setHeapCapacity(capacity);
			return outStr;
		}

#pragma endregion
		
	private:

		[[nodiscard]] constexpr forceinline bool isSso() const {
			return !(_flag & FLAG_BIT);
		}

		[[nodiscard]] constexpr forceinline u64 ssoLen() const {
			// When fetching ssoLen(), it can be assumed that the string is using sso.
			return _flag;
		}

		[[nodiscard]] constexpr forceinline u64 heapLen() const {
			return _rep._heap._length;
		}

		[[nodiscard]] constexpr forceinline u64 ssoUsedBytes() const {
			return static_cast<u64>(30) - _rep._sso._chars[30];
		}

		[[nodiscard]] constexpr forceinline u64 heapCapacity() const {			
			if (std::is_constant_evaluated()) {
				u64 heapCapacity = 0;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[0]));
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[1])) << 8;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[2])) << 16;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[3])) << 24;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[4])) << 32;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[5])) << 40;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_rep._heap._totalByteCapacityBitmask[6])) << 48;
				heapCapacity |= static_cast<u64>(static_cast<u8>(_flag)) << 56;
				return heapCapacity & HEAP_CAPACITY_BITMASK;
			}
			return *((u64*)_rep._heap._totalByteCapacityBitmask) & HEAP_CAPACITY_BITMASK;
		}

		constexpr forceinline void setSsoLen(const u64 inLen) {
			check_message(inLen <= MAX_SSO_LEN, "sso length cannot exceed the maximum of ", MAX_SSO_LEN);
			_flag = static_cast<char>(inLen);
		}

		constexpr forceinline void setHeapLen(const u64 inLen) {
			_rep._heap._length = inLen;
		}

		constexpr forceinline void setHeapFlag() {
			_flag |= FLAG_BIT;
		}

		constexpr forceinline void setSsoUsedBytes(const u64 used) {
			_rep._sso._chars[30] = (char)30 - static_cast<char>(used);
		}

		constexpr forceinline void setHeapUsedBytes(const u64 used) {
			_rep._heap._bytesOfBufferUsed = used;
		}
		
		// Also sets heap flag
		constexpr forceinline void setHeapCapacity(const u64 capacity) {
			if (std::is_constant_evaluated()) {
				_rep._heap._totalByteCapacityBitmask[0] = static_cast<char>(capacity & 0b11111111);
				_rep._heap._totalByteCapacityBitmask[1] = static_cast<char>((capacity >> 8) & 0b11111111);
				_rep._heap._totalByteCapacityBitmask[2] = static_cast<char>((capacity >> 16) & 0b11111111);
				_rep._heap._totalByteCapacityBitmask[3] = static_cast<char>((capacity >> 24) & 0b11111111);
				_rep._heap._totalByteCapacityBitmask[4] = static_cast<char>((capacity >> 32) & 0b11111111);
				_rep._heap._totalByteCapacityBitmask[5] = static_cast<char>((capacity >> 40) & 0b11111111);
				_rep._heap._totalByteCapacityBitmask[6] = static_cast<char>((capacity >> 48) & 0b11111111);
				_flag = (char)0b10000000 | static_cast<char>((capacity >> 56) & 0b11111111);		
				return;
			}
			*((u64*)_rep._heap._totalByteCapacityBitmask) = (1ULL << 63) | capacity;
		}

		constexpr void freeHeapBufferIfNotSso() {
			if (!isSso()) {
				if (_rep._heap._buffer != nullptr) {
					freeHeapBuffer(_rep._heap._buffer);
				}
			}
		}

		/* Takes in a mutable pointer to modify the capacity to something divisible by 64. 
		@return 0 filled buffer. */
		static constexpr char* mallocHeapBuffer(u64* capacityIn) {
			constexpr u64 alignment = 64;
			const u64 remainder = *capacityIn % alignment;
			if (remainder) {
				*capacityIn = *capacityIn + (alignment - remainder);
			}

			const u64 capacity = *capacityIn;

			//gk_assertm(capacity >= alignment, "String must allocate a capacity of 32 or more. Requested allocation capacity is " << capacity);
			//gk_assertm(capacity % alignment == 0, "String must allocate number that evenly divides by 32. Requested allocation capacity is " << capacity);

			if (std::is_constant_evaluated()) {
				char* constexprBuffer = new char[capacity];
				for (u64 i = 0; i < capacity; i++) {
					constexprBuffer[i] = '\0';
				}
				return constexprBuffer;
			}
			
			char* buffer = globalHeapAllocator()->mallocAlignedBuffer<char>(capacity, alignment).ok();
			memset(buffer, '\0', capacity);
			return buffer;
		}

		static constexpr void freeHeapBuffer(char* buffer) {
			if (std::is_constant_evaluated()) {
				delete[] buffer;
			}
			else {
				constexpr u64 alignment = 64;
				//globalHeapAllocator()->freeAlignedBuffer(buffer, )
				//gk_assertm(gk::isAligned(buffer, alignment), "Cannot free non-aligned buffer. Required alignment to free is " << alignment << " bytes");
				_aligned_free(buffer);
			}
		}

		static constexpr forceinline void copyChars(char* destination, const char* source, const u64 num) {
			if (std::is_constant_evaluated()) {
				for (u64 i = 0; i < num; i++) {
					destination[i] = source[i];
				}
			}
			else {
				memcpy(destination, source, num);
			}
		}

		__pragma(optimize("", off))
		static constexpr void _StripNegativeZero(double& inFloat)
		{
			// Taken from Unreal Engine
			// This works for translating a negative zero into a positive zero,
			// but if optimizations are enabled when compiling with -ffast-math
			// or /fp:fast, the compiler can strip it out.
			inFloat += 0.0;
		}
		__pragma(optimize("", on))

		static bool simdCompareStringAndStr(const String& self, const Str& str);
		static bool simdCompareStringAndString(const String& self, const String& other);
		static Option<usize> simdFindCharInString(const String& self, char c);

#pragma region Format_Functionality

			struct FormatStringStrOffset {
				const char* start;
				u64 count;
			};

			consteval static bool formatStrIsValid(const gk::Str& formatStr) {
				u64 count = 0;
				for (u64 i = 0; i < formatStr.totalBytes - 1; i++) {
					if (formatStr.str[i] == '{') {
						if (formatStr.str[i + 1] != '}') {
							return false;
						}
						count++;
					}
				}
				return true;
			}

			consteval static usize formatStrCountArgs(const gk::Str& formatStr) {
				usize count = 0;
				for (usize i = 0; i < formatStr.totalBytes - 1; i++) {
					if (formatStr.str[i] == '{') count++;
				}
				return count;
			}

			template<gk::Str formatStr>
			constexpr static void formatStrFindOffsets(FormatStringStrOffset* offsetsArr, const usize bufferSize) {
				usize currentOffset = 0;
				usize offsetIndex = 0;
				for (usize i = 0; i < formatStr.totalBytes - 1; i++) {
					if (formatStr.str[i] == '{') {
						offsetsArr[offsetIndex].start = formatStr.str + currentOffset;
						offsetsArr[offsetIndex].count = i - currentOffset;
						currentOffset = i + 2;
						offsetIndex++;
					}
				}
				if (formatStr.str[formatStr.totalBytes - 2] != '}') {
					offsetsArr[bufferSize - 1].start = formatStr.str + currentOffset;
					offsetsArr[bufferSize - 1].count = (formatStr.totalBytes - 1) - currentOffset;
				}
				else {
					offsetsArr[bufferSize - 1].start = nullptr;
					offsetsArr[bufferSize - 1].count = 0;
				}
			}

			template<typename T>
			constexpr static String formatStrConvertArg(const T& arg) {
				auto convertCheck = [](auto& t) -> decltype(String::from<T>(t)) {
					return String::from<T>(t);
				};
				constexpr bool can_make_string_from_type =
					std::is_invocable_v < decltype(convertCheck), T&>;
				static_assert(can_make_string_from_type, "String::from<T>() is not specialized for type T");
				return String::from<T>(arg);
			}

#pragma endregion

	}; // String
}

namespace std
{
	template<>
	struct hash<gk::String>
	{
		size_t operator()(const gk::String& str) const {
			return str.hash();
		}
	};
}

namespace gk 
{
	template<>
	constexpr size_t hash<gk::String>(const gk::String& key) {
		return key.hash();
	}
}