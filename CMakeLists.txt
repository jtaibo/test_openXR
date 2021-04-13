cmake_minimum_required(VERSION 3.19)

set( PROJECT_NAME "test_XR" )
project( ${PROJECT_NAME} )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

add_subdirectory("src")
