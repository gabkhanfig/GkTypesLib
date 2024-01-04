#include "json_object.h"
#include <intrin.h>
#include "../cpu_features/cpu_feature_detector.h"

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
using gk::Result;
using gk::JsonValueType;

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

test_case("JsonObject parse empty json object") {
	gk::Str jsonString = "{}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	check_eq(res.ok().fieldCount(), 0);
}

comptime_test_case(JsonObject, parse_empty_json_object, {
	gk::Str jsonString = "{}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	check_eq(res.ok().fieldCount(), 0);
});

test_case("JsonObject parse empty json object sanity") {
	gk::Str jsonString = "  {\n\n\t}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	check_eq(res.ok().fieldCount(), 0);
}

comptime_test_case(JsonObject, parse_empty_json_object_sanity, {
	gk::Str jsonString = "  {\n\n\t}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	check_eq(res.ok().fieldCount(), 0);
});

test_case("JsonObject parse json object one null value") {
	gk::Str jsonString = "{\"field\": null}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Null);
}

comptime_test_case(JsonObject, parse_json_object_one_null_value, {
	gk::Str jsonString = "{\"field\": null}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Null);
});

test_case("JsonObject parse json object one null value sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \nnull\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Null);
}

comptime_test_case(JsonObject, parse_json_object_one_null_value_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \nnull\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Null);
});

test_case("JsonObject parse json object one bool value true") {
	gk::Str jsonString = "{\"field\": true}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), true);
}

comptime_test_case(JsonObject, parse_json_object_one_bool_value_true, {
	gk::Str jsonString = "{\"field\": true}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), true);
});

test_case("JsonObject parse json object one bool value true sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \ntrue\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), true);
}

comptime_test_case(JsonObject, parse_json_object_one_bool_value_true_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \ntrue\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), true);
});

test_case("JsonObject parse json object one bool value false") {
	gk::Str jsonString = "{\"field\": false}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), false);
}

comptime_test_case(JsonObject, parse_json_object_one_bool_value_false, {
	gk::Str jsonString = "{\"field\": false}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), false);
});

test_case("JsonObject parse json object one bool value false sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \nfalse\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), false);
}

comptime_test_case(JsonObject, parse_json_object_one_bool_value_false_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \nfalse\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Bool);
	check_eq(obj.findField("field"_str).some()->boolValue(), false);
});

test_case("JsonObject parse json object one number value zero") {
	gk::Str jsonString = "{\"field\": 0}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 0.0);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_zero, {
	gk::Str jsonString = "{\"field\": 0}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 0.0);
});

test_case("JsonObject parse json object one number value zero sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n0.0\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 0.0);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_zero_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n0.0\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 0.0);
});

test_case("JsonObject parse json object one number value random positive integer") {
	gk::Str jsonString = "{\"field\": 6591}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.0);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_positive_integer, {
	gk::Str jsonString = "{\"field\": 6591}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.0);
});

test_case("JsonObject parse json object one number value random positive integer sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n6591.0\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.0);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_positive_integer_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n6591.0\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.0);
});

test_case("JsonObject parse json object one number value random negative integer") {
	gk::Str jsonString = "{\"field\": -6591}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.0);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_negative_integer, {
	gk::Str jsonString = "{\"field\": -6591}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.0);
});

test_case("JsonObject parse json object one number value random negative integer sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n-6591.0\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.0);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_negative_integer_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n-6591.0\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.0);
});

test_case("JsonObject parse json object one number value random positive decimal") {
	gk::Str jsonString = "{\"field\": 6591.1945}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.1945);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_positive_decimal, {
	gk::Str jsonString = "{\"field\": 6591.1945}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.1945);
});

test_case("JsonObject parse json object one number value random positive decimal sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n6591.1945\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.1945);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_positive_decimal_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n6591.1945\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), 6591.1945);
});

test_case("JsonObject parse json object one number value random negative decimal") {
	gk::Str jsonString = "{\"field\": -6591.1945}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.1945);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_negative_decimal, {
	gk::Str jsonString = "{\"field\": -6591.1945}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.1945);
});

test_case("JsonObject parse json object one number value random negative decimal sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n-6591.1945\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.1945);
}

comptime_test_case(JsonObject, parse_json_object_one_number_value_random_negative_decimal_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n-6591.1945\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Number);
	check_eq(obj.findField("field"_str).some()->numberValue(), -6591.1945);
});

test_case("JsonObject parse json object one string value") {
	gk::Str jsonString = "{\"field\": \"hello world!\"}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::String);
	check_eq(obj.findField("field"_str).some()->stringValue(), "hello world!"_str);
}

comptime_test_case(JsonObject, parse_json_object_one_string_value, {
	gk::Str jsonString = "{\"field\": \"hello world!\"}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::String);
	check_eq(obj.findField("field"_str).some()->stringValue(), "hello world!"_str);
	});

test_case("JsonObject parse json object one string value sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n\"hello world!\"\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::String);
	check_eq(obj.findField("field"_str).some()->stringValue(), "hello world!"_str);
}

comptime_test_case(JsonObject, parse_json_object_one_string_value_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n\"hello world!\"\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::String);
	check_eq(obj.findField("field"_str).some()->stringValue(), "hello world!"_str);
	});

test_case("JsonObject parse json object one array value one element") {
	gk::Str jsonString = "{\"field\": [0]}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = field->arrayValue();
	check_eq(arr.len(), 1);
	check_eq(arr[0].type(), JsonValueType::Number);
	check_eq(arr[0].numberValue(), 0);
}

comptime_test_case(JsonObject, parse_json_object_one_array_value_one_element, {
	gk::Str jsonString = "{\"field\": [0]}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = obj.findField("field"_str).some()->arrayValue();
	check_eq(arr.len(), 1);
	check_eq(arr[0].type(), JsonValueType::Number);
	check_eq(arr[0].numberValue(), 0);
	});

test_case("JsonObject parse json object one array value one element sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n[0]\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = obj.findField("field"_str).some()->arrayValue();
	check_eq(arr.len(), 1);
	check_eq(arr[0].type(), JsonValueType::Number);
	check_eq(arr[0].numberValue(), 0);
}

comptime_test_case(JsonObject, parse_json_object_one_array_value_one_element_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n[0]\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = obj.findField("field"_str).some()->arrayValue();
	check_eq(arr.len(), 1);
	check_eq(arr[0].type(), JsonValueType::Number);
	check_eq(arr[0].numberValue(), 0);
	});

test_case("JsonObject parse json object one array value multiple elements") {
	gk::Str jsonString = "{\"field\": [0, 1, 2]}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = field->arrayValue();
	check_eq(arr.len(), 3);
	check_eq(arr[0].numberValue(), 0);
	check_eq(arr[1].numberValue(), 1);
	check_eq(arr[2].numberValue(), 2);
}

comptime_test_case(JsonObject, parse_json_object_one_array_value_multiple_elements, {
	gk::Str jsonString = "{\"field\": [0, 1, 2]}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = field->arrayValue();
	check_eq(arr.len(), 3);
	check_eq(arr[0].numberValue(), 0);
	check_eq(arr[1].numberValue(), 1);
	check_eq(arr[2].numberValue(), 2);
	});

test_case("JsonObject parse json object one array value multiple elements sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n[0, 1, 2]\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = field->arrayValue();
	check_eq(arr.len(), 3);
	check_eq(arr[0].numberValue(), 0);
	check_eq(arr[1].numberValue(), 1);
	check_eq(arr[2].numberValue(), 2);
}

comptime_test_case(JsonObject, parse_json_object_one_array_value_multiple_elements_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n[0, 1, 2]\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = field->arrayValue();
	check_eq(arr.len(), 3);
	check_eq(arr[0].numberValue(), 0);
	check_eq(arr[1].numberValue(), 1);
	check_eq(arr[2].numberValue(), 2);
	});

test_case("JsonObject parse json object one array value no elements") {
	gk::Str jsonString = "{\"field\": []}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = field->arrayValue();
	check_eq(arr.len(), 0);
}

comptime_test_case(JsonObject, parse_json_object_one_array_value_no_elements, {
	gk::Str jsonString = "{\"field\": []}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = obj.findField("field"_str).some()->arrayValue();
	check_eq(arr.len(), 0);
	});

test_case("JsonObject parse json object one array value no elements sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n[]\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = obj.findField("field"_str).some()->arrayValue();
	check_eq(arr.len(), 0);
}

comptime_test_case(JsonObject, parse_json_object_one_array_value_no_elements_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n[]\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	check_eq(obj.findField("field"_str).some()->type(), JsonValueType::Array);
	ArrayList<JsonValue>& arr = obj.findField("field"_str).some()->arrayValue();
	check_eq(arr.len(), 0);
	});

test_case("JsonObject parse json object one object value empty") {
	gk::Str jsonString = "{\"field\": {}}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	check_eq(subobj.fieldCount(), 0);
}

comptime_test_case(JsonObject, parse_json_object_one_object_value_empty, {
	gk::Str jsonString = "{\"field\": {}}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	check_eq(subobj.fieldCount(), 0);
	});

test_case("JsonObject parse json object one object value empty sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n{ }\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	check_eq(subobj.fieldCount(), 0);
}

comptime_test_case(JsonObject, parse_json_object_one_array_object_value_empty_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n{ }\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	check_eq(subobj.fieldCount(), 0);
	});


test_case("JsonObject parse json object one object value null subfield") {
	gk::Str jsonString = "{\"field\": {\"sub\": null}}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	auto sub = subobj.findField("sub"_str).some();
	check(sub->isNull());
}

comptime_test_case(JsonObject, parse_json_object_one_object_value_null_subfield, {
	gk::Str jsonString = "{\"field\": { \"sub\": null}}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	auto sub = subobj.findField("sub"_str).some();
	check(sub->isNull());
	});

test_case("JsonObject parse json object one object value null subfield sanity") {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n{\t\t\n\"sub\":    null\n}\n}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	auto sub = subobj.findField("sub"_str).some();
	check(sub->isNull());
}

comptime_test_case(JsonObject, parse_json_object_one_object_value_null_subfield_sanity, {
	gk::Str jsonString = "{\n \"field\"  \n\t: \n{\t\t\n\"sub\":    null\n}\n}}";
	Result<JsonObject> res = JsonObject::parse(jsonString);
	check(res.isOk());
	JsonObject obj = res.ok();
	check(obj.findField("field"_str).isSome());
	auto field = obj.findField("field"_str).some();
	check_eq(field->type(), JsonValueType::Object);
	JsonObject& subobj = field->objectValue();
	auto sub = subobj.findField("sub"_str).some();
	check(sub->isNull());
	});

namespace gk {
	namespace unitTests {
		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldBoolTrue() {
			Str jsonString = "{\"field\": {\"sub\": true}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->boolValue(), true);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldBoolFalse() {
			Str jsonString = "{\"field\": {\"sub\": false}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->boolValue(), false);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldNumberZero() {
			Str jsonString = "{\"field\": {\"sub\": 0}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->numberValue(), 0);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldNumberZeroDecimal() {
			Str jsonString = "{\"field\": {\"sub\": 0.0}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->numberValue(), 0);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldNumberPositiveInteger() {
			Str jsonString = "{\"field\": {\"sub\": 156259}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->numberValue(), 156259);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldNumberPositiveDecimal() {
			Str jsonString = "{\"field\": {\"sub\": 910.56}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->numberValue(), 910.56);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldNumberNegativeInteger() {
			Str jsonString = "{\"field\": {\"sub\": -156259}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->numberValue(), -156259);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldNumberNegativeDecimal() {
			Str jsonString = "{\"field\": {\"sub\": -910.56}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->numberValue(), -910.56);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldString() {
			Str jsonString = "{\"field\": {\"sub\": \"hello world!\"}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->stringValue(), "hello world!"_str);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldArrayEmpty() {
			Str jsonString = "{\"field\": {\"sub\": []}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->arrayValue().len(), 0);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldArrayOneElement() {
			Str jsonString = "{\"field\": {\"sub\": [10]}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->arrayValue().len(), 1);
			check_eq(sub->arrayValue()[0].numberValue(), 10);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldArrayMultipleElements() {
			Str jsonString = "{\"field\": {\"sub\": [0, 1, 2]}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->arrayValue().len(), 3);
			check_eq(sub->arrayValue()[0].numberValue(), 0);
			check_eq(sub->arrayValue()[1].numberValue(), 1);
			check_eq(sub->arrayValue()[2].numberValue(), 2);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldObjectEmpty() {
			Str jsonString = "{\"field\": {\"sub\": {}}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			check_eq(sub->objectValue().fieldCount(), 0);
		}

		constexpr void testJsonObjectParseObjectOneObjectValueSubfieldObjectNullValue() {
			Str jsonString = "{\"field\": {\"sub\": {\"subsub\": null}}}";
			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();
			check(obj.findField("field"_str).isSome());
			auto field = obj.findField("field"_str).some();
			check_eq(field->type(), JsonValueType::Object);
			JsonObject& subobj = field->objectValue();
			auto sub = subobj.findField("sub"_str).some();
			auto& subsubobj = sub->objectValue();
			check(subsubobj.findField("subsub"_str).some()->isNull());
		}
	}
}

test_case("JsonObject parse one object value subfield bool true") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldBoolTrue();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_bool_true, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldBoolTrue();
});

test_case("JsonObject parse one object value subfield bool false") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldBoolFalse();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_bool_false, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldBoolFalse();
});

test_case("JsonObject parse one object value subfield number zero") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberZero();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_number_zero, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberZero();
});

test_case("JsonObject parse one object value subfield number zero decimal") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberZeroDecimal();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_number_zero_decimal, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberZeroDecimal();
});

test_case("JsonObject parse one object value subfield number positive integer") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberPositiveInteger();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_number_positive_integer, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberPositiveInteger();
});

test_case("JsonObject parse one object value subfield number positive decimal") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberPositiveDecimal();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_number_positive_decimal, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberPositiveDecimal();
});

test_case("JsonObject parse one object value subfield number negative integer") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberNegativeInteger();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_number_negative_integer, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberNegativeInteger();
});

test_case("JsonObject parse one object value subfield number negative decimal") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberNegativeDecimal();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_number_negative_decimal, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldNumberNegativeDecimal();
});

test_case("JsonObject parse one object value subfield string") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldString();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_string, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldString();
});

test_case("JsonObject parse one object value subfield array empty") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldArrayEmpty();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_array_empty, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldArrayEmpty();
});

test_case("JsonObject parse one object value subfield array one element") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldArrayOneElement();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_array_one_element, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldArrayOneElement();
});

test_case("JsonObject parse one object value subfield array multiple elements") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldArrayMultipleElements();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_array_multiple_elements, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldArrayMultipleElements();
});

test_case("JsonObject parse one object value subfield object empty") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldObjectEmpty();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_object_empty, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldObjectEmpty();
});

test_case("JsonObject parse one object value subfield object nested nested") {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldObjectNullValue();
}

comptime_test_case(JsonObject, parse_one_object_value_subfield_object_nested_nested, {
	gk::unitTests::testJsonObjectParseObjectOneObjectValueSubfieldObjectNullValue();
});

namespace gk {
	namespace unitTests {
		constexpr void testJsonObjectParseMultipleFields() {
			Str jsonString = "{\n\"nullField\": null,\n\"boolField\": true,\n\"numberField\": -15.6,\n\"stringField\": \"whoa!\",\n\"arrayField\": [],\n\"objectField\": {},\n\"lastField\": false}";

			Result<JsonObject> res = JsonObject::parse(jsonString);
			check(res.isOk());
			JsonObject obj = res.ok();

			auto nullField = obj.findField("nullField"_str).some();
			check(nullField->isNull());

			auto boolField = obj.findField("boolField"_str).some();
			check_eq(boolField->boolValue(), true);

			auto numberField = obj.findField("numberField"_str).some();
			check_eq(numberField->numberValue(), -15.6);

			auto stringField = obj.findField("stringField"_str).some();
			check_eq(stringField->stringValue(), "whoa!"_str);

			auto arrayField = obj.findField("arrayField"_str).some();
			check_eq(arrayField->arrayValue().len(), 0);

			auto objectField = obj.findField("objectField"_str).some();
			check_eq(objectField->objectValue().fieldCount(), 0);

			auto lastField = obj.findField("lastField"_str).some();
			check_eq(lastField->boolValue(), false);
		}
	}
}

test_case("JsonObject parse multiple fields") {
	gk::unitTests::testJsonObjectParseMultipleFields();
}

comptime_test_case(JsonObject, parse_multiple_fields, {
	gk::unitTests::testJsonObjectParseMultipleFields();
});

#endif