
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

struct json_value_t
    : std::variant<bool, double, std::monostate, std::string,
                   std::unordered_map<std::string, json_value_t>,
                   std::vector<json_value_t>> {
  using variant::variant;
};

auto format_as(json_value_t const& value) -> std::string {
  using namespace std::string_literals;

  return std::visit(
      overloaded{
          []([[maybe_unused]] std::monostate monostate) { return "null"s; },
          [](bool value) { return fmt::format("{}", value); },
          [](std::string const& value) { return value; },
          [](double value) { return fmt::format("{}", value); },
          [](std::unordered_map<std::string, json_value_t> const& value) {
            std::string result = "{";
            bool first{true};
            for (auto const& [key, val] : value) {
              if (!first) result += ", ";
              result += fmt::format("{}: {}", key, format_as(val));
              first = false;
            }
            result += "}";
            return result;
          },
          [](std::vector<json_value_t> const& value) {
            std::string result = "[";
            bool first{true};
            for (auto const& v : value) {
              if (!first) result += ", ";
              result += format_as(v);
              first = false;
            }
            result += "]";
            return result;
          }},
      value);
}

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
  constexpr Json() = default;
  constexpr explicit Json(auto const& value) : m_value{value} {}
  constexpr Json(Json&& other) = default;
  constexpr Json& operator=(Json&& other) = default;
  constexpr ~Json() = default;
  Json(Json const&) = delete;
  Json& operator=(Json const&) = delete;

  template <class T>
  [[nodiscard]] constexpr auto GetValue() -> T const& {
    return std::get<T>(m_value);
  }

  constexpr auto Print() const { fmt::println("{}", m_value); }

 private:
  json_value_t m_value{};

  friend constexpr auto GetRawValue(Json const& json) -> json_value_t const& {
    return json.m_value;
  }
};

constexpr auto ReadFile(std::filesystem::path&& absolute_file_path)
    -> std::optional<std::shared_ptr<std::string const>>;
constexpr auto Lex(std::shared_ptr<std::string const>&& json_content_ptr)
    -> std::tuple<std::shared_ptr<std::string const>, TokenStream>;
constexpr auto ParseTokens(std::tuple<std::shared_ptr<std::string const>,
                                      TokenStream>&& token_stream_with_buffer)
    -> Json;

export [[nodiscard]] constexpr auto Parse(
    std::filesystem::path&& absolute_file_path) -> Json {
  return ReadFile(std::move(absolute_file_path))
      .transform(Lex)
      .transform(ParseTokens)
      .value();
}

constexpr auto ReadFile(std::filesystem::path&& absolute_file_path)
    -> std::optional<std::shared_ptr<std::string const>> {
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
                rng::end(tmp_view), [](char c) { return (c == '"'); })};
            json_string_end_iter != rng::end(tmp_view)) {
          rng::advance(json_string_end_iter, 1, rng::end(tmp_view));

          // this is pointing to string that include the quotation marks
          // e.g. for a json string "foo", the lexeme_with_commas_size is
          // 3(foo)+2("") = 5
          auto const lexeme_with_commas_size{
              rng::distance(rng::begin(tmp_view), json_string_end_iter)};
          token_stream.emplace_back(
              TokenType::kString, tmp_view.substr(0, lexeme_with_commas_size));

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

    Json ret_json{};
    // switch is responsible for creating the json instance that will be
    // returned as .first
    switch (token_stream_span.front().type) {
      case kLeftBrace: {
        token_stream_span = token_stream_span.subspan(1);

        std::unordered_map<std::string, json_value_t> sub_json{};
        while (true) {
          if (token_stream_span.front().type == kRightBrace) {
            ret_json = Json{sub_json};
            break;
          }

          if (token_stream_span.at(0).type == kString and
              token_stream_span.at(1).type == kColon) {
            auto sub_json_element_key =
                self(token_stream_span).first.template GetValue<std::string>();
            token_stream_span = token_stream_span.subspan(2);

            auto [json_returned, advanced_subspan] = self(token_stream_span);
            auto sub_json_element_value = GetRawValue(json_returned);
            token_stream_span = advanced_subspan;

            sub_json.emplace(sub_json_element_key, sub_json_element_value);
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

        std::vector<json_value_t> sub_json{};
        // TODO: reserve size ?
        while (true) {
          if (token_stream_span.front().type == kRightBracket) {
            ret_json = Json{sub_json};
            break;
          }

          auto [sub_json_element, advanced_subspan] = self(token_stream_span);
          sub_json.push_back(GetRawValue(sub_json_element));
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
      return {Json{std::move(ret_json)}, token_stream_span};
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

    return {Json{std::move(ret_json)}, token_stream_span};
  }};

  return parse(std::get<TokenStream>(token_stream_with_buffer)).first;
}

}  // namespace uzleo::json

//
// Optimization Ideas:
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
// TODOs:
// [ ] mutability of json object after construction
