add_subdirectory (Clara)
add_subdirectory (burtle)

option (USE_SYSTEM_GOOGLETEST "Use system package for GoogleTest" OFF)

if (USE_SYSTEM_GOOGLETEST)
   find_package (GTest REQUIRED)
else ()
   add_subdirectory (googletest)
endif ()

option (USE_SYSTEM_SPDLOG "Use system package for spdlog" OFF)

if (USE_SYSTEM_SPDLOG)
   find_package (spdlog)
else ()
   add_subdirectory (spdlog)
endif ()
