CMAKE_MINIMUM_REQUIRED(VERSION 2.6...3.20)
PROJECT(texopt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(SDL2 REQUIRED)
message(STATUS "SDL include dir: ${SDL2_INCLUDE_DIR}")
include_directories(${SDL2_INCLUDE_DIR})
include_directories(dep/imgui)
include_directories(dep)

add_subdirectory(dep)
add_subdirectory(src)
