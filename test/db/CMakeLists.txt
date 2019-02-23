add_executable (store_test store_test.cc)
target_include_directories (store_test PRIVATE
   ${GTEST_INCLUDE_DIRS} include)
target_link_libraries (store_test PRIVATE project_options project_warnings)
target_link_libraries (store_test PRIVATE ${GTEST_BOTH_LIBRARIES}
   pthread db)

gtest_discover_tests (store_test)

add_executable (string_table_test string_table_test.cc)
target_include_directories (string_table_test PRIVATE
   ${GTEST_INCLUDE_DIRS} include)
target_link_libraries (string_table_test PRIVATE project_options project_warnings)
target_link_libraries (string_table_test PRIVATE ${GTEST_BOTH_LIBRARIES}
   pthread db)

gtest_discover_tests (string_table_test)
