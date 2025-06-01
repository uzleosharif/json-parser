
import uzleo.json;
import std;

auto main() -> int {
  auto json{uzleo::json::Parse(std::filesystem::path{"/tmp/test.json"})};
  std::println("{}", json.Dump());
}
