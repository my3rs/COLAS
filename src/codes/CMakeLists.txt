# 输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/bin)

add_definitions(-D ASLIBRARY)

# 头文件
include_directories(${PROJECT_SOURCE_DIR}/deps/shared_kodocpp/include)

# 库文件
link_directories(${PROJECT_SOURCE_DIR}/deps/shared_kodocpp)
link_directories(${CMAKE_BINARY_DIR}/lib)

# 输出
add_library(reed SHARED reed_solomon.cpp)
target_link_libraries(reed kodoc)


#add_executable(rlnc_example rlnc_example.cpp)
#target_link_libraries(rlnc_example kodoc)

#add_executable(reed_solomon_example reed_solomon_example.cpp)
#target_link_libraries(reed_solomon_example kodoc)

add_library(rlnc_rs SHARED rlnc_rs.cpp)
target_link_libraries(rlnc_rs kodoc)

#add_executable(rlnc_rs_check rlnc_rs_check.c)
#target_link_libraries(rlnc_rs_check rlnc_rs)


install(TARGETS rlnc_rs
        LIBRARY DESTINATION /usr/local/lib)