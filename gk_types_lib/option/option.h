#pragma once

#include <type_traits>
#include "../doctest/doctest_proxy.h"
#include <iostream>

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
		constexpr Option() : _ptr(nullptr) {}

		constexpr Option(T ptr) : _ptr(ptr) {}

		constexpr Option(Option<T>&& other) noexcept : _ptr(other._ptr) {
			// Ensure not use a moved instance
			other._ptr = nullptr;
		}

		constexpr ~Option() = default;

		constexpr void operator = (T ptr) {
			_ptr = ptr;
		}

		constexpr void operator = (Option<T>&& other) noexcept {
			_ptr = other._ptr; 
			other._ptr = nullptr;
		}

		[[nodiscard]] constexpr T some() {
			check_message(!none(), "Cannot get optional pointer value if its None. Either no pointer is stored, or it has been already moved out of Option ownership");
			T tempPtr = _ptr;
			_ptr = nullptr;
			return tempPtr;
		}

		[[nodiscard]] constexpr T someCopy() const {
			check_message(!none(), "Cannot get optional pointer value if its None. Either no pointer is stored, or it has been already moved out of Option ownership");
			return _ptr;
		}

		[[nodiscard]] constexpr bool none() const {
			return _ptr == nullptr;
		}

		[[nodiscard]] constexpr bool isSome() {
			return !none();
		}

	private:

		T _ptr;
	};

	/* Optional type specialization for reference types. */
	template<typename T>
	struct Option<T, typename std::enable_if_t< !std::is_pointer_v<T> && std::is_reference_v<T>>>
	{
		constexpr Option() : _hasValue(false), _emptyValue(false) {}

		constexpr ~Option() = default;

		constexpr Option(const Option<T>& other) = delete;
		constexpr Option& operator = (const Option<T>& other) = delete;

		constexpr Option(T value) : _hasValue(true), _refValue(value) {}

		constexpr Option(Option<T>&& other) : _hasValue(other._hasValue) {
			if (this->_hasValue) {
				std::construct_at(&this->_refValue, other._refValue);
			}
			else {
				std::construct_at(&this->_emptyValue, false);
			}
			other._hasValue = false;
			std::construct_at(&other._emptyValue, false);
		}

		constexpr void operator = (T value) {
			this->_hasValue = true;
			std::construct_at(&this->_refValue, value);	
		}

		constexpr void operator = (Option<T>&& other) noexcept {
			this->_hasValue = other._hasValue;
			if (this->_hasValue) {
				std::construct_at(&this->_refValue, other._refValue);
			}
			else {
				std::construct_at(&this->_emptyValue, false);
			}
			other._hasValue = false;
			std::construct_at(&other._emptyValue, false);
		}

		[[nodiscard]] constexpr T some() {
			check_message(!none(), "Cannot get optional reference value if its None. Either no reference is stored, or the reference has been already moved out of Option ownership");
			_hasValue = false;
			T temp = this->_refValue.get();
			std::construct_at(&this->_emptyValue, false);
			return temp;
		}

		[[nodiscard]] constexpr T someCopy() const {
			check_message(!none(), "Cannot get optional reference value if its None. Either no reference is stored, or the reference has been already moved out of Option ownership");
			return this->_refValue.get();
		}

		[[nodiscard]] constexpr bool none() const {
			return !_hasValue;
		}

		[[nodiscard]] constexpr bool isSome() const {
			return !none();
		}

	private:

		union {
			std::reference_wrapper<std::remove_reference_t<T>> _refValue;
			bool _emptyValue;
		};
		bool _hasValue;

	};

	/* Optional type specialization for non-pointer types. */
	template<typename T>
	struct Option<T, typename std::enable_if_t< !std::is_pointer_v<T> && !std::is_reference_v<T>>>
	{
		constexpr Option() : _hasValue(false), _value(T()) {
			if constexpr (std::is_fundamental_v<T>) {
				_value = static_cast<T>(0);
			}
		}

		constexpr Option(const T& value) : _value(value), _hasValue(true) {}

		constexpr Option(T&& value) noexcept : _value(std::move(value)), _hasValue(true) {}

		constexpr Option(Option<T>&& other) noexcept : _value(std::move(other._value)), _hasValue(other._hasValue) {
#if GK_CHECK // Ensure not use a moved instance
			other._hasValue = false;
#endif
		}

		constexpr ~Option() = default;

		constexpr void operator = (const T& value) {
			_value = value;
			_hasValue = true;
		}

		constexpr void operator = (T&& value) noexcept {
			_value = std::move(value);
			_hasValue = true;
		}

		constexpr void operator = (Option<T>&& other) noexcept {
			_value = std::move(other._value);
			_hasValue = other._hasValue;
			other._hasValue = false;
		}

		[[nodiscard]] constexpr T some() {
			check_message(!none(), "Cannot get optional value value if its None. Either no value is stored, or the value has been already moved out of Option ownership");
			_hasValue = false;
			return std::move(_value);
		}

		[[nodiscard]] constexpr T someCopy() const {
			check_message(!none(), "Cannot get optional value value if its None. Either no value is stored, or the value has been already moved out of Option ownership");
			return _value;
		}

		[[nodiscard]] constexpr bool none() const {
			return !_hasValue;
		}

		[[nodiscard]] constexpr bool isSome() const {
			return !none();
		}

	private:

		T _value;
		bool _hasValue;

	};


}