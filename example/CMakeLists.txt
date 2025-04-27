







add_executable(example)
target_sources(example
  PRIVATE main.cpp
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

