#include "HollowEnvironment.h"

#include <cstdlib>
#include <pwd.h>
#include <regex>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

HollowEnvironment&
HollowEnvironment::GetInstance()
{
  static HollowEnvironment instance;

  return instance;
}

std::string
HollowEnvironment::Get(const std::string& key) const
{
  // Find in cache
  auto it = vars_.find(key);

  if (it != vars_.end())
    return it->second;

  const char* val = getenv(key.c_str());

  return val ? std::string(val) : "";
}

void
HollowEnvironment::Set(const std::string& key,
                       const std::string& val,
                       bool isExport)
{
  vars_[key] = val;

  if (isExport)
    setenv(key.c_str(), val.c_str(), 1);
}

void
HollowEnvironment::Unset(const std::string& key)
{
  vars_.erase(key);

  unsetenv(key.c_str());
}

std::string
HollowEnvironment::Expand(const std::string& input) const
{
  std::string result = input;

  result = ExpandCommandSubstitutions(result);

  if (result.starts_with("~")) {
    size_t slashPos = result.find('/');

    if (slashPos != std::string::npos && slashPos > 1) { // ~user/path
      std::string user = result.substr(1, slashPos - 1);

      struct passwd* pw = getpwnam(user.c_str());

      if (pw) {
        std::string homeDir = pw->pw_dir;

        result.replace(0, slashPos, homeDir);
      }
    } else if (slashPos == std::string::npos && result.length() > 1) { // ~user
      std::string user = result.substr(1);

      struct passwd* pw = getpwnam(user.c_str());

      if (pw)
        result = pw->pw_dir;
    } else { // ~ or ~/
      const char* home = getenv("HOME");

      if (!home) {
        struct passwd* pw = getpwuid(getuid());

        if (pw)
          home = pw->pw_dir;
      }

      if (home) {
        if (result == "~")
          result = home;
        else if (result.starts_with("~/"))
          result.replace(0, 1, home);
      }
    }
  }

  std::string expanded;
  expanded.reserve(result.size());

  for (size_t i = 0; i < result.size(); ++i) {
    if (result[i] == '$') {
      if (i + 1 < result.size() && result[i + 1] == '(') {
        expanded += '$';

        continue;
      }

      // Handling $?
      if (result[i + 1] == '?') {
        expanded += std::to_string(GetLastExitCode());
        i++;

        continue;
      }

      size_t start = i + 1;
      size_t len = 0;

      // ${VAR}
      bool brace = false;

      if (result[start] == '{') {
        brace = true;
        start++;
      }

      while (start + len < result.size()) {
        char c = result[start + len];

        if (brace) {
          if (c == '}')
            break;
        } else {
          if (!isalnum(c) && c != '_')
            break;
        }

        len++;
      }

      std::string key = result.substr(start, len);

      expanded += Get(key);

      i = start + len;

      if (brace && i < result.size() && result[i] == '}') {
        // Pick '}' and skip `}`
      } else if (brace) {
        // ERROR
      } else
        i--;
    } else
      expanded += result[i];
  }

  return expanded;
}

void
HollowEnvironment::SetLastExitCode(int code)
{
  lastEc_ = code;
}

int
HollowEnvironment::GetLastExitCode() const
{
  return lastEc_;
}

HollowEnvironment::HollowEnvironment() {}

std::string
HollowEnvironment::ExecuteCommandSubstitution(const std::string& cmd) const
{
  int pipefd[2];

  if (pipe(pipefd) == -1)
    return "";

  pid_t pid = fork();

  if (pid == 0) { // Child
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);

    exit(127);
  } else if (pid > 0) { // Parent
    close(pipefd[1]);

    std::string output;
    char buffer[128];
    ssize_t bytesRead;

    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0)
      output.append(buffer, bytesRead);

    close(pipefd[0]);
    waitpid(pid, nullptr, 0);

    // Delete trailing newline
    while (!output.empty() &&
           (output.back() == '\n' || output.back() == '\r')) {
      output.pop_back();
    }

    return output;
  }

  return "";
}

std::string
HollowEnvironment::ExpandCommandSubstitutions(const std::string& input) const
{
  std::string result = input;
  size_t stPos = 0;

  // $(
  while ((stPos = result.find("$(", stPos)) != std::string::npos) {
    size_t openCount = 1;
    size_t endPos = stPos + 2;

    bool found = false;

    while (endPos < result.length()) {
      if (result[endPos] == '(')
        openCount++;
      else if (result[endPos] == ')') {
        openCount--;

        if (openCount == 0) {
          found = true;

          break;
        }
      }

      endPos++;
    }

    if (!found)
      break;

    // $(CMD) -> CMD
    // $(echo $(date))
    std::string innerCmd = result.substr(stPos + 2, endPos - (stPos + 2));
    innerCmd = ExpandCommandSubstitutions(innerCmd);

    std::string output = ExecuteCommandSubstitution(innerCmd);

    // Change $(...) on result
    result.replace(stPos, endPos - stPos + 1, output);

    stPos += output.length();
  }

  // `...`
  stPos = 0;

  while ((stPos = result.find('`', stPos)) != std::string::npos) {
    size_t endPos = result.find('`', stPos + 1);

    if (endPos == std::string::npos)
      break;

    std::string innerContent = result.substr(stPos + 1, endPos - (stPos + 1));

    if (innerContent.find('`') != std::string::npos) {
      stPos = endPos + 1;

      continue;
    }

    std::string output = ExecuteCommandSubstitution(innerContent);

    result.replace(stPos, endPos - stPos + 1, output);
  }

  return result;
}