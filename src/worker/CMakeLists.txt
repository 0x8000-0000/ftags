add_executable (ft_worker worker.cc)

target_include_directories (ft_worker PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (ft_worker PRIVATE project_options project_warnings)
target_link_libraries (ft_worker PRIVATE zmq)
target_link_libraries (ft_worker PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (ft_worker PRIVATE -lclang)
