cmake_minimum_required(VERSION 3.8.0)

project(meta_erc_convert VERSION 1.0.0 LANGUAGES C CXX)


if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default is Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
