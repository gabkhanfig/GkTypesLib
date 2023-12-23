#pragma once

#include <type_traits>
#include "json_object.h"
#include "../reflection/field_name.h"

namespace gk
{
	//template<typename T>
	//concept Serializable = !std::is_fundamental_v<T>;

	//template<typename T>
	//concept Deserializable = !std::is_fundamental_v<T>;

	/**
	* Serializes a struct's fields into a JsonObject.
	* Requires that all struct fields have public visibility
	* and that it can be constructed with them.
	* 
	* @param object: Object to serialize into json.
	* @return Json representation of `object`. Order is not guaranteed.
	*/
	template<typename T>
	constexpr JsonObject serialize(const T& object);
		//requires(Serializable<T>);

	/**
	* Deserializes a json object into a new instance of a struct.
	* Requires that all struct fields have public visibility
	* and that it can be constructed with them.
	* If the json object cannot be converted into the struct, an error will be returned.
	* 
	* @param T: Type to convert json into
	* @param jsonObject: The data to deserialize
	* @return An Ok variant with the deserialized data, or an Error variant if it was unsuccessful.
	*/
	template<typename T>
	constexpr Result<T> deserialize(JsonObject& jsonObject);
		//requires(Deserializable<T>);

	template<typename T>
	constexpr JsonValue toJsonValue(const T& value);

	namespace internal 
	{
		template<typename T>
		constexpr JsonValue convertArrayListToJsonValue(const ArrayList<T>& arr);

		template<typename T>
		constexpr JsonValue convertObjectToJsonValue(const T& object);

		template<typename T>
		constexpr void convertTupleElementToJsonValue(const T& elem, JsonObject& json) {
			json.addField(elem.name, toJsonValue(elem.value));
		}

		template<typename T, usize... Is>
		constexpr void forEachSerialize(const T& t, JsonObject& json, std::integer_sequence<usize, Is...>) {
			auto _ = { (convertTupleElementToJsonValue(std::get<Is>(t), json), 0)... };
		}

		template<typename T>
		constexpr bool tryAssignFieldFromJsonObject(T& field, Str name, JsonObject& jsonObject);

		template<typename T>
		constexpr bool assignFieldFromJsonValue(T& field, JsonValue* jsonValue);
	} // namespace internal
} // namespace gk

template<typename T>
inline constexpr gk::JsonValue gk::toJsonValue(const T& value)
{
	using namespace gk;

	if constexpr (std::is_same_v<T, bool>) {
		return JsonValue::makeBool(value);
	}
	else if constexpr (std::is_arithmetic_v<T>) {
		return JsonValue::makeNumber(value);
	}
	else if constexpr (std::is_enum_v<T>) {
		return JsonValue::makeString(String::from(value));
	}
	else if constexpr (std::is_same_v<T, String> || std::is_same_v<T, Str>) {
		return JsonValue::makeString(String(value));
	}
	else if constexpr (std::is_same_v<T, GlobalString>) {
		return JsonValue::makeString(String(value.toString()));
	}
	else if constexpr (internal::is_array_list<T>) {
		return internal::convertArrayListToJsonValue<typename T::ValueType>(value);
	}
	else {
		return internal::convertObjectToJsonValue(value);
	}
}

template<typename T>
inline constexpr gk::JsonValue gk::internal::convertArrayListToJsonValue(const ArrayList<T>& arr)
{
	ArrayList<JsonValue> values;
	if (!std::is_constant_evaluated()) {
		values.reserve(arr.len());
	}
	for (usize i = 0; i < arr.len(); i++) {
		values.push(toJsonValue(arr[i]));
	}
	return JsonValue::makeArray(std::move(values));
}

template<typename T>
inline constexpr gk::JsonValue gk::internal::convertObjectToJsonValue(const T& object)
{
	return JsonValue::makeObject(serialize(object));
}

template<typename T>
constexpr bool gk::internal::tryAssignFieldFromJsonObject(T& field, Str name, JsonObject& jsonObject)
{
	Option<JsonValue*> found = jsonObject.findField(name);
	if (found.none()) return false;
	assignFieldFromJsonValue(field, found.some());
	return true;
}

template<typename T>
constexpr bool gk::internal::assignFieldFromJsonValue(T& field, JsonValue* jsonValue)
{
	if constexpr (std::is_same_v<T, bool>) {
		if (jsonValue->type() != JsonValueType::Bool) {
			return false;
		}
		field = jsonValue->boolValue();
	}
	else if constexpr (std::is_arithmetic_v<T>) {
		if (jsonValue->type() != JsonValueType::Number) {
			return false;
		}
		field = static_cast<T>(jsonValue->numberValue());
	}
	else if constexpr (std::is_enum_v<T>) {
		if (jsonValue->type() != JsonValueType::String) {
			return false;
		}
		Result<T> parseEnum = jsonValue->stringValue().parse<T>();
		if (parseEnum.error()) return false;
		field = parseEnum.ok();
	}
	else if constexpr (std::is_same_v<T, String>) {
		if (jsonValue->type() != JsonValueType::String) {
			return false;
		}
		field = std::move(jsonValue->stringValue());
	}
	else if constexpr (std::is_same_v<T, GlobalString>) {
		if (jsonValue->type() != JsonValueType::String) {
			return false;
		}
		field = GlobalString::create(jsonValue->stringValue());
	}
	else if constexpr (internal::is_array_list<T>) {
		if (jsonValue->type() != JsonValueType::Array) {
			return false;
		}	
		ArrayList<JsonValue> values = jsonValue->arrayValue();
		for (usize i = 0; i < values.len(); i++) {
			typename T::ValueType temp; // value type of the array list.
			if (assignFieldFromJsonValue(temp, &values[i]) == false) {
				return false;
			}
			field.push(std::move(temp));
		}
	}
	else {
		if (jsonValue->type() != JsonValueType::Object) {
			return false;
		}
		Result<T> deserialized = deserialize<T>(jsonValue->objectValue());
		if (deserialized.isError()) return false;
		field = deserialized.ok();
	}
	return true;
}

template<typename T>
inline constexpr gk::JsonObject gk::serialize(const T& object)
	//requires(Serializable<T>)
{
	auto fieldsTuple = toNamedFields(object);
	constexpr usize fieldCount = std::tuple_size<decltype(fieldsTuple)>::value;
	JsonObject jsonObject;
	internal::forEachSerialize(fieldsTuple, jsonObject, std::make_integer_sequence<usize, fieldCount>());
	return jsonObject;
}

template<typename T>
constexpr gk::Result<T> gk::deserialize(JsonObject& jsonObject)
	//requires(Deserializable<T>)
{
	T out{};
	auto fieldsTuple = toNamedFields(out);
	constexpr usize fieldCount = std::tuple_size<decltype(fieldsTuple)>::value;

	if constexpr (fieldCount == 1) {
		auto&& [p1] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 2) {
		auto&& [p1, p2] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 3) {
		auto&& [p1, p2, p3] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 4) {
		auto&& [p1, p2, p3, p4] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p4, getFieldName<T, 3>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 5) {
		auto&& [p1, p2, p3, p4, p5] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p4, getFieldName<T, 3>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p5, getFieldName<T, 4>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 6) {
		auto&& [p1, p2, p3, p4, p5, p6] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p4, getFieldName<T, 3>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p5, getFieldName<T, 4>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p6, getFieldName<T, 5>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 7) {
		auto&& [p1, p2, p3, p4, p5, p6, p7] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p4, getFieldName<T, 3>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p5, getFieldName<T, 4>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p6, getFieldName<T, 5>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p7, getFieldName<T, 6>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 8) {
		auto&& [p1, p2, p3, p4, p5, p6, p7, p8] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p4, getFieldName<T, 3>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p5, getFieldName<T, 4>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p6, getFieldName<T, 5>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p7, getFieldName<T, 6>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p8, getFieldName<T, 7>(), jsonObject) == false) return ResultErr();
	}
	else if constexpr (fieldCount == 9) {
		auto&& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = out;
		if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p2, getFieldName<T, 1>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p3, getFieldName<T, 2>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p4, getFieldName<T, 3>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p5, getFieldName<T, 4>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p6, getFieldName<T, 5>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p7, getFieldName<T, 6>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p8, getFieldName<T, 7>(), jsonObject) == false) return ResultErr();
		if (internal::tryAssignFieldFromJsonObject(p9, getFieldName<T, 8>(), jsonObject) == false) return ResultErr();
	}

	return ResultOk<T>(std::move(out));
}