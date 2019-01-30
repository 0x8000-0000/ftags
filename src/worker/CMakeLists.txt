add_executable (ft_worker worker.cc)

target_include_directories (ft_worker PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (ft_worker PRIVATE project_options project_warnings)
target_link_libraries (ft_worker PRIVATE zmq ftags)
target_link_libraries (ft_worker PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (ft_worker PRIVATE -lclang)

add_executable (ft_indexer indexer.cc)

target_include_directories (ft_indexer PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (ft_indexer PRIVATE project_options project_warnings)
target_link_libraries (ft_indexer PRIVATE zmq ftags)
target_link_libraries (ft_indexer PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (ft_indexer PRIVATE -lclang)
