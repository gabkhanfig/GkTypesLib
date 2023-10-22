#pragma once

#include "../BasicTypes.h"
#include "../Option/Option.h"
#include "../Event/Event.h"
#include <type_traits>
#include "../Function/Fptr.h"

namespace gk
{
	template<typename T>
	concept IsWithinJobDataBufferBounds = std::alignment_of_v<T> <= 8 && sizeof(T) <= 32;

	/* Internal buffer has a maximum size of 32. It has an alignment of 8 bytes.
	Requires a valid function pointer to a function that will free the held data.
	If it does not require freeing, defaultFreeBuffer should be used. */
	struct JobRunDataBuffer {

		JobRunDataBuffer() 
			: _buffer{0} {}

		JobRunDataBuffer(const JobRunDataBuffer&) = delete; // no copying allowed

		JobRunDataBuffer(JobRunDataBuffer && other) noexcept {
			memcpy(_buffer, other._buffer, sizeof(_buffer));
			memset(other._buffer, 0, sizeof(_buffer));
			_freeBufferFunc = std::move(other._freeBufferFunc);
		}

		~JobRunDataBuffer() {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			memset(_buffer, 0, sizeof(_buffer)); 
			_freeBufferFunc = gk::Fptr<void, JobRunDataBuffer*>();
		}

		JobRunDataBuffer& operator = (const JobRunDataBuffer&) = delete; // no copying allowed

		JobRunDataBuffer& operator = (JobRunDataBuffer && other) noexcept {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			memcpy(_buffer, other._buffer, sizeof(_buffer));
			memset(other._buffer, 0, sizeof(_buffer));
			_freeBufferFunc = std::move(other._freeBufferFunc);
			return *this;
		}

		/* Overload for arithmetic template types. Obviously does not require a fptr to free. 
		For pointers, see the JobRunDataBuffer::store() overload that takes an extra bool argument */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && std::is_arithmetic_v<T> && !std::is_pointer_v<T>)
		void store(T data) {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			T* castBuffer = reinterpret_cast<T*>(_buffer);
			*castBuffer = data;
			_freeBufferFunc = gk::Fptr<void, JobRunDataBuffer*>(); // unbind
		}

		/* Overload for non pointer types to be owned and cleaned up by this data buffer. Will copy. 
		For pointers, see the JobRunDataBuffer::store() overload that takes an extra bool argument */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && !std::is_arithmetic_v<T> && !std::is_pointer_v<T>)
		void store(const T& data) {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			/* Generates lambda for function pointer that will handle cleanup.
			If the data type passed in has a destructor that leaks, this will leak naturally. */
			auto cleanupFunc = [](JobRunDataBuffer* self) {
				reinterpret_cast<T*>(self->_buffer)->~T();
			};
			T* castBuffer = reinterpret_cast<T*>(_buffer);
			*castBuffer = data;
			_freeBufferFunc = cleanupFunc;
		}		
		
		/* Overload for non pointer types to be owned and cleaned up by this data buffer. 
		Will copy and not mutate. Compile gets upset when this overload doesn't exist for some reason. 
		For pointers, see the JobRunDataBuffer::store() overload that takes an extra bool argument */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && !std::is_arithmetic_v<T> && !std::is_pointer_v<T>)
		void store(T& data) {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			/* Generates lambda for function pointer that will handle cleanup. 
			If the data type passed in has a destructor that leaks, this will leak naturally. */
			auto cleanupFunc = [](JobRunDataBuffer* self) {
				reinterpret_cast<T*>(self->_buffer)->~T();
			};
			T* castBuffer = reinterpret_cast<T*>(_buffer);
			*castBuffer = data;
			_freeBufferFunc = cleanupFunc;
		}

		/* Overload for non pointer types to be owned and cleaned up by this data buffer. 
		Will claim ownership of the data, and clean it up accordingly.
		For pointers, see the JobRunDataBuffer::store() overload that takes an extra bool argument */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && !std::is_arithmetic_v<T> && !std::is_pointer_v<T>)
		void store(T && data) {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			/* Generates lambda for function pointer that will handle cleanup.
			If the data type passed in has a destructor that leaks, this will leak naturally. */
			auto cleanupFunc = [](JobRunDataBuffer* self) {
				reinterpret_cast<T*>(self->_buffer)->~T();
			};
			T* castBuffer = reinterpret_cast<T*>(_buffer);
			*castBuffer = std::move(data);
			_freeBufferFunc = cleanupFunc;
		}

		/* Overload for pointer types. Can specify ownership over the pointer, 
		which will generate a cleanup function for it. */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && std::is_pointer_v<T>)
		void store(T ptr, bool shouldFree) {
			if (_freeBufferFunc.isBound()) {
				_freeBufferFunc.invoke(this);
			}
			_buffer[0] = (size_t)ptr;
			if (shouldFree) {
				/* Generates lambda for function pointer that will
				free the associated pointer only if the shouldFree flag is set to true. */
				auto cleanupFunc = [](JobRunDataBuffer* self) {
					T ownedPtr = reinterpret_cast<T>(self->_buffer[0]);
					delete ownedPtr;
				};
				_freeBufferFunc = cleanupFunc;
			}
			else {
				_freeBufferFunc = gk::Fptr<void, JobRunDataBuffer*>(); // unbind
			}
		}

		//bool isEmpty() const {
		//	return _buffer[0] == 0
		//		&& _buffer[1] == 0
		//		&& _buffer[2] == 0
		//		&& _buffer[3] == 0;
		//}

		/* NOT FOR POINTER TYPE -> See the JobRunDataBuffer::get(bool) overload. 
		Will get the buffer data from this object. */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && !std::is_pointer_v<T>)
		T& get() {
			if constexpr (std::is_arithmetic_v<T>) {
				return reinterpret_cast<T&>(_buffer);
			}
			return reinterpret_cast<T&>(_buffer);
		}		
		
		/* NOT FOR POINTER TYPE -> See the JobRunDataBuffer::get(bool) overload. 
		Will get the buffer data from this object. */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && !std::is_pointer_v<T>)
		const T& get() const {
			if constexpr (std::is_arithmetic_v<T>) {
				return reinterpret_cast<const T&>(_buffer);
			}
			return reinterpret_cast<const T&>(_buffer);
		}

		/* Will get the pointer from this job.
		As such, it won't perform auto-cleanup, instead relying on the destructor, or manual freeing. 
		@param takeOwnership: specifies if (true) the pointer's ownership will be moved out and will not be auto-freed, 
		or (false) a copy of the pointer will be gotten. 
		If the pointer should be freed and the bool value is true, it will need to be manually deleted. */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && std::is_pointer_v<T>)
		T get(bool takeOwnership) {
			//gk_assertm(!isEmpty(), "JobRunDataBuffer has no data stored");
			if (takeOwnership) {
				_freeBufferFunc = gk::Fptr<void, JobRunDataBuffer*>(); 
				T ptr = *reinterpret_cast<T*>(_buffer);
				memset(_buffer, 0, sizeof(_buffer));
				return ptr;
			}
			return *reinterpret_cast<T*>(_buffer);
		}

		/* Will get the pointer from this job.
		As such, it won't perform auto-cleanup, instead relying on the destructor, or manual freeing. 
		@param takeOwnership: specifies if (true) the pointer's ownership will be moved out and will not be auto-freed, 
		or (false) a copy of the pointer will be gotten. 
		If the pointer should be freed and the bool value is true, it will need to be manually deleted. */
		template<typename T>
			requires(IsWithinJobDataBufferBounds<T> && std::is_pointer_v<T>)
		const T get(bool takeOwnership) const {
			//gk_assertm(!isEmpty(), "JobRunDataBuffer has no data stored");
			if (takeOwnership) {
				_freeBufferFunc = gk::Fptr<void, JobRunDataBuffer*>(); 
				T ptr = *reinterpret_cast<const T*>(_buffer);
				//memset(_buffer, 0, sizeof(_buffer)); Cannot memset cause const. Shouldn't matter though.
				return ptr;
			}
			return *reinterpret_cast<const T*>(_buffer);
		}
	
		uint8* getBuffer() {
			return reinterpret_cast<uint8*>(_buffer);
		}

		const uint8* getBuffer() const {
			return reinterpret_cast<const uint8*>(_buffer);
		}

		uint8& operator [] (const uint64 index) {
			gk_assertm(index < 32, "Attempted to read JobRunDataBuffer buffer index of " << index << " from a buffer of size 32");
			return getBuffer()[index];
		}

		const uint8& operator [] (const uint64 index) const {
			gk_assertm(index < 32, "Attempted to read JobRunDataBuffer buffer index of " << index << " from a buffer of size 32");
			return getBuffer()[index];
		}

	private:
		size_t _buffer[4];
		gk::Fptr<void, JobRunDataBuffer*> _freeBufferFunc;
	};

	struct ALIGN_AS(64) JobData // Aligned by cache line
	{
		gk::Event<void, JobRunDataBuffer*> jobFunc;
		JobRunDataBuffer data;

		JobData() = default;

		JobData(const JobData&) = delete;

		JobData(JobData && other) = default;

		~JobData() = default;

		JobData& operator = (const JobData&) = delete;

		JobData& operator = (JobData && other) = default;

	};
}

