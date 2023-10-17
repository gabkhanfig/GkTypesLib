#pragma once

#include "Str.h"
#include <malloc.h>
#include <Windows.h>
#include "../Option/Option.h"
#include "../CpuFeatures/CpuFeatureDetector.h"
#include "../Utility.h"

namespace gk
{
	struct StringIndex {};

	template<>
	struct Option<StringIndex>
	{
		constexpr Option() : _index(INDEX_NONE) {}
		constexpr Option(const uint64 index) : _index(index) {}
		constexpr Option(const Option<StringIndex>& other) : _index(other._index) {}
		constexpr void operator = (const uint64 index) { _index = index; }
		constexpr void operator = (const Option<StringIndex>& other) { _index = other._index; }

		[[nodiscard]] constexpr uint64 some() const {
			gk_assertm(!none(), "Cannot get optional darray index value if its none");
			return _index;
		}

		[[nodiscard]] constexpr bool none() const {
			return _index == INDEX_NONE;
		}

		static constexpr uint64 INDEX_NONE = 0xFFFFFFFFFFFFFFFFULL;

	private:

		uint64 _index;

	};

	struct ALIGN_AS(8) String
	{
	private:

#pragma pack(push, 1)
		struct HeapRep
		{
			// length as utf8 characters
			uint64 _length;
			// 64 byte aligned buffer
			char* _buffer;
			// does not include null terminator
			uint64 _bytesOfBufferUsed;
			// total byte capacity of the string, used along with the _flag byte to make a uint64.
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
		static constexpr uint64 MAX_SSO_UTF8_BYTES = 31;
		static constexpr uint64 MAX_SSO_LEN = 30;
		static constexpr uint64 HEAP_CAPACITY_BITMASK = ~(1ULL << 63);

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
			const uint64 stringBytes = str.totalBytes - 1;
			if (str.totalBytes <= MAX_SSO_UTF8_BYTES) {
				if (std::is_constant_evaluated()) {
					for (uint64 i = 0; i < stringBytes; i++) {
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

			uint64 capacity = str.totalBytes;
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (uint64 i = 0; i < stringBytes; i++) {
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
			uint64 capacity = other.heapCapacity(); 
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (uint64 i = 0; i < other._rep._heap._bytesOfBufferUsed; i++) {
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
			const uint64 stringBytes = str.totalBytes - 1;
			if (str.totalBytes <= MAX_SSO_UTF8_BYTES) { // should be sso.
				freeHeapBufferIfNotSso();
				_rep._sso = SsoRep();
				if (std::is_constant_evaluated()) {
					for (uint64 i = 0; i < stringBytes; i++) {
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

			const uint64 heapCap = heapCapacity();
			if (heapCap > stringBytes) { // reuse existing allocation
				if (std::is_constant_evaluated()) {
					uint64 i = 0;
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

			uint64 capacity = str.totalBytes;
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (uint64 i = 0; i < stringBytes; i++) {
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
			const uint64 stringBytes = other.usedBytes();
			if (other.isSso()) { // should be sso.
				freeHeapBufferIfNotSso();
				_rep._sso = SsoRep();
				if (std::is_constant_evaluated()) {
					for (uint64 i = 0; i < stringBytes; i++) {
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
						uint64 i = 0;
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

			uint64 capacity = stringBytes + 1;
			char* buffer = mallocHeapBuffer(&capacity);
			if (std::is_constant_evaluated()) {
				for (uint64 i = 0; i < stringBytes; i++) {
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

		[[nodiscard]] constexpr forceinline uint64 len() const {
			if (std::is_constant_evaluated()) {
				if (isSso()) {
					return ssoLen();
				}
				return heapLen();
			}
			const uint64 isSsoRep = isSso();
			const uint64 isHeapRep = !isSso();
			return (isSsoRep * ssoLen()) + (isHeapRep * heapLen());
		}

		/* Amount of utf8 bytes used by this string (same as length for ascii only string) */
		[[nodiscard]] constexpr forceinline uint64 usedBytes() const {
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
			const uint64 length = len();
			const uint64 m = 0xc6a4a7935bd1e995ULL;
			const int r = 47;

			uint64 h = seed ^ (length * m);

			const uint64* data = (const uint64*)str;
			const uint64* end = data + (length / 8);

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

			switch (length & 7)
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
				for (uint64 i = 0; i < str.totalBytes; i++) {
					if (thisStr[i] != str.str[i]) return false;
				}
				return true;
			}

			if (isSso()) {
				if (ssoUsedBytes() != str.totalBytes - 1) return false;

				const uint64* ssoCharsAsMachineWord = reinterpret_cast<const uint64*>(_rep._sso._chars);
				uint64 buf[4] = { 0, 0, 0, 0 };
				gk_assertm(str.totalBytes <= 32, "shut up compiler");
				memcpy(buf, str.str, str.totalBytes - 1);

				((char*)buf)[30] = _rep._sso._chars[30];	// must copy sso capacity byte
				((char*)buf)[31] = _flag;									// must copy last byte for length

				return (ssoCharsAsMachineWord[0] == buf[0])
					&& (ssoCharsAsMachineWord[1] == buf[1])
					&& (ssoCharsAsMachineWord[2] == buf[2])
					&& (ssoCharsAsMachineWord[3] == buf[3]);
			}

			if (_rep._heap._bytesOfBufferUsed != str.totalBytes - 1) return false;

			return cmpEqStringStr(*this, str);
		}

		[[nodiscard]] constexpr bool operator == (const String& other) const {
			if (std::is_constant_evaluated()) {
				if (len() != other.len()) return false;
				const char* thisStr = cstr();
				const char* otherStr = other.cstr();
				for (uint64 i = 0; i < usedBytes(); i++) {
					if (thisStr[i] != otherStr[i]) return false;
				}
				return true;
			}

			if (isSso()) {
				const uint64* thisSso = reinterpret_cast<const uint64*>(_rep._sso._chars);
				const uint64* otherSso = reinterpret_cast<const uint64*>(other._rep._sso._chars);
				return (thisSso[0] == otherSso[0])
					&& (thisSso[1] == otherSso[1])
					&& (thisSso[2] == otherSso[2])
					&& (thisSso[3] == otherSso[3]);
			}

			if (_rep._heap._bytesOfBufferUsed != other._rep._heap._bytesOfBufferUsed) return false;

			return cmpEqStringString(*this, other);
		}

#pragma endregion

#pragma region Append

		constexpr String& append(const char c) {
			const uint64 length = len();
			const uint64 newLength = length + 1;
			const uint64 capacityUsedNoTerminator = usedBytes();
			const uint64 minCapacity = capacityUsedNoTerminator + 2;

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				gk_assertm(isSso(), "String representation should be sso");
				_rep._sso._chars[capacityUsedNoTerminator] = c;
				setSsoLen(newLength);
				setSsoUsedBytes(capacityUsedNoTerminator + 1);
				gk_assertm(_rep._sso._chars[capacityUsedNoTerminator + 1] == '\0', "Sso chars should already have null terminator");
				return *this;
			}

			if (isSso()) { // If it is an sso, it must reallocate.
				char temp[30]; // does not include null terminator
				copyChars(temp, _rep._sso._chars, 30);
		
				_rep._heap = HeapRep();
				uint64 mallocCapacity = 64;
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
				uint64 mallocCapacity = minCapacity;
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
			const uint64 length = len();
			const uint64 newLength = length + str.len;
			const uint64 capacityUsedNoTerminator = usedBytes();
			const uint64 strBytes = str.totalBytes - 1;
			const uint64 minCapacity = capacityUsedNoTerminator + str.totalBytes; // includes null terminator

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				gk_assertm(isSso(), "String representation should be sso");
				copyChars(_rep._sso._chars + capacityUsedNoTerminator, str.str, strBytes);
				setSsoLen(newLength);
				setSsoUsedBytes(minCapacity - 1);
				gk_assertm(_rep._sso._chars[capacityUsedNoTerminator + strBytes] == '\0', "Sso chars should already have null terminator");
				return *this;
			}

			if (isSso()) { // If it is an sso, it must reallocate.
				char temp[30]; // does not include null terminator
				copyChars(temp, _rep._sso._chars, 30);

				_rep._heap = HeapRep();
				uint64 mallocCapacity = minCapacity + str.totalBytes - 1; // overallocate
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
				uint64 mallocCapacity = minCapacity + str.len; // overallocate
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
			const uint64 length = len();
			const uint64 otherLen = other.len();
			const uint64 newLength = length + otherLen;
			const uint64 capacityUsedNoTerminator = usedBytes();
			const uint64 stringBytes = other.usedBytes();
			const uint64 minCapacity = capacityUsedNoTerminator + stringBytes + 1; // includes null terminator

			const char* otherStr = other.cstr();

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				gk_assertm(isSso(), "String representation should be sso");
				copyChars(_rep._sso._chars + capacityUsedNoTerminator, otherStr, stringBytes);
				setSsoLen(newLength);
				setSsoUsedBytes(minCapacity - 1);
				gk_assertm(_rep._sso._chars[capacityUsedNoTerminator + stringBytes] == '\0', "Sso chars should already have null terminator");
				return *this;
			}

			if (isSso()) { // If it is an sso, it must reallocate.
				char temp[30]; // does not include null terminator
				copyChars(temp, _rep._sso._chars, 30);

				_rep._heap = HeapRep();
				uint64 mallocCapacity = minCapacity + stringBytes; // overallocate
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
				uint64 mallocCapacity = minCapacity + stringBytes; // overallocate
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
			const uint64 length = lhs.len();
			const uint64 newLength = length + 1;
			const uint64 capacityUsedNoTerminator = lhs.usedBytes();
			const uint64 minCapacity = capacityUsedNoTerminator + 2;

			String newString;

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs._rep._sso._chars, capacityUsedNoTerminator);
				newString._rep._sso._chars[capacityUsedNoTerminator] = rhs;
				newString.setSsoUsedBytes(capacityUsedNoTerminator + 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			uint64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
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
			const uint64 length = lhs.len();
			const uint64 newLength = length + rhs.len;
			const uint64 capacityUsedNoTerminator = lhs.usedBytes();
			const uint64 strBytes = rhs.totalBytes - 1;
			const uint64 minCapacity = capacityUsedNoTerminator + rhs.totalBytes; // includes null terminator

			String newString; 
			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs._rep._sso._chars, capacityUsedNoTerminator);
				copyChars(newString._rep._sso._chars + capacityUsedNoTerminator, rhs.str, strBytes);
				newString.setSsoUsedBytes(minCapacity - 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			uint64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
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
			const uint64 length = lhs.len();
			const uint64 newLength = length + rhs.len();
			const uint64 capacityUsedNoTerminator = lhs.usedBytes();
			const uint64 stringBytes = rhs.usedBytes();
			const uint64 minCapacity = capacityUsedNoTerminator + stringBytes + 1; // includes null terminator

			String newString;
			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs._rep._sso._chars, capacityUsedNoTerminator);
				copyChars(newString._rep._sso._chars + capacityUsedNoTerminator, rhs._rep._sso._chars, stringBytes);
				newString.setSsoUsedBytes(minCapacity - 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			uint64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
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
			const uint64 length = rhs.len();
			const uint64 newLength = length + 1;
			const uint64 capacityUsedNoTerminator = rhs.usedBytes();
			const uint64 minCapacity = capacityUsedNoTerminator + 2;

			String newString;

			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				newString._rep._sso._chars[0] = lhs;
				copyChars(newString._rep._sso._chars + 1, rhs._rep._sso._chars, capacityUsedNoTerminator);
				newString.setSsoUsedBytes(capacityUsedNoTerminator + 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			uint64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
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
			const uint64 length = rhs.len();
			const uint64 newLength = length + lhs.len;
			const uint64 capacityUsedNoTerminator = rhs.usedBytes();
			const uint64 strBytes = lhs.totalBytes - 1;
			const uint64 minCapacity = capacityUsedNoTerminator + lhs.totalBytes; // includes null terminator

			String newString;
			if (minCapacity <= MAX_SSO_UTF8_BYTES) {
				copyChars(newString._rep._sso._chars, lhs.str, strBytes);
				copyChars(newString._rep._sso._chars + strBytes, rhs._rep._sso._chars, capacityUsedNoTerminator);
				newString.setSsoUsedBytes(minCapacity - 1);
				newString.setSsoLen(newLength);
				return newString;
			}

			newString._rep._heap = HeapRep();

			uint64 mallocCapacity = (3 * minCapacity) >> 1; // overallocate by multiplying by 1.5
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

		[[nodiscard]] constexpr static String fromInt(int64 num) {
			if (num == 0) return String('0');

			constexpr const char* digits = "9876543210123456789";
			constexpr uint64 zeroDigit = 9;
			constexpr uint64 maxChars = 20;

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

			const uint64 length = maxChars - tempAt;

			String newString; // Only needs 20 + 1 characters max, so is guaranteed to fit in sso buffer
			copyChars(newString._rep._sso._chars, tempNums + tempAt, length);
			newString.setSsoLen(length);
			newString.setSsoUsedBytes(length);
			return newString;
		}

		[[nodiscard]] constexpr static String fromUint(uint64 num) {
			if (num == 0) return String('0');

			constexpr const char* digits = "9876543210123456789";
			constexpr uint64 zeroDigit = 9;
			constexpr uint64 maxChars = 20;

			char tempNums[maxChars];
			uint64 tempAt = maxChars;

			while (num) {
				tempNums[--tempAt] = digits[zeroDigit + (num % 10LL)];
				num /= 10LL;
			}

			const uint64 length = maxChars - tempAt;

			String newString; // Only needs 20 + 1 characters max, so is guaranteed to fit in sso buffer
			copyChars(newString._rep._sso._chars, tempNums + tempAt, length);
			newString.setSsoLen(length);
			newString.setSsoUsedBytes(length);
			return newString;
		}

		/* Max precision is 19. All numbers have decimals after, including whole numbers (eg. "0.0").
		Float positive/negative infinity are "inf" and "-inf" respectively. Not a Number is "nan". */ 
		[[nodiscard]] constexpr static String fromFloat(double num, const int precision = 5) {
			gk_assertm(precision < 20, "gk::String::FromFloat precision must be 19 or less. The precision is set to " << precision);

			if (num == 0)							return String("0.0");			// Zero
			else if (num > DBL_MAX)		return String("inf");			// Positive Infinity
			else if (num < -DBL_MAX)	return String("-inf");		// Negative Infinity
			else if (num != num)			return String("nan");			// Not a Number

			_StripNegativeZero(num);
			const bool isNegative = num < 0;

			const int64 whole = static_cast<int64>(num);
			String wholeString = (isNegative && whole == 0) ? gk::Str("-0") : String::fromInt(whole);

			num -= static_cast<double>(whole);
			if (num == 0) return wholeString + gk::Str(".0"); // quick return if there's nothing after the decimal

			if (num < 0) num *= -1; // Make fractional part positive

			int zeroesInFractionBeforeFirstNonZero = 0;
			for (int i = 0; i < precision; i++) {
				num *= 10;
				if (num < 1) zeroesInFractionBeforeFirstNonZero += 1;
			}

			const uint64 fraction = static_cast<uint64>(num);
			if (fraction == 0) return wholeString + gk::Str(".0"); // quick return if after the precision check, there's nothing

			auto MakeFractionZeroesString = [](const uint64 zeroCount) {
				String str;
				for (uint64 i = 0; i < zeroCount; i++) {
					str._rep._sso._chars[i] = '0';
				}
				str.setSsoLen(zeroCount);
				str.setSsoUsedBytes(zeroCount);
				return str;
			};

			auto MakeStringWithIntWithoutEndingZeroes = [](const uint64 value, const int availableDigits) {
				String str = String::fromUint(value);
				const uint64 lastIndex = (availableDigits < str.usedBytes() - 1) ? availableDigits : str.usedBytes() - 1;

				for (uint64 i = lastIndex; i > 0; i--) {
					if (str._rep._sso._chars[i] != '0') {
						for (uint64 c = i + 1; c < 30; c++) {
							str._rep._sso._chars[c] = '\0';	
						}
						str.setSsoLen(i + 1);
						str.setSsoUsedBytes(i + 1);
						return str;
					}
					if (i == 1) { // was on last iteration
						for (uint64 c = i; c < 30; c++) {
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
		[[nodiscard]] constexpr static String from<int8>(const int8& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<int16>(const int16& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<int>(const int& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<int64>(const int64& num) { return fromInt(num); }

		template<>
		[[nodiscard]] constexpr static String from<uint8>(const uint8& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<uint16>(const uint16& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<uint32>(const uint32& num) { return fromUint(num); }

		template<>
		[[nodiscard]] constexpr static String from<uint64>(const uint64& num) { return fromUint(num); }

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
			constexpr uint64 formatStrLen = formatStr.len;
			constexpr uint64 argumentCount = sizeof...(Types);
			constexpr uint64 formatSlots = formatStrCountArgs(formatStr);
			static_assert(formatSlots == argumentCount, "Arguments passed into String::format do not match the amount of format slots specified");

			String argsAsStrings[argumentCount];

			FormatStringStrOffset offsets[argumentCount + 1];
			formatStrFindOffsets<formatStr>(offsets, argumentCount + 1);

			uint64 mallocCapacity = 0;
			uint64 i = 0;

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
			uint64 currentStringIndex = 0;
			uint64 outputLength = 0;
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

				const uint64 utf8Length = gk::utf8::strlen(output._rep._sso._chars).ok().length;
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

			const uint64 utf8Length = gk::utf8::strlen(buffer).ok().length;
			output._rep._heap._buffer = buffer;
			output.setHeapLen(utf8Length);
			output.setHeapUsedBytes(outputLength);
			output.setHeapCapacity(mallocCapacity);
			return output;
		}

#pragma endregion

#pragma region Find

		/* Find the index of a specified character in the cstr array. */
		constexpr Option<StringIndex> find(const char c) const {
			if (std::is_constant_evaluated()) {
				const char* str = cstr();
				const uint64 bytesToCheck = isSso() ? 30 : _rep._heap._bytesOfBufferUsed;
				for (uint64 i = 0; i < bytesToCheck; i++) {
					if(str[i] == c) return Option<StringIndex>(i);
				}
				return Option<StringIndex>();
			}

			if (isSso()) {
				for (int i = 0; i < 30; i++) {
					if (_rep._sso._chars[i] == c) return Option<StringIndex>(i);
				}
				return Option<StringIndex>();
			}
			return findCharInStringFunc(*this, c);
		}

		/* Find the index of a specified substring in the cstr array. */
		constexpr Option<StringIndex> find(const Str& str) const {
			const uint64 length = len();
			if(str.len > length) return Option<StringIndex>(); // If the substring is larger than this string, it cannot possibly contain it.
			if (str.len == length) {
				if (*this == str) { // If the substring is the same as this string, the found index is just the start of the string.
					return Option<StringIndex>(0);
				}
				Option<StringIndex>();
			}
			if (str.len == 1 && str.totalBytes == 2) {
				return find(str.str[0]);
			}

			const char firstChar = str.str[0];
			if (true) { // TODO constant evaluated
				const char* thisStr = cstr();
				const uint64 bytesToCheck = (isSso() ? 30 : _rep._heap._bytesOfBufferUsed) - (str.totalBytes - 1);
				for (uint64 i = 0; i < bytesToCheck; i++) {
					if (thisStr[i] == firstChar) {
						const char* thisCompareStart = thisStr + i;

						bool found = true;
						for (uint64 compareIndex = 1; compareIndex < (str.totalBytes - 1); compareIndex++) { // dont need to check the first character
							if (thisCompareStart[compareIndex] != str.str[compareIndex]) {
								found = false;
								break;
							}
						}
						if (found) {
							return Option<StringIndex>(i); // All has been checked.
						}
					}
				}
				return Option<StringIndex>();
			}
			
			
		}

		/* Find the index of a specified substring in the cstr array. */
		constexpr Option<StringIndex> find(const String& other) const {
			const uint64 length = len();
			const uint64 otherLen = other.len();
			if (otherLen > length) return Option<StringIndex>(); // If the substring is larger than this string, it cannot possibly contain it.
			if (otherLen == length) {
				if (*this == other) { // If the substring is the same as this string, the found index is just the start of the string.
					return Option<StringIndex>(0);
				}
				Option<StringIndex>();
			}

			const uint64 otherUsedBytes = other.usedBytes();
			if (otherLen == 1 && otherUsedBytes == 2) {
				return find(other._rep._sso._chars[0]);
			}

			if (true) { // TODO constant evaluated
				const char* thisStr = cstr();
				const char* otherCstr = other.cstr();
				const char firstChar = otherCstr[0];
				const uint64 bytesToCheck = (isSso() ? 30 : _rep._heap._bytesOfBufferUsed) - (otherUsedBytes);
				for (uint64 i = 0; i < bytesToCheck; i++) {
					if (thisStr[i] == firstChar) {
						const char* thisCompareStart = thisStr + i;

						bool found = true;
						for (uint64 compareIndex = 1; compareIndex < (otherUsedBytes); compareIndex++) { // dont need to check the first character
							if (thisCompareStart[compareIndex] != otherCstr[compareIndex]) {
								found = false;
								break;
							}
						}
						if (found) {
							return Option<StringIndex>(i); // All has been checked.
						}
					}
				}
				return Option<StringIndex>();
			}
		}

#pragma endregion

#pragma region Substring

		/* Uses indices into the cstr, not length offsets. With ascii, adding length will work, with multi byte characters, it will not. */
		[[nodiscard]] constexpr String substring(const uint64 startIndexInclusive, const uint64 endIndexExclusive) const {
			gk_assertm(startIndexInclusive <= usedBytes(), "Substring start index must be within string used utf8 bytes count");
			gk_assertm(endIndexExclusive <= usedBytes(), "Substring end index must be less than or equal to the string used utf8 bytes count");
			gk_assertm(startIndexInclusive < endIndexExclusive, "Substring start index must be less than end index");

			String outStr;

			const uint64 substrUsedBytes = endIndexExclusive - startIndexInclusive;
			uint64 capacity = substrUsedBytes + 1; // include null terminator
			if (capacity <= MAX_SSO_UTF8_BYTES) {
				const char* copyStart = cstr() + startIndexInclusive;
				copyChars(outStr._rep._sso._chars, copyStart, substrUsedBytes);
				const uint64 substrLength = gk::utf8::strlen(outStr._rep._sso._chars).ok().length;

				outStr.setSsoLen(substrLength);
				outStr.setSsoUsedBytes(substrUsedBytes);
				return outStr;
			}

			char* newBuffer = mallocHeapBuffer(&capacity);
			const char* copyStart = cstr() + startIndexInclusive;
			copyChars(newBuffer, copyStart, substrUsedBytes);
			outStr._rep._heap = HeapRep();
			outStr._rep._heap._buffer = newBuffer;
			const uint64 substrLength = gk::utf8::strlen(newBuffer).ok().length;

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

		[[nodiscard]] constexpr forceinline uint64 ssoLen() const {
			// When fetching ssoLen(), it can be assumed that the string is using sso.
			return _flag;
		}

		[[nodiscard]] constexpr forceinline uint64 heapLen() const {
			return _rep._heap._length;
		}

		[[nodiscard]] constexpr forceinline uint64 ssoUsedBytes() const {
			return static_cast<uint64>(30) - _rep._sso._chars[30];
		}

		[[nodiscard]] constexpr forceinline uint64 heapCapacity() const {			
			if (std::is_constant_evaluated()) {
				uint64 heapCapacity = 0;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[0]));
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[1])) << 8;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[2])) << 16;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[3])) << 24;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[4])) << 32;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[5])) << 40;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_rep._heap._totalByteCapacityBitmask[6])) << 48;
				heapCapacity |= static_cast<uint64>(static_cast<uint8>(_flag)) << 56;
				return heapCapacity & HEAP_CAPACITY_BITMASK;
			}
			return *((uint64*)_rep._heap._totalByteCapacityBitmask) & HEAP_CAPACITY_BITMASK;
		}

		constexpr forceinline void setSsoLen(const uint64 inLen) {
			gk_assertm(inLen <= MAX_SSO_LEN, "sso length cannot exceed the maximum of " << MAX_SSO_LEN);
			_flag = static_cast<char>(inLen);
		}

		constexpr forceinline void setHeapLen(const uint64 inLen) {
			_rep._heap._length = inLen;
		}

		constexpr forceinline void setHeapFlag() {
			_flag |= FLAG_BIT;
		}

		constexpr forceinline void setSsoUsedBytes(const uint64 used) {
			_rep._sso._chars[30] = (char)30 - static_cast<char>(used);
		}

		constexpr forceinline void setHeapUsedBytes(const uint64 used) {
			_rep._heap._bytesOfBufferUsed = used;
		}
		
		// Also sets heap flag
		constexpr forceinline void setHeapCapacity(const uint64 capacity) {
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
			*((uint64*)_rep._heap._totalByteCapacityBitmask) = (1ULL << 63) | capacity;
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
		static constexpr char* mallocHeapBuffer(uint64* capacityIn) {
			constexpr uint64 alignment = 64;
			const uint64 remainder = *capacityIn % alignment;
			if (remainder) {
				*capacityIn = *capacityIn + (alignment - remainder);
			}

			const uint64 capacity = *capacityIn;

			gk_assertm(capacity >= alignment, "String must allocate a capacity of 32 or more. Requested allocation capacity is " << capacity);
			gk_assertm(capacity % alignment == 0, "String must allocate number that evenly divides by 32. Requested allocation capacity is " << capacity);

			if (std::is_constant_evaluated()) {
				char* constexprBuffer = new char[capacity];
				for (uint64 i = 0; i < capacity; i++) {
					constexprBuffer[i] = '\0';
				}
				return constexprBuffer;
			}
			
			char* buffer = (char*)_aligned_malloc(capacity, alignment);
			gk_assertm(buffer != nullptr, "_aligned_malloc failed");
			gk_assertm(gk::isAligned(buffer, alignment), "Heap allocated char buffer for string must have an alignment of " << alignment << " bytes");
			memset(buffer, '\0', capacity);
			return buffer;
		}

		static constexpr void freeHeapBuffer(char* buffer) {
			if (std::is_constant_evaluated()) {
				delete[] buffer;
			}
			else {
				constexpr uint64 alignment = 64;
				gk_assertm(gk::isAligned(buffer, alignment), "Cannot free non-aligned buffer. Required alignment to free is " << alignment << " bytes");
				_aligned_free(buffer);
			}
		}

		static constexpr forceinline void copyChars(char* destination, const char* source, const uint64 num) {
			if (std::is_constant_evaluated()) {
				for (uint64 i = 0; i < num; i++) {
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

#pragma region Comparison_Dynamic_Dispatch

		typedef bool (*FuncCmpEqStringAndStr)(const String&, const Str&);
		typedef bool (*FuncCmpEqStringAndString)(const String&, const String&);

		[[nodiscard]] static bool avx512CompareEqualHeapBufferAndStr(const String& self, const Str& str) {
			constexpr uint64 equal64Bitmask = ~0;

			const __m512i* thisVec = reinterpret_cast<const __m512i*>(self._rep._heap._buffer);

			__m512i otherVec = _mm512_set1_epi8('\0');
			uint64 i = 0;
			const uint64 bytesToCheck = str.totalBytes - 1;
			if (bytesToCheck >= 64) {
				for (; i <= bytesToCheck - 64; i += 64) {
					memcpy(&otherVec, str.str + i, 64);
					if (_mm512_cmpeq_epi8_mask(*thisVec, otherVec) != equal64Bitmask) return false;
					thisVec++;
				}
			}

			for (; i < bytesToCheck; i++) {
				if (self._rep._heap._buffer[i] != str.str[i]) return false;
			}
			return true;
		}

		[[nodiscard]] static bool avx512CompareEqualHeapBufferAndOtherStringHeapBuffer(const String& self, const String& other) {
			constexpr uint64 equal64Bitmask = ~0;
			const __m512i* thisVec = reinterpret_cast<const __m512i*>(self._rep._heap._buffer);
			const __m512i* otherVec = reinterpret_cast<const __m512i*>(other._rep._heap._buffer);

			uint64 remainder = (self._rep._heap._bytesOfBufferUsed + 1) % 64;
			const uint64 bytesToCheck = remainder ? ((self._rep._heap._bytesOfBufferUsed + 1) + (64 - remainder)) : self._rep._heap._bytesOfBufferUsed + 1;
			for (uint64 i = 0; i < bytesToCheck; i += 64) {
				if (_mm512_cmpeq_epi8_mask(*thisVec, *otherVec) != equal64Bitmask) return false;
				thisVec++;
				otherVec++;
			}
			return true;
		}

		[[nodiscard]] static bool avx2CompareEqualHeapBufferAndStr(const String& self, const Str& str) {
			constexpr uint32 equal32Bitmask = ~0;

			const __m256i* thisVec = reinterpret_cast<const __m256i*>(self._rep._heap._buffer);

			__m256i otherVec = _mm256_set1_epi8('\0');
			uint64 i = 0;
			const uint64 bytesToCheck = str.totalBytes - 1;
			for (; i <= bytesToCheck - 32; i += 32) {
				memcpy(&otherVec, str.str + i, 32);
				if (_mm256_cmpeq_epi8_mask(*thisVec, otherVec) != equal32Bitmask) return false;
				thisVec++;
			}

			for (; i < bytesToCheck; i++) {
				if (self._rep._heap._buffer[i] != str.str[i]) return false;
			}
			return true;
		}

		[[nodiscard]] static bool avx2CompareEqualHeapBufferAndOtherStringHeapBuffer(const String& self, const String& other) {
			constexpr uint32 equal32Bitmask = ~0;
			const __m256i* thisVec = reinterpret_cast<const __m256i*>(self._rep._heap._buffer);
			const __m256i* otherVec = reinterpret_cast<const __m256i*>(other._rep._heap._buffer);

			uint64 remainder = (self._rep._heap._bytesOfBufferUsed + 1) % 32;
			const uint64 bytesToCheck = remainder ? ((self._rep._heap._bytesOfBufferUsed + 1) + (32 - remainder)) : self._rep._heap._bytesOfBufferUsed + 1;
			for (uint64 i = 0; i < bytesToCheck; i += 64) {
				if (_mm256_cmpeq_epi8_mask(*thisVec, *otherVec) != equal32Bitmask) return false;
				thisVec++;
				otherVec++;
			}
			return true;
		}

		[[nodiscard]] static FuncCmpEqStringAndStr loadOptimalCmpEqFuncStringAndStr() {
			if (gk::x86::isAvx512Supported()) {
				std::cout << "[String function loader]: Using AVX-512 String-Str comparison\n";
				return avx512CompareEqualHeapBufferAndStr;
			}
			else if(gk::x86::isAvx2Supported()) {
				std::cout << "[String function loader]: Using AVX-2 String-Str comparison\n";
				return avx2CompareEqualHeapBufferAndStr;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load string comparison functions if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}

		[[nodiscard]] static FuncCmpEqStringAndString loadOptimalCmpEqFuncStringAndString() {
			if (gk::x86::isAvx512Supported()) {
				std::cout << "[String function loader]: Using AVX-512 String-String comparison\n";
				return avx512CompareEqualHeapBufferAndOtherStringHeapBuffer;
			}
			else if(gk::x86::isAvx2Supported()) {
				std::cout << "[String function loader]: Using AVX-2 String-String comparison\n";
				return avx2CompareEqualHeapBufferAndOtherStringHeapBuffer;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load string comparison functions if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}

		/* Dynamic dispatch. Will use AVX-512 version if supported, or will default to AVX-2. */
		static inline FuncCmpEqStringAndStr cmpEqStringStr = loadOptimalCmpEqFuncStringAndStr();

		/* Dynamic dispatch. Will use AVX-512 version if supported, or will default to AVX-2. */
		static inline FuncCmpEqStringAndString cmpEqStringString = loadOptimalCmpEqFuncStringAndString();

#pragma endregion

#pragma region Find_Dynamic_Dispatch

		typedef Option<StringIndex>(*FuncFindCharInString)(const String&, const char);

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanForward)

		[[nodiscard]] static Option<StringIndex> avx512FindCharInString(const String& self, const char c) {
			const __m512i vecChar = _mm512_set1_epi8(c);
			const __m512i* vecThis = reinterpret_cast<const __m512i*>(self._rep._heap._buffer);
			const uint64 iterationsToDo = ((self._rep._heap._bytesOfBufferUsed) % 64 == 0 ? 
				self._rep._heap._bytesOfBufferUsed : 
				self._rep._heap._bytesOfBufferUsed + (64 - (self._rep._heap._bytesOfBufferUsed % 64))) 
				/ 64;

			for (uint64 i = 0; i < iterationsToDo; i++) {
				const uint64 bitmask = _mm512_cmpeq_epi8_mask(vecChar, *vecThis);
				if (bitmask == 0) { // nothing equal
					vecThis++;
					continue;
				}
				unsigned long index;
				_BitScanForward64(&index, bitmask);
				return Option<StringIndex>(static_cast<uint64>(index) + (i * 64));
			}
			return Option<StringIndex>();
		}

		[[nodiscard]] static Option<StringIndex> avx2FindCharInString(const String& self, const char c) {
			const __m256i vecChar = _mm256_set1_epi8(c);
			const __m256i* vecThis = reinterpret_cast<const __m256i*>(self._rep._heap._buffer);
			const uint64 iterationsToDo = ((self._rep._heap._bytesOfBufferUsed) % 32 == 0 ? 
				self._rep._heap._bytesOfBufferUsed : 
				self._rep._heap._bytesOfBufferUsed + (32 - (self._rep._heap._bytesOfBufferUsed % 32))) 
				/ 32;

			for (uint64 i = 0; i < iterationsToDo; i++) {
				const uint32 bitmask = _mm256_cmpeq_epi8_mask(vecChar, *vecThis);
				if (bitmask == 0) { // nothing equal
					vecThis++;
					continue;
				}
				unsigned long index;
				_BitScanForward(&index, bitmask);
				return Option<StringIndex>(static_cast<uint64>(index) + (i * 32));
			}
			return Option<StringIndex>();
		}

		[[nodiscard]] static FuncFindCharInString loadOptimalFindCharInStringFunction() {
			if (gk::x86::isAvx512Supported()) {
				std::cout << "[String function loader]: Using AVX-512 find char in string\n";
				return avx512FindCharInString;
			}
			else if (gk::x86::isAvx2Supported()) {
				std::cout << "[String function loader]: Using AVX-2 find char in string\n";
				return avx2FindCharInString;
			}
			else {
				std::cout << "[String function loader]: ERROR\nCannot load find char in string function if AVX-512 or AVX-2 aren't supported\n";
				abort();
			}
		}

		/* Dynamic dispatch. Will use AVX-512 version if supported, or will default to AVX-2. */
		static inline FuncFindCharInString findCharInStringFunc = loadOptimalFindCharInStringFunction();

#pragma endregion

#pragma region Format_Functionality

			struct FormatStringStrOffset {
				const char* start;
				uint64 count;
			};

			consteval static bool formatStrIsValid(const gk::Str& formatStr) {
				uint64 count = 0;
				for (uint64 i = 0; i < formatStr.totalBytes - 1; i++) {
					if (formatStr.str[i] == '{') {
						if (formatStr.str[i + 1] != '}') {
							return false;
						}
						count++;
					}
				}
				return true;
			}

			consteval static uint64 formatStrCountArgs(const gk::Str& formatStr) {
				uint64 count = 0;
				for (uint64 i = 0; i < formatStr.totalBytes - 1; i++) {
					if (formatStr.str[i] == '{') count++;
				}
				return count;
			}

			template<gk::Str formatStr>
			constexpr static void formatStrFindOffsets(FormatStringStrOffset* offsetsArr, const uint64 bufferSize) {
				uint64 currentOffset = 0;
				uint64 offsetIndex = 0;
				for (uint64 i = 0; i < formatStr.totalBytes - 1; i++) {
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

	};
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