add_library (db STATIC string_table.cc project.cc tags_builder.cc)
target_link_libraries (db PRIVATE project_options project_warnings)
target_include_directories (db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (db PRIVATE ${LIBCLANG_INCLUDEDIR})
