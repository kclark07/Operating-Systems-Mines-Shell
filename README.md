# Operating-Systems-Mines-Shell


Program 1


What it is:
  - A custom command-line shell written in C++ that mimics basic Unix shell behavior.

What it does:
  - Runs system commands like ls, cd, emacs, etc.

Supports:
  - Piping (|)
  - Redirection (<, >)
  - Background commands (&)
  - Variable assignment (VAR=value)
  - Script file execution

Commands it runs:
  - Built-in: cd, clear, exit
  - External: Anything executable via execvp() (e.g., ls, cat, emacs)

How to run it (on Ubuntu):
  - sudo apt install g++    (if not already installed)
  - g++ -o MinesShell MinesShell.cpp
  - ./MinesShell         # Run interactively
  - ./MinesShell file.txt  # Run commands from script

How it works:
  - Shows a mish> prompt
  - Takes user input
  - Parses commands
  - Handles pipes, redirection, background & built-ins
  - Executes using fork() + execvp()
