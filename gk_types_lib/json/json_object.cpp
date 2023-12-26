#include "json_object.h"
#include <intrin.h>

using gk::usize;

namespace gk {
#if GK_TYPES_LIB_DEBUG
	constexpr bool SHOULD_LOG_JSON_FUNCTION_LOADING = true;
#else
	constexpr bool SHOULD_LOG_JSON_FUNCTION_LOADING = false;
#endif
}


/**
* Allocates the hash masks, and pairs in a single allocation.
* Naturally this allows for less overhead by doing less dynamic memory
* tomfoolery.
* 
* @param hashMaskCount: Number of hash masks. Must be a multiple of 64.
* @param pairCount: Number of pairs. Must be less than or equal to `hashMaskCount`
* @return Number of i8 to allocate.
*/
static constexpr usize calculateAllocationSize(usize hashMaskCount, usize pairCount) {
	check(hashMaskCount % 64 == 0);
	check_le(pairCount, hashMaskCount);

	return hashMaskCount + (sizeof(gk::internal::JsonKeyValue) * pairCount);
}

void gk::internal::JsonObjectBucket::defaultConstructRuntime(Allocator* allocator)
{
	constexpr usize alignment = 64;
	constexpr usize allocationSize = calculateAllocationSize(64, 4);

	i8* memory = allocator->mallocAlignedBuffer<i8>(allocationSize, alignment).ok();
	memset(memory, 0, allocationSize);

	this->maskCapacity = 64;
	this->pairCapacity = 4;
	this->hashMasks = memory;
	this->pairs = reinterpret_cast<JsonKeyValue*>(memory + maskCapacity); // offset by the masks
}

inline void gk::internal::JsonObjectBucket::reallocateMasksAndPairs(usize minCapacity, Allocator* allocator)
{
	constexpr usize alignment = 64;

	const usize hashMaskMallocCapacity = minCapacity + (64 - (minCapacity % 64));
	const usize allocationSize = calculateAllocationSize(hashMaskMallocCapacity, minCapacity);

	i8* memory = allocator->mallocAlignedBuffer<i8>(allocationSize, alignment).ok();
	memset(memory, 0, allocationSize);

	i8* newMasks = memory;
	JsonKeyValue* newPairs = reinterpret_cast<JsonKeyValue*>(memory + hashMaskMallocCapacity);

	memcpy(newMasks, hashMasks, maskCapacity);
	for (usize i = 0; i < length; i++) {
		new (newPairs + i) JsonKeyValue(std::move(pairs[i]));
	}

	allocator->freeAlignedBuffer<i8>(hashMasks, calculateAllocationSize(maskCapacity, pairCapacity), alignment);

	this->maskCapacity = hashMaskMallocCapacity;
	this->pairCapacity = minCapacity;
	this->hashMasks = newMasks;
	this->pairs = newPairs;
}

namespace gk {
	namespace internal {

		typedef Option<usize>(*FindHashBitsInJsonMaskFunc)(const String&, const i8*, const JsonKeyValue*, JsonPairHashBits, usize);
		typedef Option<usize>(*FindFirstAvailableSlotJsonFunc)(const i8*, usize);

		static Option<usize> avx512FindHashBitsInJsonMask(const String& key, const i8* hashMasks, const JsonKeyValue* pairs, JsonPairHashBits hashCode, usize len) {
			const __m512i hashCodeVec = _mm512_set1_epi8(hashCode.value);
			const __m512i* masksVec = reinterpret_cast<const __m512i*>(hashMasks);

			const usize iterationsToDo = ((len) % 64 == 0 ?
				len :
				len + (64 - (len % 64)))
				/ 64;

			for (usize i = 0; i < iterationsToDo; i++) {
				u64 bitmask = _mm512_cmpeq_epi8_mask(hashCodeVec, *masksVec);
				while (true) {
					Option<usize> optIndex = bitscanForwardNext(&bitmask);
					if (optIndex.none()) {
						break;
					}
					const usize index = optIndex.someCopy() + (i * 64);
					const String& name = pairs[index].key;
					if (name == key) {
						return Option<usize>(index);
					}
				}
			}

			return Option<usize>();
		}

		static Option<usize> avx512FindFirstAvailableSlot(const i8* hashMasks, usize pairCapacity) {
			const __m512i zeroVec = _mm512_set1_epi8(0);
			//const __m512i indices = _mm512_set_epi8(
			//	63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33,
			//	32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);

			const __m512i* hashMasksVec = reinterpret_cast<const __m512i*>(hashMasks);
			const usize iterationsToDo = ((pairCapacity) % 64 == 0 ?
				pairCapacity :
				pairCapacity + (64 - (pairCapacity % 64)))
				/ 64;

			for (usize i = 0; i < iterationsToDo; i++) {
				//const i8 num = i != (iterationsToDo - 1) ? static_cast<i8>(64) : static_cast<i8>((iterationsToDo * i) - pairCapacity);
				//check_le(num, 64);

				const u64 bitmask = _mm512_cmpeq_epi8_mask(zeroVec, *hashMasksVec);
				unsigned long bcfIndex;
				if (_BitScanForward64(&bcfIndex, bitmask) == 0) {
					hashMasksVec++;
					continue;
				}

				const usize index = static_cast<usize>(bcfIndex) + (i * 64);
				if (index >= pairCapacity) {
					return Option<usize>();
				}
				else {
					return Option<usize>(index);
				}
			}
			return Option<usize>();
		}
	}
}

gk::Option<usize> gk::internal::JsonObjectBucket::findIndexOfKeyRuntime(const String& key, JsonPairHashBits hashCode) const
{
	check_ne(hashMasks, nullptr);

	static FindHashBitsInJsonMaskFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_JSON_FUNCTION_LOADING) {
				std::cout << "[Json function loader]: Using AVX-512 key find\n";
			}

			return avx512FindHashBitsInJsonMask;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_JSON_FUNCTION_LOADING) {
				std::cout << "[Json function loader]: Using AVX-2 key find\n";
			}
			abort();
			return avx512FindHashBitsInJsonMask;
		}
		else {
			std::cout << "[Json function loader]: ERROR\nCannot load json key find functions if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(key, this->hashMasks, this->pairs, hashCode, this->length);
}

gk::Option<usize> gk::internal::JsonObjectBucket::firstAvailableSlot() const
{
	check_ne(hashMasks, nullptr);

	static FindFirstAvailableSlotJsonFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (true) {
				std::cout << "[Json function loader]: Using AVX-512 key find empty slot\n";
			}

			return avx512FindFirstAvailableSlot;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (true) {
				std::cout << "[Json function loader]: Using AVX-2 key find empty slot\n";
			}
			abort();
			return avx512FindFirstAvailableSlot;
		}
		else {
			std::cout << "[Json function loader]: ERROR\nCannot load json key find empty slot functions if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(this->hashMasks, this->pairCapacity);
}

void gk::internal::JsonObjectBucket::freeRuntime(Allocator* allocator)
{
	if (hashMasks == nullptr) return;

	constexpr usize alignment = 64;
	const usize currentAllocationSize = calculateAllocationSize(maskCapacity, pairCapacity);
	allocator->freeAlignedBuffer<i8>(hashMasks, calculateAllocationSize(maskCapacity, pairCapacity), alignment);
	pairs = nullptr;
}

usize gk::JsonObject::calculateNewBucketCount(usize requiredCapacity)
{
	return requiredCapacity <= 64 ? 1 : static_cast<usize>(gk::upperPowerOfTwo(requiredCapacity / 32));
}

void gk::JsonObject::reallocateRuntime(usize requiredCapacity)
{
	using internal::JsonObjectBucket;
	Allocator* allocator = gk::globalHeapAllocator();

	const usize newBucketCount = calculateNewBucketCount(requiredCapacity);
	if (newBucketCount <= bucketCount) {
		return;
	}

	JsonObjectBucket* newBuckets = allocator->mallocBuffer<JsonObjectBucket>(newBucketCount).ok();
	for (usize i = 0; i < newBucketCount; i++) {
		new (newBuckets + i) JsonObjectBucket(allocator);
	}

	for (usize oldBucketIndex = 0; oldBucketIndex < bucketCount; oldBucketIndex++) {
		JsonObjectBucket& oldBucket = buckets[oldBucketIndex];
		for (usize i = 0; i < oldBucket.length; i++) {
			// Does not contain anything
			if (oldBucket.hashMasks[i] == 0) continue;

			internal::JsonKeyValue& pair = oldBucket.pairs[i];
			//const usize hashCode = pair.key.hash();
			const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(pair.hashCode);
			//const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

			const usize newBucketIndex = bucketBits.value % newBucketCount;
			newBuckets[newBucketIndex].insert(std::move(pair.key), std::move(pair.value), pair.hashCode, allocator);
		}
		oldBucket.length = 0;
		oldBucket.free(allocator);
	}
	if (buckets) {
		allocator->freeBuffer(buckets, bucketCount);
	}

	buckets = newBuckets;
	bucketCount = newBucketCount;
}

gk::Option<gk::JsonValue*> gk::JsonObject::addFieldRuntime(String&& name, JsonValue&& value)
{
	const usize hashCode = name.hash();
	const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
	//const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

	Option<JsonValue*> found = [&]() {
		if (elementCount == 0) {
			return Option<JsonValue*>();
		}
		const usize bucketIndex = bucketBits.value % bucketCount;
		return buckets[bucketIndex].find(name, hashCode);
	}();
	if (found.isSome()) {
		return found;
	}

	if (shouldReallocate(elementCount + 1)) {
		reallocateRuntime(elementCount + 1);
	}

	{
		const usize bucketIndex = bucketBits.value % bucketCount;
		buckets[bucketIndex].insert(std::move(name), std::move(value), hashCode, gk::globalHeapAllocator());
		elementCount++;
		return gk::Option<JsonValue*>();
	}
}

bool gk::JsonObject::eraseFieldRuntime(const String& name)
{
	const usize hashCode = name.hash();
	const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
	//const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);
	const usize bucketIndex = bucketBits.value % bucketCount;
	return buckets[bucketIndex].erase(name, hashCode, gk::globalHeapAllocator());
}

#if GK_TYPES_LIB_TEST

using gk::JsonObject;
using gk::JsonValue;
using gk::ArrayList;
using gk::String;

test_case("JsonObject cant find field when empty") {
	JsonObject obj;
	check(obj.findField(""_str).none());
}

comptime_test_case(JsonObject, CantFindFieldWhenEmpty, {
	JsonObject obj;
	check(obj.findField(""_str).none());
});

test_case("JsonObject add field null") {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue());
	check(obj.findField("some name"_str).isSome());
	check(obj.findField("some name"_str).some()->isNull());
}

comptime_test_case(JsonObject, AddFieldNull, {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue());
	check(obj.findField("some name"_str).isSome());
	check(obj.findField("some name"_str).some()->isNull());
});

test_case("JsonObject add field bool") {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue::makeBool(true));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->boolValue(), true);
}

comptime_test_case(JsonObject, AddFieldBool, {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue::makeBool(true));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->boolValue(), true);
});

test_case("JsonObject add field number") {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue::makeNumber(-1.5));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->numberValue(), -1.5);
}

comptime_test_case(JsonObject, AddFieldNumber, {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue::makeNumber(-1.5));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->numberValue(), -1.5);
});

test_case("JsonObject add field string") {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue::makeString("whoa!"_str));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->stringValue(), "whoa!"_str);
}

comptime_test_case(JsonObject, AddFieldString, {
	JsonObject obj;
	obj.addField("some name"_str, JsonValue::makeString("whoa!"_str));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->stringValue(), "whoa!"_str);
});

test_case("JsonObject add field array") {
	JsonObject obj;
	ArrayList<JsonValue> values;
	values.push(JsonValue::makeBool(true));
	obj.addField("some name"_str, JsonValue::makeArray(std::move(values)));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->arrayValue()[0].boolValue(), true);
}

comptime_test_case(JsonObject, AddFieldArray, {
	JsonObject obj;
	ArrayList<JsonValue> values;
	values.push(JsonValue::makeBool(true));
	obj.addField("some name"_str, JsonValue::makeArray(std::move(values)));
	check(obj.findField("some name"_str).isSome());
	check_eq(obj.findField("some name"_str).some()->arrayValue()[0].boolValue(), true);
});

test_case("JsonObject add field object") {
	JsonObject obj;
	JsonObject subobj;
	subobj.addField("sub field"_str, JsonValue::makeBool(true));
	obj.addField("some name"_str, JsonValue::makeObject(std::move(subobj)));
	check(obj.findField("some name"_str).isSome());
	const JsonObject& subobjRef = obj.findField("some name"_str).some()->objectValue();
	check_eq(subobjRef.findField("sub field"_str).some()->boolValue(), true);
}

comptime_test_case(JsonObject, AddFieldObject, {
	JsonObject obj;
	JsonObject subobj;
	subobj.addField("sub field"_str, JsonValue::makeBool(true));
	obj.addField("some name"_str, JsonValue::makeObject(std::move(subobj)));
	check(obj.findField("some name"_str).isSome());
	const JsonObject& subobjRef = obj.findField("some name"_str).some()->objectValue();
	check_eq(subobjRef.findField("sub field"_str).some()->boolValue(), true);
});

test_case("JsonObject to string empty") {
	JsonObject obj;
	String jsonString = obj.toString();
	check_eq(jsonString, "{}"_str);
}

comptime_test_case(JsonObject, to_string_empty, {
	JsonObject obj;
	String jsonString = obj.toString();
	check_eq(jsonString, "{}"_str);
});

test_case("JsonObject to string one null field") {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeNull());
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": null").isSome());
}

comptime_test_case(JsonObject, to_string_one_null_field, {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeNull());
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": null").isSome());
});

test_case("JsonObject to string two null fields") {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeNull());
	obj.addField("someField2"_str, JsonValue::makeNull());
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": null,").isSome());
	check(jsonString.find("\"someField2\": null").isSome());
}

comptime_test_case(JsonObject, to_string_two_null_fields, {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeNull());
	obj.addField("someField2"_str, JsonValue::makeNull());
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": null,").isSome());
	check(jsonString.find("\"someField2\": null").isSome());
});

test_case("JsonObject to string one bool field") {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeBool(true));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": true").isSome());
}

comptime_test_case(JsonObject, to_string_one_bool_field, {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeBool(true));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": true").isSome());
	});

test_case("JsonObject to string two bool fields") {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeBool(true));
	obj.addField("someField2"_str, JsonValue::makeBool(false));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": true,").isSome());
	check(jsonString.find("\"someField2\": false").isSome());
}

comptime_test_case(JsonObject, to_string_two_bool_fields, {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeBool(true));
	obj.addField("someField2"_str, JsonValue::makeBool(false));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": true,").isSome());
	check(jsonString.find("\"someField2\": false").isSome());
});

test_case("JsonObject to string one number field") {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeNumber(-15.72));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": -15.72").isSome());
}

comptime_test_case(JsonObject, to_string_one_number_field, {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeNumber(-15.72));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": -15.72").isSome());
});

test_case("JsonObject to string two number fields") {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeNumber(-15.72));
	obj.addField("someField2"_str, JsonValue::makeNumber(0));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": -15.72,").isSome());
	check(jsonString.find("\"someField2\": 0.0").isSome());
}

comptime_test_case(JsonObject, to_string_two_number_fields, {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeNumber(-15.72));
	obj.addField("someField2"_str, JsonValue::makeNumber(0));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": -15.72,").isSome());
	check(jsonString.find("\"someField2\": 0.0").isSome());
});

test_case("JsonObject to string one string field") {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeString("chicken"_str));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": \"chicken\"").isSome());
}

comptime_test_case(JsonObject, to_string_one_string_field, {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeString("chicken"_str));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": \"chicken\"").isSome());
	});

test_case("JsonObject to string two string fields") {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeString("woa"_str));
	obj.addField("someField2"_str, JsonValue::makeString(":}"_str));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": \"woa\",").isSome());
	check(jsonString.find("\"someField2\": \":}\"").isSome());
}

comptime_test_case(JsonObject, to_string_two_string_fields, {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeString("woa"_str));
	obj.addField("someField2"_str, JsonValue::makeString(":}"_str));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": \"woa\",").isSome());
	check(jsonString.find("\"someField2\": \":}\"").isSome());
});

test_case("JsonObject to string one array field") {
	JsonObject obj;
	ArrayList<JsonValue> a;
	a.push(JsonValue::makeNumber(0));
	obj.addField("someField"_str, JsonValue::makeArray(std::move(a)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": [0.0]").isSome());
}

comptime_test_case(JsonObject, to_string_one_array_field, {
	JsonObject obj;
	ArrayList<JsonValue> a;
	a.push(JsonValue::makeNumber(0));
	obj.addField("someField"_str, JsonValue::makeArray(std::move(a)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": [0.0]").isSome());
	});

test_case("JsonObject to string two array fields") {
	JsonObject obj;
	ArrayList<JsonValue> a1;
	ArrayList<JsonValue> a2;
	a1.push(JsonValue::makeNumber(0));
	a2.push(JsonValue::makeNumber(95.7));
	obj.addField("someField1"_str, JsonValue::makeArray(std::move(a1)));
	obj.addField("someField2"_str, JsonValue::makeArray(std::move(a2)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": [0.0]").isSome());
	check(jsonString.find("\"someField2\": [95.7]").isSome());
}

comptime_test_case(JsonObject, to_string_two_array_fields, {
	JsonObject obj;
	ArrayList<JsonValue> a1;
	ArrayList<JsonValue> a2;
	a1.push(JsonValue::makeNumber(0));
	a2.push(JsonValue::makeNumber(95.7));
	obj.addField("someField1"_str, JsonValue::makeArray(std::move(a1)));
	obj.addField("someField2"_str, JsonValue::makeArray(std::move(a2)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": [0.0]").isSome());
	check(jsonString.find("\"someField2\": [95.7]").isSome());
	});

test_case("JsonObject to string one array field multiple array values") {
	JsonObject obj;
	ArrayList<JsonValue> a;
	a.push(JsonValue::makeNumber(0));
	a.push(JsonValue::makeNumber(1));
	a.push(JsonValue::makeNumber(10.8));
	obj.addField("someField"_str, JsonValue::makeArray(std::move(a)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": [0.0, 1.0, 10.8]").isSome());
}

comptime_test_case(JsonObject, to_string_one_array_field_multiple_array_values, {
	JsonObject obj;
	ArrayList<JsonValue> a;
	a.push(JsonValue::makeNumber(0));
	a.push(JsonValue::makeNumber(1));
	a.push(JsonValue::makeNumber(10.8));
	obj.addField("someField"_str, JsonValue::makeArray(std::move(a)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": [0.0, 1.0, 10.8]").isSome());
	});

test_case("JsonObject to string two array fields multiple array values") {
	JsonObject obj;
	ArrayList<JsonValue> a1;
	ArrayList<JsonValue> a2;

	a1.push(JsonValue::makeNumber(10));
	a1.push(JsonValue::makeNumber(60));
	a1.push(JsonValue::makeNumber(123.45));

	a2.push(JsonValue::makeNumber(95.7));
	a2.push(JsonValue::makeNumber(-10));
	a2.push(JsonValue::makeNumber(-500));

	obj.addField("someField1"_str, JsonValue::makeArray(std::move(a1)));
	obj.addField("someField2"_str, JsonValue::makeArray(std::move(a2)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": [10.0, 60.0, 123.45]").isSome());
	check(jsonString.find("\"someField2\": [95.7, -10.0, -500.0]").isSome());
}

comptime_test_case(JsonObject, to_string_two_array_fields_multiple_array_values, {
	JsonObject obj;
	ArrayList<JsonValue> a1;
	ArrayList<JsonValue> a2;

	a1.push(JsonValue::makeNumber(10));
	a1.push(JsonValue::makeNumber(60));
	a1.push(JsonValue::makeNumber(123.45));

	a2.push(JsonValue::makeNumber(95.7));
	a2.push(JsonValue::makeNumber(-10));
	a2.push(JsonValue::makeNumber(-500));

	obj.addField("someField1"_str, JsonValue::makeArray(std::move(a1)));
	obj.addField("someField2"_str, JsonValue::makeArray(std::move(a2)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": [10.0, 60.0, 123.45]").isSome());
	check(jsonString.find("\"someField2\": [95.7, -10.0, -500.0]").isSome());
});

test_case("JsonObject to string one object field empty object") {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeObject(JsonObject()));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": {}").isSome());
}

comptime_test_case(JsonObject, to_string_one_object_field_empty_object, {
	JsonObject obj;
	obj.addField("someField"_str, JsonValue::makeObject(JsonObject()));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": {}").isSome());
	});

test_case("JsonObject to string two object fields empty objects") {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeObject(JsonObject()));
	obj.addField("someField2"_str, JsonValue::makeObject(JsonObject()));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": {},").isSome());
	check(jsonString.find("\"someField2\": {}").isSome());
}

comptime_test_case(JsonObject, to_string_two_object_fields_empty_objects, {
	JsonObject obj;
	obj.addField("someField1"_str, JsonValue::makeObject(JsonObject()));
	obj.addField("someField2"_str, JsonValue::makeObject(JsonObject()));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": {},").isSome());
	check(jsonString.find("\"someField2\": {}").isSome());
	});

test_case("JsonObject to string one object field subobject") {
	JsonObject obj;
	JsonObject subobj;
	subobj.addField("subField"_str, JsonValue::makeBool(false));
	obj.addField("someField"_str, JsonValue::makeObject(std::move(subobj)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": {").isSome());
	check(jsonString.find("\"subField\": false").isSome());
}

comptime_test_case(JsonObject, to_string_one_object_field_subobject, {
	JsonObject obj;
	JsonObject subobj;
	subobj.addField("subField"_str, JsonValue::makeBool(false));
	obj.addField("someField"_str, JsonValue::makeObject(std::move(subobj)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField\": {").isSome());
	check(jsonString.find("\"subField\": false").isSome());
	});

test_case("JsonObject to string two object field subobjects") {
	JsonObject obj;
	JsonObject subobj1;
	JsonObject subobj2;
	subobj1.addField("subField"_str, JsonValue::makeBool(true));
	subobj2.addField("subField"_str, JsonValue::makeBool(false));
	obj.addField("someField1"_str, JsonValue::makeObject(std::move(subobj1)));
	obj.addField("someField2"_str, JsonValue::makeObject(std::move(subobj2)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": {").isSome());
	check(jsonString.find("\"subField\": true").isSome());
	check(jsonString.find("},").isSome());
	check(jsonString.find("\"someField2\": {").isSome());
	check(jsonString.find("\"subField\": false").isSome());
}

comptime_test_case(JsonObject, to_string_two_object_field_subobjects, {
	JsonObject obj;
	JsonObject subobj1;
	JsonObject subobj2;
	subobj1.addField("subField"_str, JsonValue::makeBool(true));
	subobj2.addField("subField"_str, JsonValue::makeBool(false));
	obj.addField("someField1"_str, JsonValue::makeObject(std::move(subobj1)));
	obj.addField("someField2"_str, JsonValue::makeObject(std::move(subobj2)));
	String jsonString = obj.toString();
	check(jsonString.find("\"someField1\": {").isSome());
	check(jsonString.find("\"subField\": true").isSome());
	check(jsonString.find("},").isSome());
	check(jsonString.find("\"someField2\": {").isSome());
	check(jsonString.find("\"subField\": false").isSome());
	});

#endif