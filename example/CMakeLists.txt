cmake_minimum_required(VERSION 3.4)
set(libs pthread)

add_executable(ChatServer ChatRoom/ChatServer.cpp)
add_executable(ChatClient ChatRoom/ChatClient.cpp)

target_link_libraries(ChatServer ${libs})
target_link_libraries(ChatClient ${libs})