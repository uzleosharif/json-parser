
import uzleo.json;
import std;

auto main() -> int {
  auto json{uzleo::json::Parse("/tmp/test.json")};
  std::println("{}", json.Dump());
}
