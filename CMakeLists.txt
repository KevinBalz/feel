cmake_minimum_required(VERSION 3.6)
project("feel")

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_NAME "hello-feel")
set(SOURCE_FILES "src/main.cpp")
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})