
// SPDX-License-Identifier: MIT

export module uzleo.json;

import std;
import fmt;

namespace rng = std::ranges;

namespace {

using JsonValueType = std::variant<bool, double, std::monostate, std::string>;

enum class TokenType {
  kLeftBrace,
  kRightBrace,
  kNone,
  kFalse,
  kTrue,
  kNumber,
  kString,
  kNull
};

struct Token {
  TokenType type{TokenType::kNone};
  std::string_view lexeme{};
};

using TokenStream = std::vector<Token>;

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

}  // namespace

namespace uzleo::json {

export class Json final {
 public:
  constexpr Json() = default;
  Json(Json const&) = delete;
  Json& operator=(Json const&) = delete;
  constexpr Json(Json&& other) = default;
  constexpr Json& operator=(Json&& other) = default;
  constexpr ~Json() = default;

  constexpr auto Print() const {
    std::visit(
        overloaded{[](std::monostate monostate) { fmt::println("null"); },
                   [](bool value) { fmt::println("{}", value); },
                   [](std::string value) { fmt::println("{}", value); },
                   [](double value) { fmt::println("{}", value); }},
        m_value);
  }

  constexpr auto SetValue(auto value) { m_value = value; }

 private:
  JsonValueType m_value{};
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
  auto const& json_content{*json_content_ptr};

  TokenStream token_stream{};
  token_stream.reserve(100);

  static constexpr auto in_number{[](char value) noexcept {
    return (std::isdigit(value) or value == '-' or value == '.' or
            value == 'e');
  }};

  for (auto citer{rng::cbegin(json_content)}; citer != rng::cend(json_content);
       rng::advance(citer, 1, rng::cend(json_content))) {
    switch (*citer) {
      case '{': {
        token_stream.emplace_back();
        token_stream.back().type = TokenType::kLeftBrace;
        break;
      }
      case '}': {
        token_stream.emplace_back();
        token_stream.back().type = TokenType::kRightBrace;
        break;
      }
      case 'f': {
        if (std::string_view{citer,
                             rng::next(citer, 5, rng::cend(json_content))} ==
            "false") {
          token_stream.emplace_back();
          token_stream.back().type = TokenType::kFalse;
          rng::advance(citer, 5, rng::cend(json_content));
          break;
        }
        throw std::runtime_error{"Lexer got into a bad state on f."};
      }
      case 't': {
        if (std::string_view{citer,
                             rng::next(citer, 4, rng::cend(json_content))} ==
            "true") {
          token_stream.emplace_back();
          token_stream.back().type = TokenType::kTrue;
          rng::advance(citer, 4, rng::cend(json_content));
          break;
        }
        throw std::runtime_error{"Lexer got into a bad state on t."};
      }
      case 'n': {
        if (std::string_view{citer,
                             rng::next(citer, 4, rng::cend(json_content))} ==
            "null") {
          token_stream.emplace_back();
          token_stream.back().type = TokenType::kNull;
          rng::advance(citer, 4, rng::cend(json_content));
          break;
        }
        throw std::runtime_error{"Lexer got into a bad state on n."};
      }
      case '"': {
        std::string_view json_string_sv{citer, rng::cend(json_content)};
        auto json_string_end_iter{
            rng::find_if(json_string_sv | std::views::drop(1),
                         [](char c) { return (c == '"'); })};
        rng::advance(json_string_end_iter, 1, rng::end(json_string_sv));

        token_stream.emplace_back();
        token_stream.back().type = TokenType::kString;
        token_stream.back().lexeme =
            std::string_view{rng::begin(json_string_sv), json_string_end_iter};

        rng::advance(
            citer,
            rng::distance(rng::begin(json_string_sv), json_string_end_iter),
            rng::cend(json_content));
        break;
      }
      default: {
        if (in_number(*citer)) {
          std::string_view number_sv{citer, rng::cend(json_content)};
          auto number_end_iter{rng::find_if_not(number_sv, in_number)};

          token_stream.emplace_back();
          token_stream.back().type = TokenType::kNumber;
          token_stream.back().lexeme =
              std::string_view{rng::begin(number_sv), number_end_iter};

          rng::advance(citer,
                       rng::distance(rng::begin(number_sv), number_end_iter),
                       rng::cend(json_content));
          break;
        }

        throw std::runtime_error{fmt::format("can not lex {}", *citer)};
      }
    }
  }

  return std::make_tuple(std::move(json_content_ptr), std::move(token_stream));
}

constexpr auto ParseTokens(std::tuple<std::shared_ptr<std::string const>,
                                      TokenStream>&& token_stream_with_buffer)
    -> Json {
  const auto& token_stream{std::get<TokenStream>(token_stream_with_buffer)};
  Json json{};

  for (auto const& token : token_stream) {
    switch (token.type) {
      case TokenType::kLeftBrace: {
        // TODO()
        break;
      }
      case TokenType::kFalse: {
        json.SetValue(false);
        break;
      }
      case TokenType::kTrue: {
        json.SetValue(true);
        break;
      }
      case TokenType::kNumber: {
        double value{0.0};
        auto conversion_result{std::from_chars(rng::begin(token.lexeme),
                                               rng::end(token.lexeme), value)};
        json.SetValue(value);
        break;
      }
      case TokenType::kNull: {
        json.SetValue(std::monostate{});
        break;
      }
      case TokenType::kString: {
        json.SetValue(std::string{token.lexeme});
        break;
      }
      default: {
        throw std::runtime_error{"Unexpected token received."};
      }
    }
  }

  return json;
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
