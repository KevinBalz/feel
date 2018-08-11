add_library(libfeel INTERFACE)

target_sources(libfeel INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/Finger.hpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/Feel.hpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/IncomingMessage.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/Device.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/SerialDevice.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/DeviceStatus.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/FeelStatus.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/CalibrationData.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel/SimulatorDevice.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/feel.hpp")
target_include_directories(libfeel INTERFACE "${PROJECT_SOURCE_DIR}/dependencies/asio/asio/include")
target_include_directories(libfeel INTERFACE "include/")