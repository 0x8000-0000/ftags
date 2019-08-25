add_executable (ft_client client.cc)

target_link_libraries (ft_client PRIVATE project_options project_warnings)
target_link_libraries (ft_client PRIVATE zmq ftags clara db query)

