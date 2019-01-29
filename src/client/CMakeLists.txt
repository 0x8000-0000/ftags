add_executable (ft_client client.cc ${PROTO_SRCS})

target_link_libraries (ft_client PRIVATE project_options project_warnings)
target_link_libraries (ft_client PRIVATE zmq ftags)
