# Gk Types Library
## A header only library of C++ data types

**Unit Testing** is implemented to ensure accurate behavior.

<h3>Currently added types:</h3>
- [Dynamic Array](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/Array/DynamicArray.h)
- [Class Reference Dependency Injection](https://github.com/gabkhanfig/GkTypesLib/tree/master/GkTypesLib/GkTypes/ClassRef)
- [String](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/String/String.h)
- [Global String](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/String/GlobalString.h)
- [Bitset](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/Bitset/Bitset.h)

<h2>[Dynamic Array](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/Array/DynamicArray.h)</h2>

A **constexpr** valid replacement to std::vector using a smaller footprint of only 16 bytes. Supports elements by casting to allow fast conversions between different dynamic array T types. This type is also [**unit tested**](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLibTests/Source/ConstexprTests/DynamicArrayTests.cpp).

<h2>[Class Reference Dependency Injection](https://github.com/gabkhanfig/GkTypesLib/tree/master/GkTypesLib/GkTypes/ClassRef)</h2>

An object that can instantiate other objects at runtime, while also not requiring the template itself to be stored. This allows convinient storage and usage.

<h2>[String](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/String/String.h)</h2>

A **constexpr** valid replacement to std::string. This string implemention is using both the [**Small String Optimization**](https://blogs.msmvps.com/gdicanio/2016/11/17/the-small-string-optimization/), along with const data segment optimizations. By checking for const char*'s that are held within the [application's data segment](https://en.wikipedia.org/wiki/Data_segment), unnecessary copies can be avoided, and string equality can be done substantially faster doing pointer equality, along with normal character checking if necessary. This string also has a dedicated hash function.

<h2>[Global String](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/String/GlobalString.h)</h2>

A global mapped string class, which only internally stores the mapped string pointer in each instance. This allows for a smaller memory footprint and fast comparisons for strings that are used a lot. It uses the previously mentioned string type as well, which brings its optimizations to the table. This is an implementation of Unreal Engine's [FName](https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/StringHandling/FName/).

<h2>[Bitset](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLib/GkTypes/Bitset/Bitset.h)</h2>
A **constexpr** valid replacement to std::bitset that has a smaller memory footprint for small bitsets. Rather than a default smallest size of 4 bytes, this bitset has a smallest default of 1 byte, and then scaling up to 2, 4, and 8 as necessary depending on template parameters. This bitset is also [**unit tested**](https://github.com/gabkhanfig/GkTypesLib/blob/master/GkTypesLibTests/Source/ConstexprTests/BitsetTests.cpp).