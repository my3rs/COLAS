include_directories(${PROJECT_SOURCE_DIR}/src/baseprocess)
include_directories(${PROJECT_SOURCE_DIR}/src/utilities)


link_directories(${CMAKE_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_definitions(-D ASLIBRARY)
add_library(abd SHARED abd_client.c abd_reader.c abd_server.c abd_writer.c)
target_link_libraries(abd zmq czmq baseprocess pthread /usr/local/lib/libzlog.a)


install(TARGETS abd
        LIBRARY DESTINATION /usr/local/lib)