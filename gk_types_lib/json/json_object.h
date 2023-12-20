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

		constexpr Option<JsonValue*> findField(const String& name);

		constexpr Option<const JsonValue*> findField(const String& name) const;

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

		constexpr bool eraseField(const String& name);

		constexpr usize fieldCount() const { return elementCount; }

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

		private:
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
		Iterator begin() { return Iterator::iterBegin(this); }

		/**
		* End of an Iterator with immutable keys, and mutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of mutable iterator
		*/
		Iterator end() { return Iterator::iterEnd(this); }

		/**
		* Begin of an Iterator with immutable keys, and immutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return Begin of immutable iterator
		*/
		ConstIterator begin() const { return ConstIterator::iterBegin(this); }

		/**
		* End of an Iterator with immutable keys, and immutable values, over
		* each entry in the JsonObject.
		*
		* `addField()`, `eraseField()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of immutable iterator
		*/
		ConstIterator end() const { return ConstIterator::iterEnd(this); }

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
		constexpr bool boolValue() {
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
		constexpr double numberValue() {
			check_eq(_type, JsonValueType::Number);
			return _value.number;
		}

		/**
		* Will assert that the stored type is string.
		* Moves the string out of this json value, so accessing it again on this object useless.
		* 
		* @return The string json value.
		*/
		constexpr gk::String stringValue() {
			check_eq(_type, JsonValueType::String);
			return std::move(_value.string);
		}

		/**
		* Will assert that the stored type is an array of json values.
		* Moves the array out of this json value, so accessing it again on this object is useless.
		* 
		* @return The array of json values held by this value.
		*/
		constexpr ArrayList<JsonValue> arrayValue() {
			check_eq(_type, JsonValueType::Array);
			return std::move(_value.arr);
		}

		/**
		* Will assert that the stored type is an object.
		* Moves the json string object out of this json value, so accessing it again on this object is useless.
		* 
		* @return The json string containing a sub-object.
		*/
		constexpr JsonObject objectValue() {
			check_eq(_type, JsonValueType::Object);
			return std::move(_value.object);
		}

	private:

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

			constexpr JsonKeyValue() = default;
			constexpr ~JsonKeyValue() = default;

			constexpr JsonKeyValue(String&& key, JsonValue&& value)
				: key(std::move(key)), value(std::move(value))
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
			
			constexpr Option<JsonValue*> find(const String& key, JsonPairHashBits hashCode);

			constexpr Option<const JsonValue*> find(const String& key, JsonPairHashBits hashCode) const;

			constexpr void insert(String&& key, JsonValue&& value, JsonPairHashBits hashCode, Allocator* allocator);

			constexpr bool erase(const String& key, JsonPairHashBits hashCode, Allocator* allocator);

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
			
		};


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
			_value.string = other._value.string;
			break;
		case JsonValueType::Array:
			_value.arr = other._value.arr;
			break;
		case JsonValueType::Object:
			_value.object = other._value.object;
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
			_value.string = std::move(other._value.string);
			break;
		case JsonValueType::Array:
			_value.arr = std::move(other._value.arr);
			break;
		case JsonValueType::Object:
			_value.object = std::move(other._value.object);
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
			_value.string = other._value.string;
			break;
		case JsonValueType::Array:
			_value.arr = other._value.arr;
			break;
		case JsonValueType::Object:
			_value.object = other._value.object;
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
		_value.string = std::move(other._value.string);
		break;
	case JsonValueType::Array:
		_value.arr = std::move(other._value.arr);
		break;
	case JsonValueType::Object:
		_value.object = std::move(other._value.object);
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
	JsonValue value;
	value._type = JsonValueType::String;
	value._value.string = std::move(string);
	return value;
}

inline constexpr gk::JsonValue gk::JsonValue::makeArray(ArrayList<JsonValue>&& arr)
{
	JsonValue value;
	value._type = JsonValueType::Array;
	value._value.arr = std::move(arr);
	return value;
}

inline constexpr gk::JsonValue gk::JsonValue::makeObject(JsonObject&& object)
{
	JsonValue value;
	value._type = JsonValueType::Object;
	value._value.object = std::move(object);
	return value;
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

constexpr gk::Option<gk::JsonValue*> gk::internal::JsonObjectBucket::find(const String& key, JsonPairHashBits hashCode)
{
	Option<usize> index = findIndexOfKey(key, hashCode);
	if (index.none()) {
		return Option<JsonValue*>();
	}
	else {
		return Option<JsonValue*>(&pairs[index.someCopy()].value);
	}
}

inline constexpr gk::Option<const gk::JsonValue*> gk::internal::JsonObjectBucket::find(const String& key, JsonPairHashBits hashCode) const
{
	Option<usize> index = findIndexOfKey(key, hashCode);
	if (index.none()) {
		return Option<const JsonValue*>();
	}
	else {
		return Option<const JsonValue*>(&pairs[index.someCopy()].value);
	}
}

inline constexpr void gk::internal::JsonObjectBucket::insert(String&& key, JsonValue&& value, JsonPairHashBits hashCode, Allocator* allocator)
{
	JsonKeyValue newPair{ std::move(key), std::move(value) };
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
		hashMasks[length] = hashCode.value;
	}
	length++;
}

inline constexpr bool gk::internal::JsonObjectBucket::erase(const String& key, JsonPairHashBits hashCode, Allocator* allocator)
{
	Option<usize> optIndex = findIndexOfKey(key, hashCode);
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
	const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

	const usize bucketIndex = bucketBits.value % bucketCount;
	return buckets[bucketIndex].find(name, pairBits);
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
	const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

	const usize bucketIndex = bucketBits.value % bucketCount;
	return static_cast<const internal::JsonObjectBucket*>(buckets)[bucketIndex].find(name, pairBits);
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
		iter.step();
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
		iter.step();
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
			if (_currentBucket->length > 0) {
				break;
			}
		}
	}
}