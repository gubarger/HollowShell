#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class HollowEnvironment
 *
 * @brief Managing environment variables and substitutions
 *
 * Support:
 * - $VAR, ${VAR} - variables
 * - $? - return code
 * - ~, ~user - home directory
 * - $(command), `command` - command substitution
 */
class HollowEnvironment
{
public:
  static HollowEnvironment& GetInstance();

  std::string Get(const std::string& key) const;
  void Set(const std::string& key,
           const std::string& val,
           bool isExport = false);
  void Unset(const std::string& key);
  std::string Expand(const std::string& input) const;

  void SetLastExitCode(int code);
  int GetLastExitCode() const;

private:
  HollowEnvironment();

  std::string ExecuteCommandSubstitution(const std::string& cmd) const;
  std::string ExpandCommandSubstitutions(const std::string& input) const;

private:
  std::unordered_map<std::string, std::string> vars_;
  int lastEc_ = 0;
};