
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
          [](std::monostate monostate) { return "null"s; },
          [](bool value) { return fmt::format("{}", value); },
          [](std::string const& value) { return value; },
          [](double value) { return fmt::format("{}", value); },
          [](std::unordered_map<std::string, json_value_t> const& value) {
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

// TODO: move this over to uzleo::utils
template <class T>
constexpr auto ChunkSpan(std::span<T> source, std::size_t size)
    -> std::vector<std::span<T>> {
  if (rng::size(source) % size) {
    throw std::logic_error{fmt::format(
        "Source span of size: {} can not be chunked by passed size arg: {}.",
        rng::size(source), size)};
  }

  std::vector<std::span<T>> span_chunks{};
  span_chunks.reserve(rng::size(source) / size);
  for (auto citer{rng::cbegin(source)}; citer != rng::cend(source);
       rng::advance(citer, size, rng::cend(source))) {
    span_chunks.push_back(std::span{citer, size});
  }

  return span_chunks;
}

}  // namespace

namespace uzleo::json {

export class Json final {
 public:
  //  constexpr Json() = default;
  constexpr explicit Json(auto const& value) : m_value{value} {}
  constexpr Json(Json&& other) = default;
  constexpr Json& operator=(Json&& other) = default;
  constexpr ~Json() = default;
  Json(Json const&) = delete;
  Json& operator=(Json const&) = delete;

  template <class T>
  constexpr auto GetValue() -> T const& {
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

export constexpr auto Parse(std::filesystem::path&& absolute_file_path)
    -> Json {
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
          if (auto number_end_iter{rng::find_if_not(tmp_view, in_number)};
              number_end_iter != rng::end(tmp_view)) {
            auto const lexeme_size{
                rng::distance(rng::begin(tmp_view), number_end_iter)};
            token_stream.emplace_back(TokenType::kNumber,
                                      tmp_view.substr(0, lexeme_size));

            rng::advance(citer, lexeme_size, json_content_end_iter);
            break;
          }

          throw std::invalid_argument{"Failed to lex number."};
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
                      std::span<Token const> token_stream_span) -> Json {
    using enum TokenType;

    auto token_stream_span_end_iter{rng::end(token_stream_span)};

    switch (auto const& token{token_stream_span.front()}; token.type) {
      case kLeftBrace: {
        if (token_stream_span.back().type != kRightBrace) {
          throw std::invalid_argument{
              "Failed to parse tokens due to non-matching }."};
        }

        if (rng::size(token_stream_span) == 2) {
          // empty json object '{}' case
          return Json{std::unordered_map<std::string, json_value_t>{}};
        }

        // consume { token
        token_stream_span = token_stream_span.subspan(1);

        std::unordered_map<std::string, json_value_t> sub_json{};
        for (auto const token_chunk : ChunkSpan(token_stream_span, 4)) {
          // chunk: {kString, kColon, kFoo, kComma or kRightBrace}

          // FIXME: when value type is json-object/array

          if (token_chunk[0].type != kString and
              token_chunk[1].type != kColon) {
            throw std::invalid_argument{
                fmt::format("Failed to parse token chunk: {}.", token_chunk)};
          }
          if (token_chunk[3].type != kComma and
              token_chunk[3].type != kRightBrace) {
            throw std::invalid_argument{
                fmt::format("Failed to parse token chunk: {}.", token_chunk)};
          }
          // TODO(): check token_chunk.at(2)

          sub_json.emplace(self(token_chunk).template GetValue<std::string>(),
                           GetRawValue(self(token_chunk.subspan(2))));
        }

        return Json{sub_json};
      }
      case kLeftBracket: {
        if (token_stream_span.back().type != kRightBracket) {
          throw std::invalid_argument{
              "Failed to parse tokens due to non-matching ]."};
        }

        if (rng::size(token_stream_span) == 2) {
          // empty json object '[]' case
          return Json{std::vector<json_value_t>{}};
        }

        // consume { token
        token_stream_span = token_stream_span.subspan(1);

        std::vector<json_value_t> sub_json{};
        // TODO: reserve size ?
        for (auto const token_chunk : ChunkSpan(token_stream_span, 2)) {
          // chunk: {kFoo, kComma or kRightBracket}

          // FIXME: when value type is json-object/array

          // TODO(): check token_chunk.at(0) similar to above

          if (token_chunk[1].type != kComma and
              token_chunk[1].type != kRightBracket) {
            throw std::invalid_argument{
                fmt::format("Failed to parse token chunk: {}", token_chunk)};
          }

          sub_json.push_back(GetRawValue(self(token_chunk)));
        }

        return Json{sub_json};
      }
      case kString: {
        return Json{std::string{token.lexeme}};
      }
      case kTrue: {
        return Json{true};
      }
      case kFalse: {
        return Json{false};
      }
      case kNull: {
        return Json{std::monostate{}};
      }
      case kNumber: {
        double value;
        auto char_conv_result{std::from_chars(rng::begin(token.lexeme),
                                              rng::end(token.lexeme), value)};
        if (char_conv_result.ec != std::errc{}) {
          throw std::runtime_error(
              fmt::format("Failed to convert into number the token lexeme: {}",
                          token.lexeme));
        }
        return Json{value};
      }
      default: {
        throw std::logic_error{fmt::format("Unexpected token received: {}",
                                           token_stream_span.front())};
      }
    }
  }};

  return parse(std::get<TokenStream>(token_stream_with_buffer));
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
