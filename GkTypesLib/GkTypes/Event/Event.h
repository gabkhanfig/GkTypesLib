#pragma once

#include <utility>
#include "../Asserts.h"

namespace gk 
{

	template<typename... Types>
	class Event;

	namespace {
		template<typename T, typename... Types>
		static gk::Event<Types...>* _MakeEvent(T* obj, void(T::* func)(Types...));

		template<typename... Types>
		static gk::Event<Types...>* _MakeEvent(void(*func)(Types...));
	}

	/* Event that can callback a function (optionally a member function of a specific object). 
	The variadic template arguments correspond to the arguments for both the member and non-member functions.
	For example, gk::event<int, float> would correspond to an event that takes an int and float parameters.
	Member functions do not need the invisible *this argument specified. */
	template<typename... Types>
	class Event {
	public:

		virtual void Invoke(Types... vars) = 0;

		/* This function will assert if the event is created without an object. */
		template<typename T>
		T* Obj() const { return (T*)GetObj(); }

		/* Creates an event that is bound to a specific object. Use Obj() to get the internal object. 
		See the other overload for Create(). */
		template<typename T>
		static Event* Create(T* obj, void (T::* func)(Types...)) {
			gk_assertNotNull(obj);
			return _MakeEvent<T, Types...>(obj, func);
		}

		/* Creates an event that is not bound to an object. Calling Obj() will throw an assertion if GK_CHECK is true.
		See the other overload for Create(). */
		static Event* Create(void(*func)(Types...)) {
			return _MakeEvent<Types...>(func);
		}

	private:

		virtual void* GetObj() const = 0;
	
	};

	namespace {

		template<typename T, typename... Types>
		class _EventObjectImpl : public Event<Types...>
		{
			typedef void (T::* MemberFunc)(Types...);

		public:

			_EventObjectImpl(T* obj, MemberFunc func) : _obj(obj), _func(func) {}

			virtual void Invoke(Types... vars) override {
				(_obj->*_func)(vars...);
			}

		private:

			virtual void* GetObj() const override { return (void*)_obj; }

		private:

			T* _obj;
			MemberFunc _func;
		};

		template<typename... Types>
		class _EventImpl : public Event<Types...>
		{
			typedef void (*Func)(Types...);

		public:

			_EventImpl(Func func) : _func(func) {}

			virtual void Invoke(Types... vars) override {
				_func(vars...);
			}

		private:

			virtual void* GetObj() const override {
				gk_assertm(false, "Cannot get the object for an event that does not use a member function");
				return nullptr;
			}

		private:

			Func _func;
		};

		template<typename T, typename... Types>
		inline static gk::Event<Types...>* _MakeEvent(T* obj, void(T::* func)(Types...)) {
			return new _EventObjectImpl(obj, func);
		}

		template<typename... Types>
		inline static gk::Event<Types...>* _MakeEvent(void(*func)(Types...)) {
			return new _EventImpl(func);
		}

	}
	
}