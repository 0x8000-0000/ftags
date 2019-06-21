#
# GoogleTest tests
#

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

configure_file (data/functions/alpha-beta.cc data/functions/alpha-beta.cc)

#
# Stand-alone tests
#
add_executable (project_index_test project_index_test.cc)

target_include_directories (project_index_test PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (project_index_test PRIVATE project_options project_warnings)
target_link_libraries (project_index_test PRIVATE ftags db)
target_link_libraries (project_index_test PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (project_index_test PRIVATE -lclang)

add_executable (dump_records dump_records.cc)

target_include_directories (dump_records PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (dump_records PRIVATE project_options project_warnings)
target_link_libraries (dump_records PRIVATE ftags db)
target_link_libraries (dump_records PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (dump_records PRIVATE -lclang -lstdc++fs)
