

import uzleo.json;
import fmt;
import std;

namespace fs = std::filesystem;
namespace chr = std::chrono;

auto main() -> int {
  fs::path const current_source_path{__FILE__};

  try {
    auto start_time_point{chr::steady_clock::now()};
    auto json{
        uzleo::json::Parse(current_source_path.parent_path() / "config.json")};
    fmt::println("Parse took {}us",
                 chr::duration_cast<chr::microseconds>(
                     chr::steady_clock::now() - start_time_point)
                     .count());

    auto const& json_map{json.GetMap()};
    fmt::println("Welcome to {} v{}", json_map.at("app_name").GetStringView(),
                 json_map.at("version").GetDouble());
  } catch (std::exception const& ex) {
    fmt::println("Caught exception: {}", ex.what());
    return 1;
  }

  return 0;
}
