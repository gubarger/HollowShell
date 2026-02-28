#include "HollowBuiltins.h"
#include "HollowEnvironment.h"
#include "HollowJob.h"

#include <cstdlib>
#include <iostream>
#include <unistd.h>

HollowBuiltin&
HollowBuiltin::GetInstance()
{
  static HollowBuiltin instance;

  return instance;
}

bool
HollowBuiltin::IsBuiltin(const std::string& cmd) const
{
  return builtins_.find(cmd) != builtins_.end();
}

int
HollowBuiltin::Execute(const std::string& cmd,
                       const std::vector<std::string> args)
{
  if (builtins_.find(cmd) != builtins_.end())
    return builtins_[cmd](args);

  return 127; // Command not found
}

HollowBuiltin::HollowBuiltin()
{
  builtins_["cd"] = [this](auto&& args) { return BuiltinCD(args); };
  builtins_["exit"] = [this](auto&& args) { return BuiltinEXIT(args); };
  builtins_["help"] = [this](auto&& args) { return BuiltinHELP(args); };

  builtins_["true"] = [this](auto&& args) { return BuiltinTRUE(args); };
  builtins_[":"] = [this](auto&& args) {
    return BuiltinTRUE(args);
  }; // alias for true
  builtins_["false"] = [this](auto&& args) { return BuiltinFALSE(args); };
  builtins_["exec"] = [this](auto&& args) { return BuiltinEXEC(args); };
  builtins_["export"] = [this](auto&& args) { return BuiltinEXPORT(args); };
  builtins_["jobs"] = [](const std::vector<std::string>&) {
    HollowJob::GetInstance().PrintJobs();

    return 0;
  };
}

int
HollowBuiltin::BuiltinCD(const std::vector<std::string>& args)
{
  auto& env = HollowEnvironment::GetInstance();

  std::string targetDir;

  if (args.empty())
    targetDir = env.Get("HOME");
  else if (args[0] == "-") {
    targetDir = env.Get("OLDPWD");
    if (targetDir.empty()) {
      std::cerr << "cd: OLDPWD not set\n";

      return 1;
    }
    std::cout << targetDir << std::endl;
  } else
    targetDir = args[0];

  char currentCwd[4096];

  if (getcwd(currentCwd, sizeof(currentCwd)) == nullptr) {
    perror("getcwd");

    return 1;
  }

  if (chdir(targetDir.c_str()) != 0) {
    perror(("cd: " + targetDir).c_str());

    return 1;
  }

  env.Set("OLDPWD", currentCwd, true); // true = export

  // Update PWD
  char newCwd[4096];

  if (getcwd(newCwd, sizeof(newCwd)))
    env.Set("PWD", newCwd, true);

  return 0;
}

int
HollowBuiltin::BuiltinEXIT(const std::vector<std::string>& args)
{
  int code = 0;

  if (!args.empty()) {
    try {
      code = std::stoi(args[0]);
    } catch (...) {
      std::cerr << "Exit: numeric argument required\n";

      code = 255;
    }
  }

  exit(code);
}

int
HollowBuiltin::BuiltinHELP(const std::vector<std::string>& args)
{
  std::cout << "Builtins:\n";
  std::cout << "cd      Support\n";
  std::cout << "exit    Support\n";
  std::cout << "help    Support\n";
  std::cout << "true    Support\n";
  std::cout << "false   Support\n";
  std::cout << "export  Support\n";
  std::cout << "jobs    Support\n";

  return 0;
}

int
HollowBuiltin::BuiltinTRUE(const std::vector<std::string>& args)
{
  return 0;
}

int
HollowBuiltin::BuiltinFALSE(const std::vector<std::string>& args)
{
  return 1;
}

int
HollowBuiltin::BuiltinEXEC(const std::vector<std::string>& args)
{
  if (args.empty()) {
    // TODO: There should be logic here for "exec > file" (apply redirects
    // permanently).
    return 0;
  }

  std::vector<char*> argsC;

  for (const auto& arg : args)
    argsC.push_back(const_cast<char*>(arg.c_str()));

  argsC.push_back(nullptr);

  execvp(argsC[0], argsC.data());
  perror("Exec failed");

  return 1;
}

int
HollowBuiltin::BuiltinEXPORT(const std::vector<std::string>& args)
{
  if (args.empty()) {
    extern char** environ;

    for (char** env = environ; *env != 0; env++) {
      std::string entry(*env);

      std::cout << "declare -x " << entry << std::endl;
    }

    return 0;
  }

  for (const auto& arg : args) {
    // export VAR=VAL
    size_t pos = arg.find('=');

    if (pos != std::string::npos) {
      std::string key = arg.substr(0, pos);
      std::string val = arg.substr(pos + 1);

      HollowEnvironment::GetInstance().Set(key, val, true); // true = export
    } else {
      // export VAR
      std::string val = HollowEnvironment::GetInstance().Get(arg);

      HollowEnvironment::GetInstance().Set(arg, val, true);
    }
  }

  return 0;
}