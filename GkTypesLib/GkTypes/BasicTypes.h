#pragma once

/* Signed 8 bit integer. Internally uses character so when printing or doing anything that could involve strings, ensure converting to another integer type. */
typedef char int8;

/* Signed 16 bit integer. */
typedef short int16;

/* Signed 32 bit integer. Same as int, but is here for consistency. */
typedef int int32;

/* Signed 64 bit integer. */
typedef long long int64;

/* Unsigned 8 bit integer. Internally uses character so when printing or doing anything that could involve strings, ensure converting to another integer type. */
typedef unsigned char uint8;

/* Unsigned 16 bit integer. */
typedef unsigned short uint16;

/* Unsigned 32 bit integer. */
typedef unsigned int uint32;

/* Unsigned 64 bit integer. */
typedef unsigned long long uint64;

#ifdef _MSC_VER
/* Aligns data types on windows. */
#define ALIGN_AS(alignment) __declspec(align(alignment))
#else
/* Aligns data types on linux / mac. */
#define ALIGN_AS alignas(alignment)
#endif