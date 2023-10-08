#pragma once

#include <utility>
#include "../Asserts.h"

namespace gk 
{
	/* Don't need to allocate with new, and event also never calls new, only doing placement new on an internal buffer.
	Event that can callback a function (optionally a member function of a specific object).
	The first argument is the function return type.
	The variadic template arguments correspond to the arguments for both the member and non-member functions.
	For example, gk::event<void, int, float> would correspond to an event that takes an int and float parameters and returns nothing.
	Member functions do not need the invisible *this argument specified. */
	template<typename ReturnT, typename... Types>
	struct Event {
	private:

#pragma region Event_Implementation

		class _BaseEvent {
		public:
			virtual ReturnT invoke(Types... vars) const = 0;
			virtual bool isObject(const void* obj) const = 0;
			virtual void makeCopy(uint64* buffer) const = 0;
		};

		class _EventFreeFuncImpl : public _BaseEvent {
		public:

			typedef ReturnT(*Func)(Types...);

			_EventFreeFuncImpl(Func func) : _func(func) {}

			virtual ReturnT invoke(Types... vars) const override {
				return _func(vars...);
			}

			virtual bool isObject(const void* obj) const override {
				return false;
			}

			virtual void makeCopy(uint64* buffer) const override {
				new (buffer) _EventFreeFuncImpl(_func);
			}

			Func _func;
		};

		template <typename ObjT, typename FuncClassT>
		class _EventObjectImpl : public _BaseEvent {
		public:

			typedef ReturnT(FuncClassT::* MemberFunc)(Types...);
		
			_EventObjectImpl(ObjT* obj, MemberFunc func) : _obj(obj), _func(func) {}

			virtual ReturnT invoke(Types... vars) const override {
				return ((FuncClassT*)_obj->*_func)(vars...);
			}

			virtual bool isObject(const void* obj) const override {
				return _obj == obj;
			}

			virtual void makeCopy(uint64* buffer) const override {
				new (buffer) _EventObjectImpl(_obj, _func);
			}

			ObjT* _obj;
			MemberFunc _func;
		};

		template <typename ObjT, typename FuncClassT>
		class _EventConstObjectImpl : public _BaseEvent {
		public:

			typedef ReturnT(FuncClassT::* MemberFunc)(Types...) const;
		
			_EventConstObjectImpl(const ObjT* obj, MemberFunc func) : _obj(obj), _func(func) {}

			virtual ReturnT invoke(Types... vars) const override {
				return ((const FuncClassT*)_obj->*_func)(vars...);
			}

			virtual bool isObject(const void* obj) const override {
				return _obj == obj;
			}

			virtual void makeCopy(uint64* buffer) const override {
				new (buffer) _EventConstObjectImpl(_obj, _func);
			}

			const ObjT* _obj;
			MemberFunc _func;
		};

#pragma endregion

	public:

#pragma region Construct_Destruct_Assign

		Event() : _internalBuffer{ 0 } {}

		Event(ReturnT(*func)(Types...)) {
			new (_internalBuffer) _EventFreeFuncImpl(func);
		}

		template<typename T>
		Event(T* obj, ReturnT(T::* func)(Types...)) {
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(T* obj, ReturnT(Child::* func)(Types...)) {
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(Child* obj, ReturnT(T::* func)(Types...)) {
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T>
		Event(const T* obj, ReturnT(T::* func)(Types...) const) {
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(const T* obj, ReturnT(Child::* func)(Types...) const) {
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(const Child* obj, ReturnT(T::* func)(Types...) const) {
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		Event(const Event& other) {
			if (other.isBound()) {
				other.getEventObj()->makeCopy(_internalBuffer);
			}
		}

		Event(Event&& other) noexcept {
			memcpy(_internalBuffer, other._internalBuffer, 24);
			other._internalBuffer[0] = 0;
			other._internalBuffer[1] = 0;
			other._internalBuffer[2] = 0;
		}

		~Event() {
			freeEventObject();
		}

		Event& operator = (const Event& other) {
			freeEventObject();
			if (other.isBound()) {
				other.getEventObj()->makeCopy(_internalBuffer);
			}
			return *this;
		}

		Event& operator = (Event&& other) noexcept {
			memcpy(_internalBuffer, other._internalBuffer, 24);
			other._internalBuffer[0] = 0;
			other._internalBuffer[1] = 0;
			other._internalBuffer[2] = 0;
			return *this;
		}

#pragma endregion

#pragma region bind

		void bind(ReturnT(*func)(Types...)) {
			freeEventObject();
			new (_internalBuffer) _EventFreeFuncImpl(func);
		}

		template<typename T>
		void bind(T* obj, ReturnT(T::* func)(Types...)) {
			freeEventObject();
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(T* obj, ReturnT(Child::* func)(Types...)) {
			freeEventObject();
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(Child* obj, ReturnT(T::* func)(Types...)) {
			freeEventObject();
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}
		
		template<typename T>
		void bind(const T* obj, ReturnT(T::* func)(Types...)const) {
			freeEventObject();
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(const T* obj, ReturnT(Child::* func)(Types...) const) {
			freeEventObject();
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(const Child* obj, ReturnT(T::* func)(Types...) const) {
			freeEventObject();
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

#pragma endregion

		[[nodiscard]] ReturnT invoke(Types... vars) const {
			gk_assertm(isBound(), "Event not bound");
			return getEventObj()->invoke(vars...);
		}

		/* Check if the contained event object is the argument. */
		template<typename T>
		[[nodiscard]] bool isObject(const T* obj) const {
			gk_assertm(isBound(), "Event not bound");
			return getEventObj()->isObject((const void*)obj);
		}

		[[nodiscard]] bool isBound() const {
			return _internalBuffer[0] != 0;
		}

	private:

		const _BaseEvent* getEventObj() const {
			return reinterpret_cast<const _BaseEvent*>(_internalBuffer);
		}

		void freeEventObject() {
			if (getEventObj() != nullptr) {
				_internalBuffer[0] = 0;
				_internalBuffer[1] = 0;
				_internalBuffer[2] = 0;
			}
		}

	private:

		uint64 _internalBuffer[3];

	};

	
}