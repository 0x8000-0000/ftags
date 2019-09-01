add_library (util STATIC string_table.cc string_table_io.cc
   serialization.cc file_name_table.cc)
   
target_link_libraries (util PRIVATE project_options project_warnings)
target_include_directories (util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (util PUBLIC -lstdc++fs)
target_link_libraries (util PUBLIC stats)
target_link_libraries (util PUBLIC fmt spookyhash)
