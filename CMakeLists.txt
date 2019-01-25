cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

project (ftags VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "Fast Tags")

set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set (CMAKE_CXX_EXTENSIONS False)

#
# Find Clang libraries
#
if (NOT LIBCLANG_LLVM_CONFIG_EXECUTABLE)
    message (FATAL_ERROR "Could NOT find llvm-config executable and LIBCLANG_INCLUDEDIR is not set")
endif ()

execute_process (COMMAND ${LIBCLANG_LLVM_CONFIG_EXECUTABLE} --includedir OUTPUT_VARIABLE LIBCLANG_INCLUDEDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${LIBCLANG_LLVM_CONFIG_EXECUTABLE} --libdir OUTPUT_VARIABLE LIBCLANG_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

option(StrictChecking "Enable Strict Checking" OFF)

#
# Link this 'library' to set the c++ standard / compile-time options requested
#
add_library (project_options INTERFACE)

target_compile_features (project_options INTERFACE cxx_std_17)

if (StrictChecking)
target_compile_options (project_options INTERFACE -DFTAGS_STRICT_CHECKING)
endif ()


#
# Link this 'library' to use the standard warnings
#
add_library (project_warnings INTERFACE)

target_compile_options (project_warnings INTERFACE -Wall -Wextra -Werror -pedantic -Wconversion -Wunused -Wshadow)

if (CMAKE_COMPILER_IS_GNUCXX)
   target_compile_options (project_warnings INTERFACE
      -Waggressive-loop-optimizations -Warray-bounds -Wbool-operation
      -Wcast-align -Wconversion -Wdelete-incomplete
      -Wdelete-non-virtual-dtor -Wduplicated-cond
      -Wempty-body -Wendif-labels -Wfloat-conversion -Wformat=1
      -Wformat-overflow=1 -Wformat-truncation -Wimplicit-fallthrough=3
      -Wint-in-bool-context -Wlogical-op -Wmisleading-indentation
      -Wnarrowing -Wno-deprecated -Wno-ignored-qualifiers
      -Wnonnull-compare -Wno-sign-compare -Wno-switch
      -Wno-unused-parameter -Wnull-dereference -Woverflow -Wparentheses
      -Wpointer-arith -Wpointer-compare -Wreturn-type -Wsequence-point
      -Wshadow -Wshift-count-overflow -Wshift-overflow=1
      -Wsign-conversion -Wsizeof-pointer-memaccess -Wstrict-overflow=1
      -Wstringop-overflow -Wswitch-unreachable -Wuninitialized -Wunused
      -Wunused-label -Wfloat-equal)
else ()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options (project_warnings INTERFACE -Weverything
      -Wno-c++98-compat -Wno-c++98-compat-pedantic
      -Wno-global-constructors -Wno-padded -Wno-unused-macros
      -Wno-double-promotion
      -Wno-documentation-unknown-command
      -Wno-switch-enum -Wno-weak-vtables -Wno-missing-noreturn
      -Wno-undefined-func-template
      )
endif ()
endif ()

enable_testing ()
find_package (GTest REQUIRED)
include (GoogleTest)

find_package (benchmark)


#
# Tools
#
add_executable (indexer src/indexer.cc)
target_include_directories (indexer PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (indexer PRIVATE project_options project_warnings)
target_link_libraries (indexer PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (indexer PRIVATE -lclang)

#
# Tests
#
add_executable (store_test test/store_test.cc)
target_include_directories (store_test PRIVATE
   ${GTEST_INCLUDE_DIRS} include)
target_link_libraries (store_test PRIVATE project_options project_warnings)
target_link_libraries (store_test PRIVATE ${GTEST_BOTH_LIBRARIES} pthread)

gtest_discover_tests (store_test)

add_executable (string_table_test test/string_table_test.cc
   src/string_table.cc)
target_include_directories (string_table_test PRIVATE
   ${GTEST_INCLUDE_DIRS} include)
target_link_libraries (string_table_test PRIVATE project_options project_warnings)
target_link_libraries (string_table_test PRIVATE ${GTEST_BOTH_LIBRARIES} pthread)

gtest_discover_tests (string_table_test)

add_executable (index_map_test test/index_map_test.cc
   src/index_map.cc)
target_include_directories (index_map_test PRIVATE
   ${GTEST_INCLUDE_DIRS} include)
target_link_libraries (index_map_test PRIVATE project_options project_warnings)
target_link_libraries (index_map_test PRIVATE ${GTEST_BOTH_LIBRARIES} pthread)

gtest_discover_tests (index_map_test)

add_executable (sandbox_test test/sandbox.cc src/index_map.cc)
target_include_directories (sandbox_test PRIVATE include)
target_link_libraries (sandbox_test PRIVATE project_options project_warnings)

#
# Benchmarks
#
add_executable (ftags_benchmark benchmark/index_map_benchmark.cc
   src/index_map.cc)
target_include_directories (ftags_benchmark PRIVATE include)
target_link_libraries(ftags_benchmark PRIVATE benchmark::benchmark_main)
