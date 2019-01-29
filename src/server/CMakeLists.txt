add_executable (ft_server server.cc ${PROTO_SRCS})

target_link_libraries (ft_server PRIVATE project_options project_warnings)
target_link_libraries (ft_server PRIVATE zmq ${Protobuf_LIBRARIES})
