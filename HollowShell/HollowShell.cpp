// HollowShell.cpp: определяет точку входа для приложения.

#include <iostream>

// clang-format off
#include "exec/HollowEnvironment.h"
#include "exec/HollowExecutor.h"
#include "exec/HollowJob.h"
#include "feature/HollowPrompt.h"
#include "grammar/HollowLexer.h"
#include "grammar/HollowParser.h"
#include "utils/IgnoreSignal.h"
// clang-format on

#include <sys/wait.h>

/**
 * @file main.cpp
 *
 * @brief Shell Main Loop (REPL: Read-Eval-Print Loop)
 */
int
main()
{
  setpgid(0, 0);
  tcsetpgrp(STDIN_FILENO, getpgrp());

  // Setting signal handler
  SetupSignals();

  HollowExecutor executor;
  HollowPrompt prompt;

  int lastEc = 0; // For prompt and $?

  while (true) {
    HollowJob::GetInstance().ReapZombies();

    gotSigint = 0;

    prompt.PrintPrompt(lastEc);

    std::string input;

    if (!std::getline(std::cin, input)) {
      if (gotSigint) {
        gotSigint = 0;
        std::cin.clear();

        continue;
      }

      // Ctrl+D (EOF)
      std::cout << "exit" << std::endl;
      break;
    }

    if (gotSigint) {
      gotSigint = 0;
      continue;
    }

    if (input.empty()) {
      continue; // Skip
    }

    try {
      // Lexer
      HollowLexer lexer(input);
      auto tokens = lexer.Tokenize();

      // Parser
      HollowParser parser(tokens);
      auto ast = parser.Parse();

      // Executor
      if (ast) {
        lastEc = executor.Execute(ast.get());
        HollowEnvironment::GetInstance().SetLastExitCode(lastEc);
      }
    } catch (const std::exception& e) {
      lastEc = 1;

      std::cout << "Error: " << e.what() << std::endl;
    }
  }

  return 0;
}
