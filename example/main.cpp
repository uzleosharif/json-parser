
import uzleo.json;

auto main() -> int {
  auto json{uzleo::json::Parse("/tmp/test.json")};
  json.Print();
}
