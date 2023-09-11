#pragma once

#include <type_traits>
#include "../Asserts.h"

namespace gk
{
	template<typename T, typename Enable=void>
	struct Option;
	/* 
		A specialization of gk::Option<> can be made the following way:

		struct Example {
			int a;
		};

		template<>
		struct gk::Option<Example> {
			// stuff
		};
	*/

	/* Optional type specialization for pointers. 
	All optional pointers are non-nullable, since the none() condition checks if null. */
	template<typename T>
	struct Option<T, typename std::enable_if_t<std::is_pointer_v<T>>>
	{
		Option() : _ptr(nullptr) {}

		Option(T ptr) : _ptr(ptr) {}

		Option(const Option<T>& other) : _ptr(other._ptr) {}

		Option(Option<T>&& other) noexcept : _ptr(other._ptr) {
#if GK_CHECK // Ensure not use a moved instance
			other._ptr = nullptr;
#endif
		}

		~Option() {
#if GK_CHECK // Ensure cannot use after explicit delete
			_ptr = nullptr;
#endif
		}

		void operator = (T ptr) {
			_ptr = ptr;
		}

		void operator = (const Option<T>& other) {
			_ptr = other._ptr;
		}

		void operator = (Option<T>&& other) noexcept {
			_ptr = other._ptr; 
#if GK_CHECK // Ensure not use a moved instance
				other._ptr = nullptr;
#endif
		}

		[[nodiscard]] T some() {
			gk_assertm(!none(), "Cannot get optional pointer value if its none");
			return _ptr;
		}

		[[nodiscard]] const T some() const {
			gk_assertm(!none(), "Cannot get optional const pointer value if its none");
			return _ptr;
		}

		[[nodiscard]] bool none() const {
			return _ptr == nullptr;
		}

	private:

		T _ptr;
	};

	/* Optional type specialization for non-pointer types. */
	template<typename T>
	struct Option<T, typename std::enable_if_t< !std::is_pointer_v<T>>>
	{
		Option() : _hasValue(false) {
			if constexpr (std::is_fundamental_v<T>) {
				_value = static_cast<T>(0);
			}
		}

		Option(const T& value) : _value(value), _hasValue(true) {}

		Option(T&& value) noexcept : _value(std::move(value)), _hasValue(true) {}

		Option(const Option<T>& other) : _value(other._value), _hasValue(other._hasValue) {}

		Option(Option<T>&& other) noexcept : _value(std::move(other._value)), _hasValue(other._hasValue) {
#if GK_CHECK // Ensure not use a moved instance
			other._hasValue = false;
#endif
		}

		~Option() {
#if GK_CHECK // Ensure cannot use after explicit delete
			_hasValue = false;
#endif
		}

		void operator = (const T& value) {
			_value = value;
			_hasValue = true;
		}

		void operator = (T&& value) noexcept {
			_value = std::move(value);
			_hasValue = true;
		}

		void operator = (const Option<T>& other) {
			_value = other._value;
			_hasValue = other._hasValue;
		}

		void operator = (Option<T>&& other) noexcept {
			_value = std::move(other._value);
			_hasValue = other._hasValue;
		}

		[[nodiscard]] T some() {
			gk_assertm(!none(), "Cannot get optional value if its none");
			return _value;
		} 
		
		[[nodiscard]] const T some() const {
			gk_assertm(!none(), "Cannot get optional const value if its none");
			return _value;
		}

		/* Explicitly move the value and invalidate this option. */
		[[nodiscard]] T&& someMove() {
			gk_assertm(!none(), "Cannot move optional value if its none"); 
#if GK_CHECK // Ensure cannot use after value's ownership is moved
				_hasValue = false;
#endif
			return std::move(_value);
		}

		[[nodiscard]] bool none() const {
			return !_hasValue;
		}

	private:

		T _value;
		bool _hasValue;

	};


}