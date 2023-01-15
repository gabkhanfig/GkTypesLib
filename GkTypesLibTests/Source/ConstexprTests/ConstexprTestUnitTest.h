#pragma once

#define RUN_TESTS true

#define _pragmsg(s) _Pragma(#s)
#define PRAGMA_MESSAGE(msg) _pragmsg(message(#msg))

struct darrayComplexElement
{
	int* data;
	int count;

	constexpr darrayComplexElement() {
		data = new int[1];
		data[0] = 0;
		count = 1;
	}

	constexpr darrayComplexElement(const darrayComplexElement& other) {
		data = new int[other.count];
		count = other.count;
		for (int i = 0; i < count; i++) {
			data[i] = other.data[i];
		}
	}

	constexpr darrayComplexElement(darrayComplexElement&& other) noexcept {
		data = other.data;
		count = other.count;
		other.data = nullptr;
		other.count = 0;
	}

	constexpr ~darrayComplexElement() {
		if (data) delete[] data;
	}

	constexpr bool operator == (const darrayComplexElement& other) {
		if (count != other.count) {
			return false;
		}

		for (int i = 0; i < count; i++) {
			if (data[i] != other.data[i]) {
				return false;
			}
		}
		return true;
	}

	constexpr bool operator != (const darrayComplexElement& other) {
		return !(*this == other);
	}

	constexpr void operator = (const darrayComplexElement& other) {
		if (data) delete[] data;
		data = new int[other.count];
		count = other.count;
		for (int i = 0; i < count; i++) {
			data[i] = other.data[i];
		}
	}

	constexpr void operator = (darrayComplexElement&& other) noexcept {
		if (data) delete[] data;
		data = other.data;
		count = other.count;
		other.data = nullptr;
		other.count = 0;
	}
};