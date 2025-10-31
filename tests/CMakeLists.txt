cmake_minimum_required(VERSION 3.28)

project(unit-tests LANGUAGES C CXX)

find_package(GTest REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/UnitTestsOptions.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Tests.cmake)

enable_testing()

add_subdirectory(utilities)
add_subdirectory(variantx)
