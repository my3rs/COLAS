include_directories(${PROJECT_SOURCE_DIR}/src/utilities)
link_directories(${CMAKE_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_definitions(-D ASLIBRARY)
add_library(baseprocess SHARED client.c server.c)
target_link_libraries(baseprocess utilities)

install(TARGETS baseprocess
        LIBRARY DESTINATION /usr/local/lib)