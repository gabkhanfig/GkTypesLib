#pragma once

#include <utility>
#include "../Asserts.h"

namespace gk 
{
	/* Don't need to allocate with new, manages it's own memory.
	Event that can callback a function (optionally a member function of a specific object).
	The first argument is the function return type.
	The variadic template arguments correspond to the arguments for both the member and non-member functions.
	For example, gk::event<void, int, float> would correspond to an event that takes an int and float parameters and returns nothing.
	Member functions do not need the invisible *this argument specified. */
	template<typename ReturnT, typename... Types>
	struct Event {
	private:

		class _BaseEvent {
		public:
			virtual ReturnT invoke(Types... vars) const = 0;
			virtual bool isObject(const void* obj) const = 0;
			virtual const _BaseEvent* makeCopy() const = 0;
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

			virtual const _BaseEvent* makeCopy() const override {
				return new _EventFreeFuncImpl(_func);
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

			virtual const _BaseEvent* makeCopy() const override {
				return new _EventObjectImpl(_obj, _func);
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

			virtual _BaseEvent* makeCopy() const override {
				return new _EventConstObjectImpl(_obj, _func);
			}

			const ObjT* _obj;
			MemberFunc _func;
		};

	public:

#pragma region Construct_Destruct_Assign

		Event() : _eventObj(nullptr) {}

		Event(ReturnT(*func)(Types...)) {
			_eventObj = new _EventFreeFuncImpl(func);
		}

		template<typename T>
		Event(T* obj, ReturnT(T::* func)(Types...)) {
			_eventObj = new _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(T* obj, ReturnT(Child::* func)(Types...)) {
			_eventObj = new _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(Child* obj, ReturnT(T::* func)(Types...)) {
			_eventObj = new _EventObjectImpl(obj, func);
		}

		template<typename T>
		Event(const T* obj, ReturnT(T::* func)(Types...) const) {
			_eventObj = new _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(const T* obj, ReturnT(Child::* func)(Types...) const) {
			_eventObj = new _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Event(const Child* obj, ReturnT(T::* func)(Types...) const) {
			_eventObj = new _EventConstObjectImpl(obj, func);
		}

		Event(const Event& other) {
			_eventObj = other._eventObj->makeCopy();
		}

		Event(Event&& other) noexcept {
			_eventObj = other._eventObj;
			other._eventObj = nullptr;
		}

		~Event() {
			freeEventObject();
		}

		Event& operator = (const Event& other) {
			freeEventObject();
			_eventObj = other._eventObj->makeCopy();
			return *this;
		}

		Event& operator = (Event&& other) noexcept {
			_eventObj = other._eventObj;
			other._eventObj = nullptr;
			return *this;
		}

#pragma endregion

#pragma region bind

		void bind(ReturnT(*func)(Types...)) {
			freeEventObject();
			_eventObj = new _EventFreeFuncImpl(func);
		}

		template<typename T>
		void bind(T* obj, ReturnT(T::* func)(Types...)) {
			freeEventObject();
			_eventObj = new _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(T* obj, ReturnT(Child::* func)(Types...)) {
			freeEventObject();
			_eventObj = new _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(Child* obj, ReturnT(T::* func)(Types...)) {
			freeEventObject();
			_eventObj = new _EventObjectImpl(obj, func);
		}
		
		template<typename T>
		void bind(const T* obj, ReturnT(T::* func)(Types...)const) {
			freeEventObject();
			_eventObj = new _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(const T* obj, ReturnT(Child::* func)(Types...) const) {
			freeEventObject();
			_eventObj = new _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		void bind(const Child* obj, ReturnT(T::* func)(Types...) const) {
			freeEventObject();
			_eventObj = new _EventConstObjectImpl(obj, func);
		}

#pragma endregion

		[[nodiscard]] ReturnT invoke(Types... vars) const {
			gk_assertm(isBound(), "Event not bound");
			return _eventObj->invoke(vars...);
		}

		/* Check if the contained event object is the argument. */
		template<typename T>
		[[nodiscard]] bool isObject(const T* obj) const {
			gk_assertm(isBound(), "Event not bound");
			return _eventObj->isObject((const void*)obj);
		}

		[[nodiscard]] bool isBound() const {
			return _eventObj != nullptr;
		}

	private:

		void freeEventObject() {
			if (_eventObj != nullptr) {
				delete _eventObj;
			}
		}

	private:

		const _BaseEvent* _eventObj;

	};

	
}