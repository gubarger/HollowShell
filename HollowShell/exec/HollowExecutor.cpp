#include "HollowExecutor.h"
#include "HollowBuiltins.h"
#include "HollowEnvironment.h"
#include "HollowJob.h"

#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include "../utils/HandlingExecute.h"

int
HollowExecutor::Execute(ASTNode* node)
{
  if (!node)
    return 0;

  if (auto cmd = dynamic_cast<CommandNode*>(node))
    return Visit(cmd);
  if (auto pipe = dynamic_cast<PipelineNode*>(node))
    return Visit(pipe);
  if (auto seq = dynamic_cast<SequenceNode*>(node))
    return Visit(seq);
  if (auto sub = dynamic_cast<SubshellNode*>(node))
    return Visit(sub);
  if (auto logic = dynamic_cast<LogicalNode*>(node))
    return Visit(logic);
  if (auto bg = dynamic_cast<BackgroundNode*>(node))
    return Visit(bg);

  return 1;
}

int
HollowExecutor::Visit(CommandNode* node)
{
  if (node->command.empty())
    return 0;

  ExpandNode(node);

  // Vars
  if (node->command.find('=') != std::string::npos && node->args.empty()) {
    size_t pos = node->command.find('=');
    std::string key = node->command.substr(0, pos);
    std::string val = node->command.substr(pos + 1);

    val = HollowEnvironment::GetInstance().Expand(val);

    HollowEnvironment::GetInstance().Set(key, val);

    return 0;
  }

  // Builtin
  if (HollowBuiltin::GetInstance().IsBuiltin(node->command)) {
    int savedStdin = dup(STDIN_FILENO);
    int savedStdout = dup(STDOUT_FILENO);
    int savedStderr = dup(STDERR_FILENO);

    if (!ApplyRedirects(node->redirects))
      return 1;

    int ret = HollowBuiltin::GetInstance().Execute(node->command, node->args);

    dup2(savedStdin, STDIN_FILENO);
    dup2(savedStdout, STDOUT_FILENO);
    dup2(savedStderr, STDERR_FILENO);

    close(savedStdin);
    close(savedStdout);
    close(savedStderr);

    return ret;
  }

  // OS commands
  pid_t pid = fork();

  if (pid == 0) { // Child
    // Signal recovery
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    if (!ApplyRedirects(node->redirects))
      exit(1);

    std::string execPath = FindCommand(node->command);

    if (execPath.empty()) {
      std::cerr << node->command << ": command not found\n";

      exit(127);
    }
    std::vector<char*> args;

    args.push_back(const_cast<char*>(node->command.c_str()));

    for (const auto& arg : node->args)
      args.push_back(const_cast<char*>(arg.c_str()));

    args.push_back(nullptr);

    execv(execPath.c_str(), args.data());
    perror("Execv failed");

    exit(126);
  } else if (pid < 0) {
    perror("Fork");

    return 1;
  } else {
    int status;

    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
      return WEXITSTATUS(status);

    return 1;
  }
}

int
HollowExecutor::Visit(PipelineNode* node)
{
  int pipefd[2];

  if (pipe(pipefd) == -1) {
    perror("Pipe");

    return 1;
  }

  int lPid = fork();

  if (lPid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    exit(Execute(node->left.get()));
  }

  int rPid = fork();

  if (rPid == 0) {
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    exit(Execute(node->right.get()));
  }

  close(pipefd[0]);
  close(pipefd[1]);

  int status;

  waitpid(lPid, nullptr, 0);
  waitpid(rPid, &status, 0);

  return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

int
HollowExecutor::Visit(LogicalNode* node)
{
  int status = Execute(node->left.get());

  if (node->op == TokenType::AND) {
    if (status == 0)
      return Execute(node->right.get());

    return status;
  } else if (node->op == TokenType::OR) {
    if (status != 0)
      return Execute(node->right.get());

    return status;
  }

  return 0;
}

int
HollowExecutor::Visit(BackgroundNode* node)
{
  pid_t pid = fork();

  if (pid == 0) {
    int nullfd = open("/dev/null", O_RDONLY);

    dup2(nullfd, STDIN_FILENO);
    close(nullfd);

    signal(SIGINT, SIG_DFL);

    exit(Execute(node->command.get()));
  } else {
    std::string cmdStr =
      "Background task";

    int jobId = HollowJob::GetInstance().AddJob(pid, cmdStr);

    std::cout << "[" << jobId << "] " << pid << std::endl;

    return 0;
  }
}

int
HollowExecutor::Visit(SequenceNode* node)
{
  Execute(node->left.get());

  return Execute(node->right.get());
}

int
HollowExecutor::Visit(SubshellNode* node)
{
  pid_t pid = fork();

  if (pid == 0) { // CHILD (Subshell Environment)
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    if (!ApplyRedirects(node->redirects))
      exit(1);

    // Command
    int status = Execute(node->command.get());

    // Death
    exit(status);
  } else if (pid < 0) {
    perror("Fork subshell");

    return 1;
  } else {
    // PARENT
    int status;

    waitpid(pid, &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  }
}

void
HollowExecutor::ExpandNode(CommandNode* node)
{
  auto& env = HollowEnvironment::GetInstance();

  node->command = env.Expand(node->command);

  for (auto& arg : node->args)
    arg = env.Expand(arg);

  for (auto& red : node->redirects)
    red.target = env.Expand(red.target);
}

bool
HollowExecutor::ApplyRedirects(const std::vector<Redirection>& redirects)
{
  for (const auto& r : redirects) {
    if (r.type == TokenType::HEREDOC) {
      HandleHeredoc(r.target);

      continue;
    }

    int fd = -1;
    int targetFd = r.sourceFd;

    switch (r.type) {
      case TokenType::REDIRECT_OUT:
        fd = open(r.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        break;
      case TokenType::APPEND_OUT:
        fd = open(r.target.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        break;
      case TokenType::REDIRECT_IN:
        fd = open(r.target.c_str(), O_RDONLY);
        break;
      case TokenType::REDIRECT_RW:
        fd = open(r.target.c_str(), O_RDWR | O_CREAT, 0644);
        break;

      case TokenType::REDIRECT_DUP:
        try {
          int destFd = std::stoi(r.target);

          if (dup2(destFd, targetFd) < 0) {
            perror("Dup2 failed");

            return false; // ERROR
          }
        } catch (...) {
          std::cerr << "Redirection error: target must be integer\n";

          return false; // ERROR
        }
        continue; // fd didn't open

      case TokenType::HERE_STRING: {
        int pipefd[2];

        if (pipe(pipefd) == -1) {
          perror("Pipe");

          return false;
        } // ERROR

        std::string content = r.target + "\n";

        FULL_WRITE(pipefd[1], content.c_str(), content.size());
        close(pipefd[1]);

        if (dup2(pipefd[0], targetFd) < 0) {
          perror("Dup2");

          return false;
        } // ERROR

        close(pipefd[0]);

        continue;
      }
      default:
        continue;
    }

    if (fd < 0) {
      perror(("Open failed: " + r.target).c_str());

      return false;
    }

    if (dup2(fd, targetFd) < 0) {
      perror("Dup2 failed");
      close(fd);

      return false;
    }

    close(fd);
  }

  return true;
}

void
HollowExecutor::HandleHeredoc(const std::string& delimiter)
{
  int pipefd[2];

  if (pipe(pipefd) == -1) {
    perror("Heredoc pipe");

    return;
  }

  std::string line;
  bool expand = true;
  std::string actualDelimiter = delimiter;

  if ((delimiter.front() == '\'' && delimiter.back() == '\'') ||
      (delimiter.front() == '"' && delimiter.back() == '"')) {
    expand = false;
    actualDelimiter = delimiter.substr(1, delimiter.size() - 2);
  }

  while (true) {
    std::cout << "> ";

    if (!std::getline(std::cin, line))
      break;

    if (line == actualDelimiter)
      break;

    if (expand)
      line = HollowEnvironment::GetInstance().Expand(line);

    line += "\n";

    FULL_WRITE(pipefd[1], line.c_str(), line.size());
  }

  close(pipefd[1]);
  dup2(pipefd[0], STDIN_FILENO);
  close(pipefd[0]);
}

std::string
HollowExecutor::FindCommand(const std::string& cmd)
{
  if (cmd.find('/') != std::string::npos) {
    if (access(cmd.c_str(), X_OK) == 0)
      return cmd;

    return "";
  }

  // Find in PATH
  const char* olPath = getenv("PATH");

  if (!olPath)
    return "";

  std::string path(olPath);
  std::stringstream ss(path);
  std::string dir;

  while (std::getline(ss, dir, ':')) {
    std::string phFull = dir + "/" + cmd;

    if (access(phFull.c_str(), X_OK) == 0)
      return phFull;
  }

  return "";
}