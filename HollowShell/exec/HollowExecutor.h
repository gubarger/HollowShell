#pragma once

#include "../grammar/HollowAST.h"

/**
 * @class HollowExecutor
 *
 * @brief AST traversal and command execution
 *
 * - CommandNode -> fork/exec or builtin
 * - PipelineNode -> creating a pipe between processes
 * - LogicalNode -> conditional execution
 * - BackgroundNode -> fork without wait
 * - SequenceNode -> sequential execution
 * - SubshellNode -> fork with a new environment
 */
class HollowExecutor
{
public:
  /**
   * @public
   *
   * @brief Executes the command node
   *
   * @param node: root node
   *
   * @return code process (0 = success)
   */
  int Execute(ASTNode* node);

private:
  int Visit(CommandNode* node);
  int Visit(PipelineNode* node);
  int Visit(LogicalNode* node);
  int Visit(BackgroundNode* node);
  int Visit(SequenceNode* node);
  int Visit(SubshellNode* node);

  void ExpandNode(CommandNode* node);

  /**
   * @brief Applies redirections to the current process
   *
   * Supported types:
   * - File: <, >, >>, <>
   * - Descriptor: 2>, &>, >&
   * - Special: << (heredoc), <<< (here string)
   *
   * @param redirects Vector of redirections
   *
   * @return true if successful, false on error
   */
  bool ApplyRedirects(const std::vector<Redirection>& redirects);
  void HandleHeredoc(const std::string& delimiter);
  std::string FindCommand(const std::string& cmd);
};