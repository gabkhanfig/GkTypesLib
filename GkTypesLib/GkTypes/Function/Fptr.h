#pragma once

#include "../Asserts.h"

namespace gk 
{
	template<typename ReturnT, typename... Types>
	struct Fptr
	{
		typedef ReturnT(*FuncPtrT)(Types...);

		Fptr() : _func(nullptr) {}

		Fptr(const FuncPtrT func) { bind(func); }

		Fptr(const Fptr<ReturnT, Types...>& other) { _func = other._func; }

		Fptr(Fptr<ReturnT, Types...>&& other) { 
			_func = other._func;
#if GK_CHECK // Ensure not use a moved instance
			other._func = nullptr;
#endif
		}

		~Fptr() {
#if GK_CHECK // Ensure cannot use after explicit delete
			_func = nullptr;
#endif
		}

		inline void operator = (const FuncPtrT func) { bind(func); }

		inline void operator = (const Fptr<ReturnT, Types...>& other) { _func = other._func; }

		inline void operator = (Fptr<ReturnT, Types...>&& other) { 
			_func = other._func;
#if GK_CHECK // Ensure not use a moved instance
			other._func = nullptr;
#endif
		}

		inline void bind(FuncPtrT func) {
			gk_assertm(func != nullptr, "Cannot bind null function pointer to gk::FuncPtr");
			_func = func;
		}

		[[nodiscard]] inline bool isBound() const {
			return _func != nullptr;
		}

		[[nodiscard]] inline ReturnT invoke(Types... vars) const {
			gk_assertm(isBound(), "Cannot execute not bound function pointer");
			return _func(vars...);
		}

	private:

		FuncPtrT _func;

	};
}