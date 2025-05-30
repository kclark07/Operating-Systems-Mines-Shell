/* Author: Kaeli Clark
 * Date: 4/26/24
 * Class: Operating Systems
 * Project: Basic Shell - revised
 */

#include <iostream> // Allows input and output stream, e.g., cin, cout.
#include <vector> // A template for creating dynamic arrays.
#include <string> // Provides string manipulation capabilities.
#include <sstream> // Allows string stream operations, useful for parsing.
#include <unistd.h> // Provides access to the POSIX operating system API.
#include <sys/wait.h> // Provides declarations for waiting for process termination.
#include <sys/stat.h> // Defines the structure of the data returned by the stat() function.
#include <cstdlib> // Defines several general-purpose functions, including memory management, random number generation, and system commands.
#include <iterator>
#include <cstring>
#include <fcntl.h>
#include <algorithm>
#include <fstream>
#include <cctype>

using namespace std;

/**
 * Splits a string into individual words using whitespace as the delimiter.
 * @param str The string to tokenize.
 * @return A vector of tokens (words).
 */
vector<string> tokenize(const string& str); // Splits a string into tokens (words) based on whitespace.

void executeCommand(const vector<string>& tokens); // Executes a command using the list of string tokens.
string getCurrentDirectory(); // Returns the current working directory as a string.
bool isBackgroundCommand(const string& cmd); // Checks if a command should be executed in the background.
vector<char*> segment_args(const vector<string>& segment) {
    vector<char*> args;
    for (const auto& arg : segment) {
        args.push_back(const_cast<char*>(arg.c_str()));  // Unsafe cast due to constness, but execvp won't modify the strings
    }
    args.push_back(nullptr);  // execvp expects a null-terminated array
    return args;
}
vector<string> tokenize(const string& str) {
    istringstream iss(str); // Creates a string stream from the input string.
    return vector<string>{istream_iterator<string>{iss}, istream_iterator<string>{}}; // Uses istream_iterator to iterate over words in the stream and collects them into a vector.
}
/**
 * Finds the index of a specific token within a vector of strings.
 * @param tokens The vector of strings.
 * @param token The string to find.
 * @return The index of the token, or -1 if not found.
 */
int findTokenIndex(const vector<string>& tokens, const string& token) {
    auto it = find(tokens.begin(), tokens.end(), token);
    return it != tokens.end() ? distance(tokens.begin(), it) : -1;
}

/**
 * Checks if a series of tokens has any syntax errors related to redirection or piping.
 * @param tokens The command tokens.
 * @return true if there are syntax errors, false otherwise.
 */
bool hasSyntaxErrors(const vector<string>& tokens) {
    int redirectInCount = 0, redirectOutCount = 0, pipeCount = 0;
    string prevToken = "";

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "<") {
            redirectInCount++;
            if (i == 0 || tokens[i - 1] == "|" || redirectInCount > 1) {
                cerr << "mish: multiple input redirect or pipe" << endl;
                return true;
            }
        } else if (tokens[i] == ">") {
            redirectOutCount++;
            if (i == 0 || tokens[i - 1] == "|" || redirectOutCount > 1) {
                cerr << "mish: multiple output redirect or pipe" << endl;
                return true;
            }
        } else if (tokens[i] == "|") {
            pipeCount++;
            if (prevToken == "|" || i == 0 || i == tokens.size() - 1) {
                cerr << "mish: syntax error, unexpected PIPE, expecting STRING" << endl;
                return true;
            }
        }
        prevToken = tokens[i];
    }

    return false;
}

/**
 * Executes a command by forking and using execvp.
 * @param tokens The command and its arguments.
 */
void executeCommand(const vector<string>& tokens) {
    int redirectInIndex = findTokenIndex(tokens, "<");
    int redirectOutIndex = findTokenIndex(tokens, ">");
    int saved_stdout = -1;
    int saved_stdin = -1;

    vector<char*> args;

    // Prepare the arguments for execvp, excluding redirection tokens and file names
    for (int i = 0; i < tokens.size(); ++i) {
        if (i != redirectInIndex && i != redirectOutIndex &&
            (redirectInIndex == -1 || i != redirectInIndex + 1) &&
            (redirectOutIndex == -1 || i != redirectOutIndex + 1)) {
            args.push_back(const_cast<char*>(tokens[i].c_str()));
        }
    }
    args.push_back(nullptr);  // execvp expects a null-terminated array

    pid_t pid = fork();
    if (pid == -1) {
        cerr << "Failed to fork process" << endl;
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        // Handle input redirection
        if (redirectInIndex != -1 && redirectInIndex + 1 < tokens.size()) {
            int fd = open(tokens[redirectInIndex + 1].c_str(), O_RDONLY);
            if (fd == -1) {
                cerr << "Failed to open input redirection file" << endl;
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        // Handle output redirection
        if (redirectOutIndex != -1 && redirectOutIndex + 1 < tokens.size()) {
            int fd = open(tokens[redirectOutIndex + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("Failed to open output redirection file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // Execute the command
        execvp(args[0], args.data());


        // If execvp returns, it's an error
        if (errno == ENOENT) {
            cerr << "mish: '" << args[0] << "': No such file or directory" << endl;
        }
//        else {
//            cerr << "mish: Error executing '" << args[0] << "': " << strerror(errno) << endl;
//        }
        exit(EXIT_FAILURE);

    } else { // Parent process
        if (redirectOutIndex != -1) {
            // Save stdout if it's being redirected
            saved_stdout = dup(STDOUT_FILENO);
        }
        if (redirectInIndex != -1) {
            // Save stdin if it's being redirected
            saved_stdin = dup(STDIN_FILENO);
        }

        int status;
        waitpid(pid, &status, 0); // Wait for the child process to finish
        //cout << "Command executed, child exited with status " << WEXITSTATUS(status) << endl;

        // Restore original stdout and stdin if they were redirected
        if (saved_stdout != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        if (saved_stdin != -1) {
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdin);
        }
    }
}

/**
 * Executes a series of piped commands.
 * @param tokens The complete command line input split into tokens.
 */
void executePipedCommand(const vector<string>& tokens) {
    vector<vector<string>> commands;  // Store individual commands separated by pipes
    vector<int> fds;             // Store file descriptors for pipes
    vector<pid_t> child_pids;         // Store child process IDs

    // Split the input into separate commands at each pipe symbol
    auto startIt = tokens.begin();
    while (startIt != tokens.end()) {
        auto pipeIt = find(startIt, tokens.end(), "|");
        commands.emplace_back(startIt, pipeIt);
        if (pipeIt != tokens.end()) startIt = next(pipeIt);
        else break;
    }

    int in_fd = STDIN_FILENO;  // Input file descriptor starts as STDIN

    // Loop over commands to set up pipes and fork processes
    for (size_t i = 0; i < commands.size(); ++i) {
        int fd[2];
        // Set up pipe for all but the last command
        if (i < commands.size() - 1 && pipe(fd) == -1) {
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            fds.push_back(fd[0]); // Save the read end to close later
            fds.push_back(fd[1]); // Save the write end to close later
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // Child process
            // Handle input redirection for the first command
            if (i == 0) {
                int redirectInIndex = findTokenIndex(commands[i], "<");
                if (redirectInIndex != -1 && redirectInIndex + 1 < commands[i].size()) {
                    in_fd = open(commands[i][redirectInIndex + 1].c_str(), O_RDONLY);
                    if (in_fd == -1) {
                        perror("syntax error, unexpected PIPE, expecting STRING");
                        //exit(EXIT_FAILURE);
                    }
                    dup2(in_fd, STDIN_FILENO);
                    close(in_fd);
                    commands[i].erase(commands[i].begin() + redirectInIndex, commands[i].begin() + redirectInIndex + 2);
                }
            }

            // Redirect input from the previous pipe
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Set up output redirection for all but the last command
            if (i < commands.size() - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }

            // Handle output redirection for the last command
            if (i == commands.size() - 1) {
                int redirectOutIndex = findTokenIndex(commands[i], ">");
                if (redirectOutIndex != -1 && redirectOutIndex + 1 < commands[i].size()) {
                    int out_fd = open(commands[i][redirectOutIndex + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (out_fd == -1) {
                        perror("open for output redirection failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                    commands[i].erase(commands[i].begin() + redirectOutIndex,
                                      commands[i].begin() + redirectOutIndex + 2);
                }
            }

            // Execute the command
            vector<char *> args = segment_args(commands[i]);
            execvp(args[0], args.data());
            cerr << "mish: '" << args[0] << "': No such file or directory" << endl;
            exit(EXIT_FAILURE);
        } else {
            child_pids.push_back(pid);  // Parent process, store child pid
            if (in_fd != STDIN_FILENO) {
                close(in_fd);  // Close the read end of the previous pipe
            }
            if (i < commands.size() - 1) {
                close(fd[1]);  // Close the write end of the current pipe
                in_fd = fd[0]; // The next command will read from here
            }
        }
    }

    for (int fd : fds) {
        close(fd);
    }

    // Wait for all the child processes to finish
    for (pid_t pid : child_pids) {
        int status;
        waitpid(pid, &status, 0);  // This waits for the specific child process to finish
    }

    cout << flush;

}

/**
 * Retrieves the current working directory as a string.
 * @return The current working directory.
 */
string getCurrentDirectory() {
    char* cwd = getcwd(nullptr, 0); // Calls getcwd with nullptr and 0, which allocates a buffer of sufficient size automatically.
    string currentDirStr(cwd); // Constructs a C++ string from the C-style string returned by getcwd.
    free(cwd); // Frees the memory allocated by getcwd since it's no longer needed.
    return currentDirStr; // Returns the current working directory as a C++ string.
}

/**
 * Checks if a command is intended to be run in the background.
 * @param cmd The command string.
 * @return true if the command ends with an '&', false otherwise.
 */
bool isBackgroundCommand(const string& cmd) {
    return !cmd.empty() && cmd.back() == '&'; // Checks if the command is not empty and if the last character is '&'.
}

/**
 * Handles environment variable assignment within the shell.
 * @param input The full input string containing the assignment.
 */
void handleVariableAssignment(const string& input) {
    size_t equalPos = input.find('='); // Finds the position of the '=' character.
    if (equalPos != string::npos) { // Checks if '=' was found.
        string varName = input.substr(0, equalPos); // Extracts the variable name from the input.
        string value = input.substr(equalPos + 1); // Extracts the variable value.

        if (varName == "PATH" && value.empty()) { // Special case for PATH variable being set to empty.
            unsetenv("PATH"); // Unsets the PATH environment variable.
        } else {
            setenv(varName.c_str(), value.c_str(), 1); // Sets or updates the environment variable. The '1' argument means overwrite existing value.
        }
    } else {
        cerr << "Invalid assignment format." << endl; // Prints an error message if the input format is incorrect.
    }
}

/**
 * Executes a command in the background by forking and not waiting for the process to finish.
 * @param tokens The command and its arguments.
 */
void executeCommandInBackground(const vector<string>& tokens) {
    pid_t pid = fork();
    if (pid == 0) {
        vector<char*> args = segment_args(tokens);
        execvp(args[0], args.data());
        cerr << "mish: '" << args[0] << "': No such file or directory" << endl;
        exit(EXIT_FAILURE);
    }
}

/**
 * Checks for the presence of multiple redirections or an improper mix of pipes and redirections.
 * This function is intended to validate the syntax of shell commands where only certain arrangements
 * of pipes and redirections are syntactically valid.
 *
 * @param tokens The vector of command tokens to be analyzed.
 * @return true if there are multiple redirections or improper mixes, false otherwise.
 */
bool hasMultipleRedirectionsOrPipes(const vector<string>& tokens) {
    int redirectInCount = count(tokens.begin(), tokens.end(), "<");
    int redirectOutCount = count(tokens.begin(), tokens.end(), ">");
    int pipeCount = count(tokens.begin(), tokens.end(), "|");

    if (redirectInCount > 1) {
        cerr << "mish: multiple input redirect or pipe" << endl;
        return true;
    }
    if (redirectOutCount > 1) {
        cerr << "mish: multiple output redirect or pipe" << endl;
        return true;
    }
    if (pipeCount > 0 && (redirectInCount > 0 || redirectOutCount > 0)) {
        // Checking specifically for cases where pipes and redirections might not be properly sequenced
        // E.g., cases like `cat file1.txt > file2.txt > file3.txt` or improper mixing should trigger this
        bool errorFound = false;
        for (int i = 0; i < tokens.size(); i++) {
            if (tokens[i] == "|") {
                if ((i > 0 && (tokens[i-1] == ">" || tokens[i-1] == "<")) ||
                    (i < tokens.size() - 1 && (tokens[i+1] == ">" || tokens[i+1] == "<"))) {
                    cerr << "Error: Improper mixing of pipes and redirections." << endl;
                    errorFound = true;
                    break;
                }
            }
        }
        if (errorFound) {
            return true;
        }
    }

    return false;
}


string checkWhiteSpaces(const string& input) {
    string newInput;
    const char* specialChars = "|<>&"; // Define special characters to format.

    for (size_t i = 0; i < input.length(); ++i) {
        char current = input[i];

        // Check if the current character is special and needs spaces around it
        if (strchr(specialChars, current)) {
            // Add a space before the special character if it's not the first character and the previous character is not a space
            if (i > 0 && !isspace(input[i - 1])) {
                newInput += ' ';
            }
            newInput += current;
            // Add a space after the special character if it's not the last character and the next character is not a space
            if (i < input.length() - 1 && !isspace(input[i + 1])) {
                newInput += ' ';
            }
        } else {
            newInput += current;
        }
    }
    return newInput;
}

int main(int argc, char* argv[]) {

    if (argc > 1) {
        ifstream scriptFile(argv[1]);
        string command;
        while (getline(scriptFile, command)) {
            // Process each line of the script here
            auto tokens = tokenize(command);
            if (!tokens.empty()) executeCommand(tokens);
        }
        return 0;
    }

    string homeDir = getenv("HOME") ? getenv("HOME")
                                    : "."; // Tries to get the user's home directory from the environment variable. If not found, it defaults to the current directory (".").
    string mishDir =
            homeDir + "/.mish"; // Constructs a path for a directory named ".mish" inside the user's home directory.
    mkdir(mishDir.c_str(),
          0755); // Attempts to create the ".mish" directory with read, write, and execute permissions for the owner, and read and execute permissions for group and others.

    string initialDir = getCurrentDirectory(); // Stores the current directory before any changes are made.


//    // Change directory to .mish directory
//    if (chdir(mishDir.c_str()) != 0) { // Attempts to change the current working directory to ".mish".
//        perror("chdir failed to change to mish directory"); // If changing the directory fails, it prints an error message.
//        return EXIT_FAILURE; // Exits the program with a failure status.
//    }


    // Command execution loop
    string input;
    while (true) { // Enters an infinite loop to continuously accept commands from the user.
        string currentDir = getCurrentDirectory();
        size_t mishDirPos = currentDir.find("/.mish");
        if (mishDirPos != string::npos) {
            // Only show the part of the path after '/.mish'
            cout << "mish" << currentDir.substr(mishDirPos + 6) << "> ";
        } else {
            // Fall back to full path if for some reason we are outside the .mish directory
            cout << "mish" << currentDir << "> ";
        }

        if(!getline(cin, input)) {
            if(cin.eof()) {
                cout << endl;
                break;
            }
            else {
                cin.clear();
                continue;
            }
        }

        //getline(cin, input); // Reads a line of input from the user.
        if (input == "exit") break; // If the input command is "exit", breaks out of the loop to terminate the program.

        input = checkWhiteSpaces(input);

        auto tokens = tokenize(input);
        if (tokens.empty()) continue; // If no tokens were found (empty input), skip the rest of the loop.


        if (hasMultipleRedirectionsOrPipes(tokens)) {
            continue;
        }

        if (hasSyntaxErrors(tokens)) continue;

        if (isBackgroundCommand(input)) {
            input.pop_back(); // Remove '&' from the end
            executeCommandInBackground(tokens);
        }

        int pipeIndex = findTokenIndex(tokens, "|");
        int redirectOutIndex = findTokenIndex(tokens, ">");
        int redirectInIndex = findTokenIndex(tokens, "<");
        if (pipeIndex != -1) {
            // The command contains a pipe
            executePipedCommand(tokens);
        } else if (redirectOutIndex != -1 || redirectInIndex != -1) {
            // The command contains redirection
            executeCommand(tokens); // This is your existing function that handles redirection
        } else {

            if (tokens[0] == "cd") { // If the first token is "cd", attempts to change the directory.
                if (tokens.size() == 2) {
                    if (chdir(tokens[1].c_str()) != 0) {
                        perror("cd failed");
                    }
                } else {
                    cerr << "Usage: cd <directory>" << endl;
                }
            } else if (tokens[0] == "ls" && tokens.size() == 2 && tokens[1] == "-al") {
                // Specific handling for 'ls -al'
                executeCommand(tokens);
            } else if (tokens[0] == "ls") { // If the command is "ls", lists directories and files.
                executeCommand(tokens);
                // listDirectoriesAndFiles(tokens.size() > 1 ? tokens[1] : getCurrentDirectory()); // Passes a specific directory if provided, otherwise uses the current directory.
            } else if (tokens[0] == "rm") { // Handles the "rm" command to remove files or directories.
                // Further processing for "rm" command.
            } else if (tokens[0] == "clear") {
                write(STDOUT_FILENO, "\033[H\033[2J", 7);
            } else if (tokens[0] == "emacs") {
                executeCommand(tokens);
            } else if (input.find('=') !=
                       string::npos) { // Looks for variable assignment commands (e.g., "PATH=/usr/bin").
                handleVariableAssignment(input); // Processes variable assignment.
            } else {
                executeCommand(tokens); // Executes the command specified by the tokens.
            }
        }
    }

    return 0;
}