#pragma once

#include "../doctest/doctest_proxy.h"
#include "../utility.h"
#include "../allocator/allocator.h"
#include "../option/option.h"
#include <type_traits>
#include "../error/result.h"

namespace gk
{
	namespace internal 
	{
		constexpr static usize ARRAY_LIST_SIMD_T_MALLOC_ALIGN = 64;

		constexpr AllocatorRef arrayListDefaultAllocator() {
			if (std::is_constant_evaluated()) {
				return AllocatorRef();
			}
			else {
				return gk::globalHeapAllocatorRef();
			}

		}

		Option<usize> doSimdArrayElementFind1Byte(const i8* arrayListData, usize length, i8 toFind);
		Option<usize> doSimdArrayElementFind2Byte(const i16* arrayListData, usize length, i16 toFind);
		Option<usize> doSimdArrayElementFind4Byte(const i32* arrayListData, usize length, i32 toFind);
		Option<usize> doSimdArrayElementFind8Byte(const i64* arrayListData, usize length, i64 toFind);

		template<typename T>
		forceinline static gk::Option<usize> doSimdArrayElementFind(const T* arrayListData, usize length, T toFind) {
			if constexpr (sizeof(T) == 1) {
				return doSimdArrayElementFind1Byte(reinterpret_cast<const i8*>(arrayListData), length, static_cast<i8>(toFind));
			}
			else if constexpr (sizeof(T) == 2) {
				return doSimdArrayElementFind2Byte(reinterpret_cast<const i16*>(arrayListData), length, static_cast<i16>(toFind));
			}
			else if constexpr (sizeof(T) == 4) {
				return doSimdArrayElementFind4Byte(reinterpret_cast<const i32*>(arrayListData), length, static_cast<i32>(toFind));
			}
			else {
				return doSimdArrayElementFind8Byte(reinterpret_cast<const i64*>(arrayListData), length, static_cast<i64>(toFind));
			}
		}

	}

	template<typename T>
	struct ArrayList
	{
	private:

		constexpr static bool IS_T_SIMD = (std::is_arithmetic_v<T> || std::is_pointer_v<T> || std::is_enum_v<T>);

		/**
		* Simple constructor to initialize the ArrayList with a specified allocator.
		* For actual use, call ArrayList::init() for whichever overload necessary.
		*/
		ArrayList(AllocatorRef&& inAllocator) : _data(nullptr), _length(0), _capacity(0), _allocator(std::move(inAllocator)) {}

	public:

		using ValueType = T;

		/**
		* Default constructor of the ArrayList. In a constexpr context, no allocator is used.
		* In a runtime context, gk::globalHeapAllocator() is used.
		*/
		constexpr ArrayList();

		/**
		* The copy constructor of ArrayList will make a clone of the other's allocator.
		* Beware of running out of memory in a custom allocator from this behaviour.
		* Requires that T is copy assignable.
		* 
		* @param other: Other ArrayList to copy elements and allocator from.
		*/
		constexpr ArrayList(const ArrayList& other);

		/**
		* During move construction, the other ArrayList will be fully invalidated.
		* The other will have it's data and allocator set to null/invalid.
		* 
		* @param other: Other ArrayList to take ownership of it's held data.
		*/
		constexpr ArrayList(ArrayList&& other) noexcept;		

		/**
		* Upon destruction, with a valid allocator, it will have it's ref count decremented.
		*/
		constexpr ~ArrayList();

		/**
		* The copy assignment operator of ArrayList will make a clone of the other's allocator.
		* Beware of running out of memory in a custom allocator from this behaviour.
		* Requires that T is copy assignable.
		* 
		* @param other: Other ArrayList to copy elements and allocator from.
		*/
		constexpr ArrayList& operator = (const ArrayList& other);

		/**
		* During move assignment, the other ArrayList will be fully invalidated.
		* The other will have it's data and allocator set to null/invalid.
		* The old allocator held by this ArrayList will have it's ref count decremented.
		*
		* @param other: Other ArrayList to take ownership of it's held data.
		*/
		constexpr ArrayList& operator = (ArrayList&& other) noexcept;

		/**
		* Create a new ArrayList given an allocator to take ownership of.
		* 
		* @param inAllocator: Allocator to own
		*/
		[[nodiscard]] static ArrayList init(AllocatorRef&& inAllocator) { return ArrayList(std::move(inAllocator)); }

		/**
		* Creates a new ArrayList given an allocator to take ownership of, and another ArrayList to copy elements.
		* 
		* @param inAllocator: Allocator to own
		* @param other: other ArrayList to copy from
		*/
		[[nodiscard]] static ArrayList initCopy(AllocatorRef&& inAllocator, const ArrayList& other);

		/**
		* Creates a new ArrayList given an allocator to take ownership of, and an initializer list to copy elements.
		* 
		* @param inAllocator: Allocator to own
		* @param initializerList: Elements to copy
		*/
		[[nodiscard]] static ArrayList initList(AllocatorRef&& inAllocator, const std::initializer_list<T>& initializerList);

		/**
		* Creates a new ArrayList given an allocator to take ownership of, 
		* a pointer to some data, and a number of elements to copy from the pointer.
		* 
		* @param inAllocator: Allocator to own
		* @param ptr: Start of buffer of elements to copy
		* @param elementsToCopy: Number to copy. Accessing beyond the bounds of `ptr` is undefined behaviour.
		*/
		[[nodiscard]] static ArrayList initBufferCopy(AllocatorRef&& inAllocator, const T* buffer, usize elementsToCopy);
		
		/**
		* Create a new ArrayList given an allocator to take ownership of, and a pre reserved minimum capacity.
		* The capacity of the new ArrayList will be greater than or equal to `minCapacity`.
		*
		* @param inAllocator: Allocator to to take ownership of
		* @param minCapacity: Minimum allocation size
		*/
		[[nodiscard]] static ArrayList withCapacity(AllocatorRef&& inAllocator, usize minCapacity);

		/**
		* Create a new ArrayList given an allocator to take ownership of, a pre reserved minimum capacity,
		* and another ArrayList to copy elements.
		* The capacity of the new ArrayList will be greater than or equal to the max value between
		* `minCapacity` and `other.len()`.
		*
		* @param inAllocator: Allocator to take ownership of
		* @param minCapacity: Minimum allocation size
		* @param other: other ArrayList to copy from
		*/
		[[nodiscard]] static ArrayList withCapacityCopy(AllocatorRef&& inAllocator, usize minCapacity, const ArrayList& other);

		/**
		* Create a new ArrayList given an allocator to take ownership of, a pre reserved minimum capacity,
		* and an initializer list to copy elements.
		* The capacity of the new ArrayList will be greater than or equal to the max value between
		* `minCapacity` and `initializerList.size()`.
		*
		* @param inAllocator: Allocator to take ownership of
		* @param minCapacity: Minimum allocation size
		* @param initializerList: Elements to copy
		*/
		[[nodiscard]] static ArrayList withCapacityList(AllocatorRef&& inAllocator, usize minCapacity, const std::initializer_list<T>& initializerList);

		/**
		* Create a new ArrayList given an allocator to take ownership of, a pre reserved minimum capacity,
		* a pointer to some data, and a number of elements to copy from the pointer.
		* The capacity of the new ArrayList will be greater than or equal to the max value between
		* `minCapacity` and `elementsToCopy`.
		*
		* @param inAllocator: Allocator to clone
		* @param minCapacity: Minimum allocation size
		* @param ptr: Start of buffer of elements to copy
		* @param elementsToCopy: Number to copy. Accessing beyond the bounds of `ptr` is undefined behaviour.
		*/
		[[nodiscard]] static ArrayList withCapacityBufferCopy(AllocatorRef&& inAllocator, usize minCapacity, const T* buffer, usize elementsToCopy);

		/**
		* The number of elements contained in the ArrayList.
		*/
		[[nodiscard]] constexpr usize len() const { return _length; }

		/**
		* The number of elements this ArrayList can store without reallocation.
		*/
		[[nodiscard]] constexpr usize capacity() const { return _capacity; }

		/**
		* A mutable pointer to the data held by this ArrayList. Accessing beyond `len()`. is undefined behaviour.
		*/
		[[nodiscard]] constexpr T* data() { return _data; }

		/**
		* An immutable pointer to the data held by this ArrayList. Accessing beyond `len()`. is undefined behaviour.
		*/
		[[nodiscard]] constexpr const T* data() const { return _data; }

		/**
		* In constexpr, no allocator is used, so it does not make sense for this function
		* to be constexpr.
		* 
		* @return The allocator used by the ArrayList. Can be copied.
		*/
		[[nodiscard]] const AllocatorRef& allocator() const { return _allocator; }

		/**
		* Get a mutable reference to an element in the array at a specified index.
		* Will assert if index is out of range.
		* 
		* @param index: The element to get. Asserts that is less than `len()`.
		*/
		[[nodiscard]] constexpr T& operator [] (usize index);

		/**
		* Get an immutable reference to an element in the array at a specified index.
		* Will assert if index is out of range.
		*
		* @param index: The element to get. Asserts that is less than `len()`.
		*/
		[[nodiscard]] constexpr const T& operator [] (usize index) const;

		/**
		* Pushes a copy of `element` onto the end of the ArrayList, increasing the length by 1.
		* May reallocate if there isn't enough capacity already. Requires that T is copyable.
		* 
		* @param element: Element to be copied to the end of the ArrayList buffer.
		*/
		constexpr void push(const T& element);

		/**
		* Moves `element` onto the end of the ArrayList, increasing the length by 1.
		* May reallocate if there isn't enough capacity already.
		* 
		* @param element: Element to be moved to the end of the ArrayList buffer.
		*/
		constexpr void push(T&& element);

		/**
		* Reserves additional capacity in the ArrayList. The new capacity will be greater than or equal to `len()` + `additional`.
		* May allocate more to avoid frequent reallocation, or any SIMD requirements.
		* Use `reserveExact()` if you know extra allocation won't happen.
		* 
		* @param additional: Minimum amount to increase the capacity by
		*/
		constexpr void reserve(usize additional);

		/**
		* Reserves additional capacity in the ArrayList. The new capacity will be greater than or equal to `len()` + `additional`.
		* Will allocate the smallest amount possible to satisfy the reserve, and potential SIMD requirements.
		* Use `reserve()` if more allocations will happen.
		* 
		* @param additional: Minimum amount to increase the capacity by
		*/
		constexpr void reserveExact(usize additional);

		/**
		* Finds the first index of an element in the ArrayList. For data types that supports it, will use SIMD to find.
		* The index will be returned if it exists, or None if it doesn't.
		* 
		* @param element: Element to check if in the ArrayList.
		* @return The found index, or None
		*/
		[[nodiscard]] constexpr gk::Option<usize> find(const T& element) const;

		/**
		* Removes the element at `index` and returns it, shuffling down all subsequent elements
		* to maintain order. If order is not required, use `swapRemove()` as it will perform better in general.
		* 
		* @param index: The element to remove. Asserts that is less than `len()`.
		* @return The removed element. Can be ingored.
		*/
		constexpr T remove(usize index);

		/**
		* Removes the element at `index`, swapping the last element in place.
		* Does not maintain order. Due to not shuffling elements, in general, performance
		* is superior to `remove()`. If order needs to be maintained, use `remove()`.
		* 
		* @param index: The element to remove. Asserts that is less than `len()`.
		* @return The removed element. Can be ingored.
		*/
		constexpr T swapRemove(usize index);
		
		/**
		* Insert a copy of `element` at `index`, shuffling up all subsequent elements to maintain order.
		* If order is not required use `insertSwap()` as it will perform better in general.
		*
		* @param index: Where to insert a copy of `element`. Asserts that is less than or equal to `len()`.
		* @param element: The element to copy and insert at `index`.
		*/
		constexpr void insert(usize index, const T& element);
		
		/**
		* Insert `element` at `index`, shuffling up all subsequent elements to maintain order.
		* If order is not required use `insertSwap()` as it will perform better in general.
		* 
		* @param index: Where to insert `element`. Asserts that is less than or equal to `len()`.
		* @param element: The element to insert at `index`.
		*/
		constexpr void insert(usize index, T&& element);

		/**
		* Insert copt of `element` at `index`, swapping the current element at `index` to the
		* end of the ArrayList if it's not the last element. Does not maintain order.
		* If order is required, use `insert()`.
		*
		* @param index: Where to insert a copy of `element`. Asserts that is less than or equal to `len()`.
		* @param element: The element to copy and insert at `index`.
		*/
		constexpr void insertSwap(usize index, const T& element);
		
		/**
		* Insert `element` at `index`, swapping the current element at `index` to the 
		* end of the ArrayList if it's not the last element. Does not maintain order.
		* If order is required, use `insert()`.
		* 
		* @param index: Where to insert `element`. Asserts that is less than or equal to `len()`.
		* @param element: The element to insert at `index`.
		*/
		constexpr void insertSwap(usize index, T&& element);
		
		/**
		* Reallocates the ArrayList to occupy the minimum amount of memory required to store
		* `len()` elements. For SIMD types, will do the minimum allocation for SIMD requirements,
		* and the number of elements.
		*/
		constexpr void shrinkToFit();

		/**
		* Reallocates the ArrayList to occupy the minimum amount of memory required to store
		* either `len()` elements, or `minCapacity` elements, whichever is greater.
		* 
		* @param minCapacity: Minimum allocation size to shrink to, provided it is greater than `len()`.
		*/
		constexpr void shrinkTo(usize minCapacity);

		/**
		* Shortens the length of the ArrayList, keeping the first `newLength` elements,
		* and destructing the rest. If `newLength` is greater than or equal to the ArrayList's current
		* `len()`, this function does nothing.
		* This function has no effect on the allocated capacity of the ArrayList.
		* 
		* @param newLength: Number of elements to keep.
		*/
		constexpr void truncate(usize newLength);

		/**
		* Appends a copy of another ArrayList's elements to the end of this ArrayList.
		* 
		* @param other: ArrayList to copy elements from.
		*/
		constexpr void appendCopy(const ArrayList& other);

		/**
		* Appends the elements in an initializer list to the end of this ArrayList.
		* 
		* @param initializeList: Elements to copy.
		*/
		constexpr void appendList(const std::initializer_list<T>& initializerList);

		/**
		* Appends the data held at `buffer` to the end of this ArrayList.
		* `buffer` must be non-null, and be valid up to `buffer[elementsToCopy - 1].
		* If `elementsToCopy` is out of the buffer's range, this function will execute undefined behaviour.
		* 
		* @param buffer: Non null pointer of T's to copy.
		* @param elementsToCopy: Total number of elements to copy from `buffer`.
		*/
		constexpr void appendBufferCopy(const T* buffer, usize elementsToCopy);

		/**
		* Resizes the ArrayList in-place so that `len()` is equal to `newLength`.
		* If `newLength` is greater than `len()`, this ArrayList is extended by the difference, with
		* each additional slot filled with `fill`. If `newLength` is less than `len()`, the ArrayList
		* is truncated.
		* 
		* @param newLength: New length of the ArrayList.
		* @param fill: Object to copy into all additional slots if `newLength` is greater than `len()`.
		*/
		constexpr void resize(usize newLength, const T& fill);

		struct Iterator;
		struct ConstIterator;
		struct ReverseIterator;
		struct ReverseConstIterator;

		/**
		* Beginning of an iterator over the ArrayList's elements with mutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		* 
		* @return Begin of a mutable iterator.
		*/
		constexpr Iterator begin();

		/**
		* End of an iterator over the ArrayList's elements with mutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return End of a mutable iterator.
		*/
		constexpr Iterator end();

		/**
		* Beginning of an iterator over the ArrayList's elements with immutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return Begin of an immutable iterator.
		*/
		constexpr ConstIterator begin() const;

		/**
		* End of an iterator over the ArrayList's elements with immutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return End of an immutable iterator.
		*/
		constexpr ConstIterator end() const;

		/**
		* Beginning of a reverse iterator (starts from the last element) over the ArrayList's elements with mutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return Begin of a mutable reverse iterator.
		*/
		constexpr ReverseIterator rbegin();

		/**
		* End of a reverse iterator (ends at the first element) over the ArrayList's elements with mutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return Begin of a mutable reverse iterator.
		*/
		constexpr ReverseIterator rend();

		/**
		* Beginning of a reverse iterator (starts from the last element) over the ArrayList's elements with immutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return Begin of a immutable reverse iterator.
		*/
		constexpr ReverseConstIterator rbegin() const;

		/**
		* End of a reverse iterator (ends at the first element) over the ArrayList's elements with immutable access.
		* Any mutation operations to the ArrayList can be assumed to invalidate the iterator.
		*
		* @return Begin of a immutable reverse iterator.
		*/
		constexpr ReverseConstIterator rend() const;

		struct Iterator {
			constexpr Iterator(T* inData) : arrayData(inData) {}
			constexpr bool operator ==(const Iterator& other) const { return this->arrayData == other.arrayData; }
			constexpr T& operator*() const { return *arrayData; }
			constexpr Iterator& operator++() { arrayData++; return *this; }
		private:
			T* arrayData;
		};

		struct ConstIterator {
			constexpr ConstIterator(const T* inData) : arrayData(inData) {}
			constexpr bool operator ==(const ConstIterator& other) const { return this->arrayData == other.arrayData; }
			constexpr const T& operator*() const { return *arrayData; }
			constexpr ConstIterator& operator++() { arrayData++; return *this; }
		private:
			const T* arrayData;
		};

		struct ReverseIterator {
			constexpr ReverseIterator(T* inData) : arrayData(inData) {}
			constexpr bool operator ==(const ReverseIterator& other) const { return this->arrayData == other.arrayData; }
			constexpr T& operator*() const { return *arrayData; }
			constexpr ReverseIterator& operator++() { arrayData--; return *this; }
		private:
			T* arrayData;
		};

		struct ReverseConstIterator {
			constexpr ReverseConstIterator(const T* inData) : arrayData(inData) {}
			constexpr bool operator ==(const ReverseConstIterator& other) const { return this->arrayData == other.arrayData; }
			constexpr const T& operator*() const { return *arrayData; }
			constexpr ReverseConstIterator& operator++() { arrayData--; return *this; }
		private:
			const T* arrayData;
		};

	private:

		constexpr void deleteExistingBuffer() {
			if (_data == nullptr) return;

			if (!std::is_constant_evaluated()) {
				for (usize i = 0; i < _length; i++) {
					_data[i].~T();
				}
			}
			
			freeArrayListBuffer(_data, _capacity, _allocator);
		}

		/**
		* At runtime, creates a 0 initialized buffer.
		* At comptime, just makes a new heap array.
		*/
		constexpr static T* mallocArrayListBuffer(usize* requiredCapacity, AllocatorRef& allocatorToUse) {
			if (std::is_constant_evaluated()) {
				const usize capacity = *requiredCapacity;
				return new T[capacity];
			}

			T* outBuffer;
			if constexpr (IS_T_SIMD) {
				constexpr usize numPerSimd = 64 / sizeof(T);
				const usize remainder = *requiredCapacity % numPerSimd;
				if (remainder != 0) {
					*requiredCapacity = *requiredCapacity + (numPerSimd - remainder);
				}
				outBuffer = allocatorToUse.mallocAlignedBuffer<T>(*requiredCapacity, internal::ARRAY_LIST_SIMD_T_MALLOC_ALIGN).ok();
				// Likely dont need to memset cause no constructor tomfoolery
				//memset(outBuffer, 0, (*requiredCapacity) * sizeof(T));
				return outBuffer;
			}

			outBuffer = allocatorToUse.mallocBuffer<T>(*requiredCapacity).ok();
			memset(outBuffer, 0, (*requiredCapacity) * sizeof(T));
			return outBuffer;
		}

		constexpr static void freeArrayListBuffer(T*& buffer, usize bufferCapacity, AllocatorRef& allocatorToUse) {
			if (std::is_constant_evaluated()) {
				delete[] buffer;
				return;
			}

			if constexpr (IS_T_SIMD) {
				allocatorToUse.freeAlignedBuffer(buffer, bufferCapacity, internal::ARRAY_LIST_SIMD_T_MALLOC_ALIGN);
				return;
			}
			allocatorToUse.freeBuffer(buffer, bufferCapacity);
		}

		constexpr void reallocate(usize capacity) {
			const usize currentLength = _length;
			T* newData = mallocArrayListBuffer(&capacity, _allocator);

			for (usize i = 0; i < currentLength; i++) {
				// TODO see if assignment is better.
				if (std::is_constant_evaluated()) {
					newData[i] = std::move(_data[i]);
					//_data[i] = T(); // ensure upon delete[], no weird destructor stuff happens.
				}
				else {
					new (newData + i) T(std::move(_data[i]));
				}
			}
			if (_data != nullptr) {
				freeArrayListBuffer(_data, _capacity, _allocator);
			}

			_data = newData;
			_capacity = capacity;
		}

	private:

		T* _data;
		usize _length;
		usize _capacity;
		AllocatorRef _allocator;
		
	}; // struct ArrayList
} // namespace gk

template<typename T>
inline constexpr gk::ArrayList<T>::ArrayList()
	: _data(nullptr), _length(0), _capacity(0), _allocator(internal::arrayListDefaultAllocator())
{}

template<typename T>
inline constexpr gk::ArrayList<T>::ArrayList(const ArrayList& other)
	: _length(other._length), _allocator(other._allocator)
{
	if (_length == 0) {
		_data = nullptr;
		_capacity = 0;
		return;
	}

	usize requiredCapacity = _length;
	_data = mallocArrayListBuffer(&requiredCapacity, _allocator);
	_capacity = requiredCapacity;

	for (usize i = 0; i < _length; i++) {
		_data[i] = other._data[i];
	}
}

template<typename T>
inline constexpr gk::ArrayList<T>::ArrayList(ArrayList&& other) noexcept
	: _data(other._data), _length(other._length), _capacity(other._capacity), _allocator(std::move(other._allocator))
{
	other._data = nullptr;
}

template<typename T>
inline constexpr gk::ArrayList<T>::~ArrayList()
{
	deleteExistingBuffer();
}

template<typename T>
inline constexpr gk::ArrayList<T>& gk::ArrayList<T>::operator=(const ArrayList& other)
{
	deleteExistingBuffer();
	check_eq(_data, nullptr);
	_length = other._length;
	_allocator = other._allocator.clone();
	if (_length == 0) {
		_data = nullptr;
		_capacity = 0;
		return *this;
	}

	usize requiredCapacity = _length;
	_data = mallocArrayListBuffer(&requiredCapacity, _allocator);
	_capacity = requiredCapacity;

	for (usize i = 0; i < _length; i++) {
		_data[i] = other._data[i];
	}
	return *this;
}

template<typename T>
inline constexpr gk::ArrayList<T>& gk::ArrayList<T>::operator=(ArrayList&& other) noexcept
{
	deleteExistingBuffer();
	_data = other._data;
	_length = other._length;
	_capacity = other._capacity;
	_allocator = std::move(other._allocator);
	other._data = nullptr;
	return *this;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::initCopy(AllocatorRef&& inAllocator, const ArrayList& other)
{
	ArrayList out = ArrayList(std::move(inAllocator));
	out._length = other._length;
	if (out._length == 0) {
		out._data = nullptr;
		out._capacity = 0;
		return out;
	}

	usize requiredCapacity = out._length;
	out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
	out._capacity = requiredCapacity;
	for (usize i = 0; i < out._length; i++) {
		new (out._data + i) T(other._data[i]);
	}
	return out;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::initList(AllocatorRef&& inAllocator, const std::initializer_list<T>& initializerList)
{
	ArrayList out = ArrayList(std::move(inAllocator));
	out._length = initializerList.size();
	if (out._length == 0) {
		out._data = nullptr;
		out._capacity = 0;
		return out;
	}

	usize requiredCapacity = out._length;
	out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
	out._capacity = requiredCapacity;

	usize i = 0;
	for (const auto& elem : initializerList) {
		new (out._data + i) T(elem);
		i++;
	}
	return out;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::initBufferCopy(AllocatorRef&& inAllocator, const T* buffer, usize elementsToCopy)
{
	ArrayList out = ArrayList(std::move(inAllocator));

	out._length = elementsToCopy;
	if (out._length == 0) {
		out._data = nullptr;
		out._capacity = 0;
		return out;
	}

	usize requiredCapacity = out._length;
	out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
	out._capacity = requiredCapacity;
	for (usize i = 0; i < elementsToCopy; i++) {
		new (out._data + i) T(buffer[i]);
	}

	return out;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::withCapacity(AllocatorRef&& inAllocator, usize minCapacity)
{
	ArrayList out = ArrayList(std::move(inAllocator));
	if (minCapacity == 0) [[unlikely]] {
		return out;
	}

	out._data = mallocArrayListBuffer(&minCapacity, out._allocator);
	out._capacity = minCapacity;
	return out;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::withCapacityCopy(AllocatorRef&& inAllocator, usize minCapacity, const ArrayList& other)
{
	ArrayList out = ArrayList(std::move(inAllocator));
	out._length = other._length;

	if (out._length == 0 && minCapacity == 0) [[unlikely]] {
		out._data = nullptr;
		out._capacity = 0;
		return out;
	}

	usize requiredCapacity = out._length > minCapacity ? out._length : minCapacity;
	out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
	out._capacity = requiredCapacity;
	for (usize i = 0; i < out._length; i++) {
		new (out._data + i) T(other._data[i]);
	}
	return out;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::withCapacityList(AllocatorRef&& inAllocator, usize minCapacity, const std::initializer_list<T>& initializerList)
{
	ArrayList out = ArrayList(std::move(inAllocator));
	out._length = initializerList.size();

	if (out._length == 0 && minCapacity == 0) [[unlikely]] {
		out._data = nullptr;
		out._capacity = 0;
		return out;
	}

	usize requiredCapacity = out._length > minCapacity ? out._length : minCapacity;
	out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
	out._capacity = requiredCapacity;

	usize i = 0;
	for (const auto& elem : initializerList) {
		new (out._data + i) T(elem);
		i++;
	}
	return out;
}

template<typename T>
inline gk::ArrayList<T> gk::ArrayList<T>::withCapacityBufferCopy(AllocatorRef&& inAllocator, usize minCapacity, const T* buffer, usize elementsToCopy)
{
	ArrayList out = ArrayList(std::move(inAllocator));
	out._length = elementsToCopy;

	if (out._length == 0 && minCapacity == 0) [[unlikely]] {
		out._data = nullptr;
		out._capacity = 0;
		return out;
	}

	usize requiredCapacity = out._length > minCapacity ? out._length : minCapacity;
	out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
	out._capacity = requiredCapacity;
	for (usize i = 0; i < elementsToCopy; i++) {
		new (out._data + i) T(buffer[i]);
	}

	return out;
}

template<typename T>
inline constexpr T& gk::ArrayList<T>::operator[](usize index)
{
	check_message(index < _length, "Index out of bounds! Attempted to access index ", index, " from ArrayList of length ", _length);
	return _data[index];
}

template<typename T>
inline constexpr const T& gk::ArrayList<T>::operator[](usize index) const
{
	check_message(index < _length, "Index out of bounds! Attempted to access index ", index, " from ArrayList of length ", _length);
	return _data[index];
}

template<typename T>
inline constexpr void gk::ArrayList<T>::push(const T& element)
{
	if (_length == _capacity) {
		reallocate((_capacity + 1) * 2);
	}

	if (std::is_constant_evaluated()) {
		_data[_length] = element;
	}
	else {
		new (_data + _length) T(element);
	}
	_length++;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::push(T&& element)
{
	if (_length == _capacity) {
		reallocate((_capacity + 1) * 2);
	}

	if (std::is_constant_evaluated()) {
		_data[_length] = std::move(element);
	}
	else {
		new (_data + _length) T(std::move(element));
	}
	_length++;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::reserve(usize additional)
{
	const usize addedLength = _length + additional;
	if (addedLength <= _capacity || addedLength == 0) return;

	const usize standardCapacityIncrease = (_capacity + 1) * 2;
	usize newCapacity = addedLength < standardCapacityIncrease ? standardCapacityIncrease : addedLength;

	reallocate(newCapacity);
}

template<typename T>
inline constexpr void gk::ArrayList<T>::reserveExact(usize additional)
{
	const usize newCapacity = _length + additional;
	if (newCapacity <= _capacity || newCapacity == 0) return;

	reallocate(newCapacity);
}

template<typename T>
inline constexpr gk::Option<gk::usize> gk::ArrayList<T>::find(const T& element) const
{
	if (std::is_constant_evaluated() || !IS_T_SIMD) { // constexpr and/or NOT simd
		for (usize i = 0; i < _length; i++) {
			if (_data[i] == element) {
				return gk::Option<usize>(i);
			}
		}
		return gk::Option<usize>();
	}

	return internal::doSimdArrayElementFind(_data, _length, element);
}

template<typename T>
inline constexpr T gk::ArrayList<T>::remove(usize index)
{
	check_message(index < _length, "Index out of bounds! Attempted to removed element index ", index, " from ArrayList of length ", _length);

	T temp = std::move(_data[index]);
	for (usize i = index; i < _length - 1; i++) {
		if (std::is_constant_evaluated()) {
			_data[i] = std::move(_data[i + 1]);
		}
		else {
			new (_data + i) T(std::move(_data[i + 1]));
		}
	}

	_length--;
	return temp;
}

template<typename T>
inline constexpr T gk::ArrayList<T>::swapRemove(usize index)
{
	check_message(index < _length, "Index out of bounds! Attempted to removed element index ", index, " from ArrayList of length ", _length);

	T temp = std::move(_data[index]);
	if (index != (_length - 1)) { // swap the last element in place if it's not the one being removed
		if (std::is_constant_evaluated()) {
			_data[index] = std::move(_data[_length - 1]);
		}
		else {
			new (_data + index) T(std::move(_data[_length - 1]));
		}
	}

	_length--;
	return temp;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::insert(usize index, const T& element)
{
	check_message(index <= _length, "Index out of bounds! Attempted to removed element index ", index, " from ArrayList of length ", _length);

	// TODO investigate handling shuffling with reallocation
	if (_length == _capacity) {
		reallocate((_capacity + 1) * 2);
	}

	for (usize i = index + 1; i < (_length + 1); i++) {
		if (std::is_constant_evaluated()) {
			_data[i] = std::move(_data[i - 1]);
		}
		else {
			new (_data + i) T(std::move(_data[i - 1]));
		}
	}

	if (std::is_constant_evaluated()) {
		_data[index] = element;
	}
	else {
		new (_data + index) T(_data[element]);
	}
	_length++;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::insert(usize index, T&& element)
{
	check_message(index <= _length, "Index out of bounds! Attempted to removed element index ", index, " from ArrayList of length ", _length);

	// TODO investigate handling shuffling with reallocation
	if (_length == _capacity) {
		reallocate((_capacity + 1) * 2);
	}

	for (usize i = index + 1; i < (_length + 1); i++) {
		if (std::is_constant_evaluated()) {
			_data[i] = std::move(_data[i - 1]);
		}
		else {
			new (_data + i) T(std::move(_data[i - 1]));
		}
	}

	if (std::is_constant_evaluated()) {
		_data[index] = std::move(element);
	}
	else {
		new (_data + index) T(std::move(_data[element]));
	}
	_length++;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::insertSwap(usize index, const T& element)
{
	check_message(index <= _length, "Index out of bounds! Attempted to removed element index ", index, " from ArrayList of length ", _length);

	if (index == _length) {
		push(element);
		return;
	}

	// TODO investigate handling shuffling with reallocation
	if (_length == _capacity) {
		reallocate((_capacity + 1) * 2);
	}

	if (std::is_constant_evaluated()) {
		_data[_length] = std::move(_data[index]);
		_data[index] = element;
	}
	else {
		new (_data + _length) T(std::move(_data[index]));
		new (_data + index) T(element);
	}
	_length++;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::insertSwap(usize index, T&& element)
{
	check_message(index <= _length, "Index out of bounds! Attempted to removed element index ", index, " from ArrayList of length ", _length);

	if (index == _length) {
		push(std::move(element));
		return;
	}

	// TODO investigate handling shuffling with reallocation
	if (_length == _capacity) {
		reallocate((_capacity + 1) * 2);
	}

	if (std::is_constant_evaluated()) {
		_data[_length] = std::move(_data[index]);
		_data[index] = std::move(element);
	}
	else {
		new (_data + _length) T(std::move(_data[index]));
		new (_data + index) T(std::move(element));
	}
	_length++;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::shrinkToFit()
{
	reallocate(_length);
}

template<typename T>
inline constexpr void gk::ArrayList<T>::shrinkTo(usize minCapacity)
{
	if (minCapacity > _length) {
		reallocate(minCapacity);
	}
	else {
		reallocate(_length);
	}
}

template<typename T>
inline constexpr void gk::ArrayList<T>::truncate(usize newLength)
{
	if (newLength >= _length) {
		return;
	}

	for (usize i = newLength; i < _length; i++) {
		data[i].~T();
		if (std::is_constant_evaluated()) {
			data[i] = T(); // set to default for how constexpr destructor works
		}
	}
	_length = newLength;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::appendCopy(const ArrayList& other)
{
	if (_length + other._length == _capacity) {
		reallocate(gk::upperPowerOfTwo(_length + other._length));
	}

	for (usize i = 0; i < other._length; i++) {
		if (std::is_constant_evaluated()) {
			_data[_length + i] = other[i];
		}
		else {
			new (_data + _length + i) T(other[i]);
		}
	}
	_length = _length + other._length;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::appendList(const std::initializer_list<T>& initializerList)
{
	if (_length + initializerList.size() == _capacity) {
		reallocate(gk::upperPowerOfTwo(_length + initializerList.size()));
	}

	usize i = 0;
	for (const auto& elem : initializerList) {
		if (std::is_constant_evaluated()) {
			_data[_length + i] = elem;
		}
		else {
			new (_data + _length + i) T(elem);
		}
		i++;
	}
	_length = _length + initializerList.size();
}

template<typename T>
inline constexpr void gk::ArrayList<T>::appendBufferCopy(const T* buffer, usize elementsToCopy)
{
	check_ne(buffer, nullptr);

	if (_length + elementsToCopy == _capacity) {
		reallocate(gk::upperPowerOfTwo(_length + elementsToCopy));
	}

	for (usize i = 0; i < elementsToCopy; i++) {
		if (std::is_constant_evaluated()) {
			_data[_length + i] = buffer[i];
		}
		else {
			new (_data + _length + i) T(buffer[i]);
		}
	}
	_length = _length + elementsToCopy;
}

template<typename T>
inline constexpr void gk::ArrayList<T>::resize(usize newLength, const T& fill)
{
	if (newLength == _length) {
		return;
	}

	if (newLength < _length) {
		truncate(newLength);
		return;
	}

	if (newLength > _capacity) {
		reallocate(gk::upperPowerOfTwo(newLength));
	}

	for (usize i = _length; i < newLength; i++) {
		if (std::is_constant_evaluated()) {
			_data[i] = fill;
		}
		else {
			new (_data + i) T(fill);
		}
	}
	_length = newLength;
}

template<typename T>
inline constexpr gk::ArrayList<T>::Iterator gk::ArrayList<T>::begin()
{
	return Iterator(_data);
}

template<typename T>
inline constexpr gk::ArrayList<T>::Iterator gk::ArrayList<T>::end()
{
	return Iterator(_data + _length);
}

template<typename T>
inline constexpr gk::ArrayList<T>::ConstIterator gk::ArrayList<T>::begin() const
{
	return ConstIterator(_data);
}

template<typename T>
inline constexpr gk::ArrayList<T>::ConstIterator gk::ArrayList<T>::end() const
{
	return ConstIterator(_data + _length);
}

template<typename T>
inline constexpr gk::ArrayList<T>::ReverseIterator gk::ArrayList<T>::rbegin()
{
	return ReverseIterator(_data + _length - 1);
}

template<typename T>
inline constexpr gk::ArrayList<T>::ReverseIterator gk::ArrayList<T>::rend()
{
	return ReverseIterator(_data - 1);
}

template<typename T>
inline constexpr gk::ArrayList<T>::ReverseConstIterator gk::ArrayList<T>::rbegin() const
{
	return ReverseConstIterator(_data + _length - 1);
}

template<typename T>
inline constexpr gk::ArrayList<T>::ReverseConstIterator gk::ArrayList<T>::rend() const
{
	return ReverseConstIterator(_data - 1);
}