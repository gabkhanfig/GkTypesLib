import sys

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

""" nth ptr
else if constexpr (has_n_fields<T, 2>) {
	static_assert(N < 2, "Cannot access a field beyond the number within the field. T has 2 fields");
	auto&& [p1, p2] = t;
	if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
	if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
}
"""

""" to named fields
else if constexpr (internal::has_n_fields<T, 2>) {
	auto&& [p1, p2] = t;
	return std::tuple(
		NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
		NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2});
}
"""

def generateNthPtr(n: int) -> str:
    if(n == 1):
        return """if constexpr (has_n_fields<T, 1>) {
    static_assert(N < 1, "Cannot access a field beyond the number within the field. T has 1 field");
    auto&& [p1] = t;
    if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
}"""
    
    accumulate = "else if constexpr (has_n_fields<T, " + str(n) 
    accumulate += ">) {\n   static_assert(N < " + str(n)
    accumulate += ", \"Cannot access a field beyond the number within the field. T has " + str(n) + " fields\");"

    accumulate += "\n   auto&& ["
    for i in range(n):
        accumulate += "p" + str(i + 1)
        if((i + 1) != n):
            accumulate += ", "
    accumulate += "] = t;"

    for i in range(n):
        accumulate += "\n   if constexpr (N == " + str(i) + ") return ptr<decltype(p" + str(i + 1) + ")>{&p" + str(i + 1) + "};"

    accumulate += "\n}"
    return accumulate

def generateFieldNames(n: int) -> str:
    if(n == 1):
        return """if constexpr (internal::has_n_fields<T, 1>) {
	auto&& [p1] = t;
	return std::tuple(
		NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1});
}"""

    accumulate = "else if constexpr (internal::has_n_fields<T, " + str(n)
    accumulate += ">) {"

    accumulate += "\n   auto&& ["
    for i in range(n):
        accumulate += "p" + str(i + 1)
        if((i + 1) != n):
            accumulate += ", "
    accumulate += "] = t;"

    accumulate += "\n   return std::tuple("
    for i in range(n):
        accumulate += "\n       NamedField<decltype(p" + str(i + 1) + ")>{.name = getFieldName<T, " + str(i) + ">(), .value = p" + str(i + 1) + "}"
        if((i + 1) != n):
            accumulate += ","
        else:
            accumulate += ");"

    accumulate += "\n}"
    return accumulate

def deserialize(n: int) -> str:
    if(n == 1):
        return """if constexpr (fieldCount == 1) {
	auto&& [p1] = out; 
	if (internal::tryAssignFieldFromJsonObject(p1, getFieldName<T, 0>(), jsonObject) == false) return ResultErr();
}"""

    accumulate = "else if constexpr (fieldCount == " + str(n) + ") {"

    accumulate += "\n   auto&& ["
    for i in range(n):
        accumulate += "p" + str(i + 1)
        if((i + 1) != n):
            accumulate += ", "
    accumulate += "] = out;"

    for i in range(n):
        #accumulate += "\n   if constexpr (N == " + str(i) + ") return ptr<decltype(p" + str(i + 1) + ")>{&p" + str(i + 1) + "};"
        accumulate += "\n   if (internal::tryAssignFieldFromJsonObject(p" + str(i + 1) + "getFieldName<T, " + str(i) + ">(), jsonObject) == false) return ResultErr();"

    accumulate += "\n}"
    return accumulate

if(len(sys.argv) != 2):
    print("Usage:\n   python generate_nth_ptr.py [fieldCount]")
    exit()

fieldCount = int(sys.argv[1])
if(fieldCount < 1):
    print(bcolors.FAIL + "fieldCount cannot be less than 1" + bcolors.ENDC)
    exit()

print(bcolors.OKCYAN + "==== nthPtr ====\n" + bcolors.ENDC)
print(generateNthPtr(fieldCount))
print(bcolors.OKCYAN + "\n==== fieldNames ====\n" + bcolors.ENDC)
print(generateFieldNames(fieldCount))
print(bcolors.OKCYAN + "\n==== deserialize ====\n" + bcolors.ENDC)
print(deserialize(fieldCount))