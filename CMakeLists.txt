cmake_minimum_required (VERSION 3.12)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("GkTypesLib")

set(CMAKE_CXX_STANDARD 20)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_library(GkTypesLib STATIC 
"gk_types_lib/lib.cpp" 
"gk_types_lib/allocator/allocator.cpp"
"gk_types_lib/array/array_list.cpp" 
"gk_types_lib/function/callback.cpp" 
"gk_types_lib/function/function_ptr.cpp" 
"gk_types_lib/cpu_features/cpu_feature_detector.cpp" 
"gk_types_lib/hash/hashmap.cpp" 
"gk_types_lib/option/option.cpp" 
"gk_types_lib/queue/ring_queue.cpp" 
"gk_types_lib/string/utf8.cpp"
"gk_types_lib/string/str.cpp" 
"gk_types_lib/sync/mutex.cpp" 
"gk_types_lib/sync/rw_lock.cpp" 
"gk_types_lib/string/string.cpp" 
"gk_types_lib/string/global_string.cpp" 
"gk_types_lib/job/job_thread.cpp"
"gk_types_lib/job/job_system.cpp" 
"gk_types_lib/error/result.cpp" 
"gk_types_lib/doctest/doctest_proxy.cpp" 
"gk_types_lib/utility.cpp" 
"gk_types_lib/string/string_type_conversion.cpp"
"gk_types_lib/json/json_object.cpp"
"gk_types_lib/json/serialization.cpp" 
"gk_types_lib/ptr/unique_ptr.cpp" "gk_types_lib/ptr/shared_ptr.cpp")

add_executable(GkTypesLibTest 
"gk_types_lib/test.cpp" 
"gk_types_lib/allocator/allocator.cpp"
"gk_types_lib/array/array_list.cpp" 
"gk_types_lib/function/callback.cpp" 
"gk_types_lib/function/function_ptr.cpp" 
"gk_types_lib/cpu_features/cpu_feature_detector.cpp" 
"gk_types_lib/hash/hashmap.cpp" 
"gk_types_lib/option/option.cpp" 
"gk_types_lib/queue/ring_queue.cpp" 
"gk_types_lib/string/utf8.cpp"
"gk_types_lib/string/str.cpp"
"gk_types_lib/sync/mutex.cpp" 
"gk_types_lib/sync/rw_lock.cpp" 
"gk_types_lib/string/string.cpp" 
"gk_types_lib/string/global_string.cpp" 
"gk_types_lib/job/job_thread.cpp"
"gk_types_lib/job/job_system.cpp" 
"gk_types_lib/error/result.cpp" 
"gk_types_lib/doctest/doctest_proxy.cpp" 
"gk_types_lib/utility.cpp" 
"gk_types_lib/string/string_type_conversion.cpp"
"gk_types_lib/json/json_object.cpp"
"gk_types_lib/json/serialization.cpp" 
"gk_types_lib/ptr/unique_ptr.cpp" "gk_types_lib/ptr/shared_ptr.cpp")

# https://github.com/doctest/doctest/blob/master/doc/markdown/faq.md#why-are-my-tests-in-a-static-library-not-getting-registered
#include(doctest_force_link_static_lib_in_target.cmake)
#doctest_force_link_static_lib_in_target(GkTypesLibTest GkTypesLib)

target_compile_definitions(GkTypesLib PUBLIC
	$<$<CONFIG:Debug>:GK_TYPES_LIB_DEBUG>
	$<$<CONFIG:RelWithDebInfo>:GK_TYPES_LIB_DEBUG>
	$<$<CONFIG:Release>:GK_TYPES_LIB_SHIPPING>
)

target_compile_definitions(GkTypesLibTest PUBLIC GK_TYPES_LIB_TEST)
target_compile_definitions(GkTypesLibTest PUBLIC GK_TYPES_LIB_DEBUG)
target_compile_definitions(GkTypesLibTest PUBLIC GK_TYPES_LIB_LOCAL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/") 

target_include_directories(GkTypesLib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# target_link_libraries(GkTypesLibTest GkTypesLib)
