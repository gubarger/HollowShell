#pragma once
#include <string>

/**
 * @class HollowPrompt
 * @brief Generates a colored prompt with useful information
 *
 * Color scheme:
 * - Green (22): last command successful (code 0)
 * - Red (160): last command failed
 * - Blue (31): directory
 * - Orange (208): git branch
 */
class HollowPrompt
{
public:
  void PrintPrompt(int lastExitCode);

private:
  std::string GetGitBranch();
  std::string ShortenPath(const std::string& cwd);
  void PrintSegment(const std::string& text, int fg, int bg, int nextBg);
};