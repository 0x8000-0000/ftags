add_executable (store_test store_test.cc)
target_link_libraries (store_test PRIVATE project_options project_warnings)
target_link_libraries (store_test PRIVATE gtest_main pthread)
target_link_libraries (store_test PRIVATE util)

gtest_discover_tests (store_test)

add_executable (serialization_test serialization_test.cc)
target_link_libraries (serialization_test PRIVATE project_options project_warnings)
target_link_libraries (serialization_test PRIVATE gtest_main)
target_link_libraries (serialization_test PRIVATE util)

gtest_discover_tests (serialization_test)

add_executable (string_table_test string_table_test.cc)
target_link_libraries (string_table_test PRIVATE project_options project_warnings)
target_link_libraries (string_table_test PRIVATE gtest_main)
target_link_libraries (string_table_test PRIVATE util)

gtest_discover_tests (string_table_test)

add_executable (long_string_table_test long_string_table_test.cc)
target_link_libraries (long_string_table_test PRIVATE project_options project_warnings)
target_link_libraries (long_string_table_test PRIVATE gtest_main)
target_link_libraries (long_string_table_test PRIVATE util)

# don't run the long tests by default
# gtest_discover_tests (long_string_table_test)

add_executable (flat_map_test flat_map_test.cc)
target_link_libraries (flat_map_test PRIVATE project_options project_warnings)
target_link_libraries (flat_map_test PRIVATE gtest_main)
target_link_libraries (flat_map_test PRIVATE util)

gtest_discover_tests (flat_map_test)

add_executable (file_name_table_test file_name_table_test.cc)
target_link_libraries (file_name_table_test PRIVATE project_options project_warnings)
target_link_libraries (file_name_table_test PRIVATE gtest_main)
target_link_libraries (file_name_table_test PRIVATE util)

gtest_discover_tests (file_name_table_test)
