protobuf_generate_cpp (PROTO_SRCS PROTO_HDRS ftags.proto)

add_library (ftags STATIC ${PROTO_SRCS} zmq_logger_sink.cc)
target_link_libraries (ftags PRIVATE project_options project_warnings)
target_include_directories (ftags PUBLIC ${CMAKE_BINARY_DIR}/src/ftags)
target_include_directories (ftags PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (ftags PUBLIC ${Protobuf_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options (ftags PUBLIC
      -Wno-zero-as-null-pointer-constant
      -Wno-missing-prototypes
      -Wno-missing-variable-declarations)
endif ()

