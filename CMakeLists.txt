cmake_minimum_required(VERSION 3.4)

project(Kcp)

set(CMAKE_CXX_FLAGS "-std=c++11 -g")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${PROJECT_SOURCE_DIR}/include)


set(TEST OFF)

if(TEST)
add_executable(rtt_test test/rtt_test.cpp)
add_executable(base_test test/base_test.cpp)
add_executable(multithread_test test/multithread_test.cpp)
target_link_libraries(multithread_test pthread)
endif()

add_executable(ChatServer example/ChatServer.cpp)
target_link_libraries(ChatServer pthread)


add_executable(ChatClient example/ChatClient.cpp)
target_link_libraries(ChatClient pthread)
