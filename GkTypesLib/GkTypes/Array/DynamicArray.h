#pragma once

#include "../Core.h"
#include "../Utility.h"
#include <utility>
//#include <stdexcept>
#include <array>
#include <type_traits>
#include <iostream>
#include "../BasicTypes.h"
#include "../Asserts.h"

namespace gk 
{
	/* Dynamic array. */
	template <typename T>
	struct darray
	{
	public:

		struct OptionalIndex {

			static constexpr uint32 INDEX_NONE = 0xFFFFFFFFU;

			OptionalIndex(uint32 index) : _index(index) {}

			OptionalIndex(const OptionalIndex& other) : _index(other._index) {}

			void operator = (const OptionalIndex& other) { _index = other._index; }

			bool IsValidIndex() const { return _index != INDEX_NONE; }

			/* Will throw an assertion if IsValidIndex() returns false. */
			uint32 Get() const {
				gk_assertm(IsValidIndex(), "Cannot get the an invalid optional index");
				return _index;
			}

			bool operator == (OptionalIndex rhs) const { return _index == rhs._index; }
			bool operator == (uint32 rhs) const { return _index == rhs; }

		private:

			uint32 _index;
		};

		static constexpr uint32 DEFAULT_CAPACITY = 1;

		/* @return The number of elements currently held in the array. */
		uint32 Size() const {
			return size;
		}

		/* @return The capacity of the allocated data. */
		uint32 Capacity() const {
			return capacity;
		}

		/* DANGEROUS. @return Raw array data pointer. */
		T* Data() const {
			return data;
		}

		/* Darray custom iterator. */
		class iterator
		{
		public:

			iterator(T* _data) : data(_data) {}

			iterator operator++() { ++data; return *this; }

			bool operator!=(const iterator& other) const { return data != other.data; }

			const T& operator*() const { return *data; }

		private:

			T* data;
		};
		
		/* Iterator begin. */
		iterator begin() const { return iterator(data); }

		/* Iterator end. */
		iterator end() const { return iterator(data + size); }

		/* Defualt constructor. See darray::INITIAL_CAPACITY. */
		darray() {
			data = new T[DEFAULT_CAPACITY];
			size = 0;
			capacity = DEFAULT_CAPACITY;
		}

		/* Copy constructor. The copied darray is still valid. */
		darray(const darray<T>& other) {
			ConstructCopy(other);
		}

		/* Move constructor. The moved darray is not valid after this. */
		darray(darray<T>&& other) noexcept {
			ConstructMove(std::move(other));
		}

		/* Initializer list construction. */
		darray(const std::initializer_list<T>& il) {
			ConstructInitializerList(il);
		}

		/* Data to copy constructor. */
		darray(const T* dataToCopy, uint32 num) {
			ConstructData(dataToCopy, num);
		}

		/**/
		~darray() {
			if (data != nullptr)
				delete[] data;
		}

		/* Reserves capacity in the darray. If the new capacity is less than the array's current capacity, this function does nothing. */
		void Reserve(uint32 newCapacity) {
			if (newCapacity < capacity) return;

			if (newCapacity == 0) newCapacity = 1;

			if (data == nullptr) {
				data = new T[newCapacity];
				capacity = newCapacity;
				size = 0;
				return;
			}

			T* newData = new T[newCapacity];
			for (uint32 i = 0; i < Size(); i++) {
				newData[i] = std::move(data[i]);
			}
			delete[] data;
			data = newData;
			capacity = newCapacity;
		}

		/* Get an element at the specified index by reference. */
		T& At(uint32 index) const {
			gk_assertm(index < Size(), "Darray index out of bounds! Tried to reach index " << index << " from a darray of size " << Size());

			return data[index];
		}

		/* Get an element at the specified index by reference. */
		T& operator [] (uint32 index) const {
			return At(index);
		}

		/* Add an element to the end of the darray by move. */
		uint32 Add(T&& element) {
			if (size == capacity) {
				Reallocate(capacity * 2);
			}

			data[size] = std::move(element);
			size++;
			return size - 1;
		}

		/* Add an element to the end of the darray by copy. */
		uint32 Add(const T& element) {
			if (size == capacity) {
				Reallocate(capacity * 2);
			}

			data[size] = element;
			size++;
			return size - 1;
		}

		/* Set this darray equal to another darray by copy. Wipes the array of any previously held data. */
		darray<T>& SetEqualTo(const darray<T>& other) {
			DeleteData();
			ConstructCopy(other);
			return *this;
		}

		/* Set this darray equal to another darray by move. The moved darray is not valid after this. Wipes the array of any previously held data. */
		darray<T>& SetEqualTo(darray<T>&& other) {
			DeleteData();
			ConstructMove(std::move(other));
			return *this;
		}

		/* Set this darray equal to another template typed darray by static cast. Wipes the array of any previously held data. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		darray<T>& SetEqualTo(const darray<U>& other) {
			DeleteData();
			CopyDataCast<U>(other.Data(), other.Size());
			return *this;
		}

		/* Set this darray equal to a bunch of data given a pointer to the start of it and the number of elements. Wipes the array of any previously held data. */
		darray<T>& SetEqualTo(const T* dataToCopy, uint32 num) {
			DeleteData();
			CopyData(dataToCopy, num);
			return *this;
		}

		/* Set this darray equal to a bunch of template typed data to static cast given a pointer to the start of it and the number of elements. Wipes the array of any previously held data. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		darray<T>& SetEqualTo(const U* dataToCopy, uint32 num) {
			DeleteData();
			CopyDataCast<U>(dataToCopy, num);
			return *this;
		}

		/* Set this darray equal to an initializer list. Wipes the array of any previously held data. */
		darray<T>& SetEqualTo(const std::initializer_list<T>& il) {
			DeleteData();
			ConstructInitializerList(il);
			return *this;
		}

		/* Set this darray equal to another darray by copy. Wipes the array of any previously held data. */
		darray<T>& operator = (const darray<T>& other) {
			return SetEqualTo(other);
		}

		/* Set this darray equal to another darray by move. The moved darray is not valid after this. Wipes the array of any previously held data. */
		darray<T>& operator = (darray<T>&& other) noexcept {
			return SetEqualTo(std::move(other));
		}

		/* Set this darray equal to an initializer list. Wipes the array of any previously held data. */
		darray<T>& operator = (const std::initializer_list<T>& il) {
			return SetEqualTo(il);
		}

		/* Set this darray equal to another template typed darray by static cast. Wipes the array of any previously held data. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		darray<T>& operator = (const darray<U>& other) {
			return SetEqualTo<U>(other);
		}

		/* Compare two arrays for if the elements they contain are equal. */
		[[nodiscard]] bool operator == (const darray<T>& other) const {
			const uint32 elementCount = Size();
			if (elementCount != other.Size()) {
				return false;
			}
			const T* left = Data();
			const T* right = other.Data();
			for (uint32 i = 0; i < elementCount; i++) {
				if (left[i] != right[i]) {
					return false;
				}
			}
			return true;
		}

		/* Check if the darray contains a provided element. */
		[[nodiscard]] bool Contains(const T& element) const {
			for (uint32 i = 0; i < Size(); i++) {
				if (data[i] == element) return true;
			}
			return false;
		}

		/* Empty the array. Basically sets it back to the state of default construction. */
		void Empty() {
			DeleteData();
			data = new T[DEFAULT_CAPACITY];
			size = 0;
			capacity = DEFAULT_CAPACITY;
		}

		/* Find the first index of an element in the array. */
		[[nodiscard]] OptionalIndex Find(const T& element) const {
			for (uint32 i = 0; i < Size(); i++) {
				if (data[i] == element) return i;
			}
			return OptionalIndex::INDEX_NONE;
		}

		/* Find the last index of an element in the array. */
		[[nodiscard]] OptionalIndex FindLast(const T& element) const {
			for (uint32 i = Size() - 1; i != OptionalIndex::INDEX_NONE; i--) { // Potential UB?
				if (data[i] == element) return i;
			}
			return OptionalIndex::INDEX_NONE;
		}

		/* Find all indices of a provided element. */
		[[nodiscard]] darray<uint32> FindAll(const T& element) const {
			darray<uint32> elements;
			for (int i = 0; i < Size(); i++) {
				if (data[i] == element) elements.Add(i);
			}
			return elements;
		}

		/* Get the how many of the provided element exist in the darray. */
		[[nodiscard]] uint32 Count(const T& element) const {
			uint32 count = 0;
			for (int i = 0; i < Size(); i++) {
				if (data[i] == element) count++;
			}
			count;
		}

		/* Remove the first occurrence of a given element. */
		void RemoveFirst(const T& element) {
			const OptionalIndex index = Find(element);
			if (!index.IsValidIndex()) return;
			Internal_RemoveAt(index.Get());
		}

		/* Remove the last occurrence of a given element. */
		void RemoveLast(const T& element) {
			const OptionalIndex index = FindLast(element);
			if (!index.IsValidIndex()) return;
			Internal_RemoveAt(index.Get());
		}

		/* Remove an element at a specific index. */
		void RemoveAt(uint32 index) {
			gk_assertm(index < Size(), "Array index out of bounds. Cannot remove element at index " << index << " from array of size " << Size());
			Internal_RemoveAt(index);
		}

		/* Remove all occurrences of a specified element. Also shrinks the array to where the capacity is the initial size. Does perform a reallocation so be mindful of performance. */
		void RemoveAll(const T& element) {
			const uint32 initialSize = size;
			T* newData = new T[initialSize];
			uint32 newIndex = 0;

			for (uint32 i = 0; i < initialSize; i++) {
				if (data[i] == element) {
					size--;
					continue;
				}
				newData[newIndex] = std::move(data[i]);
				newIndex++;
			}

			delete[] data;
			data = newData;
			capacity = initialSize;
		}

		/* Append this array with another array, potentially reallocating. */
		darray<T>& Append(const darray<T>& other) {
			const uint32 newSize = size + other.size;
			if (newSize > capacity) {
				const uint32 newCapacity = newSize > (capacity * 2) ? newSize : capacity * 2;
				Reallocate(newCapacity);
			}
			for (uint32 i = 0; i < other.size; i++) {
				data[size++] = other.data[i];
			}
			return *this;
		}

		/* Append this array with another array by type casting, potentially reallocating. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		darray<T>& Append(const darray<U>& other) {
			const uint32 newSize = size + other.Size();
			if (newSize > capacity) {
				const uint32 newCapacity = newSize > (capacity * 2) ? newSize : capacity * 2;
				Reallocate(newCapacity);
			}
			for (uint32 i = 0; i < other.Size(); i++) {
				data[size++] = other.Data()[i];
			}
			return *this;
		}

		/* Append this array with a bunch of elements, potentially reallocating. */
		darray<T>& Append(const T* dataToCopy, uint32 num) {
			const uint32 newSize = size + num;
			if (newSize > capacity) {
				const uint32 newCapacity = newSize > (capacity * 2) ? newSize : capacity * 2;
				Reallocate(newCapacity);
			}
			for (uint32 i = 0; i < num; i++) {
				data[size++] = dataToCopy[i];
			}
			return *this;
		}

		/* Append this array with a bunch of elements by type casting, potentially reallocating. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		darray<T>& Append(const U* dataToCopy, uint32 num) {
			const uint32 newSize = size + num;
			if (newSize > capacity) {
				const uint32 newCapacity = newSize > (capacity * 2) ? newSize : capacity * 2;
				Reallocate(newCapacity);
			}
			for (uint32 i = 0; i < num; i++) {
				data[size++] = dataToCopy[i];
			}
			return *this;
		}

		/* Append this array with an initializer list, potentially reallocating. */
		darray<T>& Append(const std::initializer_list<T>& il) {
			const uint32 newSize = size + il.size();
			if (newSize > capacity) {
				const uint32 newCapacity = newSize > (capacity * 2) ? newSize : capacity * 2;
				Reallocate(newCapacity);
			}
			for (const T& elem : il) {
				data[size++] = elem;
			}
			return *this;
		}

		/* Append this array with another array, potentially reallocating. */
		darray<T>& operator += (const darray<T>& other) {
			return Append(other);
		}

		/* Append this array with another array by type casting, potentially reallocating. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		darray<T>& operator += (const darray<U>& other) {
			return Append(other);
		}

		/* Append this array with an initializer list, potentially reallocating. */
		darray<T>& operator += (const std::initializer_list<T>& il) {
			return Append(il);
		}

		/* Create a copy of this array with the contents of the other array appended to the copy. */
		[[nodiscard]] darray<T> Concatenate(const darray<T>& other) const {
			gk::darray<T> arr = *this;
			arr.Append(other);
			return arr;
		}

		/* Create a copy of this array with the contents of the other array appended to the copy by type casting. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		[[nodiscard]] darray<T> Concatenate(const darray<U>& other) const {
			gk::darray<T> arr = *this;
			arr.Append(other);
			return arr;
		}

		/* Create a copy of this array with the elements passed in appended to the copy. */
		[[nodiscard]] darray<T> Concatenate(const T* dataToCopy, uint32 num) const {
			gk::darray<T> arr = *this;
			arr.Append(dataToCopy, num);
			return arr;
		}

		/* Create a copy of this array with the elements passed in appended to the copy by type casting. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		[[nodiscard]] darray<T> Concatenate(const U* dataToCopy, uint32 num) const {
			gk::darray<T> arr = *this;
			arr.Append(dataToCopy, num);
			return arr;
		}

		/* Create a copy of this array with the initializer list contents appended to the copy. */
		[[nodiscard]] darray<T> Concatenate(const std::initializer_list<T>& il) const {
			gk::darray<T> arr = *this;
			arr.Append(il);
			return arr;
		}

		/* Create a copy of this array with the contents of the other array appended to the copy. */
		[[nodiscard]] darray<T> operator + (const darray<T>& other) const {
			return Concatenate(other);
		}

		/* Create a copy of this array with the contents of the other array appended to the copy by type casting. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		[[nodiscard]] darray<T> operator + (const darray<U>& other) const {
			return Concatenate(other);
		}

		/* Create a copy of this array with the initializer list contents appended to the copy. */
		[[nodiscard]] darray<T> operator + (const std::initializer_list<T>& il) const {
			return Concatenate(il);
		}

	private:

		/* Reallocate and move the pre-existing data over. Increases capacity by 2x. */
		void Reallocate(uint32 newCapacity) {
			T* newData = new T[newCapacity];
			for (uint32 i = 0; i < Size(); i++) {
				newData[i] = std::move(data[i]);
			}
			delete[] data;
			data = newData;
			capacity = newCapacity;
		}

		/* Copy num amount of data onto the end of the darray. */
		void CopyData(const T* dataToCopy, const uint32 num) {
			if (capacity - size < num) {
				Reserve(capacity + num);
			}

			for (uint32 i = 0; i < num; i++) {
				data[size] = dataToCopy[i];
				size++;
			}
		}

		/* Copy and cast num amount of data onto the end of the darray. U is required to be castable to T. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		void CopyDataCast(const U* dataToCopy, const uint32 num) {
			if (capacity - size < num) {
				Reserve(capacity + num);
			}

			for (uint32 i = 0; i < num; i++) {
				data[size] = static_cast<T>(dataToCopy[i]);
				size++;
			}
		}

		/* Move num amount of data onto the end of the darray. */
		void MoveData(T* dataToMove, const uint32 num) {
			if (capacity - size < num) {
				Reserve(capacity + num);
			}

			for (uint32 i = 0; i < num; i++) {
				data[size] = std::move(dataToMove[i]);
				size++;
			}
		}

		/* Construct this darray given another darray by copy. */
		void ConstructCopy(const darray<T>& other) {
			const uint32 num = other.Size();
			data = nullptr;
			capacity = 0;
			Reserve(num);

			for (uint32 i = 0; i < num; i++) {
				data[i] = other.data[i];
			}
			size = num;
		}

		/* Construct this darray given another darray by move. */
		void ConstructMove(darray<T>&& other) noexcept {
			data = other.data;
			size = other.size;
			capacity = other.capacity;
			other.data = nullptr;
		}

		/* Construct this darray given an initializer list. */
		void ConstructInitializerList(const std::initializer_list<T>& il) {
			size = 0;
			capacity = static_cast<uint32>(il.size());
			data = new T[capacity];
			for (const T& i : il) {
				data[size] = i;
				size++;
			}
		}

		/* Construct this darray given a pointer to a bunch of data. */
		void ConstructData(const T* dataToCopy, const uint32 num) {
			capacity = 0;
			size = 0;
			CopyData(dataToCopy, num);
		}

		/* Try to delete data. Also sets size and capacity to 0. */
		void DeleteData() {
			if (data != nullptr) {
				delete[] data;
				data = nullptr;
			}
			size = 0;
			capacity = 0;
		}

		void Internal_RemoveAt(uint32 index) {
			for (uint32 i = index; i < Size() - 1; i++) {
				data[i] = std::move(data[i + 1]);
			}
			size--;
		}

	private:

		T* data;
		uint32 size;
		uint32 capacity;

	};

	/* Static array. */
	template<typename T, size_t Size>
	using sarray = std::array<T, Size>;
}



