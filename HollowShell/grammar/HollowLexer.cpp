#include "HollowLexer.h"

#include <cctype>

HollowLexer::HollowLexer(std::string_view input)
  : input_(input)
{
}

std::vector<Token>
HollowLexer::Tokenize()
{
  std::vector<Token> tokens;

  while (!IsEnd()) {
    SkipWhitespace();

    if (IsEnd())
      break;

    char c = Peek();

    if (c == '(') {
      tokens.push_back({ TokenType::LPAREN, "(" });

      Advance();

      continue;
    } else if (c == ')') {
      tokens.push_back({ TokenType::RPAREN, ")" });

      Advance();

      continue;
    }

    if (isdigit(c)) {
      size_t offset = 0;

      while (isdigit(Peek(offset)))
        offset++;

      char nextC = Peek(offset);

      if (nextC == '>' || nextC == '<') {
        std::string numStr;

        while (isdigit(Peek()))
          numStr += Advance();

        tokens.push_back({ TokenType::WORD, numStr });

        continue;
      }
    }

    if (c == ';') {
      tokens.push_back({ TokenType::SEMICOLON, ";" });

      Advance();
    } else if (c == '|' || c == '&' || c == '<' || c == '>') {
      tokens.push_back(ReadOperator());
    } else
      tokens.push_back(ReadWord());
  }

  tokens.push_back({ TokenType::END, "" });

  return tokens;
}

Token
HollowLexer::ReadWord()
{
  std::string value;

  bool quot = false;
  char cquote = '\0';
  int parenBns = 0; // For counting seq $()

  while (!IsEnd()) {
    char c = Peek();

    if ((c == '"' || c == '\'')) {
      if (quot && c == cquote) {
        quot = false; // Close quote

        Advance();

        continue;
      } else if (!quot) {
        quot = true;
        cquote = c;

        Advance();

        continue;
      }
    }

    if (!quot) {
      if (c == '$' && Peek(1) == '(')
        parenBns++;
      else if (c == ')') {
        if (parenBns > 0) {
          parenBns--;

          value += Advance();

          continue;
        }
      }

      if (parenBns == 0) {
        if (std::isspace(c))
          break;

        if (c == '(' || c == ')')
          break;

        if (c == '|' || c == '&' || c == '<' || c == '>' || c == ';')
          break;
      }
    }

    value += Advance();
  }

  return { TokenType::WORD, value };
}

Token
HollowLexer::ReadOperator()
{
  char c = Advance();

  if (c == ';')
    return { TokenType::SEMICOLON, ";" }; // delete it

  if (c == '|') {
    if (Peek() == '|') {
      Advance();

      return { TokenType::OR, "||" };
    }

    return { TokenType::PIPE, "|" };
  }

  if (c == '&') {
    if (Peek() == '&') {
      Advance();

      return { TokenType::AND, "&&" };
    }

    if (Peek() == '>') {
      Advance();

      return { TokenType::REDIRECT_BOTH, "&>" };
    }

    return { TokenType::BACKGROUND, "&" };
  }

  if (c == '<') {
    if (Peek() == '<') {
      Advance();

      if (Peek() == '<') {
        Advance();

        return { TokenType::HERE_STRING, "<<<" };
      }

      return { TokenType::HEREDOC, "<<" };
    }

    if (Peek() == '>') {
      Advance();

      return { TokenType::REDIRECT_RW, "<>" };
    }

    return { TokenType::REDIRECT_IN, "<" };
  }

  if (c == '>') {
    if (Peek() == '>') {
      Advance();

      return { TokenType::APPEND_OUT, ">>" };
    }

    if (Peek() == '&') {
      Advance();

      return { TokenType::REDIRECT_DUP, ">&" };
    }

    return { TokenType::REDIRECT_OUT, ">" };
  }

  return { TokenType::WORD, std::string(1, c) };
}

char
HollowLexer::Peek(size_t offset) const
{
  if (pos_ + offset >= input_.length())
    return '\0';

  return input_[pos_ + offset];
}

char
HollowLexer::Advance()
{
  if (IsEnd())
    return '\0';

  return input_[pos_++];
}

bool
HollowLexer::IsEnd() const
{
  return pos_ >= input_.length();
}

void
HollowLexer::SkipWhitespace()
{
  while (!IsEnd() && std::isspace(Peek()))
    Advance();
}