

// NOTE: these are some end-to-end tests for uzleo::json API

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
  if (not njson.is_object()) {
    throw std::runtime_error{"json is not object."};
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_object()) {
    throw std::runtime_error{"json is not object."};
  }
  if (njson.size() != 1) {
    throw std::runtime_error{"json size is wrong."};
  }
  if (not njson.contains("key")) {
    throw std::runtime_error{"key does not exist."};
  }
  if (njson["key"] != "value") {
    throw std::runtime_error{"value is wrong."};
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_array()) {
    throw std::runtime_error("json is not array.");
  }
  if (njson.size() != 0) {
    throw std::runtime_error{"size is not 0."};
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_array()) {
    throw std::runtime_error("json is not array.");
  }
  if (njson.size() != 3) {
    throw std::runtime_error{"size is wrong."};
  }
  if (not std::ranges::equal(njson.get<std::vector<int>>(),
                             std::vector{1, 2, 3})) {
    throw std::runtime_error{"content is wrong."};
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_string()) {
    throw std::runtime_error("json is not root string.");
  }
  if (njson.get<std::string>() != "hello") {
    throw std::runtime_error("content is wrong.");
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_number()) {
    throw std::runtime_error("json is not root number.");
  }
  if (njson.get<double>() != -3.14) {
    throw std::runtime_error("json number is wrong.");
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_boolean()) {
    throw std::runtime_error("json is not root bool.");
  }
  if (njson.get<bool>() != false) {
    throw std::runtime_error("content is wrong.");
  }
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
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_null()) {
    throw std::runtime_error("json is not root null.");
  }
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

auto ParseObjectWithMultiplePairs() {
  // setup
  fmt::println("testing ParseObjectWithMultiplePairs ...");
  WriteFile(R"(
  { "a": 1, "b": 2, "c": 3 }
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_object()) {
    throw std::runtime_error("json is not object.");
  }
  if (njson.size() != 3) {
    throw std::runtime_error("object size is wrong.");
  }
  if (njson["a"] != 1 || njson["b"] != 2 || njson["c"] != 3) {
    throw std::runtime_error("object content is wrong.");
  }
}

auto ParseObjectWithMixedTypes() {
  // setup
  fmt::println("testing ParseObjectWithMixedTypes ...");
  WriteFile(R"(
  { "str": "abc", "num": 123, "bool": true, "nullv": null }
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_object()) {
    throw std::runtime_error("json is not object.");
  }
  if (njson.size() != 4) {
    throw std::runtime_error("object size is wrong.");
  }
  if (njson["str"] != "abc" || njson["num"] != 123 || njson["bool"] != true ||
      not njson["nullv"].is_null()) {
    throw std::runtime_error("object content is wrong.");
  }
}

auto ParseArrayWithMixedTypes() {
  // setup
  fmt::println("testing ParseArrayWithMixedTypes ...");
  WriteFile(R"(
  ["hello", 42, false, null]
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_array()) {
    throw std::runtime_error("json is not array.");
  }
  if (njson.size() != 4) {
    throw std::runtime_error("array size is wrong.");
  }
  if (njson[0] != "hello" || njson[1] != 42 || njson[2] != false ||
      not njson[3].is_null()) {
    throw std::runtime_error("array content is wrong.");
  }
}

auto ParseNestedArray() {
  // setup
  fmt::println("testing ParseNestedArray ...");
  WriteFile(R"(
  [1, [2, 3], 4]
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_array()) {
    throw std::runtime_error("json is not array.");
  }
  if (njson.size() != 3) {
    throw std::runtime_error("array size is wrong.");
  }
  if (njson[0] != 1 || njson[2] != 4) {
    throw std::runtime_error("array content at edges is wrong.");
  }
  if (not njson[1].is_array() ||
      not std::ranges::equal(njson[1].get<std::vector<int>>(),
                             std::vector{2, 3})) {
    throw std::runtime_error("nested array is wrong.");
  }
}

auto ParseNestedObject() {
  // setup
  fmt::println("testing ParseNestedObject ...");
  WriteFile(R"(
  { "outer": { "inner": 42 } }
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_object()) {
    throw std::runtime_error("json is not object.");
  }
  if (not njson.contains("outer") || not njson["outer"].is_object()) {
    throw std::runtime_error("outer object missing or wrong.");
  }
  if (njson["outer"]["inner"] != 42) {
    throw std::runtime_error("inner object value is wrong.");
  }
}

auto ParseObjectWithArrayValue() {
  // setup
  fmt::println("testing ParseObjectWithArrayValue ...");
  WriteFile(R"(
  { "list": [1, 2, 3] }
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_object()) {
    throw std::runtime_error("json is not object.");
  }
  if (not njson.contains("list") || not njson["list"].is_array()) {
    throw std::runtime_error("list array missing or wrong.");
  }
  if (not std::ranges::equal(njson["list"].get<std::vector<int>>(),
                             std::vector{1, 2, 3})) {
    throw std::runtime_error("list array content is wrong.");
  }
}

auto ParseArrayOfObjects() {
  // setup
  fmt::println("testing ParseArrayOfObjects ...");
  WriteFile(R"(
  [ { "a": 1 }, { "b": 2 }, { "c": 3 } ]
  )");

  // invoke api
  auto json = uzleo::json::Parse(kFilePath);

  // verify
  auto njson = nlohmann::json::parse(json.Dump());
  if (not njson.is_array()) {
    throw std::runtime_error("json is not array.");
  }
  if (njson.size() != 3) {
    throw std::runtime_error("array size is wrong.");
  }
  if (njson[0]["a"] != 1 || njson[1]["b"] != 2 || njson[2]["c"] != 3) {
    throw std::runtime_error("array of objects content is wrong.");
  }
}

auto SimpleCompositionPositiveCases() {
  ParseObjectWithMultiplePairs();
  ParseObjectWithMixedTypes();
  ParseArrayWithMixedTypes();
  ParseNestedArray();
  ParseNestedObject();
  ParseObjectWithArrayValue();
  ParseArrayOfObjects();
}

auto ParseEmptyKeyObject() {
  fmt::println("testing ParseEmptyKeyObject ...");
  WriteFile(R"({ "": 123 })");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (not njson.is_object()) throw std::runtime_error{"not object"};
  if (not njson.contains("")) throw std::runtime_error{"empty key missing"};
  if (njson[""].get<int>() != 123) throw std::runtime_error{"value mismatch"};
}

auto ParseEscapedString() {
  fmt::println("testing ParseEscapedString ...");
  WriteFile(R"({ "text": "line1\nline2\tTabbed\\Backslash\"Quote" })");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (njson["text"].get<std::string>() !=
      "line1\nline2\tTabbed\\Backslash\"Quote") {
    throw std::runtime_error{"escaped string content wrong"};
  }
}

auto ParseDeepNestedArrays() {
  fmt::println("testing ParseDeepNestedArrays ...");
  WriteFile(R"([1, [2, [3, [4]]]])");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (not njson.is_array()) throw std::runtime_error{"not array"};

  if (njson[1][1][1][0].get<int>() != 4) {
    throw std::runtime_error{"deep nested value wrong"};
  }
}

auto ParseObjectArrayObject() {
  fmt::println("testing ParseObjectArrayObject ...");
  WriteFile(R"({ "outer": [ { "inner": "deep" } ] })");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (njson["outer"][0]["inner"].get<std::string>() != "deep") {
    throw std::runtime_error{"nested object value wrong"};
  }
}

auto ParseWhitespaceStress() {
  fmt::println("testing ParseWhitespaceStress ...");
  WriteFile(R"(
    {      "key"        :       "value"     }
  )");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (njson["key"].get<std::string>() != "value") {
    throw std::runtime_error{"value wrong in whitespace stress"};
  }
}

auto ParseUnicodeString() {
  fmt::println("testing ParseUnicodeString ...");
  WriteFile(R"({ "greet": "こんにちは" })");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (njson["greet"].get<std::string>() != "こんにちは") {
    throw std::runtime_error{"unicode content wrong"};
  }
}

auto ParseLargeNumber() {
  fmt::println("\033[31m[DISABLED] ParsingLargeNumber\033[0m");
  return;

  fmt::println("testing ParseLargeNumber ...");
  WriteFile(R"(1234567890123456789)");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (njson.get<int64_t>() != 1234567890123456789LL) {
    throw std::runtime_error{"large number mismatch"};
  }
}

auto ParseBooleanNullArray() {
  fmt::println("testing ParseBooleanNullArray ...");
  WriteFile(R"([true, false, null])");

  auto json = uzleo::json::Parse(kFilePath);
  auto njson = nlohmann::json::parse(json.Dump());

  if (not(njson[0].get<bool>() == true && njson[1].get<bool>() == false &&
          njson[2].is_null())) {
    throw std::runtime_error{"boolean/null array content wrong"};
  }
}

auto EdgeCasePositiveCases() {
  ParseEmptyKeyObject();
  ParseEscapedString();
  ParseDeepNestedArrays();
  ParseObjectArrayObject();
  ParseWhitespaceStress();
  ParseUnicodeString();
  ParseLargeNumber();
  ParseBooleanNullArray();
}

}  // namespace

auto main() -> int {
  try {
    fmt::println("*** Testing SimplePositiveTestCases ***");
    SimplePositiveCases();

    fmt::println("*** Testing SimpleCompositionPositiveTestCases ***");
    SimpleCompositionPositiveCases();

    fmt::println("*** Testing EdgeCasePositiveTestCases ***");
    EdgeCasePositiveCases();

    fmt::println("-----------");
    fmt::println("all tests passed.");
  } catch (std::exception const& ex) {
    fmt::println("    failed with info: {}", ex.what());
  }

  return 0;
}
