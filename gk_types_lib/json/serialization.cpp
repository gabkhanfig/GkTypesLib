#include "serialization.h"

#if GK_TYPES_LIB_TEST

using gk::serialize;
using gk::deserialize;
using gk::JsonObject;
using gk::JsonValue;
using gk::String;
using gk::ArrayList;

namespace gk {
	namespace unitTests {
		struct SerializeExample1 {
			bool someFlag;
		};

		struct SerializeExample2 {
			double health;
			u32 power;
		};

		struct SerializeExample3 {
			bool someFlag;
			String name;
		};

		struct SerializeExample4 {
			ArrayList<int> numbers;
		};

		struct SerializeExample5 {
			SerializeExample2 nested;
		};
	}
}

using gk::unitTests::SerializeExample1;
using gk::unitTests::SerializeExample2;
using gk::unitTests::SerializeExample3;
using gk::unitTests::SerializeExample4;
using gk::unitTests::SerializeExample5;

test_case("Serialize one bool field") {
	SerializeExample1 e;
	e.someFlag = true;
	JsonObject obj = serialize(e);
	check(obj.findField("someFlag"_str).isSome());
	check_eq(obj.findField("someFlag"_str).some()->boolValue(), true);
}

comptime_test_case(SerializeOneBoolField, {
	SerializeExample1 e;
	e.someFlag = true;
	JsonObject obj = serialize(e);
	check(obj.findField("someFlag"_str).isSome());
	check_eq(obj.findField("someFlag"_str).some()->boolValue(), true);
});

test_case("Deserialize one bool field") {
	JsonObject obj;
	obj.addField("someFlag"_str, JsonValue::makeBool(true));
	SerializeExample1 des = deserialize<SerializeExample1>(obj).ok();
	check_eq(des.someFlag, true);
}

comptime_test_case(DeserializeOneBoolField, {
	JsonObject obj;
	obj.addField("someFlag"_str, JsonValue::makeBool(true));
	SerializeExample1 des = deserialize<SerializeExample1>(obj).ok();
	check_eq(des.someFlag, true);
});

test_case("Serialize and deserialize one bool field") {
	SerializeExample1 e;
	e.someFlag = true;
	JsonObject obj = serialize(e);
	SerializeExample1 des = deserialize<SerializeExample1>(obj).ok();
	check_eq(des.someFlag, true);
}

comptime_test_case(SerializeAndDeserializeOneBoolField, {
	SerializeExample1 e;
	e.someFlag = true;
	JsonObject obj = serialize(e);
	SerializeExample1 des = deserialize<SerializeExample1>(obj).ok();
	check_eq(des.someFlag, true);
});

test_case("Serialize two number fields") {
	SerializeExample2 e;
	e.health = 195.5;
	e.power = 10;
	JsonObject obj = serialize(e);
	check(obj.findField("health"_str).isSome());
	check(obj.findField("power"_str).isSome());
	check_eq(obj.findField("health"_str).some()->numberValue(), 195.5);
	check_eq(obj.findField("power"_str).some()->numberValue(), 10);
}

comptime_test_case(SerializeTwoNumberFields, {
	SerializeExample2 e;
	e.health = 195.5;
	e.power = 10;
	JsonObject obj = serialize(e);
	check(obj.findField("health"_str).isSome());
	check(obj.findField("power"_str).isSome());
	check_eq(obj.findField("health"_str).some()->numberValue(), 195.5);
	check_eq(obj.findField("power"_str).some()->numberValue(), 10);
});

test_case("Deserialize two number fields") {
	JsonObject obj;
	obj.addField("health"_str, JsonValue::makeNumber(60.1));
	obj.addField("power"_str, JsonValue::makeNumber(5));
	SerializeExample2 des = deserialize<SerializeExample2>(obj).ok();
	check_eq(des.health, 60.1);
	check_eq(des.power, 5);
}

comptime_test_case(DeserializeTwoNumberFields, {
	JsonObject obj;
	obj.addField("health"_str, JsonValue::makeNumber(60.1));
	obj.addField("power"_str, JsonValue::makeNumber(5));
	SerializeExample2 des = deserialize<SerializeExample2>(obj).ok();
	check_eq(des.health, 60.1);
	check_eq(des.power, 5);
});

test_case("Serialize and deserialize two number fields") {
	SerializeExample2 e;
	e.health = -1.7;
	e.power = 4;
	JsonObject obj = serialize(e);
	SerializeExample2 des = deserialize<SerializeExample2>(obj).ok();
	check_eq(des.health, -1.7);
	check_eq(des.power, 4);
}

comptime_test_case(SerializeAndDeserializeTwoNumberFields, {
	SerializeExample2 e;
	e.health = -1.7;
	e.power = 4;
	JsonObject obj = serialize(e);
	SerializeExample2 des = deserialize<SerializeExample2>(obj).ok();
	check_eq(des.health, -1.7);
	check_eq(des.power, 4);
});

test_case("Serialize with string field") {
	SerializeExample3 e;
	e.someFlag = true;
	e.name = "pslang"_str;
	JsonObject obj = serialize(e);
	check(obj.findField("someFlag"_str).isSome());
	check(obj.findField("name"_str).isSome());
	check_eq(obj.findField("someFlag"_str).some()->boolValue(), true);
	check_eq(obj.findField("name"_str).some()->stringValue(), "pslang"_str);
}

comptime_test_case(SerializeWithStringField, {
	SerializeExample3 e;
	e.someFlag = true;
	e.name = "pslang"_str;
	JsonObject obj = serialize(e);
	check(obj.findField("someFlag"_str).isSome());
	check(obj.findField("name"_str).isSome());
	check_eq(obj.findField("someFlag"_str).some()->boolValue(), true);
	check_eq(obj.findField("name"_str).some()->stringValue(), "pslang"_str);
	});

test_case("Deserialize with string field") {
	JsonObject obj;
	obj.addField("someFlag"_str, JsonValue::makeBool(true));
	obj.addField("name"_str, JsonValue::makeString("pslang"_str));
	SerializeExample3 des = deserialize<SerializeExample3>(obj).ok();
	check_eq(des.someFlag, true);
	check_eq(des.name, "pslang"_str);
}

comptime_test_case(DeserializeWithStringField, {
	JsonObject obj;
	obj.addField("someFlag"_str, JsonValue::makeBool(true));
	obj.addField("name"_str, JsonValue::makeString("pslang"_str));
	SerializeExample3 des = deserialize<SerializeExample3>(obj).ok();
	check_eq(des.someFlag, true);
	check_eq(des.name, "pslang"_str);
	});

test_case("Serialize and deserialize with string field") {
	SerializeExample3 e;
	e.someFlag = true;
	e.name = "pslang"_str;
	JsonObject obj = serialize(e);
	SerializeExample3 des = deserialize<SerializeExample3>(obj).ok();
	check_eq(des.someFlag, true);
	check_eq(des.name, "pslang"_str);
}

comptime_test_case(SerializeAndDeserializeWithStringField, {
	SerializeExample3 e;
	e.someFlag = true;
	e.name = "pslang"_str;
	JsonObject obj = serialize(e);
	SerializeExample3 des = deserialize<SerializeExample3>(obj).ok();
	check_eq(des.someFlag, true);
	check_eq(des.name, "pslang"_str);
	});

test_case("Serialize with array field") {
	SerializeExample4 e;
	for (int i = 0; i < 4; i++) {
		e.numbers.push(i);
	}
	JsonObject obj = serialize(e);
	check(obj.findField("numbers"_str).isSome());
	ArrayList<JsonValue>& values = obj.findField("numbers"_str).some()->arrayValue();
	check_eq(values[0].numberValue(), 0);
	check_eq(values[1].numberValue(), 1);
	check_eq(values[2].numberValue(), 2);
	check_eq(values[3].numberValue(), 3);
}

comptime_test_case(SerializeWithArrayField, { 
	SerializeExample4 e;
	for (int i = 0; i < 4; i++) {
		e.numbers.push(i);
	}
	JsonObject obj = serialize(e);
	check(obj.findField("numbers"_str).isSome());
	ArrayList<JsonValue>& values = obj.findField("numbers"_str).some()->arrayValue();
	check_eq(values[0].numberValue(), 0);
	check_eq(values[1].numberValue(), 1);
	check_eq(values[2].numberValue(), 2);
	check_eq(values[3].numberValue(), 3);
	});

test_case("Deserialize with array field") {
	JsonObject obj;
	ArrayList<JsonValue> values;
	values.push(JsonValue::makeNumber(10));
	obj.addField("numbers"_str, JsonValue::makeArray(std::move(values)));
	SerializeExample4 des = deserialize<SerializeExample4>(obj).ok();
	check_eq(des.numbers.len(), 1);
	check_eq(des.numbers[0], 10);
}

comptime_test_case(DeserializeWithArrayField, {
	JsonObject obj;
	ArrayList<JsonValue> values;
	values.push(JsonValue::makeNumber(10));
	obj.addField("numbers"_str, JsonValue::makeArray(std::move(values)));
	SerializeExample4 des = deserialize<SerializeExample4>(obj).ok();
	check_eq(des.numbers.len(), 1);
	check_eq(des.numbers[0], 10);
	});

test_case("Serialize and deserialize with array field") {
	SerializeExample4 e;
	for (int i = 0; i < 2; i++) {
		e.numbers.push(i + 10);
	}
	JsonObject obj = serialize(e);
	SerializeExample4 des = deserialize<SerializeExample4>(obj).ok();
	check_eq(des.numbers.len(), 2);
	check_eq(des.numbers[0], 10);
	check_eq(des.numbers[1], 11);
}

comptime_test_case(SerializeAndDeserializeWithArrayField, {
	SerializeExample4 e;
	for (int i = 0; i < 2; i++) {
		e.numbers.push(i + 10);
	}
	JsonObject obj = serialize(e);
	SerializeExample4 des = deserialize<SerializeExample4>(obj).ok();
	check_eq(des.numbers.len(), 2);
	check_eq(des.numbers[0], 10);
	check_eq(des.numbers[1], 11);
	});





test_case("Serialize nested") {
	SerializeExample5 e;
	e.nested.health = 195.5;
	e.nested.power = 10;
	JsonObject obj = serialize(e);
	check(obj.findField("nested"_str).isSome());
	JsonObject& subobj = obj.findField("nested"_str).some()->objectValue();
	check(subobj.findField("health"_str).isSome());
	check(subobj.findField("power"_str).isSome());
	check_eq(subobj.findField("health"_str).some()->numberValue(), 195.5);
	check_eq(subobj.findField("power"_str).some()->numberValue(), 10);
}

comptime_test_case(SerializeNested, {
	SerializeExample5 e;
	e.nested.health = 195.5;
	e.nested.power = 10;
	JsonObject obj = serialize(e);
	check(obj.findField("nested"_str).isSome());
	JsonObject& subobj = obj.findField("nested"_str).some()->objectValue();
	check(subobj.findField("health"_str).isSome());
	check(subobj.findField("power"_str).isSome());
	check_eq(subobj.findField("health"_str).some()->numberValue(), 195.5);
	check_eq(subobj.findField("power"_str).some()->numberValue(), 10);
	});

test_case("Deserialize nested") {
	JsonObject subobj;
	subobj.addField("health"_str, JsonValue::makeNumber(60.1));
	subobj.addField("power"_str, JsonValue::makeNumber(5));
	JsonObject obj;
	obj.addField("nested"_str, JsonValue::makeObject(std::move(subobj)));
	SerializeExample5 des = deserialize<SerializeExample5>(obj).ok();
	check_eq(des.nested.health, 60.1);
	check_eq(des.nested.power, 5);
}

comptime_test_case(DeserializeNested, {
	JsonObject subobj;
	subobj.addField("health"_str, JsonValue::makeNumber(60.1));
	subobj.addField("power"_str, JsonValue::makeNumber(5));
	JsonObject obj;
	obj.addField("nested"_str, JsonValue::makeObject(std::move(subobj)));
	SerializeExample5 des = deserialize<SerializeExample5>(obj).ok();
	check_eq(des.nested.health, 60.1);
	check_eq(des.nested.power, 5);
	});

test_case("Serialize and deserialize nested") {
	SerializeExample5 e;
	e.nested.health = -1.7;
	e.nested.power = 4;
	JsonObject obj = serialize(e);
	SerializeExample5 des = deserialize<SerializeExample5>(obj).ok();
	check_eq(des.nested.health, -1.7);
	check_eq(des.nested.power, 4);
}

comptime_test_case(SerializeAndDeserializeNested, {
	SerializeExample5 e;
	e.nested.health = -1.7;
	e.nested.power = 4;
	JsonObject obj = serialize(e);
	SerializeExample5 des = deserialize<SerializeExample5>(obj).ok();
	check_eq(des.nested.health, -1.7);
	check_eq(des.nested.power, 4);
});

#endif