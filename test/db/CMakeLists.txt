#
# GoogleTest tests
#

add_executable (store_test store_test.cc)
target_link_libraries (store_test PRIVATE project_options project_warnings)
target_link_libraries (store_test PRIVATE gtest_main pthread db)

gtest_discover_tests (store_test)

add_executable (serialization_test serialization_test.cc)
target_link_libraries (serialization_test PRIVATE project_options project_warnings)
target_link_libraries (serialization_test PRIVATE gtest_main pthread db)

gtest_discover_tests (serialization_test)

add_executable (string_table_test string_table_test.cc)
target_link_libraries (string_table_test PRIVATE project_options project_warnings)
target_link_libraries (string_table_test PRIVATE gtest_main pthread db)

gtest_discover_tests (string_table_test)

add_executable (flat_map_test flat_map_test.cc)
target_link_libraries (flat_map_test PRIVATE project_options project_warnings)
target_link_libraries (flat_map_test PRIVATE gtest_main pthread db)

gtest_discover_tests (flat_map_test)

add_executable (project_serialization_test project_serialization_test.cc)
target_link_libraries (project_serialization_test PRIVATE project_options project_warnings)
target_link_libraries (project_serialization_test PRIVATE gtest_main pthread db-parse stdc++fs)

gtest_discover_tests (project_serialization_test
   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable (tags_index_test tags_index_test.cc)
target_link_libraries (tags_index_test PRIVATE project_options project_warnings)
target_link_libraries (tags_index_test PRIVATE gtest_main pthread db-parse stdc++fs)

gtest_discover_tests (tags_index_test
   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

configure_file (data/hello/hello.cc data/hello/hello.cc)

configure_file (data/functions/alpha-beta.cc data/functions/alpha-beta.cc)

configure_file (data/multi-module/lib.cc data/multi-module/lib.cc)
configure_file (data/multi-module/lib.h data/multi-module/lib.h)
configure_file (data/multi-module/test.cc data/multi-module/test.cc)

#
# Stand-alone tests
#
add_executable (dump_records dump_records.cc)

target_link_libraries (dump_records PRIVATE project_options project_warnings)
target_link_libraries (dump_records PRIVATE ftags db-parse clara)
