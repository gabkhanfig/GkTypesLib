#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"

namespace gk
{
	template<typename T>
	struct RingQueue
	{
		/* Loops around the ring queue. Requires a non-const instance, because it will move out the values held. */
		class iterator
		{
		public:

			constexpr static iterator begin(RingQueue* queue, uint32 index) {
				iterator iter;
				iter._queue = queue;
				iter._index = index;
				return iter;
			}

			constexpr static iterator end(RingQueue* queue, uint32 totalIterations) {
				iterator iter;
				iter._queue = queue;
				iter._iterations = totalIterations;
				return iter;
			}

			constexpr iterator& operator++() {
				_index = (_index + 1) % _queue->capacity();
				_iterations++;
				return *this; 
			}

			constexpr bool operator!=(const iterator& other) const {
				return _iterations != other._iterations;//&& _buffer != other._buffer && _capacity != other._capacity;
			}

			constexpr T operator*() const {
				return std::move(_queue->pop());
			}

		private:

			constexpr iterator() : _queue(nullptr), _index(0), _iterations(0) {}

		private:

			RingQueue* _queue;
			uint32 _index;
			uint32 _iterations;
		};
		
		/* Iterator begin. */
		constexpr iterator begin() {
			return iterator::begin(this, _readIndex);
		}

		/* Iterator end. */
		constexpr iterator end() {
			return iterator::end(this, len());
		}

		constexpr RingQueue(uint32 inCapacity) : _capacity(inCapacity), _len(0), _readIndex(0), _writeIndex(0) {
			gk_assertm(inCapacity > 1, "RingQueue capacity must be greater than 1");
			_buffer = new T[inCapacity];
		}

		constexpr RingQueue(const RingQueue& other) : _capacity(other._capacity), _len(other._len), _readIndex(0), _writeIndex(0) {
			_buffer = new T[_capacity];
			for (T element : other) {
				_buffer[_writeIndex] = std::move(element);
				_writeIndex++;
			}
		}

		constexpr RingQueue(RingQueue&& other) noexcept {
			_buffer = other._buffer;
			_capacity = other._capacity;
			_len = other._len;
			_readIndex = other._readIndex;
			_writeIndex = other._writeIndex;
			other._buffer = nullptr;
		}

		constexpr ~RingQueue() {
			freeBuffer();
		}

		RingQueue& operator = (const RingQueue& other) {
			freeBuffer();
			_capacity = other._capacity;
			_len = other._len;
			_readIndex = 0;
			_writeIndex = 0; 
			_buffer = new T[_capacity];
			for (T element : other) {
				_buffer[_writeIndex] = std::move(element);
				_writeIndex++;
			}
			return *this;
		}

		RingQueue& operator = (RingQueue&& other) noexcept {
			freeBuffer();
			_buffer = other._buffer;
			_capacity = other._capacity;
			_len = other._len;
			_readIndex = other._readIndex;
			_writeIndex = other._writeIndex;
			other._buffer = nullptr;
			return *this;
		}

		[[nodiscard]] constexpr bool isFull() const {
			return _len == _capacity;
		}

		[[nodiscard]] constexpr bool isEmpty() const {
			return _len == 0;
		}

		[[nodiscard]] constexpr uint32 len() const {
			return _len;
		}

		[[nodiscard]] constexpr uint32 capacity() const { return _capacity; }

		constexpr void push(const T& element) {
			gk_assertm(!isFull(), "Ring queue is full");
			_buffer[_writeIndex] = element;
			_writeIndex = (_writeIndex + 1) % _capacity;
			_len++;
		}

		constexpr void push(T&& element) {
			gk_assertm(!isFull(), "Ring queue is full");
			_buffer[_writeIndex] = std::move(element);
			_writeIndex = (_writeIndex + 1) % _capacity;
			_len++;
		}

		/* Moves the oldest element out of the queue. */
		[[nodiscard]] constexpr T pop() {
			gk_assertm(len() > 0, "Ring queue is empty");
			const uint32 i = _readIndex;
			_readIndex = (_readIndex + 1) % _capacity;
			_len--;
			return std::move(_buffer[i]);
		}

	private:

		constexpr void freeBuffer() {
			if (_buffer) delete[] _buffer;
		}

	private:

		T* _buffer;
		uint32 _capacity;
		uint32 _len;
		uint32 _readIndex;
		uint32 _writeIndex;
	};
}

