add_library(libfeel INTERFACE)

target_sources(libfeel INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include/Finger.hpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/include/Feel.hpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/include/IncomingMessage.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/Device.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/SerialDevice.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/DeviceStatus.hpp")
target_include_directories(libfeel INTERFACE "${PROJECT_SOURCE_DIR}/dependencies/asio/asio/include")
target_include_directories(libfeel INTERFACE "include/")