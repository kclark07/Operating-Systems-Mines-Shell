# Mines Shell (mish)

A custom Unix-style shell developed in C++ for an Operating Systems course. This project implements a simplified command-line interpreter that can execute programs, manage processes, handle environment variables, support input/output redirection, enable background execution, and facilitate inter-process communication via pipes.

## Overview

Mines Shell (`mish`) is a lightweight Linux shell designed to demonstrate fundamental operating system concepts, including:

* Process creation and management
* Parent and child process communication
* Command parsing and execution
* Environment variable handling
* Input and output redirection
* Background process execution
* Pipe-based process communication

The shell continuously accepts user commands, creates child processes to execute them, and returns control to the user when execution is complete.

---

## Features

### Command Execution

Execute standard Linux commands using the system PATH.

Example:

```bash
ls -la
pwd
whoami
```

### Built-in Commands

#### Exit Shell

```bash
exit
```

Terminates the shell process.

#### Change Directory

```bash
cd /home/user/Documents
```

Changes the current working directory.

#### Environment Variable Assignment

```bash
PATH=/bin:/usr/bin
```

Creates or updates environment variables directly within the shell.

---

### Input Redirection

Redirect file contents into a program.

```bash
sort < input.txt
```

---

### Output Redirection

Redirect command output to a file.

```bash
ls -la > output.txt
```

If the file already exists, it is overwritten.

---

### Input and Output Redirection

```bash
sort < unsorted.txt > sorted.txt
```

Reads input from one file and writes output to another.

---

### Pipes

Connect the output of one command to the input of another.

```bash
cat file.txt | grep error
```

Multiple pipes are supported.

```bash
cat file.txt | grep error | sort
```

---

### Background Processes

Execute commands without blocking the shell.

```bash
sleep 30 &
```

The shell immediately returns to accept additional commands while the process executes in the background.

---

### Error Handling

The shell validates command syntax and reports:

* Invalid commands
* Improper pipe usage
* Multiple redirection errors
* Missing files
* Invalid built-in command arguments

Error messages are displayed to standard error.

---

## Technologies Used

* C++
* POSIX System Calls
* Linux/Unix Process Management
* File Descriptor Manipulation
* Process Synchronization

Key APIs:

* `fork()`
* `execvp()`
* `waitpid()`
* `pipe()`
* `dup2()`
* `open()`
* `chdir()`
* `setenv()`
* `unsetenv()`

---

## Building the Project

Compile the shell using g++:

```bash
g++ -o shell MinesShell.cpp
```

This creates an executable named `shell`.

---

## Running the Shell

Launch the shell:

```bash
./shell
```

Example session:

```bash
$ ./shell

mish> pwd
/home/user

mish> ls
file.txt  notes.txt

mish> cat file.txt | grep test

mish> PATH=/bin:/usr/bin

mish> cd Documents

mish> exit
```

---

## Learning Outcomes

This project strengthened understanding of:

* Operating system process management
* Process creation and termination
* Linux shell architecture
* Inter-process communication
* File descriptor redirection
* Concurrent process execution
* Systems programming in C++

---

## Author

**Kaeli Clark**

Developed for CSC 458 – Operating Systems.

