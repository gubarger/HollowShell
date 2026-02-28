#pragma once

#include <memory>
#include <string>
#include <vector>

#include "HollowLexer.h"

struct ASTNode
{
  virtual ~ASTNode() = default;
};

struct Redirection
{
  TokenType type;
  std::string target; // File name or descriptor
  int sourceFd = -1;
};

struct CommandNode : ASTNode
{
  std::string command;
  std::vector<std::string> args;
  std::vector<Redirection> redirects;
};

struct PipelineNode : ASTNode
{
  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
};

struct LogicalNode : ASTNode
{
  TokenType op;

  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
};

struct BackgroundNode : ASTNode
{
  std::unique_ptr<ASTNode> command;
};

struct SequenceNode : ASTNode
{
  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
};

struct SubshellNode : public ASTNode
{
  std::unique_ptr<ASTNode> command;
  std::vector<Redirection> redirects;
};