







add_executable(example)
target_sources(example
  PRIVATE main.cpp
)
target_link_libraries(example
  PRIVATE
    json
    fmt::fmt
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
