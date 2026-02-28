#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class HollowBuiltin
 *
 * @brief Register and execute built-in commands
 *
 * Builtins are executed in the shell process itself (without forking)
 * with saving/restoring file descriptors for redirections
 */
class HollowBuiltin
{
public:
  /**
   * @public
   *
   * @brief
   *
   * @return code
   */
  using BuilinFunc = std::function<int(const std::vector<std::string>&)>;

  static HollowBuiltin& GetInstance();

  bool IsBuiltin(const std::string& cmd) const;
  int Execute(const std::string& cmd, const std::vector<std::string> args);

private:
  HollowBuiltin();

  int BuiltinCD(const std::vector<std::string>& args);
  int BuiltinEXIT(const std::vector<std::string>& args);
  int BuiltinHELP(const std::vector<std::string>& args);
  int BuiltinTRUE(const std::vector<std::string>& args);
  int BuiltinFALSE(const std::vector<std::string>& args);
  int BuiltinEXEC(const std::vector<std::string>& args);
  int BuiltinEXPORT(const std::vector<std::string>& args);

private:
  std::unordered_map<std::string, BuilinFunc> builtins_;
};