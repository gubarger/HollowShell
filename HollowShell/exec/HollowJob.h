#pragma once

#include <string>
#include <sys/types.h>
#include <vector>

/**
 * @struct Job
 *
 * @brief Background job information
 *
 * @var id Unique job number (1, 2, 3...)
 * @var pid Process PID
 * @var cmd Command to display
 * @var running Status (running/stopped)
 */
struct Job
{
  int id;
  pid_t pid;
  std::string cmd;
  bool running;
};

/**
 * @class HollowJob
 *
 * @brief Background process management (singleton)
 */
class HollowJob
{
public:
  static HollowJob& GetInstance();
  static void SetupSignals();

  int AddJob(pid_t pid, const std::string& cmd); // return id
  void RemoveJob(pid_t pid);
  void ReapZombies();
  void PrintJobs() const;

private:
  HollowJob() = default;

private:
  std::vector<Job> jobs_;
  int nextId_ = 1;
};