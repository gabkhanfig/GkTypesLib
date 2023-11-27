#pragma once

#include "../basic_types.h"
#include <utility>
#include "../doctest/doctest_proxy.h"
#include <type_traits>

namespace gk 
{
	/* Don't need to allocate with new, and Callback also never calls new, only doing placement new on an internal buffer.
	Callback that can callback a function (optionally a member function of a specific object).
	The first argument is the function return type.
	The variadic template arguments correspond to the arguments for both the member and non-member functions.
	For example, gk::Callback<void, int, float> would correspond to an callback that takes an int and float parameters and returns nothing.
	Member functions do not need the invisible *this argument specified. */
	template<typename ReturnT, typename... Types>
	struct Callback {
	private:

#pragma region Event_Implementation

		class _BaseEvent {
		public:
			virtual ReturnT invoke(Types... vars) const = 0;
			virtual bool isObject(const void* obj) const = 0;
			virtual void makeCopy(u64* buffer) const = 0;
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

			virtual void makeCopy(u64* buffer) const override {
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

			virtual void makeCopy(u64* buffer) const override {
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

			virtual void makeCopy(u64* buffer) const override {
				new (buffer) _EventConstObjectImpl(_obj, _func);
			}

			const ObjT* _obj;
			MemberFunc _func;
		};

#pragma endregion

	public:

#pragma region Construct_Destruct_Assign

		Callback() : _internalBuffer{ 0 } {}

		Callback(ReturnT(*func)(Types...)) {
			new (_internalBuffer) _EventFreeFuncImpl(func);
		}

		template<typename T>
		Callback(T* obj, ReturnT(T::* func)(Types...)) {
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Callback(T* obj, ReturnT(Child::* func)(Types...)) {
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Callback(Child* obj, ReturnT(T::* func)(Types...)) {
			new (_internalBuffer) _EventObjectImpl(obj, func);
		}

		template<typename T>
		Callback(const T* obj, ReturnT(T::* func)(Types...) const) {
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Callback(const T* obj, ReturnT(Child::* func)(Types...) const) {
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		template<typename T, typename Child>
			requires(std::is_base_of_v<T, Child>)
		Callback(const Child* obj, ReturnT(T::* func)(Types...) const) {
			new (_internalBuffer) _EventConstObjectImpl(obj, func);
		}

		Callback(const Callback& other) {
			if (other.isBound()) {
				other.getEventObj()->makeCopy(_internalBuffer);
			}
		}

		Callback(Callback&& other) noexcept {
			memcpy(_internalBuffer, other._internalBuffer, 24);
			other._internalBuffer[0] = 0;
			other._internalBuffer[1] = 0;
			other._internalBuffer[2] = 0;
		}

		~Callback() {
			freeEventObject();
		}

		Callback& operator = (const Callback& other) {
			freeEventObject();
			if (other.isBound()) {
				other.getEventObj()->makeCopy(_internalBuffer);
			}
			return *this;
		}

		Callback& operator = (Callback&& other) noexcept {
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
			check_message(isBound(), "Event not bound");
			return getEventObj()->invoke(vars...);
		}

		/* Check if the contained event object is the argument. */
		template<typename T>
		[[nodiscard]] bool isObject(const T* obj) const {
			check_message(isBound(), "Event not bound");
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
			if (isBound()) {
				_internalBuffer[0] = 0;
				_internalBuffer[1] = 0;
				_internalBuffer[2] = 0;
			}
		}

	private:

		u64 _internalBuffer[3];

	};

	
}