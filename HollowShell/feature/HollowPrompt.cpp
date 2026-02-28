#include "HollowPrompt.h"
#include <array>
#include <iostream>
#include <limits.h>
#include <memory>
#include <pwd.h>
#include <unistd.h>
#include <vector>

// Nerd Fonts
#define ICON_OS "\uf31a"
#define ICON_DIR "\uf07c"
#define ICON_GIT "\ue0a0"
#define ICON_CHECK "\uf00c"
#define ICON_CROSS "\uf00d"
#define ARROW_RIGHT "\ue0b0"

#define C_RESET "\033[0m"

void
HollowPrompt::PrintSegment(const std::string& text, int fg, int bg, int nextBg)
{
  std::cout << "\033[38;5;" << fg << "m"
            << "\033[48;5;" << bg << "m"
            << " " << text << " ";

  if (nextBg >= 0) {
    std::cout << "\033[38;5;" << bg << "m"
              << "\033[48;5;" << nextBg << "m" << ARROW_RIGHT;
  } else {
    std::cout << C_RESET << "\033[38;5;" << bg << "m" << ARROW_RIGHT << C_RESET
              << " ";
  }
}

std::string
HollowPrompt::ShortenPath(const std::string& cwd)
{
  std::string home;
  const char* homeEnv = getenv("HOME");

  if (homeEnv) {
    home = homeEnv;
  } else {
    struct passwd* pw = getpwuid(getuid());

    if (pw) {
      home = pw->pw_dir;
    }
  }

  if (!home.empty() && cwd.find(home) == 0) {
    if (cwd.length() == home.length()) {
      return "~";
    } else {
      return "~" + cwd.substr(home.length());
    }
  }
  return cwd;
}

std::string
HollowPrompt::GetGitBranch()
{
  std::string branch;
  std::array<char, 256> buffer;

  std::unique_ptr<FILE, decltype(&pclose)> pipe(
    popen("git rev-parse --abbrev-ref HEAD 2>/dev/null", "r"), pclose);

  if (!pipe) {
    return "";
  }

  if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    branch = buffer.data();

    if (!branch.empty() && branch.back() == '\n') {
      branch.pop_back();
    }

    if (branch == "HEAD") {
      std::unique_ptr<FILE, decltype(&pclose)> pipe2(
        popen("git rev-parse --short HEAD 2>/dev/null", "r"), pclose);

      if (pipe2 &&
          fgets(buffer.data(), buffer.size(), pipe2.get()) != nullptr) {
        branch = buffer.data();
        if (!branch.empty() && branch.back() == '\n') {
          branch.pop_back();
        }
        branch = ":" + branch;
      }
    }
  }

  return branch;
}

void
HollowPrompt::PrintPrompt(int lastExitCode)
{
  char* rawCwd = getcwd(NULL, 0);
  std::string cwd = rawCwd ? ShortenPath(rawCwd) : "?";

  if (rawCwd) {
    free(rawCwd);
  }

  std::string gitBranch = GetGitBranch();

  int bgStatus = (lastExitCode == 0) ? 22 : 160;
  int bgDir = 31;  // Blue
  int bgGit = 208; // Orange

  // Status
  std::string statusIcon = (lastExitCode == 0) ? ICON_OS " " : ICON_CROSS " ";
  statusIcon += std::to_string(lastExitCode);

  PrintSegment(statusIcon, 15 /*White*/, bgStatus, bgDir);

  // Dir
  int nextForDir = gitBranch.empty() ? -1 : bgGit;
  PrintSegment(ICON_DIR " " + cwd, 15 /*White*/, bgDir, nextForDir);

  // Git
  if (!gitBranch.empty()) {
    PrintSegment(ICON_GIT " " + gitBranch, 0 /*Black*/, bgGit, -1);
  }

  std::cout << "\n$ ";
  std::cout.flush();
}