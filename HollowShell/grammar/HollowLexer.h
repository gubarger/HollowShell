#pragma once

#include <string>
#include <vector>
#include <stdint.h>

/**
 * @file HollowLexer.h
 */

/**
 * @brief Token types supported (simpled)
 */
enum class TokenType
{
  WORD,          // A common word, command or argument
  PIPE,          // |
  OR,            // ||
  AND,           // &&
  BACKGROUND,    // &
  REDIRECT_IN,   // <
  HEREDOC,       // <<
  REDIRECT_OUT,  // >
  APPEND_OUT,    // >>
  REDIRECT_ERR,  // 2>
  REDIRECT_BOTH, // &> or 2>&1 (simplified)
  HERE_STRING,   // <<<
  REDIRECT_RW,   // <>
  REDIRECT_DUP,  // >&
  SEMICOLON,     // ;
  LPAREN,        // (
  RPAREN,        // )
  END            // End
};

/**
 * @struct
 *
 * @brief Describes one token
 */
struct Token
{
  TokenType type;
  std::string value;
};

/**
 * @class HollowLexer
 *
 * @brief Converts an input string to a sequence of tokens
 *
 * Supported operators:
 * | || & && ; ( ) < > >> 2> &> << <<< <>
 *
 * - Handling of quotes (' and ")
 * - Support for numeric file descriptors (2>) etc.
 * - Correct handling of nested $()
 */
class HollowLexer
{
public:
  explicit HollowLexer(std::string_view input);

  /**
   * @public
   *
   * @brief Splits the input string into a list of tokens
   *
   * @return Token vector
   */
  std::vector<Token> Tokenize();

private:
  std::string_view input_;
  uint32_t pos_{ 0 };

private:
  Token ReadWord();
  Token ReadOperator();

  char Peek(size_t offset = 0UL) const;
  char Advance();
  bool IsEnd() const;
  void SkipWhitespace();
};