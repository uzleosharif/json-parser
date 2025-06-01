
// SPDX-License-Identifier: MIT

export module uzleo.json;

import std;
import fmt;

namespace rng = std::ranges;

namespace {

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

enum class TokenType {
  kLeftBrace,
  kRightBrace,
  kLeftBracket,
  kRightBracket,
  kFalse,
  kTrue,
  kNumber,
  kString,
  kColon,
  kComma,
  kNull
};

struct Token {
  TokenType type{TokenType::kNull};
  std::string_view lexeme{};
};

using TokenStream = std::vector<Token>;

auto format_as(Token const& token) -> std::string {
  using enum TokenType;

  switch (token.type) {
    case kLeftBrace: {
      return "{";
    }
    case kRightBrace: {
      return "}";
    }
    case kFalse: {
      return "false";
    }
    case kTrue: {
      return "true";
    }
    case kNumber: {
      return "number";
    }
    case kString: {
      return "string";
    }
    case kNull: {
      return "null";
    }
    case kColon: {
      return "colon";
    }
    case kComma: {
      return "comma";
    }
    case kLeftBracket: {
      return "[";
    }
    case kRightBracket: {
      return "]";
    }
  }
}

}  // namespace

namespace uzleo::json {

export class Json final {
 public:
  using json_object_t = std::unordered_map<std::string, Json>;
  using json_array_t = std::vector<Json>;

  constexpr explicit Json(bool value) : m_value{value} {}
  constexpr explicit Json(double value) : m_value{value} {}
  constexpr explicit Json(std::monostate monostate) : m_value{monostate} {}
  constexpr explicit Json(std::string_view value)
      : m_value{std::string(value)} {}
  constexpr explicit Json(json_object_t&& value) : m_value{std::move(value)} {}
  constexpr explicit Json(json_array_t&& value) : m_value{std::move(value)} {}

  constexpr Json(Json&& other) = default;
  constexpr Json& operator=(Json&& other) = default;
  constexpr ~Json() = default;
  Json(Json const&) = delete;
  Json& operator=(Json const&) = delete;

  /// formats the internal json representation into a string
  constexpr auto Dump() const { return fmt::format("{}", *this); }

  /// check if the json object contains a specific key
  /// @return true if key exists, false otherwise
  /// @throws std::logic_error if called on a non-object json instance
  [[nodiscard]] constexpr auto Contains(std::string_view key) const -> bool {
    if (not std::holds_alternative<json_object_t>(m_value)) {
      throw std::logic_error{"json is not an object."};
    }

    return std::get<json_object_t>(m_value).contains(std::string{key});
  }

  template <class T>
  [[nodiscard]] constexpr auto IsType() const -> bool {
    return std::holds_alternative<T>(m_value);
  }

  [[nodiscard]] constexpr auto GetStringView() const -> std::string_view {
    if (not IsType<std::string>()) {
      throw std::invalid_argument{"does not contain string value."};
    }

    return std::string_view{std::get<std::string>(m_value)};
  }

  [[nodiscard]] constexpr auto GetDouble() const -> double {
    if (not IsType<double>()) {
      throw std::invalid_argument{"does not contain double value."};
    }

    return std::get<double>(m_value);
  }

  [[nodiscard]] constexpr auto GetArray() const -> std::span<Json const> {
    if (not IsType<json_array_t>()) {
      throw std::invalid_argument{"does not contain array value."};
    }

    return std::get<json_array_t>(m_value);
  }

  [[nodiscard]] constexpr auto GetMap() const -> json_object_t const& {
    if (not IsType<json_object_t>()) {
      throw std::invalid_argument{"does not contain map value."};
    }

    return std::get<json_object_t>(m_value);
  }

  [[nodiscard]] constexpr auto GetJson(std::string_view key) const
      -> Json const& {
    if (not Contains(key)) {
      throw std::invalid_argument{fmt::format("does not contain {} key.", key)};
    }
    if (not IsType<json_object_t>()) {
      throw std::invalid_argument{"is not a json map object."};
    }

    return std::get<json_object_t>(m_value).at(std::string{key});
  }

 private:
  struct json_value_t : std::variant<bool, double, std::monostate, std::string,
                                     json_object_t, json_array_t> {
    using variant::variant;
  };

  json_value_t m_value;

  friend constexpr auto format_as(Json const& json) -> std::string;
};

constexpr auto format_as(Json const& json) -> std::string {
  using namespace std::string_literals;

  return std::visit(
      overloaded{
          []([[maybe_unused]] std::monostate monostate) { return "null"s; },
          [](bool value) { return fmt::format("{}", value); },
          [](std::string const& value) { return fmt::format("\"{}\"", value); },
          [](double value) { return fmt::format("{}", value); },
          [](Json::json_object_t const& value) {
            std::string result = "{";
            bool first{true};
            for (auto const& [key, val] : value) {
              if (!first) result += ", ";
              result += fmt::format("\"{}\": {}", key, format_as(val));
              first = false;
            }
            result += "}";
            return result;
          },
          [](Json::json_array_t const& value) {
            std::string result = "[";
            bool first{true};
            for (auto const& val : value) {
              if (!first) result += ", ";
              result += format_as(val);
              first = false;
            }
            result += "]";
            return result;
          }},
      json.m_value);
}

/// this allocates the string buffer (holding json) data on heap so that it is
/// alive throughout the program. This allows views over it to be used for
/// lex/parse algorithms
constexpr auto ReadFile(std::filesystem::path&& absolute_file_path)
    -> std::shared_ptr<std::string const> {
  if (not std::filesystem::exists(absolute_file_path)) {
    throw std::invalid_argument{
        fmt::format("Json {} file does not exist.", absolute_file_path)};
  }
  std::ifstream file_stream{absolute_file_path};
  if (not file_stream.is_open()) {
    throw std::runtime_error{
        fmt::format("Failed to open json {} file.", absolute_file_path)};
  }

  return std::make_shared<std::string const>(
      rng::istream_view<char>{file_stream} | rng::to<std::string>());
}

constexpr auto Lex(std::shared_ptr<std::string const>&& json_content_ptr)
    -> std::tuple<std::shared_ptr<std::string const>, TokenStream> {
  static constexpr auto in_number{[](char value) noexcept {
    return (std::isdigit(value) or value == '-' or value == '.' or
            value == 'e');
  }};

  auto const& json_content{*json_content_ptr};
  auto const json_content_end_iter{rng::cend(json_content)};

  TokenStream token_stream{};
  token_stream.reserve(100);

  auto citer{rng::cbegin(json_content)};
  while (citer != json_content_end_iter) {
    switch (std::string_view tmp_view{citer, json_content_end_iter}; *citer) {
      case '{': {
        token_stream.emplace_back(TokenType::kLeftBrace);

        rng::advance(citer, 1, json_content_end_iter);
        break;
      }
      case '}': {
        token_stream.emplace_back(TokenType::kRightBrace);

        rng::advance(citer, 1, json_content_end_iter);
        break;
      }
      case 'f': {
        if (tmp_view.starts_with("false")) {
          token_stream.emplace_back(TokenType::kFalse, tmp_view.substr(0, 5));

          rng::advance(citer, 5, json_content_end_iter);
          break;
        }

        throw std::invalid_argument{"Failed to lex false."};
      }
      case 't': {
        if (tmp_view.starts_with("true")) {
          token_stream.emplace_back(TokenType::kTrue, tmp_view.substr(0, 4));

          rng::advance(citer, 4, json_content_end_iter);
          break;
        }

        throw std::invalid_argument{"Failed to lex true."};
      }
      case 'n': {
        if (tmp_view.starts_with("null")) {
          token_stream.emplace_back(TokenType::kNull, tmp_view.substr(0, 4));

          rng::advance(citer, 4, json_content_end_iter);
          break;
        }

        throw std::invalid_argument{"Failed to lex null."};
      }
      case '"': {
        if (auto json_string_end_iter{rng::find_if(
                rng::next(rng::begin(tmp_view), 1, rng::end(tmp_view)),
                rng::end(tmp_view),
                [escaped = false](char c) mutable {
                  if (escaped) {
                    escaped = false;
                    return false;
                  }
                  if (c == '\\') {
                    escaped = true;
                    return false;
                  }

                  return (c == '"');
                })};
            json_string_end_iter != rng::end(tmp_view)) {
          rng::advance(json_string_end_iter, 1, rng::end(tmp_view));

          // this is pointing to string that include the quotation marks
          // e.g. for a json string "foo", the lexeme_with_commas_size is
          // 3(foo)+2("") = 5
          auto const lexeme_with_commas_size{
              rng::distance(rng::begin(tmp_view), json_string_end_iter)};
          token_stream.emplace_back(
              TokenType::kString,
              tmp_view.substr(1, lexeme_with_commas_size - 2));

          rng::advance(citer, lexeme_with_commas_size, json_content_end_iter);
          break;
        }

        throw std::invalid_argument{"Failed to lex string."};
      }
      case ':': {
        token_stream.emplace_back(TokenType::kColon);

        rng::advance(citer, 1, json_content_end_iter);
        break;
      }
      case ',': {
        token_stream.emplace_back(TokenType::kComma);

        rng::advance(citer, 1, json_content_end_iter);
        break;
      }
      case '[': {
        token_stream.emplace_back(TokenType::kLeftBracket);

        rng::advance(citer, 1, json_content_end_iter);
        break;
      }
      case ']': {
        token_stream.emplace_back(TokenType::kRightBracket);

        rng::advance(citer, 1, json_content_end_iter);
        break;
      }
      default: {
        if (in_number(*citer)) {
          auto number_end_iter{rng::find_if_not(tmp_view, in_number)};
          auto const lexeme_size{
              rng::distance(rng::begin(tmp_view), number_end_iter)};
          token_stream.emplace_back(TokenType::kNumber,
                                    tmp_view.substr(0, lexeme_size));

          rng::advance(citer, lexeme_size, json_content_end_iter);
          break;
        }

        throw std::invalid_argument{
            fmt::format("Failed to lex character: {}", *citer)};
      }
    }
  }

  return std::make_tuple(std::move(json_content_ptr), std::move(token_stream));
}

constexpr auto ParseTokens(std::tuple<std::shared_ptr<std::string const>,
                                      TokenStream>&& token_stream_with_buffer)
    -> Json {
  auto const parse{[](this auto const& self,
                      std::span<Token const> token_stream_span)
                       -> std::pair<Json, std::span<Token const>> {
    using enum TokenType;

    if (rng::size(token_stream_span) == 0) {
      throw std::invalid_argument(
          "Recursive parse called with empty token-stream.");
    }

    Json ret_json{std::monostate{}};
    // switch is responsible for creating the json instance that will be
    // returned as .first
    switch (token_stream_span.front().type) {
      case kLeftBrace: {
        token_stream_span = token_stream_span.subspan(1);

        Json::json_object_t inner_json{};
        while (true) {
          if (token_stream_span.front().type == kRightBrace) {
            ret_json = Json{std::move(inner_json)};
            break;
          }

          if (token_stream_span.at(0).type == kString and
              token_stream_span.at(1).type == kColon) {
            auto const parse_result_key{self(token_stream_span)};
            token_stream_span = parse_result_key.second.subspan(1);

            auto parse_result_value = self(token_stream_span);
            inner_json.emplace(
                parse_result_key.first.GetStringView() | rng::to<std::string>(),
                std::move(parse_result_value.first));
            token_stream_span = parse_result_value.second;
          } else {
            throw std::runtime_error{fmt::format(
                "Parser expectes string and colon for map key, token-stream: "
                "{}",
                token_stream_span)};
          }
        }

        break;
      }

      case kLeftBracket: {
        token_stream_span = token_stream_span.subspan(1);

        Json::json_array_t inner_json{};
        // TODO: reserve size ?
        while (true) {
          if (token_stream_span.front().type == kRightBracket) {
            ret_json = Json{std::move(inner_json)};
            break;
          }

          auto [inner_json_element, advanced_subspan] = self(token_stream_span);
          inner_json.push_back(std::move(inner_json_element));
          token_stream_span = advanced_subspan;
        }

        break;
      }

      case kString: {
        ret_json = Json{std::string{token_stream_span.front().lexeme}};
        break;
      }

      case kTrue: {
        ret_json = Json{true};
        break;
      }

      case kFalse: {
        ret_json = Json{false};
        break;
      }

      case kNull: {
        ret_json = Json{std::monostate{}};
        break;
      }

      case kNumber: {
        double value;
        auto const& token{token_stream_span.front()};

        auto char_conv_result{std::from_chars(rng::begin(token.lexeme),
                                              rng::end(token.lexeme), value)};
        if (char_conv_result.ec != std::errc{} or
            char_conv_result.ptr != rng::end(token.lexeme)) {
          throw std::runtime_error(
              fmt::format("Failed to convert into number the token lexeme: {}",
                          token.lexeme));
        }
        ret_json = Json{value};
        break;
      }

      default: {
        throw std::logic_error{fmt::format("Unexpected token received: {}",
                                           token_stream_span.front())};
      }
    }

    token_stream_span = token_stream_span.subspan(1);
    if (rng::size(token_stream_span) == 0) {
      return {std::move(ret_json), token_stream_span};
    }

    auto const token_type{token_stream_span.front().type};
    if (not(token_type == kComma or token_type == kColon or
            token_type == kRightBrace or token_type == kRightBracket)) {
      throw std::runtime_error(fmt::format(
          "Parser expecting `,:}}]`, but token-stream: {}", token_stream_span));
    }

    if (token_type == kComma) {
      token_stream_span = token_stream_span.subspan(1);

      if (token_stream_span.front().type == kRightBrace or
          token_stream_span.front().type == kRightBracket) {
        throw std::runtime_error(
            fmt::format("Parser not expecting }}], but token-stream: {}",
                        token_stream_span));
      }
    }

    return {std::move(ret_json), token_stream_span};
  }};

  return parse(std::get<TokenStream>(token_stream_with_buffer)).first;
}

export [[nodiscard]] constexpr auto Parse(
    std::filesystem::path&& absolute_file_path) -> Json {
  return std::optional{ReadFile(std::move(absolute_file_path))}
      .transform(Lex)
      .transform(ParseTokens)
      .value();
}

export [[nodiscard]] constexpr auto Parse(std::string_view json_content)
    -> Json {
  return std::optional{std::make_shared<std::string const>(json_content)}
      .transform(Lex)
      .transform(ParseTokens)
      .value();
}

}  // namespace uzleo::json

//
//
//
// TODOs:
// *********
// [x] 1) access operator
// [ ] 2) is_root(), is_array(), is_object()
// [x] 3) mutability of json object after construction
// [x] 4) return references instead of new objects
// [ ] 5) able to iterate over items
// [ ] 6) fix disabled test of ParseLargeNumber
// [x] 7) doxygen documentation
// [x] 8) contains() API
// [x] 9) merge Json() constructor with Json(value)
//
// [ ] 10) allow <int> on GetObjectValue() API
// [ ] 11) get array, map as value in GetObjectValue()
// [ ] 12) replace fmt with std so as to avoid shipping fmt module
//
// Optimization Ideas:
// **************************
// [x] remove temporarry string between ReadFile() and Lex()
// and instead use istream_view .. make ifstream object
// static to extend lifetime [ ] noexcept of functions
// -> this does not work, as for lexing we need to be able to peek ahead i.e.
// we want to do multi-pass moves over the string data. Using stream-view this
// is not possible as stream-view/iterator are single-pass only
//
// [x] can we avoid string copying of same data between
// Lexer, tokens, json_value -> would a Parser class help that share state ?
//
// [ ] parse the json content lazily .. this helps in reducing memory usage for
// holding json-content, token-stream buffers at runtime .. possibly use libcoro
// framework for co-routines
//
// [ ] disable copy operations on json_value_t
//
