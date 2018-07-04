cmake_minimum_required(VERSION 3.6)
project("feel")

set(CMAKE_CXX_STANDARD 14)

include_directories("dependencies/asio/asio/include")
include_directories("include")
set(EXECUTABLE_NAME "hello-feel")
set(SOURCE_FILES 
	"src/main.cpp"
	"include/Finger.hpp" 
	"include/Feel.hpp" 
	"include/IncomingMessage.hpp"
	"include/Device.hpp"
	"include/SerialDevice.hpp")
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})