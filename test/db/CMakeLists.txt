add_executable (store_test store_test.cc)
target_link_libraries (store_test PRIVATE project_options project_warnings)
target_link_libraries (store_test PRIVATE ${GTEST_BOTH_LIBRARIES}
   pthread db)

gtest_discover_tests (store_test)

add_executable (string_table_test string_table_test.cc)
target_link_libraries (string_table_test PRIVATE project_options project_warnings)
target_link_libraries (string_table_test PRIVATE ${GTEST_BOTH_LIBRARIES}
   pthread db)

gtest_discover_tests (string_table_test)

add_executable (tags_index_test tags_index_test.cc)
target_link_libraries (tags_index_test PRIVATE project_options project_warnings)
target_link_libraries (tags_index_test PRIVATE ${GTEST_BOTH_LIBRARIES}
   pthread db stdc++fs)

gtest_discover_tests (tags_index_test
   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

configure_file (data/hello/hello.cc data/hello/hello.cc)
