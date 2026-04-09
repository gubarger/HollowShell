#include "IgnoreSignal.h"
#include <iostream>

volatile sig_atomic_t gotSigint = 0;

static void
HandleSigaction(int)
{
  gotSigint = 1;
  write(STDOUT_FILENO, "\n", 1);
}

void
SetupSignals()
{
  signal(SIGINT, HandleSigaction);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
}