

cmake_minimum_required(VERSION 4.0.0 FATAL_ERROR)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")

project(uzleo.json
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt CONFIG REQUIRED)
set_target_properties(fmt::fmt
  PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

add_library(json)
target_sources(json
  PUBLIC
    FILE_SET modules
      TYPE CXX_MODULES
      FILES src/json.cppm
)
target_compile_features(json
  PUBLIC
    cxx_std_26
)
target_link_libraries(json
  PRIVATE
    fmt::fmt
)
target_compile_options(json
  PRIVATE
    "-Wall"
    "-Wextra"
    "-Wshadow"
    "-Wpedantic"
    "-Werror"
)


add_executable(example)
target_sources(example
  PRIVATE example/main.cpp
)
target_link_libraries(example
  PRIVATE
    json
    fmt::fmt
)
target_compile_options(example
  PRIVATE
    "-fsanitize=address,leak"
)
target_link_options(example
  PRIVATE
    "-fsanitize=address,leak"
)

add_executable(test)
target_sources(test
  PRIVATE test/test_json.cpp
)
target_link_libraries(test
  PRIVATE
    json
    fmt::fmt
)

