


add_executable(test)
target_sources(test
  PRIVATE test_json.cpp
)
target_link_libraries(test
  PRIVATE
    json
    fmt::fmt
)
target_compile_options(test
  PRIVATE
    "-fsanitize=address,leak"
)
target_link_options(test
  PRIVATE
    "-fsanitize=address,leak"
)
target_compile_features(test
  PRIVATE
    cxx_std_26
)

