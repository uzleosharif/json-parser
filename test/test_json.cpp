
#include <nlohmann/json.hpp>

import uzleo.json;
import fmt;
import std;

namespace {

static constexpr std::string_view kFilePath{"/tmp/test.json"};

auto WriteFile(std::string content) {
  std::ofstream file_stream{std::string{kFilePath}};
  file_stream << content;
}

auto ParseEmptyObject() {
  // setup
  fmt::println("testing ParseEmptyObject ...");
  WriteFile(R"(
  {}
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (njson.size() != 0) {
    throw std::runtime_error{
        fmt::format("json content is weird. Dump(): {}", json.Dump())};
  }
}

auto ParseSimpleObject() {
  // setup
  fmt::println("testing ParseSimpleObject ...");
  WriteFile(R"(
  {"key": "value"}
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto ParseEmptyArray() {
  // setup
  fmt::println("testing ParseEmptyArray ...");
  WriteFile(R"(
  []
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto ParseSimpleArray() {
  // setup
  fmt::println("testing ParseSimpleArray ...");
  WriteFile(R"(
  [1, 2, 3]
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto ParseStringRoot() {
  // setup
  fmt::println("testing ParseStringRoot ...");
  WriteFile(R"(
  "hello"
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto ParseNumberRoot() {
  // setup
  fmt::println("testing ParseNumberRoot ...");
  WriteFile(R"(
  -3.14
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto ParseBooleanRoot() {
  // setup
  fmt::println("testing ParseBooleanRoot ...");
  WriteFile(R"(
  false
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto ParseNullRoot() {
  // setup
  fmt::println("testing ParseNullRoot ...");
  WriteFile(R"(
  null
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  // TODO
}

auto SimplePositiveCases() {
  ParseEmptyObject();
  ParseSimpleObject();
  ParseEmptyArray();
  ParseSimpleArray();
  ParseStringRoot();
  ParseNumberRoot();
  ParseBooleanRoot();
}

}  // namespace

auto main() -> int {
  try {
    SimplePositiveCases();

    fmt::println("-----------");
    fmt::println("all tests passed.");
  } catch (std::exception const& ex) {
    fmt::println("    failed with info: {}", ex.what());
  }

  return 0;
}
