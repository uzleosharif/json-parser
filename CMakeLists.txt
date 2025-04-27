

cmake_minimum_required(VERSION 4.0.0 FATAL_ERROR)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")

project(uzleo.json
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TEST OFF)
option(BUILD_EXAMPLE OFF)

set(FMT_MODULE ON)
set(FMT_INSTALL OFF)
add_subdirectory(deps/fmt/)
set_target_properties(fmt
  PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

add_subdirectory(src/)

if(BUILD_TEST)
  add_subdirectory(test/)
endif()

if(BUILD_EXAMPLE)
  add_subdirectory(example/)
endif()
