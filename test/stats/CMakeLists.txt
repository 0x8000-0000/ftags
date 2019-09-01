add_executable (five_numbers_test five_numbers_test.cc)
target_link_libraries (five_numbers_test PRIVATE gtest_main)
target_link_libraries (five_numbers_test PUBLIC stats)

gtest_discover_tests (five_numbers_test)
