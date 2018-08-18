cmake_minimum_required(VERSION 3.6)
project("feel")

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(libfeel)
add_subdirectory(libfeelc)

set(EXECUTABLE_NAME "hello-feel")
set(SOURCE_FILES "hello-feel/src/main.cpp")
set(_WIN32_WINNT NTDDI_VISTA)

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
target_compile_options(${EXECUTABLE_NAME} PRIVATE -D_WIN32_WINNT=0x0600)
target_link_libraries(${EXECUTABLE_NAME} libfeel)