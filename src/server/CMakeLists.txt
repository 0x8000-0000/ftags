add_executable (ft_server server.cc)

target_link_libraries (ft_server PRIVATE project_options project_warnings)
target_link_libraries (ft_server PRIVATE zmq ftags db-parse clara)
target_link_libraries (ft_server PRIVATE -lstdc++fs)
