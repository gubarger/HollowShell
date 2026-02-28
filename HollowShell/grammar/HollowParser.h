#pragma once

#include "HollowAST.h"
#include "HollowLexer.h"

/**
 * @class HollowParser
 *
 * @brief Build an AST from a list of tokens using recursive descent
 */
class HollowParser
{
public:
  explicit HollowParser(const std::vector<Token>& tokens);

  /**
   * @public
   *
   * @brief Parsing command
   *
   * @return *ASTNode (root)
   */
  std::unique_ptr<ASTNode> Parse();

private:
  const std::vector<Token>& tokens_;
  size_t pos_{ 0 };

private:
  const Token& Peek() const;
  const Token& Advance();

  bool Match(TokenType type);
  bool Check(TokenType type) const;
  bool IsEnd() const;

  bool IsRedirection(TokenType type);
  bool IsNumber(const std::string str);

  std::unique_ptr<ASTNode> ParseBackground();
  std::unique_ptr<ASTNode> ParseLogical();
  std::unique_ptr<ASTNode> ParsePipeline();
  std::unique_ptr<ASTNode> ParseCommand();
  std::unique_ptr<ASTNode> ParseSequence();
};