cmake_minimum_required(VERSION 3.1)
project(fs-tool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall")
include_directories("include")

set(SOURCE_FILES
	Main.cpp
	Application.cpp
	FatFS.cpp
	Ext2FS.cpp
	Utils.cpp)

add_executable(fs ${SOURCE_FILES})
