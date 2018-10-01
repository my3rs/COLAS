if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(abd)
add_subdirectory(codes)
add_subdirectory(soda)
add_subdirectory(sodaw)
add_subdirectory(baseprocess)
add_subdirectory(utilities)

include_directories(${PROJECT_SOURCE_DIR}/src/utilities)
include_directories(${PROJECT_SOURCE_DIR}/src/baseprocess)
include_directories(${PROJECT_SOURCE_DIR}/src/codes)

link_directories(${CMAKE_BINARY_DIR}/lib)
link_directories(${PROJECT_SOURCE_DIR}/deps/shared_kodocpp)

add_executable(processc process.c)
target_link_libraries(processc baseprocess abd sodaw utilities )
#target_link_libraries(processc ${CMAKE_DL_LIBS})
