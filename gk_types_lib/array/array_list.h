#pragma once

#include "../doctest/doctest_proxy.h"
#include "../allocator/allocator.h"
#include "../allocator/heap_allocator.h"
#include "../option/option.h"
#include <type_traits>
#include "../error/result.h"

namespace gk
{
	namespace internal {
		constexpr Allocator arrayListDefaultAllocator() {
			if (std::is_constant_evaluated()) {
				return Allocator();
			}
			else {
				return gk::globalHeapAllocator()->clone();
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
		constexpr static usize SIMD_T_MALLOC_ALIGN = 64;

	private:

		/**
		* Simple constructor to initialize the ArrayList with a specified allocator.
		* For actual use, call ArrayList::init() for whichever overload necessary.
		*/
		ArrayList(Allocator&& inAllocator)
			: _data(nullptr), _length(0), _capacity(0), _allocator(std::move(inAllocator))
		{}

	public:

		using ValueType = T;

#pragma region Construct_Destruct_Assign

		/**
		* Default constructor of the ArrayList. In a constexpr context, no allocator is used.
		* In a runtime context, gk::globalHeapAllocator() is used.
		*/
		constexpr ArrayList()
			: _data(nullptr), _length(0), _capacity(0), _allocator(internal::arrayListDefaultAllocator())
		{}

		/**
		* The copy constructor of ArrayList will make a clone of the other's allocator.
		* Beware of running out of memory in a custom allocator from this behaviour.
		* Requires that T is copy assignable.
		* 
		* @param other: Other ArrayList to copy elements and allocator from.
		*/
		constexpr ArrayList(const ArrayList& other)
			: _length(other._length), _allocator(other._allocator.clone())
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

		/**
		* During move construction, the other ArrayList will be fully invalidated.
		* The other will have it's data and allocator set to null/invalid.
		* 
		* @param other: Other ArrayList to take ownership of it's held data.
		*/
		constexpr ArrayList(ArrayList&& other) noexcept
			: _data(other._data), _length(other._length), _capacity(other._capacity), _allocator(other._allocator)
		{
			other._data = nullptr;
		}

		/**
		* Upon destruction, with a valid allocator, it will have it's ref count decremented.
		*/
		constexpr ~ArrayList() {
			deleteExistingBuffer();
		}

		/**
		* The copy assignment operator of ArrayList will make a clone of the other's allocator.
		* Beware of running out of memory in a custom allocator from this behaviour.
		* Requires that T is copy assignable.
		* 
		* @param other: Other ArrayList to copy elements and allocator from.
		*/
		constexpr ArrayList& operator = (const ArrayList& other) {
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

		/**
		* During move assignment, the other ArrayList will be fully invalidated.
		* The other will have it's data and allocator set to null/invalid.
		* The old allocator held by this ArrayList will have it's ref count decremented.
		*
		* @param other: Other ArrayList to take ownership of it's held data.
		*/
		constexpr ArrayList& operator = (ArrayList&& other) noexcept {
			deleteExistingBuffer();
			_data = other._data;
			_length = other._length;
			_capacity = other._capacity;
			_allocator = std::move(other._allocator);
			other._data = nullptr;
			return *this;
		}

#pragma endregion

#pragma region Construct_With_Allocator

		/**
		* Create a new ArrayList given an allocator to be cloned.
		* 
		* @param inAllocator: Allocator to clone
		*/
		static ArrayList init(const Allocator& inAllocator) {
			return ArrayList::init(inAllocator.clone());
		}

		/**
		* Create a new ArrayList given an allocator to take ownership of.
		* 
		* @param inAllocator: Allocator to own
		*/
		static ArrayList init(Allocator&& inAllocator) {
			return ArrayList(std::move(inAllocator));
		}

		/**
		* Creates a new ArrayList given an allocator to be cloned, and another ArrayList to copy elements.
		* 
		* @param inAllocator: Allocator to clone
		* @param other: other ArrayList to copy from
		*/
		static ArrayList init(const Allocator& inAllocator, const ArrayList& other) 
		requires(std::is_copy_constructible_v<T>) {
			return ArrayList::init(inAllocator.clone(), other);
		}

		/**
		* Creates a new ArrayList given an allocator to take ownership of, and another ArrayList to copy elements.
		* 
		* @param inAllocator: Allocator to own
		* @param other: other ArrayList to copy from
		*/
		static ArrayList init(Allocator&& inAllocator, const ArrayList& other) 
		requires(std::is_copy_constructible_v<T>) {
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
				//out._data[i] = other._data[i];
				new (out._data + i) T(other._data[i]);
			}
			return out;
		}

		/**
		* Creates a new ArrayList given an allocator to be cloned, and an initializer list to copy elements.
		* 
		* @param inAllocator: Allocator to clone
		* @param initializerList: Elements to copy
		*/
		static ArrayList init(const Allocator& inAllocator, const std::initializer_list<T>& initializerList) 
		requires(std::is_copy_constructible_v<T>) {
			return ArrayList::init(inAllocator.clone(), initializerList);
		}

		/**
		* Creates a new ArrayList given an allocator to take ownership of, and an initializer list to copy elements.
		* 
		* @param inAllocator: Allocator to own
		* @param initializerList: Elements to copy
		*/
		static ArrayList init(Allocator&& inAllocator, const std::initializer_list<T>& initializerList)
		requires(std::is_copy_constructible_v<T>) {
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
				//out._data[i] = elem;
				new (out._data + i) T(elem);
				i++;
			}
			return out;
		}

		/**
		* Creates a new ArrayList given an allocator to be cloned, 
		* a pointer to some data, and a number of elements to copy from the pointer.
		* 
		* @param inAllocator: Allocator to clone
		* @param ptr: Start of buffer of elements to copy
		* @param elementsToCopy: Number to copy. Accessing beyond the bounds of `ptr` is undefined behaviour.
		*/
		static ArrayList init(const Allocator& inAllocator, const T* ptr, usize elementsToCopy)
		requires(std::is_copy_constructible_v<T>) {
			return ArrayList::init(inAllocator.clone(), ptr, elementsToCopy);
		}

		/**
		* Creates a new ArrayList given an allocator to take ownership of, 
		* a pointer to some data, and a number of elements to copy from the pointer.
		* 
		* @param inAllocator: Allocator to own
		* @param ptr: Start of buffer of elements to copy
		* @param elementsToCopy: Number to copy. Accessing beyond the bounds of `ptr` is undefined behaviour.
		*/
		static ArrayList init(Allocator&& inAllocator, const T* ptr, usize elementsToCopy)
		requires(std::is_copy_constructible_v<T>) {
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
				//out._data[i] = ptr[i];
				new (out._data + i) T(ptr[i]);
			}

			return out;
		}

#pragma endregion

#pragma region Construct_With_Allocator_And_Capacity

		/**
		* Create a new ArrayList given an allocator to be cloned, and a pre reserved minimum capacity.
		* The capacity of the new ArrayList will be greater than or equal to `minCapacity`.
		*
		* @param inAllocator: Allocator to clone
		* @param minCapacity: Minimum allocation size
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, usize minCapacity) {
			return ArrayList::withCapacity(inAllocator.clone(), minCapacity);
		}
		
		/**
		* Create a new ArrayList given an allocator to take ownership of, and a pre reserved minimum capacity.
		* The capacity of the new ArrayList will be greater than or equal to `minCapacity`.
		*
		* @param inAllocator: Allocator to to take ownership of
		* @param minCapacity: Minimum allocation size
		*/
		static ArrayList withCapacity(Allocator&& inAllocator, usize minCapacity) {
			ArrayList out = ArrayList(std::move(inAllocator));
			if (minCapacity == 0) [[unlikely]] {
				return out;
			}

			out._data = mallocArrayListBuffer(&minCapacity, out._allocator);
			out._capacity = minCapacity;
			return out;
		}

		/**
		* Create a new ArrayList given an allocator to be cloned, a pre reserved minimum capacity,
		* and another ArrayList to copy elements.
		* The capacity of the new ArrayList will be greater than or equal to the max value between 
		* `minCapacity` and `other.len()`.
		*
		* @param inAllocator: Allocator to clone
		* @param minCapacity: Minimum allocation size
		* @param other: other ArrayList to copy from
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, usize minCapacity, const ArrayList& other)
		requires(std::is_copy_constructible_v<T>) {
			return ArrayList::withCapacity(inAllocator, minCapacity, other);
		}

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
		static ArrayList withCapacity(Allocator&& inAllocator, usize minCapacity, const ArrayList& other)
		requires(std::is_copy_constructible_v<T>) {
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
				//out._data[i] = other._data[i];
				new (out._data + i) T(other._data[i]);
			}
			return out;
		}

		/**
		* Create a new ArrayList given an allocator to be cloned, a pre reserved minimum capacity,
		* and an initializer list to copy elements.
		* The capacity of the new ArrayList will be greater than or equal to the max value between
		* `minCapacity` and `initializerList.size()`.
		*
		* @param inAllocator: Allocator to clone
		* @param minCapacity: Minimum allocation size
		* @param initializerList: Elements to copy
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, usize minCapacity, const std::initializer_list<T>& initializerList)
		requires(std::is_copy_constructible_v<T>) {
			return ArrayList::withCapacity(inAllocator, minCapacity, initializerList);
		}

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
		static ArrayList withCapacity(Allocator&& inAllocator, usize minCapacity, const std::initializer_list<T>& initializerList)
		requires(std::is_copy_constructible_v<T>) {
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
				//out._data[i] = elem;
				new (out._data + i) T(elem);
				i++;
			}
			return out;
		}

		/**
		* Create a new ArrayList given an allocator to be cloned, a pre reserved minimum capacity,
		* a pointer to some data, and a number of elements to copy from the pointer.
		* The capacity of the new ArrayList will be greater than or equal to the max value between
		* `minCapacity` and `elementsToCopy`.
		*
		* @param inAllocator: Allocator to clone
		* @param minCapacity: Minimum allocation size
		* @param ptr: Start of buffer of elements to copy
		* @param elementsToCopy: Number to copy. Accessing beyond the bounds of `ptr` is undefined behaviour.
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, usize minCapacity, const T* ptr, usize elementsToCopy)
		requires(std::is_copy_constructible_v<T>) {
			return ArrayList::withCapacity(inAllocator, minCapacity, ptr, elementsToCopy);
		}

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
		static ArrayList withCapacity(Allocator&& inAllocator, usize minCapacity, const T* ptr, usize elementsToCopy)
		requires(std::is_copy_constructible_v<T>) {
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
				//out._data[i] = ptr[i];
				new (out._data + i) T(ptr[i]);
			}

			return out;
		}

#pragma endregion

#pragma region Access

		/**
		* The number of elements contained in the ArrayList.
		*/
		constexpr usize len() const { return _length; }

		/**
		* The number of elements this ArrayList can store without reallocation.
		*/
		constexpr usize capacity() const { return _capacity; }

		/**
		* A mutable pointer to the data held by this ArrayList. Accessing beyond len() is undefined behaviour.
		*/
		constexpr T* data() { return _data; }

		/**
		* An immutable pointer to the data held by this ArrayList. Accessing beyond len() is undefined behaviour.
		*/
		constexpr const T* data() const { return _data; }

		/**
		* The allocator used by the ArrayList. Can be cloned.
		*/
		constexpr const Allocator& allocator() const { return _allocator; }

		/**
		* Get a mutable reference to an element in the array at a specified index.
		* Will assert if index is out of range.
		* 
		* @param index: The element to get. Asserts that is less than len().
		*/
		[[nodiscard]] constexpr T& operator [] (usize index) {
			check_message(index < _length, "Index out of bounds! Attempted to access index ", index, " from ArrayList of length ", _length);
			return _data[index];
		}

		/**
		* Get an immutable reference to an element in the array at a specified index.
		* Will assert if index is out of range.
		*
		* @param index: The element to get. Asserts that is less than len().
		*/
		[[nodiscard]] constexpr const T& operator [] (usize index) const {
			check_message(index < _length, "Index out of bounds! Attempted to access index ", index, " from ArrayList of length ", _length);
			return _data[index];
		}

#pragma endregion

		/**
		* Pushes a copy of `element` onto the end of the ArrayList, increasing the length by 1.
		* May reallocate if there isn't enough capacity already. Requires that T is copyable.
		* 
		* @param element: Element to be copied to the end of the ArrayList buffer.
		*/
		constexpr void push(const T& element) 
		requires(std::is_copy_assignable_v<T>) {
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

		/**
		* Moves `element` onto the end of the ArrayList, increasing the length by 1.
		* May reallocate if there isn't enough capacity already.
		* 
		* @param element: Element to be moved to the end of the ArrayList buffer.
		*/
		constexpr void push(T&& element) {
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

		/**
		* Reserves additional capacity in the ArrayList. The new capacity will be greater than or equal to `len()` + `additional`.
		* May allocate more to avoid frequent reallocation, or any SIMD requirements.
		* Use `reserveExact()` if you know extra allocation won't happen.
		* 
		* @param additional: Minimum amount to increase the capacity by
		*/
		void reserve(usize additional) {
			const usize addedLength = _length + additional;
			if (addedLength <= _capacity || addedLength == 0) return;

			const usize standardCapacityIncrease = (_capacity + 1) * 2;
			usize newCapacity = addedLength < standardCapacityIncrease ? standardCapacityIncrease : addedLength;
			
			reallocate(newCapacity);
		}

		/**
		* Reserves additional capacity in the ArrayList. The new capacity will be greater than or equal to `len()` + `additional`.
		* Will allocate the smallest amount possible to satisfy the reserve, and potential SIMD requirements.
		* Use `reserve()` if more allocations will happen.
		* 
		* @param additional: Minimum amount to increase the capacity by
		*/
		void reserveExact(usize additional) {
			const usize newCapacity = _length + additional;
			if (newCapacity <= _capacity || newCapacity == 0) return;

			reallocate(newCapacity);
		}

		/**
		* Finds the first index of an element in the ArrayList. For data types that supports it, will use SIMD to find.
		* The index will be returned if it exists, or None if it doesn't.
		* 
		* @param element: Element to check if in the ArrayList.
		* @return The found index, or None
		*/
		constexpr gk::Option<usize> find(const T& element) const {
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

		//constexpr T remove(usize index);
		//constexpr T swapRemove(usize index);
		//constexpr void insert(usize index, const T& element);
		//constexpr void insert(usize index, T&& element);
		//constexpr void shrinkToFit();
		//constexpr void shrinkTo(usize minCapacity);
		//constexpr void truncate(usize newLength);
		//constexpr void retain(bool(*compFunc)(const T&));
		//constexpr void retain(bool(*compFunc)(T&));
		//constexpr void extend(const ArrayList& other);
		//constexpr void resize(usize newLength, const T& fill);

#pragma region Iterator



#pragma endregion

	private:

		constexpr void deleteExistingBuffer() {
			if (_data == nullptr) return;

			for (usize i = 0; i < _length; i++) {
				_data[i].~T();
			}
			freeArrayListBuffer(_data, _capacity, _allocator);
		}

		/**
		* At runtime, creates a 0 initialized buffer.
		* At comptime, just makes a new heap array.
		*/
		constexpr static T* mallocArrayListBuffer(usize* requiredCapacity, Allocator& allocatorToUse) {
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
				outBuffer = allocatorToUse.mallocAlignedBuffer<T>(*requiredCapacity, SIMD_T_MALLOC_ALIGN).ok();
				// Likely dont need to memset cause no constructor tomfoolery
				//memset(outBuffer, 0, (*requiredCapacity) * sizeof(T));
				return outBuffer;
			}

			outBuffer = allocatorToUse.mallocBuffer<T>(*requiredCapacity).ok();
			memset(outBuffer, 0, (*requiredCapacity) * sizeof(T));
			return outBuffer;
		}

		constexpr static void freeArrayListBuffer(T*& buffer, usize bufferCapacity, Allocator& allocatorToUse) {
			if (std::is_constant_evaluated()) {
				delete[] buffer;
				return;
			}

			if constexpr (IS_T_SIMD) {
				allocatorToUse.freeAlignedBuffer(buffer, bufferCapacity, SIMD_T_MALLOC_ALIGN);
				return;
			}
			allocatorToUse.freeBuffer(buffer, bufferCapacity);
		}

		constexpr void reallocate(usize capacity) {
			const usize currentLength = len();
			T* newData = mallocArrayListBuffer(&capacity, _allocator);

			for (usize i = 0; i < currentLength; i++) {
				// TODO see if assignment is better.
				if (std::is_constant_evaluated()) {
					newData[i] = std::move(_data[i]);
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
		Allocator _allocator;
		
	};
	
} // namespace gk