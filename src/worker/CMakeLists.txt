add_executable (ft_scanner scanner.cc)

target_include_directories (ft_scanner PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (ft_scanner PRIVATE project_options project_warnings)
target_link_libraries (ft_scanner PRIVATE zmq ftags clara)
target_link_libraries (ft_scanner PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (ft_scanner PRIVATE -lclang -lstdc++fs)

add_executable (ft_indexer indexer.cc)

target_include_directories (ft_indexer PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (ft_indexer PRIVATE project_options project_warnings)
target_link_libraries (ft_indexer PRIVATE zmq ftags db-parse)
target_link_libraries (ft_indexer PRIVATE -L${LIBCLANG_LIBDIR})
target_link_libraries (ft_indexer PRIVATE -lclang)

add_executable (ft_logger logger.cc)

target_include_directories (ft_logger PRIVATE ${LIBCLANG_INCLUDEDIR})
target_link_libraries (ft_logger PRIVATE project_options project_warnings)
target_link_libraries (ft_logger PRIVATE zmq ftags)
target_link_libraries (ft_logger PRIVATE -L${LIBCLANG_LIBDIR})
