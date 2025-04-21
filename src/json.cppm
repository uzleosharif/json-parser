
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

constexpr auto ReadFile(std::filesystem::path&& absolute_file_path)
    -> std::optional<std::string>;
constexpr auto Lex(std::string const& json_content)
    -> std::tuple<std::string const&, TokenStream>;

export class Json final {
 public:
  constexpr auto Print() const {
    std::visit(
        overloaded{[](std::monostate monostate) { fmt::println("null"); },
                   [](bool value) { fmt::println("{}", value); },
                   [](std::string value) { fmt::println("{}", value); },
                   [](double value) { fmt::println("{}", value); }},
        m_value);
  }  // namespace uzleo::json

  constexpr auto SetValue(auto value) { m_value = value; }

 private:
  JsonValueType m_value{};
};

constexpr auto ParseTokens(
    std::tuple<std::string const&, TokenStream&&> token_stream_with_buffer)
    -> Json;

export constexpr auto Parse(std::filesystem::path&& absolute_file_path)
    -> Json {
  // `ReadFile` is producer of string buffer, while `Lex`, `ParseTokens` are
  // read-only consumers (string const&) of it

  // NOTE: ReadFile() returns optional (even though there is no need) to make it
  // functionally composable in a function-pipeline.

  // NOTE: there is no need to pass the shared string buffer between `Lex` and
  // `ParseTokens`, but for efficiency reasons we want `ParseTokens` to use
  // views over the string buffer in `Lex`. To keep views sane, we need to make
  // sure that the string buffer is outliving `ParseTokens` routine. Hence, we
  // pass it via const& from `Lex` to `ParseTokens` so as to extend the lifetime
  // of string buffer so as to keep views making sense.
  // There could be an alternative where we pass std::sting&& so that each
  // preceeding stage owns the string buffer. To me this looks ugly as each
  // next stage is not really meant to mutate the buffer so what is the
  // point of owning the buffer. If a functions owns a buffer, there is no
  // guarantee that, that function can't mutate the buffer. If the function,
  // is instead a const& then we are guaranteed that it can not
  // modify it.
  // std::string (i.e. value semantics) won't work as we need views over
  // original allocated buffer.

  return ReadFile(std::move(absolute_file_path))
      .transform(Lex)
      .transform(ParseTokens)
      .value();
}

}  // namespace uzleo::json

namespace uzleo::json {

constexpr auto ReadFile(std::filesystem::path&& absolute_file_path)
    -> std::optional<std::string> {
  if (not std::filesystem::exists(absolute_file_path)) {
    throw std::invalid_argument{
        fmt::format("Json {} file does not exist.", absolute_file_path)};
  }
  std::ifstream file_stream{absolute_file_path};
  if (not file_stream.is_open()) {
    throw std::runtime_error{
        fmt::format("Failed to open json {} file.", absolute_file_path)};
  }

  return std::views::istream<char>(file_stream) | rng::to<std::string>();
}

constexpr auto Lex(std::string const& json_content)
    -> std::tuple<std::string const&, TokenStream> {
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

  return std::make_tuple(json_content, std::move(token_stream));
}

constexpr auto ParseTokens(
    std::tuple<std::string const&, TokenStream&&> token_stream_with_buffer)
    -> Json {
  Json json{};

  auto token_stream{std::get<TokenStream&&>(token_stream_with_buffer)};
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
// [ ] remove temporarry string between ReadFile() and Lex()
// and instead use istream_view .. make ifstream object
// static to extend lifetime [ ] noexcept of functions
//
// [x] can we avoid string copying of same data between
// Lexer, tokens, json_value -> would a Parser class help that share state ?
