add_library (db STATIC string_table.cc string_table_io.cc
   serialization.cc attributes.cc
   project.cc record.cc translation_unit.cc project_debug.cc cursor_set.cc)
target_link_libraries (db PRIVATE project_options project_warnings)
target_include_directories (db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (db PUBLIC -lstdc++fs)
target_link_libraries (db PUBLIC ftags zmq)
target_link_libraries (db PUBLIC spookyhash)

add_library (db-parse STATIC tags_builder.cc project_index.cc)
target_link_libraries (db-parse PRIVATE project_options project_warnings)
target_link_directories (db-parse PUBLIC ${LIBCLANG_LIBDIR})
target_include_directories (db-parse PUBLIC ${LIBCLANG_INCLUDEDIR})
target_link_libraries (db-parse PUBLIC db)
target_link_libraries (db-parse PUBLIC clang)
