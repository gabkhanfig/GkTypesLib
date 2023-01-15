#pragma once

#include <utility>
#include <stdexcept>
#include <array>
#include <type_traits>
#include "../BasicTypes.h"

/* Array integer type */
typedef uint32 arrint;

namespace gk 
{
	/* Dynamic array. */
	template <typename T>
	struct darray
	{
	private:

		T* data;
		arrint size;
		arrint capacity;

	public:

		static constexpr arrint DEFAULT_CAPACITY = 1;

		/* @return The number of elements currently held in the array. */
		constexpr arrint Size() const {
			return size;
		}

		/* @return The capacity of the allocated data. */
		constexpr arrint Capacity() const {
			return capacity;
		}

		/* DANGEROUS. @return Raw array data pointer. */
		constexpr T* Data() const {
			return data;
		}

		/* Darray custom iterator. */
		class iterator
		{
		public:

			constexpr iterator(T* _data) : data(_data) {}

			iterator operator++() { ++data; return *this; }

			bool operator!=(const iterator& other) const { return data != other.data; }

			const T& operator*() const { return *data; }

		private:

			T* data;
		};

	private:

		/* Reallocate and move the pre-existing data over. Increases capacity by 2x. */
		constexpr void Reallocate() {
			const arrint newCapacity = capacity * 2;

			T* newData = new T[newCapacity];
			for (arrint i = 0; i < Size(); i++) {
				newData[i] = std::move(data[i]);
			}
			delete[] data;
			data = newData;
			capacity = newCapacity;
		}

		/* Copy num amount of data onto the end of the darray. */
		constexpr void CopyData(const T* dataToCopy, const arrint num) {
			if (capacity - size < num) {
				Reserve(capacity + num);
			}

			for (arrint i = 0; i < num; i++) {
				data[size] = dataToCopy[i];
				size++;
			}
		}
		
		/* Copy and cast num amount of data onto the end of the darray. U is required to be castable to T. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		constexpr void CopyDataCast(const U* dataToCopy, const arrint num) {
			if (capacity - size < num) {
				Reserve(capacity + num);
			}

			for (arrint i = 0; i < num; i++) {
				data[size] = static_cast<T>(dataToCopy[i]);
				size++;
			}
		}

		/* Move num amount of data onto the end of the darray. */
		constexpr void MoveData(T* dataToMove, const arrint num) {
			if (capacity - size < num) {
				Reserve(capacity + num);
			}

			for (arrint i = 0; i < num; i++) {
				data[size] = std::move(dataToMove[i]);
				size++;
			}
		}

		/* Construct this darray given another darray by copy. */
		constexpr void ConstructCopy(const darray<T>& other) {
			const arrint num = other.Size();
			data = nullptr;
			capacity = 0;
			Reserve(num);

			for (arrint i = 0; i < num; i++) {
				data[i] = other.data[i];
			}
			size = num;
		}

		/* Construct this darray given another darray by move. */
		constexpr void ConstructMove(darray<T>&& other) noexcept {
			data = other.data;
			size = other.size;
			capacity = other.capacity;
			other.data = nullptr;
		}

		/* Construct this darray given an initializer list. */
		constexpr void ConstructInitializerList(const std::initializer_list<T>& il) {
			size = 0;
			capacity = il.size();
			data = new T[capacity];
			for (const T& i : il) {
				data[size] = i;
				size++;
			}
		}

		/* Construct this darray given a pointer to a bunch of data. */
		constexpr void ConstructData(const T* dataToCopy, const arrint num) {
			capacity = 0;
			size = 0;
			CopyData(dataToCopy, num);
		}

		/* Try to delete data. Also sets size and capacity to 0. */
		constexpr void DeleteData() {
			if (data != nullptr) {
				delete[] data;
				data = nullptr;
			}
			size = 0;
			capacity = 0;
		}

	public:

		/* Defualt constructor. See darray::INITIAL_CAPACITY. */
		constexpr darray() {
			data = new T[DEFAULT_CAPACITY];
			size = 0;
			capacity = DEFAULT_CAPACITY;
		}

		/* Copy constructor. The copied darray is still valid. */
		constexpr darray(const darray<T>& other) {
			ConstructCopy(other);
		}

		/* Move constructor. The moved darray is not valid after this. */
		constexpr darray(darray<T>&& other) noexcept {
			ConstructMove(std::move(other));
		}

		/* Initializer list construction. */
		constexpr darray(const std::initializer_list<T>& il) {
			ConstructInitializerList(il);
		}

		/* Data to copy constructor. */
		constexpr darray(const T* dataToCopy, arrint num) {
			ConstructData(dataToCopy, num);
		}

		/**/
		constexpr ~darray() {
			if (data != nullptr)
				delete[] data;
		}

		/* Reserves capacity in the darray. If the new capacity is less than the array's current capacity, this function does nothing. */
		constexpr void Reserve(arrint newCapacity) {
			if (newCapacity < capacity) return;

			if (newCapacity == 0) newCapacity = 1;

			if (data == nullptr) {
				data = new T[newCapacity];
				capacity = newCapacity;
				size = 0;
				return;
			}
			
			T* newData = new T[newCapacity];
			for (arrint i = 0; i < Size(); i++) {
				newData[i] = std::move(data[i]);
			}
			delete[] data;
			data = newData;
			capacity = newCapacity;
		}

		/* Get an element at the specified index by reference. */
		constexpr T& At(arrint index) const {
			if (index >= Size()) {
				throw std::out_of_range("darray element index is out of bounds!");
			}

			return data[index];
		}

		/* Get an element at the specified index by reference. */
		constexpr T& operator [] (arrint index) const {
			return At(index);
		}

		/* Add an element to the end of the darray by move. */
		constexpr arrint Add(T&& element) {
			if (size == capacity) {
				Reallocate();
			}

			data[size] = std::move(element);
			size++;
			return size - 1;
		}

		/* Add an element to the end of the darray by copy. */
		constexpr arrint Add(const T& element) {
			if (size == capacity) {
				Reallocate();
			}

			data[size] = element;
			size++;
			return size - 1;
		}

		/* Set this darray equal to another darray by copy. Wipes the array of any previously held data. */
		constexpr darray<T>& Set(const darray<T>& other) {
			DeleteData();
			ConstructCopy(other);
			return *this;
		}

		/* Set this darray equal to another darray by move. The moved darray is not valid after this. Wipes the array of any previously held data. */
		constexpr darray<T>& Set(darray<T>&& other) {
			DeleteData();
			ConstructMove(std::move(other));
			return *this;
		}

		/* Set this darray equal to another template typed darray by static cast. Wipes the array of any previously held data. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		constexpr darray<T>& Set(const darray<U>& other) {
			DeleteData();
			CopyDataCast<U>(other.Data(), other.Size());
			return *this;
		}

		/* Set this darray equal to a bunch of data given a pointer to the start of it and the number of elements. Wipes the array of any previously held data. */
		constexpr darray<T>& Set(const T* dataToCopy, arrint num) {
			DeleteData();
			CopyData(dataToCopy, num);
			return *this;
		}

		/* Set this darray equal to a bunch of template typed data to static cast given a pointer to the start of it and the number of elements. Wipes the array of any previously held data. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		constexpr darray<T>& Set(const U* dataToCopy, arrint num) {
			DeleteData();
			CopyDataCast<U>(dataToCopy, num);
			return *this;
		}

		/* Set this darray equal to an initializer list. Wipes the array of any previously held data. */
		constexpr darray<T>& Set(const std::initializer_list<T>& il) {
			DeleteData();
			ConstructInitializerList(il);
			return *this;
		}

		/* Set this darray equal to another darray by copy. Wipes the array of any previously held data. */
		constexpr darray<T>& operator = (const darray<T>& other) {
			return Set(other);
		}

		/* Set this darray equal to another darray by move. The moved darray is not valid after this. Wipes the array of any previously held data. */
		constexpr darray<T>& operator = (darray<T>&& other) {
			return Set(std::move(other));
		}

		/* Set this darray equal to an initializer list. Wipes the array of any previously held data. */
		constexpr darray<T>& operator = (const std::initializer_list<T>& il) {
			return Set(il);
		}

		/* Set this darray equal to another template typed darray by static cast. Wipes the array of any previously held data. */
		template<typename U>
			requires (std::convertible_to<U, T>)
		constexpr darray<T>& operator = (const darray<U>& other) {
			return Set<U>(other);
		}

		/* Compare two arrays for if the elements they contain are equal. */
		[[nodiscard]] constexpr bool operator == (const darray<T>& other) const {
			const arrint elementCount = Size();
			if (elementCount != other.Size()) {
				return false;
			}
			const T* left = Data();
			const T* right = other.Data();
			for (arrint i = 0; i < elementCount; i++) {
				if (left[i] != right[i]) {
					return false;
				}
			}
			return true;
		}

	};

	/* Static array. */
	template<typename T, size_t Size>
	using sarray = std::array<T, Size>;
}



