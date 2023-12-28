#pragma once

#include "../error/result.h"
#include "../string/string.h"
#include "../array/array_list.h"
#include "../hash/hashmap.h"
#include "../string/string_type_conversion.h"

// https://www.json.org/json-en.html

namespace gk
{
	enum class JsonValueType : usize {
		Null,
		Bool,
		Number,
		String,
		Array,
		Object
	};

	struct JsonValue;

	namespace internal
	{
		struct JsonKeyValue;
		struct JsonObjectBucket;
	}

	/**
	*/
	struct JsonObject {

		constexpr JsonObject() : buckets(nullptr), bucketCount(0), elementCount(0) {}

		constexpr JsonObject(const JsonObject& other);

		constexpr JsonObject(JsonObject&& other) noexcept
			: buckets(other.buckets), bucketCount(other.bucketCount), elementCount(other.elementCount)
		{
			other.buckets = nullptr;
			other.bucketCount = 0;
			other.elementCount = 0;
		}

		constexpr JsonObject& operator = (JsonObject&& other) noexcept;

		constexpr JsonObject& operator = (const JsonObject& other);

		constexpr ~JsonObject();

		/**
		* Parse a string slice into a JsonObject, returning an Ok variant with the object.
		* If the slice could not be successfully parsed, an Error variant is returned.
		* 
		* @param jsonString: String slice representing a json object.
		* @return An Ok variant with the parsed JsonObject, or an Error.
		*/
		[[nodiscard]] static constexpr Result<JsonObject> parse(Str jsonString);

		/**
		* Convert this JsonObject into string representation following JSON formatting.
		* 
		* @return This JsonObject as a string.
		*/
		[[nodiscard]] constexpr String toString(usize nestCount = 0) const;

		/**
		* Find a mutable json value contained within this JsonObject.
		* If it exists, a Some option containing the JsonValue will be returned,
		* otherwise a None variant is returned.
		* 
		* NOTE: The JsonObject does not have pointer stability. Subsequent mutation operations on the JsonObject may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		* 
		* @param name: The name/key of the field.
		* @return The value if it exists, or None.
		*/
		[[nodiscard]] constexpr Option<JsonValue*> findField(const String& name);

		/**
		* Find an immutable json value contained within this JsonObject.
		* If it exists, a Some option containing the JsonValue will be returned,
		* otherwise a None variant is returned.
		*
		* NOTE: The JsonObject does not have pointer stability. Subsequent mutation operations on the JsonObject may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @param name: The name/key of the field.
		* @return The value if it exists, or None.
		*/
		[[nodiscard]] constexpr Option<const JsonValue*> findField(const String& name) const;

		/**
		* Invalidates any iterators.
		* Inserts an entry into the JsonObject if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		*
		* NOTE: The JsonObject does not have pointer stability. Subsequent mutation operations on the JsonObject may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @param name: The name/key of the field.
		* @param value: The json value itself.
		* @return The entry if it already exists in the JsonObject, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		constexpr Option<JsonValue*> addField(String&& name, JsonValue&& value);

		/**
		* Invalidates any iterators.
		* Inserts an entry into the JsonObject if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		*
		* NOTE: The JsonObject does not have pointer stability. Subsequent mutation operations on the JsonObject may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @param name: The name/key of the field.
		* @param value: The json value itself.
		* @return The entry if it already exists in the JsonObject, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		constexpr Option<JsonValue*> addField(const String& name, JsonValue&& value);

		/**
		* Invalidates any iterators.
		* Inserts an entry into the JsonObject if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		*
		* NOTE: The JsonObject does not have pointer stability. Subsequent mutation operations on the JsonObject may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @param name: The name/key of the field.
		* @param value: The json value itself.
		* @return The entry if it already exists in the JsonObject, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		constexpr Option<JsonValue*> addField(String&& name, const JsonValue& value);

		/**
		* Invalidates any iterators.
		* Inserts an entry into the JsonObject if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		*
		* NOTE: The JsonObject does not have pointer stability. Subsequent mutation operations on the JsonObject may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @param name: The name/key of the field.
		* @param value: The json value itself.
		* @return The entry if it already exists in the JsonObject, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		constexpr Option<JsonValue*> addField(const String& name, const JsonValue& value);

		/**
		* Invalidates any iterators.
		* Removes an entry from the JsonObject if it exists.
		* 
		* @return True if the entry does exist and was erased, or false if it doesn't exist. Can be ignored.
		*/
		constexpr bool eraseField(const String& name);

		/**
		* @return The number of fields within the JsonObject.
		*/
		[[nodiscard]] constexpr usize fieldCount() const { return elementCount; }

		friend class Iterator;
		friend class ConstIterator;

		class Iterator {
		public:

			struct Pair {
				constexpr Pair(const String& name, JsonValue& value) : name(name), value(value) {}

				const String& name;
				JsonValue& value;
			};

			constexpr ~Iterator() = default;

			constexpr static Iterator iterBegin(JsonObject* object);

			constexpr static Iterator iterEnd(JsonObject* object);

			constexpr Iterator& operator++();

			constexpr bool operator ==(const Iterator& other) const;

			constexpr Pair operator*() const;

			constexpr void step();

		private:
			constexpr Iterator() : _object(nullptr), _currentBucket(nullptr), _currentElementIndex(0) {}

			JsonObject* _object;
			internal::JsonObjectBucket* _currentBucket;
			usize _currentElementIndex;
		}; // class Iterator

		class ConstIterator {
		public:

			struct Pair {
				constexpr Pair(const String& name, const JsonValue& value) : name(name), value(value) {}

				const String& name;
				const JsonValue& value;
			};

			constexpr ~ConstIterator() = default;

			constexpr static ConstIterator iterBegin(const JsonObject* object);

			constexpr static ConstIterator iterEnd(const JsonObject* object);

			constexpr ConstIterator& operator++();

			constexpr bool operator ==(const ConstIterator& other) const;

			constexpr Pair operator*() const;

			constexpr void step();

		public:
			constexpr ConstIterator() : _object(nullptr), _currentBucket(nullptr), _currentElementIndex(0) {}

			const JsonObject* _object;
			const internal::JsonObjectBucket* _currentBucket;
			usize _currentElementIndex;
		}; // class ConstIterator

		/**
		* Begin of an Iterator with immutable keys, and mutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return Begin of mutable iterator
		*/
		constexpr Iterator begin() { return Iterator::iterBegin(this); }

		/**
		* End of an Iterator with immutable keys, and mutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of mutable iterator
		*/
		constexpr Iterator end() { return Iterator::iterEnd(this); }

		/**
		* Begin of an Iterator with immutable keys, and immutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return Begin of immutable iterator
		*/
		constexpr ConstIterator begin() const { return ConstIterator::iterBegin(this); }

		/**
		* End of an Iterator with immutable keys, and immutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of immutable iterator
		*/
		constexpr ConstIterator end() const { return ConstIterator::iterEnd(this); }

	private:

		constexpr bool shouldReallocate(usize requiredCapacity);

		constexpr void reallocate(usize requiredCapacity);

		usize calculateNewBucketCount(usize requiredCapacity);

		void reallocateRuntime(usize requiredCapacity);

		Option<JsonValue*> addFieldRuntime(String&& name, JsonValue&& value);

		bool eraseFieldRuntime(const String& name);

	private:

		internal::JsonObjectBucket* buckets;
		usize bucketCount;
		usize elementCount;

	}; // struct JsonObject

	namespace internal
	{
		union JsonValueUnion {
			usize null;
			bool boolean;
			double number;
			gk::String string;
			ArrayList<JsonValue> arr;
			JsonObject object;

			constexpr JsonValueUnion() : null(0) {}
			constexpr JsonValueUnion(String&& inString) : string(std::move(inString)) {}
			constexpr JsonValueUnion(ArrayList<JsonValue>&& inArray) : arr(std::move(inArray)) {}
			constexpr JsonValueUnion(JsonObject&& inObject) : object(std::move(inObject)) {}
			constexpr ~JsonValueUnion() {}
		};
	}
	
	/**
	*/
	struct JsonValue {

		constexpr JsonValue() : _type(JsonValueType::Null) {}

		constexpr JsonValue(const JsonValue& other);

		constexpr JsonValue(JsonValue&& other) noexcept;

		constexpr JsonValue& operator = (const JsonValue& other);

		constexpr JsonValue& operator = (JsonValue&& other) noexcept;

		constexpr ~JsonValue();

		static constexpr JsonValue makeNull() { return JsonValue(); }

		static constexpr JsonValue makeBool(bool b);

		static constexpr JsonValue makeNumber(double num);

		static constexpr JsonValue makeString(String&& string);

		static constexpr JsonValue makeString(const String& string) { return JsonValue::makeString(String(string)); }

		static constexpr JsonValue makeArray(ArrayList<JsonValue>&& arr);

		static constexpr JsonValue makeArray(const ArrayList<JsonValue>& arr) { return JsonValue::makeArray(ArrayList<JsonValue>(arr)); }

		static constexpr JsonValue makeObject(JsonObject&& object);

		static constexpr JsonValue makeObject(const JsonObject& object) { return JsonValue::makeObject(JsonObject(object)); }

		/**
		* Gets the type of the json value data.
		*/
		constexpr JsonValueType type() const {
			return _type;
		}

		/**
		* @return If the value is set to null.
		*/
		constexpr bool isNull() const {
			return _type == JsonValueType::Null;
		}

		/**
		* Will assert that the stored type is bool.
		* 
		* @return The boolean json value.
		*/
		constexpr bool& boolValue() {
			check_eq(_type, JsonValueType::Bool);
			return _value.boolean;
		}

		/**
		* Will assert that the stored type is bool.
		* 
		* @return The boolean json value.
		*/
		constexpr const bool& boolValue() const {
			check_eq(_type, JsonValueType::Bool);
			return _value.boolean;
		}

		/**
		* Will assert that the stored type is number.
		* Due to compatibility, only doubles are used by the number value.
		* This means 2^53 is the maximum integer value without precision loss.
		* For higher precision, use strings.
		*
		* @return The number json value.
		*/
		constexpr double& numberValue() {
			check_eq(_type, JsonValueType::Number);
			return _value.number;
		}

		/**
		* Will assert that the stored type is number.
		* Due to compatibility, only doubles are used by the number value.
		* This means 2^53 is the maximum integer value without precision loss.
		* For higher precision, use strings.
		*
		* @return The number json value.
		*/
		constexpr const double& numberValue() const {
			check_eq(_type, JsonValueType::Number);
			return _value.number;
		}

		/**
		* Will assert that the stored type is string.
		* 
		* @return The string json value.
		*/
		constexpr gk::String& stringValue() {
			check_eq(_type, JsonValueType::String);
			return _value.string;
		}

		/**
		* Will assert that the stored type is string.
		* 
		* @return The string json value.
		*/
		constexpr const gk::String& stringValue() const {
			check_eq(_type, JsonValueType::String);
			return _value.string;
		}

		/**
		* Will assert that the stored type is an array of json values.
		* 
		* @return The array of json values held by this value.
		*/
		constexpr ArrayList<JsonValue>& arrayValue() {
			check_eq(_type, JsonValueType::Array);
			return _value.arr;
		}

		/**
		* Will assert that the stored type is an array of json values.
		* 
		* @return The array of json values held by this value.
		*/
		constexpr const ArrayList<JsonValue>& arrayValue() const {
			check_eq(_type, JsonValueType::Array);
			return _value.arr;
		}

		/**
		* Will assert that the stored type is an object.
		* 
		* @return The json string containing a sub-object.
		*/
		constexpr JsonObject& objectValue() {
			check_eq(_type, JsonValueType::Object);
			return _value.object;
		}

		/**
		* Will assert that the stored type is an object.
		* 
		* @return The json string containing a sub-object.
		*/
		constexpr const JsonObject& objectValue() const {
			check_eq(_type, JsonValueType::Object);
			return _value.object;
		}

		/**
		* Get the string representation of the json value.
		*/
		constexpr String toString(usize objectNestCount = 0) const;

	private:

		constexpr JsonValue(String&& inString)
			: _type(JsonValueType::String), _value(std::move(inString))
		{}

		constexpr JsonValue(ArrayList<JsonValue>&& inArr)
			: _type(JsonValueType::Array), _value(std::move(inArr))
		{}

		constexpr JsonValue(JsonObject&& inObject)
			: _type(JsonValueType::Object), _value(std::move(inObject))
		{}

		JsonValueType _type;
		internal::JsonValueUnion _value;
	};

	namespace internal
	{
		/**
		*
		*/
		struct JsonKeyValue {
			String key;
			JsonValue value;
			usize hashCode;

			constexpr JsonKeyValue() : hashCode(0) {};
			constexpr ~JsonKeyValue() = default;

			constexpr JsonKeyValue(String&& key, JsonValue&& value, usize hashCode)
				: key(std::move(key)), value(std::move(value)), hashCode(hashCode)
			{}
		};

		struct JsonHashBucketBits {
			static constexpr usize BITMASK = ~0b01111111ULL;
			usize value;

			constexpr JsonHashBucketBits(const usize hashCode)
				: value((hashCode& BITMASK) >> 7) {}
		};

		struct JsonPairHashBits {
			static constexpr i8 BITMASK = 0b01111111;
			i8 value;

			constexpr JsonPairHashBits(const usize hashCode)
				: value((hashCode& BITMASK) | 0b10000000) {}
		};

		/**
		*/
		struct JsonObjectBucket {

			constexpr JsonObjectBucket(Allocator* allocator);

			constexpr JsonObjectBucket(JsonObjectBucket&& other) noexcept;

			constexpr ~JsonObjectBucket() = default;

			constexpr JsonObjectBucket(const JsonObjectBucket&) = delete;
			constexpr JsonObjectBucket& operator = (const JsonObjectBucket&) = delete;
			constexpr JsonObjectBucket& operator = (JsonObjectBucket&&) = delete;

			constexpr void free(Allocator* allocator);
			
			constexpr Option<JsonValue*> find(const String& key, usize hashCode);

			constexpr Option<const JsonValue*> find(const String& key, usize hashCode) const;

			constexpr void insert(String&& key, JsonValue&& value, usize hashCode, Allocator* allocator);

			constexpr bool erase(const String& key, usize hashCode, Allocator* allocator);

			i8* hashMasks;
			JsonKeyValue* pairs;
			usize length;
			usize pairCapacity;
			usize maskCapacity;

		private:

			void defaultConstructRuntime(Allocator* allocator);

			constexpr Option<usize> findIndexOfKey(const String& key, JsonPairHashBits hashCode) const;

			Option<usize> findIndexOfKeyRuntime(const String& key, JsonPairHashBits hashCode) const;

			constexpr void reallocate(usize minCapacity, Allocator* allocator);

			void reallocateMasksAndPairs(usize minCapacity, Allocator* allocator);

			Option<usize> firstAvailableSlot() const;

			void freeRuntime(Allocator* allocator);
			
		}; // struct JsonObjectBucket


		constexpr bool isWhitespaceChar(char c);

		constexpr Result<void> parseNullValue(usize* valueEnd, usize valueStart, const Str& jsonString);

		constexpr Result<bool> parseBoolValue(usize* valueEnd, usize valueStart, const Str& jsonString);

		constexpr Result<double> parseNumberValue(usize* valueEnd, usize valueStart, const Str& jsonString);

		constexpr Result<String> parseStringValue(usize* valueEnd, usize valueStart, const Str& jsonString);

		constexpr Result<ArrayList<JsonValue>> parseArrayValue(usize* valueEnd, usize valueStart, const Str& jsonString);

		constexpr Result<JsonObject> parseObjectValue(usize* valueEnd, usize valueStart, const Str& jsonString);

	}
}

inline constexpr gk::JsonValue::JsonValue(const JsonValue& other)
	: _type(other._type)
{
	switch (_type) {
		case JsonValueType::Null:
			break;
		case JsonValueType::Bool:
			_value.boolean = other._value.boolean;
			break;
		case JsonValueType::Number:
			_value.number = other._value.number;
			break;
		case JsonValueType::String:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, String(other._value.string));
			}
			else {
				new (&_value.string) String(other._value.string);
			}
			break;
		case JsonValueType::Array:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, ArrayList<JsonValue>(other._value.arr));
			}
			else {
				new (&_value.arr) ArrayList<JsonValue>(other._value.arr);
			}
			break;
		case JsonValueType::Object:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, JsonObject(other._value.object));
			}
			else {
				new (&_value.object) JsonObject(other._value.object);
			}
			break;
	}
}

inline constexpr gk::JsonValue::JsonValue(JsonValue&& other) noexcept
	: _type(other._type)
{
	other._type = JsonValueType::Null;

	switch (_type) {
		case JsonValueType::Null:
			break;
		case JsonValueType::Bool:
			_value.boolean = other._value.boolean;
			break;
		case JsonValueType::Number:
			_value.number = other._value.number;
			break;
		case JsonValueType::String:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, std::move(other._value.string));
			}
			else {
				new (&_value.string) String(std::move(other._value.string));
			}
			break;
		case JsonValueType::Array:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, std::move(other._value.arr));
			}
			else {
				new (&_value.arr) ArrayList<JsonValue>(std::move(other._value.arr));
			}
			break;
		case JsonValueType::Object:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, std::move(other._value.object));
			}
			else {
				new (&_value.object) JsonObject(std::move(other._value.object));
			}
			break;
	}
}

inline constexpr gk::JsonValue& gk::JsonValue::operator=(const JsonValue& other)
{
	this->~JsonValue();
	_type = other._type;
	switch (_type) {
		case JsonValueType::Null:
			break;
		case JsonValueType::Bool:
			_value.boolean = other._value.boolean;
			break;
		case JsonValueType::Number:
			_value.number = other._value.number;
			break;
		case JsonValueType::String:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, String(other._value.string));
			}
			else {
				new (&_value.string) String(other._value.string);
			}
			break;
		case JsonValueType::Array:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, ArrayList<JsonValue>(other._value.arr));
			}
			else {
				new (&_value.arr) ArrayList<JsonValue>(other._value.arr);
			}
			break;
		case JsonValueType::Object:
			if (std::is_constant_evaluated()) {
				std::construct_at(&_value, JsonObject(other._value.object));
			}
			else {
				new (&_value.object) JsonObject(other._value.object);
			}
			break;
	}
	return *this;
}

inline constexpr gk::JsonValue& gk::JsonValue::operator=(JsonValue&& other) noexcept
{
	this->~JsonValue();
	_type = other._type;
	other._type = JsonValueType::Null;
	switch (_type) {
	case JsonValueType::Null:
		break;
	case JsonValueType::Bool:
		_value.boolean = other._value.boolean;
		break;
	case JsonValueType::Number:
		_value.number = other._value.number;
		break;
	case JsonValueType::String:
		if (std::is_constant_evaluated()) {
			std::construct_at(&_value, std::move(other._value.string));
			//_value.string = std::move(other._value.string);
		}
		else {
			new (&_value.string) String(std::move(other._value.string));
		}
		break;
	case JsonValueType::Array:
		if (std::is_constant_evaluated()) {
			std::construct_at(&_value, std::move(other._value.arr));
		}
		else {
			new (&_value.arr) ArrayList<JsonValue>(std::move(other._value.arr));
		}
		break;
	case JsonValueType::Object:
		if (std::is_constant_evaluated()) {
			std::construct_at(&_value, std::move(other._value.object));
		}
		else {
			new (&_value.object) JsonObject(std::move(other._value.object));
		}
		break;
	}
	return *this;
}

inline constexpr gk::JsonValue::~JsonValue()
{
	switch (_type) {
	case JsonValueType::String:
		_value.string.~String();
		break;
	case JsonValueType::Object:
		_value.object.~JsonObject();
		break;
	case JsonValueType::Array:
		_value.arr.~ArrayList();
		break;
	default:
		break;
	}
}

inline constexpr gk::JsonValue gk::JsonValue::makeBool(bool b)
{
	JsonValue value;
	value._type = JsonValueType::Bool;
	value._value.boolean = b;
	return value;
}

inline constexpr gk::JsonValue gk::JsonValue::makeNumber(double num)
{
	JsonValue value;
	value._type = JsonValueType::Number;
	value._value.number = num;
	return value;
}

inline constexpr gk::JsonValue gk::JsonValue::makeString(String&& string)
{
	JsonValue value = JsonValue(std::move(string));
	return value;
}

inline constexpr gk::JsonValue gk::JsonValue::makeArray(ArrayList<JsonValue>&& arr)
{
	JsonValue value = JsonValue(std::move(arr));
	return value;
}

inline constexpr gk::JsonValue gk::JsonValue::makeObject(JsonObject&& object)
{
	JsonValue value = JsonValue(std::move(object));
	return value;
}

inline constexpr gk::String gk::JsonValue::toString(usize objectNestCount) const
{
	constexpr i32 DECIMAL_PRECISION = 10;

	switch (_type) {
		case JsonValueType::Null:
			return "null"_str;
		case JsonValueType::Bool:
			return String::fromBool(boolValue());
		case JsonValueType::Number:
			return String::fromFloat(numberValue(), DECIMAL_PRECISION);
		case JsonValueType::String:
			return String('\"' + stringValue() + '\"');
		case JsonValueType::Array:
			{
				const ArrayList<JsonValue>& values = arrayValue();
				String outString = '[';
				for (usize i = 0; i < values.len(); i++) {
					outString.append(values[i].toString());
					if (i != (values.len() - 1)) {
						outString.append(", "_str);
					}
				}
				outString.append(']');
				return outString;
			}	
		case JsonValueType::Object:
			return objectValue().toString(objectNestCount + 1);
	}
}

inline constexpr gk::internal::JsonObjectBucket::JsonObjectBucket(Allocator* allocator)
	: hashMasks(nullptr), pairs(nullptr), length(0), maskCapacity(0), pairCapacity(0)
{
	if (std::is_constant_evaluated()) {
		pairs = new JsonKeyValue[4]; // 4 seems like a reasonable default capacity
		pairCapacity = 4;
	}
	else {
		defaultConstructRuntime(allocator);
	}
}

inline constexpr gk::internal::JsonObjectBucket::JsonObjectBucket(JsonObjectBucket&& other) noexcept
	: hashMasks(other.hashMasks), pairs(other.pairs), length(other.length), maskCapacity(other.maskCapacity), pairCapacity(other.pairCapacity)
{
	other.hashMasks = nullptr;
	other.pairs = nullptr;
	other.length = 0;
	other.maskCapacity = 0;
	other.pairCapacity = 0;
}

inline constexpr gk::Option<gk::usize> gk::internal::JsonObjectBucket::findIndexOfKey(const String& key, JsonPairHashBits hashCode) const
{
	if (std::is_constant_evaluated()) {
		for (usize i = 0; i < this->length; i++) {
			if (pairs[i].key == key) {
				return Option<usize>(i);
			}
		}
		return Option<usize>();
	}
	else {
		return findIndexOfKeyRuntime(key, hashCode);
	}
}

constexpr void gk::internal::JsonObjectBucket::reallocate(usize minCapacity, Allocator* allocator)
{
	if (std::is_constant_evaluated()) {
		JsonKeyValue* newPairs = new JsonKeyValue[minCapacity];
		for (usize i = 0; i < length; i++) {
			newPairs[i] = std::move(pairs[i]);
		}
		delete[] pairs;
		pairs = newPairs;
	}
	else {
		reallocateMasksAndPairs(minCapacity, allocator);
	}
}

inline constexpr void gk::internal::JsonObjectBucket::free(Allocator* allocator)
{
	if (std::is_constant_evaluated()) {
		delete[] pairs;
		pairs = nullptr;
	}
	else {
		freeRuntime(allocator);
	}
	maskCapacity = 0;
	pairCapacity = 0;
	length = 0;
}

constexpr gk::Option<gk::JsonValue*> gk::internal::JsonObjectBucket::find(const String& key, usize hashCode)
{
	Option<usize> index = findIndexOfKey(key, JsonPairHashBits(hashCode));
	if (index.none()) {
		return Option<JsonValue*>();
	}
	else {
		return Option<JsonValue*>(&pairs[index.someCopy()].value);
	}
}

inline constexpr gk::Option<const gk::JsonValue*> gk::internal::JsonObjectBucket::find(const String& key, usize hashCode) const
{
	Option<usize> index = findIndexOfKey(key, JsonPairHashBits(hashCode));
	if (index.none()) {
		return Option<const JsonValue*>();
	}
	else {
		return Option<const JsonValue*>(&pairs[index.someCopy()].value);
	}
}

inline constexpr void gk::internal::JsonObjectBucket::insert(String&& key, JsonValue&& value, usize hashCode, Allocator* allocator)
{
	JsonKeyValue newPair{ std::move(key), std::move(value), hashCode };
	if (length == pairCapacity) {
		if (pairCapacity == 0) {
			reallocate(4, allocator);
		}
		else {
			reallocate(pairCapacity * 2, allocator);
		}	
	}

	if (std::is_constant_evaluated()) {
		pairs[length] = std::move(newPair);
	}
	else {
		//Option<usize> availableSlot = firstAvailableSlot();
		//check(availableSlot.isSome());
		new (pairs + length) JsonKeyValue(std::move(newPair));
		hashMasks[length] = JsonPairHashBits(hashCode).value;
	}
	length++;
}

inline constexpr bool gk::internal::JsonObjectBucket::erase(const String& key, usize hashCode, Allocator* allocator)
{
	Option<usize> optIndex = findIndexOfKey(key, JsonPairHashBits(hashCode));
	if (optIndex.none()) {
		return false;
	}

	const usize index = optIndex.someCopy();
	pairs[index].~JsonKeyValue();
	
	if (std::is_constant_evaluated()) {
		for (usize i = index + 1; i < length; i++) {
			pairs[i - 1] = std::move(pairs[i]); // shift all down
		}
	}
	else {
		for (usize i = index; i < (length - 1); i++) {
			new (pairs + i) JsonKeyValue(std::move(pairs[i + 1]));
			hashMasks[i] = hashMasks[i + 1];
		}
	}
	length--;
	return true;
}

inline constexpr gk::JsonObject::JsonObject(const JsonObject& other)
	: buckets(nullptr), bucketCount(0), elementCount(0)
{
	if (other.elementCount == 0) {
		return;
	}

	reallocate(other.elementCount);
	for (ConstIterator::Pair pair : other) {
		addField(pair.name, pair.value);
	}
}

inline constexpr gk::JsonObject& gk::JsonObject::operator=(JsonObject&& other) noexcept
{
	if (buckets != nullptr) {
		if (std::is_constant_evaluated()) {
			for (usize i = 0; i < bucketCount; i++) {
				buckets[i].free(nullptr);
			}
			delete[] buckets;
			buckets = nullptr;
		}
		else {
			Allocator* globalAllocator = globalHeapAllocator();
			for (usize i = 0; i < bucketCount; i++) {
				buckets[i].free(globalAllocator);
			}
			globalAllocator->freeBuffer(buckets, bucketCount);
		}
	}

	buckets = other.buckets;
	bucketCount = other.bucketCount;
	elementCount = other.elementCount;

	other.buckets = nullptr;
	other.bucketCount = 0;
	other.elementCount = 0;

	return *this;
}

inline constexpr gk::JsonObject& gk::JsonObject::operator=(const JsonObject& other)
{
	if (other.elementCount == 0) { // just free all memory of this one.
		if (buckets == nullptr) return *this;

		if (std::is_constant_evaluated()) {
			check_eq(bucketCount, 1);
			buckets->free(nullptr);
			delete buckets;
			buckets = nullptr;
		}
		else {
			Allocator* globalAllocator = globalHeapAllocator();
			for (usize i = 0; i < bucketCount; i++) {
				buckets[i].free(globalAllocator);
			}
			globalAllocator->freeBuffer(buckets, bucketCount);
		}
		buckets = nullptr;
		bucketCount = 0;
		elementCount = 0;
		return *this;
	}

	if (std::is_constant_evaluated()) {
		if (buckets != nullptr) {
			buckets->free(nullptr);
			elementCount = 0;
		}
		reallocate(other.elementCount);
		for (ConstIterator::Pair pair : other) {
			addField(pair.name, pair.value);
		}
		return *this;
	}

	for (usize i = 0; i < bucketCount; i++) {
		buckets[i].free(globalHeapAllocator());
	}
	if (shouldReallocate(other.elementCount)) {
		reallocate(other.elementCount);
	}
	for (ConstIterator::Pair pair : other) {
		addField(pair.name, pair.value);
	}
	return *this;
}

constexpr gk::JsonObject::~JsonObject()
{
	if (buckets == nullptr) return;

	if (std::is_constant_evaluated()) {
		buckets->free(nullptr);
		delete buckets;
		buckets = nullptr;
	}
	else {
		Allocator* globalAllocator = globalHeapAllocator();
		for (usize i = 0; i < bucketCount; i++) {
			buckets[i].free(globalAllocator);
		}
		globalAllocator->freeBuffer(buckets, bucketCount);
	}
}

inline constexpr gk::Result<gk::JsonObject> gk::JsonObject::parse(Str jsonString)
{
	if (jsonString.len < 2) {
		return ResultErr(); // must be at least "{}"
	}

	// find start of object '{'
	usize begin = 0;
	{
		if (internal::isWhitespaceChar(jsonString.buffer[0])) {
			for (usize i = 1; i < jsonString.len; i++) {
				if (internal::isWhitespaceChar(jsonString.buffer[i])) {
					continue;
				}
				if (jsonString.buffer[i] == '{') {
					begin = i;
					break;
				}
				else {
					return ResultErr(); // improperly formatted
				}
			}
		}
	}

	// find end of object '}'
	usize end = jsonString.len;
	{
		if (internal::isWhitespaceChar(jsonString.buffer[jsonString.len - 1])) {
			for (usize i = jsonString.len; i > 0; i--) {
				if (i == begin) {
					return ResultErr(); // could not find '}' until it got back to the beginning
				}

				if (internal::isWhitespaceChar(jsonString.buffer[i - 1])) {
					continue;
				}
				if (jsonString.buffer[i - 1] == '}') {
					end = i;
					break;
				}
				else {
					return ResultErr(); // improperly formatted
				}
			}
		}
	}

	if (begin != 0 || end != jsonString.len) { // if '{' and/or '}' are offset
		jsonString = jsonString.substring(begin, end);
	}

	// at this point, jsonString has been isolated to just start with '{' and end with '}'
	usize _ = ~0;
	return internal::parseObjectValue(&_, 0, jsonString);
}

inline constexpr gk::String gk::JsonObject::toString(usize nestCount) const
{
	if (elementCount == 0) {
		return "{}"_str;
	}

	String outString = '{';

	usize i = 0;
	for (ConstIterator::Pair pair : *this) {
		const bool isLastIteration = i == (elementCount - 1);

		outString.append('\n');
		for (usize n = 0; n < (nestCount + 1); n++) {
			outString.append("   ");
		}
		outString.append('\"' + pair.name + "\": "_str + pair.value.toString(nestCount));
		if (!isLastIteration) {
			outString.append(',');
		}
		i++;
	}

	outString.append('\n');
	for (usize n = 0; n < nestCount; n++) {
		outString.append("   ");
	}
	outString.append('}');
	return outString;
}

inline constexpr gk::Option<gk::JsonValue*> gk::JsonObject::findField(const String& name)
{
	if (elementCount == 0) {
		return Option<JsonValue*>();
	}

	check_message(buckets != nullptr, "Buckets of groups array should be valid after the above check");
	check_message(bucketCount > 0, "After this check, bucket count should be greater than 0");

	if (std::is_constant_evaluated()) {
		check_eq(bucketCount, 1);
		return buckets->find(name, 0);
	}

	const usize hashCode = name.hash();
	const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
	//const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

	const usize bucketIndex = bucketBits.value % bucketCount;
	return buckets[bucketIndex].find(name, hashCode);
}

inline constexpr gk::Option<const gk::JsonValue*> gk::JsonObject::findField(const String& name) const
{
	if (elementCount == 0) {
		return Option<const JsonValue*>();
	}

	check_message(buckets != nullptr, "Buckets of groups array should be valid after the above check");
	check_message(bucketCount > 0, "After this check, bucket count should be greater than 0");

	if (std::is_constant_evaluated()) {
		check_eq(bucketCount, 1);
		return static_cast<const internal::JsonObjectBucket*>(buckets)->find(name, 0); // i dont know why static cast
	}

	const usize hashCode = name.hash();
	const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
	//const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

	const usize bucketIndex = bucketBits.value % bucketCount;
	return static_cast<const internal::JsonObjectBucket*>(buckets)[bucketIndex].find(name, hashCode);
}

inline constexpr gk::Option<gk::JsonValue*> gk::JsonObject::addField(String&& name, JsonValue&& value)
{
	if (std::is_constant_evaluated()) {
		Option<JsonValue*> found = findField(name);
		if (found.isSome()) {
			return found;
		}
		if (shouldReallocate(elementCount + 1)) {
			reallocate(elementCount + 1);
		}
		buckets->insert(std::move(name), std::move(value), 0, nullptr);
		elementCount++;
		return Option<JsonValue*>();
	}
	else {
		return addFieldRuntime(std::move(name), std::move(value));
	}
}

inline constexpr gk::Option<gk::JsonValue*> gk::JsonObject::addField(const String& name, JsonValue&& value)
{
	return addField(String(name), std::move(value));
}

inline constexpr gk::Option<gk::JsonValue*> gk::JsonObject::addField(String&& name, const JsonValue& value)
{
	return addField(std::move(name), JsonValue(value));
}

inline constexpr gk::Option<gk::JsonValue*> gk::JsonObject::addField(const String& name, const JsonValue& value)
{
	return addField(String(name), JsonValue(value));
}

inline constexpr bool gk::JsonObject::eraseField(const String& name)
{
	if (std::is_constant_evaluated()) {
		if (elementCount == 0) {
			return false;
		}

		return buckets->erase(name, 0, nullptr);
	}
	else {
		return eraseFieldRuntime(name);
	}
}

inline constexpr bool gk::JsonObject::shouldReallocate(gk::usize requiredCapacity)
{
	if (bucketCount == 0) {
		return true;
	}
	const usize loadFactorScaledPairCount = (elementCount >> 2) * 3; // multiply by 0.75
	return requiredCapacity > loadFactorScaledPairCount;
}

inline constexpr void gk::JsonObject::reallocate(usize requiredCapacity)
{
	if (std::is_constant_evaluated()) {
		if (bucketCount == 0) {
			buckets = new internal::JsonObjectBucket(nullptr);
			bucketCount = 1;
		}
		return;
	}
	else {
		reallocateRuntime(requiredCapacity);
	}
}

inline constexpr gk::JsonObject::Iterator gk::JsonObject::Iterator::iterBegin(JsonObject* object)
{
	Iterator iter;
	iter._object = object;
	iter._currentBucket = object->buckets;
	if (object->buckets != nullptr) {
		while (iter._currentBucket->length == 0) {
			iter._currentBucket++;
			if (iter._currentBucket == object->buckets + object->bucketCount) {
				break;
			}
		}
	}
	return iter;
}

inline constexpr gk::JsonObject::Iterator gk::JsonObject::Iterator::iterEnd(JsonObject* object)
{
	Iterator iter;
	iter._object = object;
	iter._currentBucket = object->buckets + object->bucketCount;
	return iter;
}

inline constexpr gk::JsonObject::Iterator& gk::JsonObject::Iterator::operator++() 
{
	step();
	return *this;
}

inline constexpr bool gk::JsonObject::Iterator::operator==(const Iterator& other) const
{
	return _currentBucket == other._currentBucket;
}

inline constexpr gk::JsonObject::Iterator::Pair gk::JsonObject::Iterator::operator*() const
{
	return Pair{ _currentBucket->pairs[_currentElementIndex].key, _currentBucket->pairs[_currentElementIndex].value };
}

inline constexpr void gk::JsonObject::Iterator::step() {
	_currentElementIndex++;
	if (_currentElementIndex >= _currentBucket->length) {
		while (true) {
			_currentElementIndex = 0;
			_currentBucket++;
			if (_currentBucket == _object->buckets + _object->bucketCount) {
				break;
			}
			if (_currentBucket->length > 0) {
				break;
			}
		}
	}
}

inline constexpr gk::JsonObject::ConstIterator gk::JsonObject::ConstIterator::iterBegin(const JsonObject* object)
{
	ConstIterator iter;
	iter._object = object;
	iter._currentBucket = object->buckets;
	if (object->buckets != nullptr) {
		while (iter._currentBucket->length == 0) {
			iter._currentBucket++;
			if (iter._currentBucket == object->buckets + object->bucketCount) {
				break;
			}
		}
	}
	return iter;
}

inline constexpr gk::JsonObject::ConstIterator gk::JsonObject::ConstIterator::iterEnd(const JsonObject* object)
{
	ConstIterator iter;
	iter._object = object;
	iter._currentBucket = object->buckets + object->bucketCount;
	return iter;
}

inline constexpr gk::JsonObject::ConstIterator& gk::JsonObject::ConstIterator::operator++()
{
	step();
	return *this;
}

inline constexpr bool gk::JsonObject::ConstIterator::operator==(const ConstIterator& other) const
{
	return _currentBucket == other._currentBucket;
}

inline constexpr gk::JsonObject::ConstIterator::Pair gk::JsonObject::ConstIterator::operator*() const
{
	return Pair{ _currentBucket->pairs[_currentElementIndex].key, _currentBucket->pairs[_currentElementIndex].value };
}

inline constexpr void gk::JsonObject::ConstIterator::step() {
	_currentElementIndex++;
	if (_currentElementIndex >= _currentBucket->length) {
		while (true) {
			_currentElementIndex = 0;
			_currentBucket++;
			if (_currentBucket == _object->buckets + _object->bucketCount) {
				break;
			}
			if (_currentBucket->length > 0) {
				break;
			}
		}
	}
}

constexpr bool gk::internal::isWhitespaceChar(char c)
{
	constexpr char NEWLINE = '\n';
	constexpr char CARRIAGE_RETURN = '\r';
	constexpr char TAB = '\t';
	constexpr char SPACE = ' ';

	return c == NEWLINE || c == CARRIAGE_RETURN || c == TAB || c == SPACE;
}

constexpr gk::Result<void> gk::internal::parseNullValue(usize* valueEnd, usize valueStart, const Str& jsonString)
{
	for (usize valueIter = valueStart + 1; valueIter < jsonString.len; valueIter++) {
		char c = jsonString.buffer[valueIter];

		if (isWhitespaceChar(c) || c == ',' || c == '}') {
			*valueEnd = valueIter;
			const Str value = jsonString.substring(valueStart, *valueEnd);
			if (value == "null"_str) {
				return ResultOk<void>();
			}
			else {
				return ResultErr();
			}
		}
	}
	return ResultErr();
}

constexpr gk::Result<bool> gk::internal::parseBoolValue(usize* valueEnd, usize valueStart, const Str& jsonString)
{
	for (usize valueIter = valueStart + 1; valueIter < jsonString.len; valueIter++) {
		char c = jsonString.buffer[valueIter];
		if (isWhitespaceChar(c) || c == ',' || c == ']' || c == '}') {
			*valueEnd = valueIter;
			const Str value = jsonString.substring(valueStart, *valueEnd);
			return value.parseBool();
		}
		if (c >= 'a' || c <= 'z') {
			continue;
		}
	}
	return ResultErr();
}

constexpr gk::Result<double> gk::internal::parseNumberValue(usize* valueEnd, usize valueStart, const Str& jsonString)
{
	for (usize valueIter = valueStart + 1; valueIter < jsonString.len; valueIter++) {
		char c = jsonString.buffer[valueIter];
		if (c == '-') {
			continue;
		}
		else if (c >= '0' && c <= '9') {
			continue;
		}
		else if (c == '.') {
			continue;
		}
		else if (isWhitespaceChar(c) || c == ',' || c == ']' || c == '}') {
			*valueEnd = valueIter;
			const Str value = jsonString.substring(valueStart, *valueEnd);
			return value.parseFloat();
		}
	}
	return ResultErr();
}

constexpr gk::Result<gk::String> gk::internal::parseStringValue(usize* valueEnd, usize valueStart, const Str& jsonString)
{
	for (usize valueIter = valueStart + 1; valueIter < jsonString.len; valueIter++) {
		char c = jsonString.buffer[valueIter];
		
		if (c == '\"' && jsonString.buffer[valueIter - 1] != '\\') {
			*valueEnd = valueIter + 1;
			const Str value = jsonString.substring(valueStart + 1, *valueEnd - 1);
			return ResultOk<String>(String(value));
		}
	}
	return ResultErr();
}

constexpr gk::Result<gk::ArrayList<gk::JsonValue>> gk::internal::parseArrayValue(usize* valueEnd, usize valueStart, const Str& jsonString)
{
	ArrayList<JsonValue> accumulate;

	usize valueIter = valueStart + 1;
	while (valueIter < jsonString.len) {
		char c = jsonString.buffer[valueIter];

		if (isWhitespaceChar(c)) {
			valueIter++;
			continue;
		}


		usize currentValueStart = valueIter;
		usize currentValueEnd = ~0;
		bool didParseCurrentValue = false;
		JsonValueType hint = JsonValueType::Null;

		if (c == 'n') {
			hint = JsonValueType::Null;
			Result<void> value = parseNullValue(&currentValueEnd, currentValueStart, jsonString);
			if (!value.isError()) {
				accumulate.push(JsonValue::makeNull());
				didParseCurrentValue = true;
			}
		}
		else if (c == 't' || c == 'f') {
			hint = JsonValueType::Bool;
			Result<bool> value = parseBoolValue(&currentValueEnd, currentValueStart, jsonString);
			if (!value.isError()) {
				accumulate.push(JsonValue::makeBool(value.ok()));
				didParseCurrentValue = true;
			}
		}
		else if (c >= '0' && c <= '9') {
			hint = JsonValueType::Number;
			Result<double> value = parseNumberValue(&currentValueEnd, currentValueStart, jsonString);
			if (!value.isError()) {
				accumulate.push(JsonValue::makeNumber(value.ok()));
				didParseCurrentValue = true;
			}
		}
		else if (c == '-') {
			hint = JsonValueType::Number;
			Result<double> value = parseNumberValue(&currentValueEnd, currentValueStart, jsonString);
			if (!value.isError()) {
				accumulate.push(JsonValue::makeNumber(value.ok()));
				didParseCurrentValue = true;
			}
		}
		else if (c == '\"') {
			hint = JsonValueType::String;
			Result<String> value = parseStringValue(&currentValueEnd, currentValueStart, jsonString);
			if (!value.isError()) {
				accumulate.push(JsonValue::makeString(value.ok()));
				didParseCurrentValue = true;
			}
		}
		else if (c == '[') {
			hint = JsonValueType::Array;
			Result<ArrayList<JsonValue>> value = parseArrayValue(&currentValueEnd, currentValueStart, jsonString);
			if (!value.isError()) {
				accumulate.push(JsonValue::makeArray(value.ok()));
				didParseCurrentValue = true;
			}
		}
		else if (c == '{') {
			hint = JsonValueType::Object;
			check(false);
			break;
		}


		if (!didParseCurrentValue) {
			return ResultErr();
		}

		for (usize nextValueIter = currentValueEnd; nextValueIter < jsonString.len; nextValueIter++) {
			if (isWhitespaceChar(jsonString.buffer[nextValueIter])) {
				continue;
			}
			if (jsonString.buffer[nextValueIter] == ',') {
				valueIter += (nextValueIter - valueIter) + 1;			
				break;
			}
			if (jsonString.buffer[nextValueIter] == ']') {
				*valueEnd = nextValueIter + 1;
				return ResultOk<ArrayList<JsonValue>>(std::move(accumulate));
			}

			return ResultErr();		
		}
	}
	return ResultErr();
}

constexpr gk::Result<gk::JsonObject> gk::internal::parseObjectValue(usize* valueEnd, usize valueStart, const Str& jsonString)
{
	JsonObject accumulate;

	usize valueIter = valueStart + 1;
	while (valueIter < jsonString.len) {
		char c = jsonString.buffer[valueIter];

		if (isWhitespaceChar(c)) {
			valueIter++;
			continue;
		}
		else if (c == '}') { // end of object
			return ResultOk<JsonObject>(accumulate);
		}
		else if (c != '\"') {
			return ResultErr();
		}

		const usize nameStart = valueIter + 1;
		usize nameEnd = ~0;
		for (usize nameIter = nameStart; nameIter < jsonString.len; nameIter++) {
			if (jsonString.buffer[nameIter] == '\"' && jsonString.buffer[nameIter - 1] != '\\') {
				nameEnd = nameIter;
				break;
			}
		}
		if (nameEnd == ~0) { // could not get to end of name
			return ResultErr();
		}

		const Str name = jsonString.substring(nameStart, nameEnd);

		usize delimiterPosition = ~0;
		for (usize delimiterIter = nameEnd + 1; delimiterIter < jsonString.len; delimiterIter++) {
			if (internal::isWhitespaceChar(jsonString.buffer[delimiterIter])) {
				continue;
			}
			if (jsonString.buffer[delimiterIter] == ':') {
				delimiterPosition = delimiterIter;
				break;
			}
			else {
				return ResultErr();
			}
		}
		if (delimiterPosition == ~0) { // could not find delimiter
			return ResultErr();
		}
		
		usize currentValueStart = ~0;
		JsonValueType hint = JsonValueType::Null;
		for (usize valueStartIter = delimiterPosition + 1; valueStartIter < jsonString.len; valueStartIter++) {
			char c = jsonString.buffer[valueStartIter];
			if (internal::isWhitespaceChar(c)) {
				continue;
			}
			if (c == 'n') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::Null;
				break;
			}
			if (c == 't' || c == 'f') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::Bool;
				break;
			}
			if (c >= '0' && c <= '9') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::Number;
				break;
			}
			if (c == '-') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::Number;
				break;
			}
			if (c == '\"') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::String;
				break;
			}
			if (c == '[') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::Array;
				break;
			}
			if (c == '{') {
				currentValueStart = valueStartIter;
				hint = JsonValueType::Object;
				break;
			}
			else {
				return ResultErr();
			}
		}
		if (currentValueStart == ~0) { // could not find start of value
			return ResultErr();
		}

		usize currentValueEnd = ~0;

		switch (hint) {

		case JsonValueType::Null:
		{
			Result<void> value = internal::parseNullValue(&currentValueEnd, currentValueStart, jsonString);
			if (value.isError()) {
				return ResultErr();
			}
			else {
				accumulate.addField(String(name), JsonValue::makeNull());
			}
		}
		break;

		case JsonValueType::Bool:
		{
			Result<bool> value = internal::parseBoolValue(&currentValueEnd, currentValueStart, jsonString);
			if (value.isError()) {
				return ResultErr();
			}
			else {
				accumulate.addField(String(name), JsonValue::makeBool(value.ok()));
			}
		}
		break;

		case JsonValueType::Number:
		{
			Result<double> value = internal::parseNumberValue(&currentValueEnd, currentValueStart, jsonString);
			if (value.isError()) {
				return ResultErr();
			}
			else {
				accumulate.addField(String(name), JsonValue::makeNumber(value.ok()));
			}
		}
		break;


		case JsonValueType::String:
		{
			Result<String> value = internal::parseStringValue(&currentValueEnd, currentValueStart, jsonString);
			if (value.isError()) {
				return ResultErr();
			}
			else {
				accumulate.addField(String(name), JsonValue::makeString(value.ok()));
			}
		}
		break;

		case JsonValueType::Array:
		{
			Result<ArrayList<JsonValue>> value = internal::parseArrayValue(&currentValueEnd, currentValueStart, jsonString);
			if (value.isError()) {
				return ResultErr();
			}
			else {
				accumulate.addField(String(name), JsonValue::makeArray(value.ok()));
			}
		}
		break;

		case JsonValueType::Object:
		{
			Result<JsonObject> value = internal::parseObjectValue(&currentValueEnd, currentValueStart, jsonString);
			if (value.isError()) {
				return ResultErr();
			}
			else {
				accumulate.addField(String(name), JsonValue::makeObject(value.ok()));
			}
		}
		break;
		}
		if (currentValueEnd == ~0) {
			return ResultErr();
		}

		for (usize nextStart = currentValueEnd; nextStart < jsonString.len; nextStart++) {
			char c = jsonString.buffer[nextStart];

			if (internal::isWhitespaceChar(c)) {
				continue;
			}
			else if (c == ',') {
				valueIter += (nextStart - valueIter) + 1;
				break;
			}
			else if (c == '}') {
				*valueEnd = nextStart + 1;
				return ResultOk<JsonObject>(accumulate);
			}
			else {
				return ResultErr();
			}
		}
	}
	return ResultErr();
}
