add_library (stats INTERFACE)
target_include_directories (stats INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (stats INTERFACE project_options project_warnings)
