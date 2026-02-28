#include "HollowParser.h"

#include <algorithm>
#include <iostream>

HollowParser::HollowParser(const std::vector<Token>& tokens)
  : tokens_(tokens)
{
}

std::unique_ptr<ASTNode>
HollowParser::Parse()
{
  if (IsEnd())
    return nullptr;

  return ParseSequence();
}

const Token&
HollowParser::Peek() const
{
  if (pos_ >= tokens_.size()) {
    static const Token endToken{ TokenType::END, "" };

    return endToken;
  }

  return tokens_[pos_];
}

const Token&
HollowParser::Advance()
{
  if (!IsEnd())
    pos_++;

  return tokens_[pos_ - 1];
}

bool
HollowParser::Match(TokenType type)
{
  if (Check(type)) {
    Advance();

    return true;
  }

  return false;
}

bool
HollowParser::Check(TokenType type) const
{
  if (IsEnd())
    return false;

  return Peek().type == type;
}

bool
HollowParser::IsEnd() const
{
  return Peek().type == TokenType::END;
}

bool
HollowParser::IsRedirection(TokenType type)
{
  return type == TokenType::REDIRECT_IN || type == TokenType::REDIRECT_OUT ||
         type == TokenType::APPEND_OUT || type == TokenType::REDIRECT_ERR ||
         type == TokenType::REDIRECT_BOTH || type == TokenType::HEREDOC ||
         type == TokenType::SEMICOLON;
}

bool
HollowParser::IsNumber(const std::string str)
{
  return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

std::unique_ptr<ASTNode>
HollowParser::ParseBackground()
{
  auto node = ParseLogical();

  if (Match(TokenType::BACKGROUND)) {
    auto bgNode = std::make_unique<BackgroundNode>();
    bgNode->command = std::move(node);

    return bgNode;
  }

  return node;
}

std::unique_ptr<ASTNode>
HollowParser::ParseLogical()
{
  auto left = ParsePipeline();

  while (Check(TokenType::AND) || Check(TokenType::OR)) {
    TokenType op = Peek().type;

    Advance();

    auto right = ParsePipeline();
    auto lcNode = std::make_unique<LogicalNode>();

    lcNode->op = op;
    lcNode->left = std::move(left);
    lcNode->right = std::move(right);

    left = std::move(lcNode);
  }

  return left;
}

std::unique_ptr<ASTNode>
HollowParser::ParsePipeline()
{
  auto left = ParseCommand();

  while (Match(TokenType::PIPE)) {
    auto right = ParseCommand();
    auto peNode = std::make_unique<PipelineNode>();

    peNode->left = std::move(left);
    peNode->right = std::move(right);

    left = std::move(peNode);
  }

  return left;
}

std::unique_ptr<ASTNode>
HollowParser::ParseCommand()
{
  std::unique_ptr<ASTNode> resultNode;
  std::vector<Redirection> redirects;

  // Subshell
  if (Check(TokenType::LPAREN)) {
    Advance(); // Pick '('

    auto inner = ParseSequence();

    if (!Match(TokenType::RPAREN))
      throw std::runtime_error("Expected ')' after subshell");

    auto subNode = std::make_unique<SubshellNode>();

    subNode->command = std::move(inner);
    resultNode = std::move(subNode);
  } else { // command
    auto cmdNode = std::make_unique<CommandNode>();

    resultNode = std::move(cmdNode);
  }

  // Parse args
  while (!IsEnd()) {
    TokenType type = Peek().type;

    // End command
    if (type == TokenType::PIPE || type == TokenType::AND ||
        type == TokenType::OR || type == TokenType::BACKGROUND ||
        type == TokenType::SEMICOLON || type == TokenType::RPAREN) {
      break;
    }

    // FD check
    int explicitFd = -1;
    bool isFdRedirect = false;

    if (type == TokenType::WORD && IsNumber(Peek().value)) {
      if (pos_ + 1 < tokens_.size()) {
        TokenType next = tokens_[pos_ + 1].type;

        if (IsRedirection(next) && next != TokenType::SEMICOLON) {
          explicitFd = std::stoi(Advance().value);
          isFdRedirect = true;
          type = Peek().type;
        }
      }
    }

    if (IsRedirection(type) && type != TokenType::SEMICOLON) {
      // Redirect
      TokenType redirType = Advance().type;

      if (!Match(TokenType::WORD))
        throw std::runtime_error("Expected filename");

      std::string target = tokens_[pos_ - 1].value;

      // FD
      if (explicitFd == -1) {
        explicitFd = (redirType == TokenType::REDIRECT_IN ||
                      redirType == TokenType::HEREDOC ||
                      redirType == TokenType::HERE_STRING)
                       ? 0
                       : 1;
      }

      // Add redirect
      if (auto cmd = dynamic_cast<CommandNode*>(resultNode.get()))
        cmd->redirects.push_back({ redirType, target, explicitFd });
      else if (auto sub = dynamic_cast<SubshellNode*>(resultNode.get()))
        sub->redirects.push_back({ redirType, target, explicitFd });
    } else if (!isFdRedirect) { // Word
      if (auto cmd = dynamic_cast<CommandNode*>(resultNode.get())) {
        if (type == TokenType::WORD) {
          if (cmd->command.empty())
            cmd->command = Advance().value;
          else
            cmd->args.push_back(Advance().value);
        } else
          Advance(); // Skip unknown
      } else {
        std::string val = Peek().value;

        if (val.empty())
          Advance();
        else
          throw std::runtime_error("Subshell cannot have arguments: " + val);
      }
    }
  }

  // Empty command
  if (auto cmd = dynamic_cast<CommandNode*>(resultNode.get())) {
    if (cmd->command.empty() && !cmd->redirects.empty())
      cmd->command = ":";

    if (cmd->command.empty())
      return nullptr;
  }

  return resultNode;
}

std::unique_ptr<ASTNode>
HollowParser::ParseSequence()
{
  auto left = ParseBackground();

  while (Match(TokenType::SEMICOLON)) {
    if (IsEnd())
      break;

    auto right = ParseBackground();

    auto seqNode = std::make_unique<SequenceNode>();

    seqNode->left = std::move(left);
    seqNode->right = std::move(right);

    left = std::move(seqNode);
  }

  return left;
}