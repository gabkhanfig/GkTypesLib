#include <GkTypes/String/String.h>
#include "ConstexprTestUnitTest.h"

#if RUN_TESTS == true
#define test(test, message) \
PRAGMA_MESSAGE([String Unit Test]: test); \
static_assert(test(), "[String Unit Test]: " #test "... " message)
#else
#define test(test, message)
#endif