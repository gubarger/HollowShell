#include "HollowJob.h"

#include <algorithm>
#include <iostream>
#include <sys/wait.h>

HollowJob&
HollowJob::GetInstance()
{
  static HollowJob instance;

  return instance;
}

void
HollowJob::SetupSignals()
{
  signal(SIGINT, SIG_IGN);  // Ignore Ctrl+C
  signal(SIGTSTP, SIG_IGN); // Ignore Ctrl+Z
}

int
HollowJob::AddJob(pid_t pid, const std::string& cmd)
{
  Job job{ nextId_++, pid, cmd, true };

  jobs_.push_back(job);

  return job.id;
}

void
HollowJob::RemoveJob(pid_t pid)
{
  auto it = std::remove_if(jobs_.begin(), jobs_.end(), [pid](const Job& job) {
    return job.pid == pid;
  });

  if (it != jobs_.end())
    jobs_.erase(it, jobs_.end());
}

void
HollowJob::ReapZombies()
{
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      std::cout << "\n[Job " << pid
                << "] Done.\n";

      RemoveJob(pid);
    }
    // WIFSTOPPED - Ctrl+Z - STOPPED
  }
}

void
HollowJob::PrintJobs() const
{
  for (const auto& job : jobs_) {
    std::cout << "[" << job.id << "] " << job.pid << " "
              << (job.running ? "Running" : "Done") << " " << job.cmd
              << std::endl;
  }
}