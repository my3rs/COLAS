include_directories(${PROJECT_SOURCE_DIR}/src/baseprocess)
include_directories(${PROJECT_SOURCE_DIR}/src/utilities)

link_directories(${PROJECT_BINARY_DIR}/lib)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)



add_library(utilities SHARED algo_utils.c base64.c helpers.c md-primitive.c)
target_link_libraries(utilities czmq zmq)


install(TARGETS utilities
        LIBRARY DESTINATION /usr/local/lib)