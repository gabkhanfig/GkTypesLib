#pragma once

#include "../doctest/doctest_proxy.h"

namespace gk 
{
	/* Function Pointer handler. 
	The first template argument is the return type, 
	and the variadic arguments after are the function arguments. */
	template<typename ReturnT, typename... Types>
	struct Fptr
	{
		typedef ReturnT(*FuncPtrT)(Types...);

		Fptr() : _func(nullptr) {}

		Fptr(const FuncPtrT func) { bind(func); }

		Fptr(const Fptr<ReturnT, Types...>& other) { _func = other._func; }

		Fptr(Fptr<ReturnT, Types...>&& other) noexcept { 
			_func = other._func;
			other._func = nullptr;
		}

		~Fptr() {
			_func = nullptr;
		}

		inline void operator = (const FuncPtrT func) { bind(func); }

		inline void operator = (const Fptr<ReturnT, Types...>& other) { _func = other._func; }

		inline void operator = (Fptr<ReturnT, Types...>&& other) noexcept {
			_func = other._func;
			other._func = nullptr;
		}

		/* Will assert if func is nullptr. */
		inline void bind(FuncPtrT func) {
			check_message(func != nullptr, "Cannot bind null function pointer to gk::Fptr");
			_func = func;
		}

		[[nodiscard]] inline bool isBound() const {
			return _func != nullptr;
		}

		/* Execute the bound function. Asserts if the function is not bound. See gk::Fptr<>::isBound() */
		[[nodiscard]] inline ReturnT invoke(Types... vars) const {
			check_message(isBound(), "Cannot execute not bound function pointer");
			return _func(vars...);
		}

	private:

		FuncPtrT _func;

	};
}