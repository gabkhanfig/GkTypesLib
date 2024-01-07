#include "string_type_conversion.h"

#if GK_TYPES_LIB_TEST

using gk::String;
using gk::Str;
using gk::GlobalString;
using gk::toString;
using gk::parseStr;
using gk::parseString;
using gk::ArrayList;

test_case("toString bool true") {
	String a = toString(true);
	check_eq(a, "true"_str);
}

test_case("toString bool false") {
	String a = toString(false);
	check_eq(a, "false"_str);
}

test_case("toString from signed int zero") {
	String a = toString(0);
	check_eq(a, '0');
}

test_case("toString from unsigned int zero") {
	String a = toString(0U);
	check_eq(a, '0');
}

test_case("toString from signed int small value") {
	String a = toString(35);
	check_eq(a, "35"_str);
}

test_case("toString from unsigned int small value") {
	String a = toString(35U);
	check_eq(a, "35"_str);
}

test_case("toString from signed int small negative value") {
	String a = toString(-35);
	check_eq(a, "-35"_str);
}

test_case("toString from signed int max value") {
	String a = toString(std::numeric_limits<gk::i64>::max());
	check_eq(a, "9223372036854775807"_str);
}

test_case("toString from unsigned int max value") {
	String a = toString(std::numeric_limits<gk::i64>::min());
	check_eq(a, "-9223372036854775808"_str);
}

test_case("toString from signed int max value") {
	String a = toString(std::numeric_limits<gk::u64>::max());
	check_eq(a, "18446744073709551615"_str);
}

test_case("toString from float zero") {
	String a = toString(0.0);
	check_eq(a, "0.0"_str);
}

test_case("toString from float one decimal place") {
	String a = toString(0.5);
	check_eq(a, "0.5"_str);
}

test_case("toString from float one decimal place sanity") {
	String a = toString(65.5);
	check_eq(a, "65.5"_str);
}

test_case("toString from float negative one decimal place") {
	String a = toString(-0.5);
	check_eq(a, "-0.5"_str);
}

test_case("toString from float negative one decimal place sanity") {
	String a = toString(-65.5);
	check_eq(a, "-65.5"_str);
}

test_case("toString from float many decimals") {
	String a = toString(0.1234);
	check_eq(a, "0.1234"_str);
}

test_case("toString from float many decimals sanity") {
	String a = toString(65.1234);
	check_eq(a, "65.1234"_str);
}

test_case("toString from float negative many decimals") {
	String a = toString(-0.1234);
	check_eq(a, "-0.1234"_str);
}

test_case("toString from float negative many decimals sanity") {
	String a = toString(-65.1234);
	check_eq(a, "-65.1234"_str);
}

test_case("toString from other string") {
	String a = "hello to this absolutely joyous world"_str;
	String b = toString(a);
	check_eq(b, "hello to this absolutely joyous world"_str);
}

test_case("toString from string slice") {
	String a = toString("hello to this absolutely joyous world"_str);
	check_eq(a, "hello to this absolutely joyous world"_str);
}

test_case("toString from global string") {
	GlobalString g = GlobalString::create("hello to this absolutely joyous world"_str);
	String a = toString(g);
	check_eq(a, "hello to this absolutely joyous world"_str);
}

test_case("toString from array list of int one value") {
	ArrayList<int> a = ArrayList<int>::initList(gk::globalHeapAllocator(), { 500 });
	String s = toString(a);
	check_eq(s, "[500]"_str);
}

test_case("toString from array list of int two values") {
	ArrayList<int> a = ArrayList<int>::initList(gk::globalHeapAllocator(), { -20, 35 });
	String s = toString(a);
	check_eq(s, "[-20, 35]"_str);
}

test_case("toString from array list of int many values") {
	ArrayList<int> a = ArrayList<int>::initList(gk::globalHeapAllocator(), { -20, 35, 1234, -6, 0, 14 });
	String s = toString(a);
	check_eq(s, "[-20, 35, 1234, -6, 0, 14]"_str);
}

test_case("toString from array list of string one value") {
	ArrayList<String> a = ArrayList<String>::initList(gk::globalHeapAllocator(), { "hello world!"_str});
	String s = toString(a);
	check_eq(s, "[\"hello world!\"]"_str);
}

test_case("toString from array list of string two values") {
	ArrayList<String> a = ArrayList<String>::initList(gk::globalHeapAllocator(), { "hello world!"_str, "woa."_str});
	String s = toString(a);
	check_eq(s, "[\"hello world!\", \"woa.\"]"_str);
}

test_case("toString from array list of string many values") {
	ArrayList<String> a = ArrayList<String>::initList(gk::globalHeapAllocator(), { "hello world!"_str, "woa."_str, 'c', 'b', "lmao"_str});
	String s = toString(a);
	check_eq(s, "[\"hello world!\", \"woa.\", \"c\", \"b\", \"lmao\"]"_str);
}

#endif