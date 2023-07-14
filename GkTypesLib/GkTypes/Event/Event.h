#pragma once

#include <utility>
#include "../Asserts.h"

namespace gk 
{

	template<typename ReturnT, typename... Types>
	class Event;

	namespace {
		template<typename T, typename ReturnT, typename... Types>
		static gk::Event<ReturnT, Types...>* _MakeEvent(T* obj, ReturnT(T::* func)(Types...));

		template<typename T, typename ReturnT, typename... Types>
		static gk::Event<ReturnT, Types...>* _MakeEvent(T* obj, ReturnT(T::* func)(Types...)const);

		template<typename ReturnT, typename... Types>
		static gk::Event<ReturnT, Types...>* _MakeEvent(ReturnT(*func)(Types...));
	}

	/* Event that can callback a function (optionally a member function of a specific object).
	The first argument is the function return type.
	The variadic template arguments correspond to the arguments for both the member and non-member functions.
	For example, gk::event<void, int, float> would correspond to an event that takes an int and float parameters and returns nothing.
	Member functions do not need the invisible *this argument specified. */
	template<typename ReturnT, typename... Types>
	class Event {
	public:

		[[nodiscard]] virtual ReturnT Invoke(Types... vars) = 0;

		/* This function will assert if the event is created without an object. 
		This function will also assert if the event is created with a const object / function. */
		template<typename T>
		T* Obj() const { return (T*)GetObj(); }

		/* Creates an event that is bound to a specific object. Use Obj() to get the internal object.
		Explicitly uses non-const pointers to objects. */
		template<typename T>
		static Event* Create(T* obj, ReturnT (T::* func)(Types...)) {
			gk_assertNotNull(obj);
			return _MakeEvent<T, ReturnT, Types...>(obj, func);
		}

		/* Creates an event that is bound to a specific object's const member function. Use Obj() to get the internal object.
		Explicitly uses non-const pointers to objects. */
		template<typename T>
		static Event* Create(T* obj, ReturnT(T::* func)(Types...) const) {
			gk_assertNotNull(obj);
			return _MakeEvent<T, ReturnT, Types...>(obj, func);
		}

		/* Creates an event that is not bound to an object. Calling Obj() or ConstObj() will throw an assertion.
		See the other overload for Create(). */
		static Event* Create(ReturnT(*func)(Types...)) {
			return _MakeEvent<ReturnT, Types...>(func);
		}

	private:

		virtual void* GetObj() const = 0;
	
	};

	namespace {

		template<typename T, typename ReturnT, typename... Types>
		class _EventObjectImpl : public Event<ReturnT, Types...>
		{
			typedef ReturnT (T::* MemberFunc)(Types...);

		public:

			_EventObjectImpl(T* obj, MemberFunc func) : _obj(obj), _func(func) {}

			[[nodiscard]] virtual ReturnT Invoke(Types... vars) override {
				return (_obj->*_func)(vars...);
			}

		private:

			virtual void* GetObj() const override { return (void*)_obj; }

		private:

			T* _obj;
			MemberFunc _func;
		};

		template<typename T, typename ReturnT, typename... Types>
		class _EventObjectImplConst : public Event<ReturnT, Types...>
		{
			typedef ReturnT(T::* MemberFunc)(Types...) const;

		public:

			_EventObjectImplConst(T* obj, MemberFunc func) : _obj(obj), _func(func) {}

			[[nodiscard]] virtual ReturnT Invoke(Types... vars) override {
				return (_obj->*_func)(vars...);
			}

		private:

			virtual void* GetObj() const override { return (void*)_obj; }

		private:

			T* _obj;
			MemberFunc _func;
		};

		template<typename ReturnT, typename... Types>
		class _EventImpl : public Event<ReturnT, Types...>
		{
			typedef ReturnT (*Func)(Types...);

		public:

			_EventImpl(Func func) : _func(func) {}

			[[nodiscard]] virtual ReturnT Invoke(Types... vars) override {
				return _func(vars...);
			}

		private:

			virtual void* GetObj() const override {
				gk_assertm(false, "Cannot get the object for an event that does not use a member function");
				return nullptr;
			}

		private:

			Func _func;
		};

		template<typename T, typename ReturnT, typename... Types>
		inline static gk::Event<ReturnT, Types...>* _MakeEvent(T* obj, ReturnT(T::* func)(Types...)) {
			return new _EventObjectImpl(obj, func);
		}

		template<typename T, typename ReturnT, typename... Types>
		static gk::Event<ReturnT, Types...>* _MakeEvent(T* obj, ReturnT(T::* func)(Types...)const) {
			return new _EventObjectImplConst(obj, func);
		}

		template<typename ReturnT, typename... Types>
		inline static gk::Event<ReturnT, Types...>* _MakeEvent(ReturnT(*func)(Types...)) {
			return new _EventImpl(func);
		}

	}
	
}