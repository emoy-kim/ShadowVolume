cmake_minimum_required(VERSION 3.8)

project("ShadowVolume")

include(cmake/check-compiler.cmake)

set(CMAKE_CXX_STANDARD 17)

set(
	SOURCE_FILES 
		main.cpp
		source/text.cpp
		source/light.cpp
		source/camera.cpp
		source/object.cpp
		source/shader.cpp
		source/renderer.cpp
)

configure_file(include/project_constants.h.in ${PROJECT_BINARY_DIR}/project_constants.h @ONLY)

include_directories("include")
include(cmake/add-libraries-linux.cmake)

add_executable(ShadowVolume ${SOURCE_FILES})

include(cmake/target-link-libraries-linux.cmake)

target_include_directories(ShadowVolume PUBLIC ${CMAKE_BINARY_DIR})