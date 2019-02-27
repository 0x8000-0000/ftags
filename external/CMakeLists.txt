add_subdirectory (Clara)

add_library (plf_colony INTERFACE)
target_include_directories (plf_colony INTERFACE
   ${CMAKE_CURRENT_SOURCE_DIR}/plf_colony)
