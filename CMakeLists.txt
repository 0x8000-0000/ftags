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

option(FTAGS_STRICT_CHECKING "Enable Strict Checking" OFF)
option(FTAGS_PROFILE "Enable Profiling" OFF)
option(FTAGS_COVERAGE "Enable Collecting Coverage" OFF)

#
# Link this 'library' to set the c++ standard / compile-time options requested
#
add_library (project_options INTERFACE)

target_compile_features (project_options INTERFACE cxx_std_17)

if (FTAGS_STRICT_CHECKING)
target_compile_options (project_options INTERFACE
   -DFTAGS_STRICT_CHECKING -DENABLE_THOROUGH_VALIDITY_CHECKS)
endif ()

if (FTAGS_PROFILE)
target_compile_options (project_options INTERFACE -pg)
target_link_options (project_options INTERFACE -pg)
endif ()

if (FTAGS_COVERAGE)
   target_compile_options (project_options INTERFACE --coverage)
   target_link_options (project_options INTERFACE --coverage)
   # After build and test run: gcov -p -f -m -w -j -a $(find . -name \*.gcno)
endif ()

if (SANITIZE_ADDRESS)
   target_compile_options (project_options INTERFACE -fsanitize=address)
   target_link_options (project_options INTERFACE -fsanitize=address)
endif ()

if (SANITIZE_UNDEFINED)
   target_compile_options (project_options INTERFACE -fsanitize=undefined)
   target_link_options (project_options INTERFACE -fsanitize=undefined)
endif ()

if (SANITIZE_THREAD)
   target_compile_options (project_options INTERFACE -fsanitize=thread)
   target_link_options (project_options INTERFACE -fsanitize=thread)
endif ()



#
# Link this 'library' to use the standard warnings
#
add_library (project_warnings INTERFACE)

target_compile_options (project_warnings INTERFACE -Wall -Wextra -Werror
   -Wunused
   # disabled by PEGTL -Wconversion
   # disabled by spdlog/fmt -pedantic -Wshadow
   )

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   target_compile_options (project_warnings INTERFACE
      -Waggressive-loop-optimizations -Warray-bounds -Wbool-operation
      -Wcast-align -Wdelete-incomplete
      -Wdelete-non-virtual-dtor -Wduplicated-cond
      -Wempty-body -Wendif-labels -Wfloat-conversion -Wformat=1
      -Wformat-overflow=1 -Wformat-truncation -Wimplicit-fallthrough=3
      -Wint-in-bool-context -Wlogical-op -Wmisleading-indentation
      -Wnarrowing -Wno-deprecated -Wno-ignored-qualifiers
      -Wnonnull-compare -Wno-sign-compare -Wno-switch
      -Wno-unused-parameter -Wnull-dereference -Woverflow -Wparentheses
      -Wpointer-arith -Wpointer-compare -Wreturn-type -Wsequence-point
      -Wshift-count-overflow -Wshift-overflow=1
      -Wsign-conversion -Wsizeof-pointer-memaccess -Wstrict-overflow=1
      -Wstringop-overflow -Wswitch-unreachable -Wuninitialized -Wunused
      -Wunused-label
      # disabled by fmt -Wfloat-equal
   )

else ()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options (project_warnings INTERFACE -Weverything
      -Wno-c++98-compat -Wno-c++98-compat-pedantic
      -Wno-global-constructors -Wno-padded -Wno-unused-macros
      -Wno-double-promotion
      -Wno-documentation-unknown-command
      -Wno-switch-enum -Wno-weak-vtables -Wno-missing-noreturn
      -Wno-undefined-func-template -Wno-disabled-macro-expansion
      -Wno-unused-member-function -Wno-exit-time-destructors
      -Wno-extra-semi -Wno-float-equal -Wno-shadow-field-in-constructor
      -Wno-newline-eof -Wno-signed-enum-bitfield
   )

   if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
      target_compile_options (project_warnings INTERFACE
         -Wno-c++2a-compat)
   endif()
endif ()
endif ()


find_package (Protobuf REQUIRED)

# fails on Debian, and not needed but left here for other platforms
#find_package (ZeroMQ REQUIRED)

add_subdirectory (external)

# This is used when importing GoogleTest from the environment
include (GoogleTest)
enable_testing ()

add_subdirectory (src)
add_subdirectory (test)

