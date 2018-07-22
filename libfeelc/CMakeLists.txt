add_library(libfeelc SHARED
	"src/libfeel.cpp")
target_compile_options(libfeelc PRIVATE -D_WIN32_WINNT=0x0600)
target_link_libraries(libfeelc libfeel)