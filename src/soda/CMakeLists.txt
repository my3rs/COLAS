include_directories(${PROJECT_SOURCE_DIR}/src/utilities)
include_directories(${PROJECT_SOURCE_DIR}/src/baseprocess)
include_directories(${PROJECT_SOURCE_DIR}/src/codes)
include_directories(${PROJECT_SOURCE_DIR}/src/ZMQ/include)
include_directories(${PROJECT_SOURCE_DIR}/deps/shared_kodocpp/include)
link_directories(${PROJECT_SOURCE_DIR}/deps/shared_kodocpp)

link_directories(${CMAKE_BINARY_DIR}/lib)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


add_library(soda SHARED soda_reader.c)
target_link_libraries(soda zmq czmq rlnc_rs)
