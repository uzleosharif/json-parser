






add_library(json)
add_library(uzleo::json ALIAS json)
target_sources(json
  PUBLIC
    FILE_SET modules
      TYPE CXX_MODULES
      FILES json.cppm
)
target_compile_features(json
  PRIVATE
    cxx_std_26
)
target_link_libraries(json
  PUBLIC
    fmt
)
target_compile_options(json
  PRIVATE
    "-Wall"
    "-Wextra"
    "-Wshadow"
    "-Wpedantic"
    "-Werror"
)
