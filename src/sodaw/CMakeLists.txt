include_directories(${PROJECT_SOURCE_DIR}/src/utilities)
include_directories(${PROJECT_SOURCE_DIR}/src/codes)
include_directories(${PROJECT_SOURCE_DIR}/src/ZMQ/include)

link_directories(${PROJECT_BINARY_DIR}/lib)
link_directories(${PROJECT_SOURCE_DIR}/deps/shared_kodocpp)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_definitions(-D ASLIBRARY)
add_library(sodaw SHARED sodaw_client.c sodaw_reader.c sodaw_server.c sodaw_writer.c)
target_link_libraries(sodaw zmq czmq rlnc_rs utilities zlog)

install(TARGETS sodaw
        LIBRARY DESTINATION /usr/local/lib)