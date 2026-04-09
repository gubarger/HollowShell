#pragma once

#include <csignal>
#include <termios.h>
#include <unistd.h>

extern volatile sig_atomic_t gotSigint;

void
SetupSignals();