
cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")

project(little-man-computer)

add_executable(${PROJECT_NAME}
	little-man-computer.cpp
)

