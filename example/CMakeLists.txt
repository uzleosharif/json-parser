







add_executable(example)
target_sources(example
  PRIVATE main.cpp
)
target_link_libraries(example
  PRIVATE
    json
    fmt::fmt
)
target_compile_features(example
  PRIVATE
    cxx_std_26
)

add_executable(wsc)
target_sources(wsc
  PRIVATE
    weather-station-client/main.cpp
)
target_link_libraries(wsc
  PRIVATE
    json
    fmt::fmt
)
target_compile_features(wsc
  PRIVATE
    cxx_std_26
)
