cmake_minimum_required(VERSION 3.28)

project(variantx LANGUAGES C CXX)

include(cmake/Output.cmake)
include(cmake/CompileOptions.cmake)

include_directories(headers)

add_subdirectory(tests)
add_subdirectory(third-party)
