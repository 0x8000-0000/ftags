add_subdirectory (Clara)
add_subdirectory (burtle)
add_subdirectory (fmt)
add_subdirectory (spdlog)
add_subdirectory (PEGTL)

option (USE_SYSTEM_GOOGLETEST "Use system package for GoogleTest" OFF)

if (USE_SYSTEM_GOOGLETEST)
   find_package (GTest REQUIRED)
else ()
   add_subdirectory (googletest)
endif ()

