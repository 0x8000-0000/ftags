add_subdirectory (Clara)
add_subdirectory (burtle)

set (SPDLOG_FMT_EXTERNAL ON)
add_subdirectory (fmt)
set (fmt_DIR fmt)
add_subdirectory (spdlog)

add_subdirectory (PEGTL)

option (USE_SYSTEM_GOOGLETEST "Use system package for GoogleTest" OFF)

if (USE_SYSTEM_GOOGLETEST)
   find_package (GTest REQUIRED)
else ()
   add_subdirectory (googletest)
endif ()

