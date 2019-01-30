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
option(Profile "Enable Profiling" OFF)

#
# Link this 'library' to set the c++ standard / compile-time options requested
#
add_library (project_options INTERFACE)

target_compile_features (project_options INTERFACE cxx_std_17)

if (StrictChecking)
target_compile_options (project_options INTERFACE -DFTAGS_STRICT_CHECKING)
endif ()

if (Profile)
target_compile_options (project_options INTERFACE -pg)
target_link_options (project_options INTERFACE -pg)
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
find_package (Protobuf REQUIRED)
find_package (spdlog)

# fails on Debian, and not needed but left here for other platforms
#find_package (ZeroMQ REQUIRED)

add_subdirectory (src)

