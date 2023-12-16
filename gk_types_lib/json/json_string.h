#pragma once

#include "../error/result.h"
#include "../string/string.h"
#include "../array/array_list.h"
#include "../hash/hashmap.h"
#include "../string/string_type_conversion.h"

// https://www.json.org/json-en.html

namespace gk
{
	//struct JsonValue;

	enum class JsonValueType : usize {
		Null,
		Bool,
		Number,
		String,
		Array,
		Object
	};

	
	
}



