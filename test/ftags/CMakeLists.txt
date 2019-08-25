add_executable (query_test query_test.cc)
target_link_libraries (query_test PRIVATE gtest_main)
target_link_libraries (query_test PUBLIC query)

gtest_discover_tests (query_test)
