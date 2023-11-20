#pragma once

#include "../Allocator/Allocator.h"
#include "../Allocator/HeapAllocator.h"
#include "../Option/Option.h"
#include <type_traits>
#include "../Error/Result.h"

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
	}

	template<typename T>
	struct ArrayList
	{
	private:

		constexpr static bool IS_T_SIMD = (std::is_arithmetic_v<T> || std::is_pointer_v<T>);
		constexpr static size_t SIMD_T_MALLOC_ALIGN = 64;

	private:

		/**
		* Simple constructor to initialize the ArrayList with a specified allocator.
		* For actual use, call ArrayList::init() for whichever overload necessary.
		*/
		ArrayList(Allocator&& inAllocator)
			: _data(nullptr), _length(0), _capacity(0), _allocator(std::move(inAllocator))
		{}

	public:

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
			requires(std::is_copy_assignable_v<T>)
			: _length(other._length), _allocator(other._allocator.clone())
		{
			if (_length == 0) {
				_data = nullptr;
				_capacity = 0;
				return;
			}

			size_t requiredCapacity = _length;
			_data = mallocArrayListBuffer(&requiredCapacity, _allocator);
			_capacity = requiredCapacity;

			for (size_t i = 0; i < _length; i++) {
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
			gk_assert(_data == nullptr);
			_length = other._length;
			_allocator = other._allocator.clone();
			if (_length == 0) {
				_data = nullptr;
				_capacity = 0;
				return;
			}

			size_t requiredCapacity = _length;
			_data = mallocArrayListBuffer(&requiredCapacity, _allocator);
			_capacity = requiredCapacity;

			for (size_t i = 0; i < _length; i++) {
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
		*/
		static ArrayList init(const Allocator& inAllocator) {
			return ArrayList::init(inAllocator.clone());
		}

		/**
		*/
		static ArrayList init(Allocator&& inAllocator) {
			return ArrayList(std::move(inAllocator));
		}

		/**
		*/
		static ArrayList init(const Allocator& inAllocator, const ArrayList& other) 
		requires(std::is_copy_assignable_v<T>) {
			return ArrayList::init(inAllocator.clone(), other);
		}

		/**
		*/
		static ArrayList init(Allocator&& inAllocator, const ArrayList& other) 
		requires(std::is_copy_assignable_v<T>) {
			ArrayList out = ArrayList(std::move(inAllocator));
			out._length = other._length;
			if (out._length == 0) {
				out._data = nullptr;
				out._capacity = 0;
				return;
			}

			size_t requiredCapacity = out._length;
			out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
			out._capacity = requiredCapacity;
			for (size_t i = 0; i < out._length; i++) {
				out._data[i] = other._data[i];
			}
			return out;
		}

		/**
		*/
		static ArrayList init(const Allocator& inAllocator, const std::initializer_list<T>& initializerList) 
		requires(std::is_copy_assignable_v<T>) {
			return ArrayList::init(inAllocator.clone(), initializerList);
		}

		/**
		*/
		static ArrayList init(Allocator&& inAllocator, const std::initializer_list<T>& initializerList)
		requires(std::is_copy_assignable_v<T>) {
			ArrayList out = ArrayList(std::move(inAllocator));
			out._length = initializerList.size();
			if (out._length == 0) {
				out._data = nullptr;
				out._capacity = 0;
				return;
			}

			size_t requiredCapacity = out._length;
			out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
			out._capacity = requiredCapacity;

			size_t i = 0;
			for (const auto& elem : initializerList) {
				out._data[i] = elem;
				i++;
			}
			return out;
		}

		/**
		*/
		static ArrayList init(const Allocator& inAllocator, const T* ptr, size_t elementsToCopy)
		requires(std::is_copy_assignable_v<T>) {
			return ArrayList::init(inAllocator.clone(), ptr, elementsToCopy);
		}

		/**
		*/
		static ArrayList init(Allocator&& inAllocator, const T* ptr, size_t elementsToCopy)
		requires(std::is_copy_assignable_v<T>) {
			ArrayList out = ArrayList(std::move(inAllocator));

			out._length = elementsToCopy; 
			if (out._length == 0) {
				out._data = nullptr;
				out._capacity = 0;
				return;
			}

			size_t requiredCapacity = out._length;
			out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
			out._capacity = requiredCapacity;
			for (size_t i = 0; i < elementsToCopy; i++) {
				out._data[i] = ptr[i];
			}

			return out;
		}

#pragma endregion

#pragma region Construct_With_Allocator_And_Capacity

		/**
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, size_t inCapacity) {
			return ArrayList::withCapacity(inAllocator.clone(), inCapacity);
		}
		
		/**
		*/
		static ArrayList withCapacity(Allocator&& inAllocator, size_t inCapacity) {
			ArrayList out = ArrayList(std::move(inAllocator));
			if (inCapacity == 0) [[unlikely]] {
				return out;
			}

			out._data = mallocArrayListBuffer(&inCapacity, out._allocator);
			out._capacity = inCapacity;
			return out;
		}

		/**
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, size_t inCapacity, const ArrayList& other)
		requires(std::is_copy_assignable_v<T>) {
			return ArrayList::withCapacity(inAllocator, inCapacity, other);
		}

		/**
		*/
		static ArrayList withCapacity(Allocator&& inAllocator, size_t inCapacity, const ArrayList& other)
		requires(std::is_copy_assignable_v<T>) {
			ArrayList out = ArrayList(std::move(inAllocator));
			out._length = other._length;

			if (out._length == 0 && inCapacity == 0) [[unlikely]] {
				out._data = nullptr;
				out._capacity = 0;
				return;
			}

			size_t requiredCapacity = out._length > inCapacity ? out._length : inCapacity;
			out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
			out._capacity = requiredCapacity;
			for (size_t i = 0; i < out._length; i++) {
				out._data[i] = other._data[i];
			}
			return out;
		}

		/**
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, size_t inCapacity, const std::initializer_list<T>& initializerList)
		requires(std::is_copy_assignable_v<T>) {
			return ArrayList::withCapacity(inAllocator, inCapacity, initializerList);
		}

		/**
		*/
		static ArrayList withCapacity(Allocator&& inAllocator, size_t inCapacity, const std::initializer_list<T>& initializerList)
		requires(std::is_copy_assignable_v<T>) {
			ArrayList out = ArrayList(std::move(inAllocator));
			out._length = initializerList.size();

			if (out._length == 0 && inCapacity == 0) [[unlikely]] {
				out._data = nullptr;
				out._capacity = 0;
				return;
			}

			size_t requiredCapacity = out._length > inCapacity ? out._length : inCapacity;
			out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
			out._capacity = requiredCapacity;

			size_t i = 0;
			for (const auto& elem : initializerList) {
				out._data[i] = elem;
				i++;
			}
			return out;
		}

		/**
		*/
		static ArrayList withCapacity(const Allocator& inAllocator, size_t inCapacity, const T* ptr, size_t elementsToCopy)
		requires(std::is_copy_assignable_v<T>) {
			return ArrayList::withCapacity(inAllocator, inCapacity, ptr, elementsToCopy);
		}

		/**
		*/
		static ArrayList withCapacity(Allocator&& inAllocator, size_t inCapacity, const T* ptr, size_t elementsToCopy)
		requires(std::is_copy_assignable_v<T>) {
			ArrayList out = ArrayList(std::move(inAllocator));
			out._length = elementsToCopy;

			if (out._length == 0 && inCapacity == 0) [[unlikely]] {
				out._data = nullptr;
				out._capacity = 0;
				return;
			}

			size_t requiredCapacity = out._length > inCapacity ? out._length : inCapacity;
			out._data = mallocArrayListBuffer(&requiredCapacity, out._allocator);
			out._capacity = requiredCapacity;
			for (size_t i = 0; i < elementsToCopy; i++) {
				out._data[i] = ptr[i];
			}

			return out;
		}

#pragma endregion

#pragma region Access

		/**
		* The number of elements contained in the ArrayList.
		*/
		constexpr size_t len() const { return _length; }

		/**
		* The number of elements this ArrayList can store without reallocation.
		*/
		constexpr size_t capacity() const { return _capacity; }

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
		* @param index: The element to get. Must be less than len().
		*/
		[[nodiscard]] constexpr T& operator [] (size_t index) {
			gk_assertm(index < _length, "Index out of bounds! Attempted to access index " << index << " from ArrayList of length " << _length);
			return _data[index];
		}

		/**
		* Get an immutable reference to an element in the array at a specified index.
		* Will assert if index is out of range.
		*
		* @param index: The element to get. Must be less than len().
		*/
		[[nodiscard]] constexpr const T& operator [] (size_t index) const {
			gk_assertm(index < _length, "Index out of bounds! Attempted to access index " << index << " from ArrayList of length " << _length);
			return _data[index];
		}

#pragma endregion

		constexpr void push(const T& element) {
			if (_length == _capacity) {
				reallocate((_capacity + 1) * 2);
			}

			_data[_length] = element;
			_length++;
		}

		constexpr void push(T&& element) {
			if (_length == _capacity) {
				reallocate((_capacity + 1) * 2);
			}

			_data[_length] = std::move(element);
			_length++;
		}

		//constexpr void reserve(size_t additional);
		//constexpr void reserveExact(size_t additional);
		//constexpr gk::Option<size_t> find(const T& element);
		//constexpr T remove(size_t index);
		//constexpr T swapRemove(size_t index);
		//constexpr void insert(size_t index, const T& element);
		//constexpr void insert(size_t index, T&& element);
		//constexpr void shrinkToFit();
		//constexpr void shrinkTo(size_t minCapacity);
		//constexpr void truncate(size_t newLength);
		//constexpr void retain(bool(*compFunc)(const T&));
		//constexpr void retain(bool(*compFunc)(T&));
		//constexpr void extend(const ArrayList& other);
		//constexpr void resize(size_t newLength, const T& fill);

#pragma region Iterator



#pragma endregion

	private:

		constexpr void deleteExistingBuffer() {
			if (_data == nullptr) return;

			for (size_t i = 0; i < _length; i++) {
				_data[i].~T();
			}
			freeArrayListBuffer(_data, _capacity, _allocator);
		}

		/**
		* At runtime, creates a 0 initialized buffer.
		* At comptime, just makes a new heap array.
		*/
		constexpr static T* mallocArrayListBuffer(size_t* requiredCapacity, Allocator& allocatorToUse) {
			if (std::is_constant_evaluated()) {
				const size_t capacity = *requiredCapacity;
				return new T[capacity];
			}

			T* outBuffer;
			if constexpr (IS_T_SIMD) {
				constexpr size_t numPerSimd = 64 / sizeof(T);
				const size_t remainder = *requiredCapacity % numPerSimd;
				if (remainder != 0) {
					*requiredCapacity = *requiredCapacity + (numPerSimd - remainder);
				}
				outBuffer = allocatorToUse.mallocAlignedBuffer<T>(*requiredCapacity, SIMD_T_MALLOC_ALIGN).ok();
				memset(outBuffer, 0, (*requiredCapacity) * sizeof(T));
				return outBuffer;
			}

			outBuffer = allocatorToUse.mallocBuffer<T>(*requiredCapacity).ok();
			memset(outBuffer, 0, (*requiredCapacity) * sizeof(T));
			return outBuffer;
		}

		constexpr static void freeArrayListBuffer(T*& buffer, size_t bufferCapacity, Allocator& allocatorToUse) {
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

		constexpr void reallocate(size_t capacity) {
			const size_t currentLength = len();
			T* newData = mallocArrayListBuffer(&capacity, _allocator);

			for (size_t i = 0; i < currentLength; i++) {
				// TODO see if assignment is better.
				//newData[i] = std::move(_data[i]);
				new (newData + i) T(std::move(_data[i]));
			}
			if (_data != nullptr) {
				freeArrayListBuffer(_data, _capacity, _allocator);
			}

			_data = newData;
			_capacity = capacity;
		}

	private:

		T* _data;
		size_t _length;
		size_t _capacity;
		Allocator _allocator;
		
	};
	
} // namespace gk