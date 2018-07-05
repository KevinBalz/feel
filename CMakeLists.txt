cmake_minimum_required(VERSION 3.6)
project("feel")

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(libfeel)
add_subdirectory(libfeelc)

set(EXECUTABLE_NAME "hello-feel")
set(SOURCE_FILES "hello-feel/src/main.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
target_link_libraries(${EXECUTABLE_NAME} libfeel)