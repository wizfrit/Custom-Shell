#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

const int MAX_HISTORY_SIZE = 10;
string history[MAX_HISTORY_SIZE];
int history_count = 0;

void execute_command(char* args[]) {
    execvp(args[0], args);
    perror("Error executing command");
    exit(1);
}

void tokenization(const char* input, char* array) {
    int tokin = 0;
    char* del = " \t\n";
    char* token = strtok(const_cast<char*>(input), del);

    while (token != nullptr) {
        int toklen = strlen(token);
        if (tokin + toklen < 199) {
            strncat(array, token, toklen);
            array[tokin + toklen] = ' ';
            tokin += toklen + 1;
        } else {
            cout << "Error!!!! Input command is too long!" << endl;
            exit(1);
        }
        token = strtok(nullptr, del);
    }
    array[tokin] = '\0';
}

void add_to_history(const string& command) {
    if (history_count < MAX_HISTORY_SIZE) {
        history[history_count++] = command;
    } else {
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; ++i) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY_SIZE - 1] = command;
    }
}

void print_history() {
    for (int i = history_count - 1, num = 1; i >= 0; --i, ++num) {
        cout << num << ": " << history[i] << endl;
    }
}
void execute_from_history(int index) {
    if (index >= 1 && index <= history_count) {
        char array[200] = {};
        tokenization(history[index - 1].c_str(), array);
        
        char* args[20];
        char* arg = strtok(array, " ");
        int arg_count = 0;
        while (arg != nullptr && arg_count < 19) {
            args[arg_count++] = arg;
            arg = strtok(nullptr, " ");
        }
        args[arg_count] = nullptr;

        execute_command(args);
    } else {
        cout << "No such command in history." << endl;
    }
}


int main() {
    string input;

    do {
        cout << "Enter Command: ";
        getline(cin, input);

        if (input == "exit") {
            break;
        }
        if (input == "history") {
           print_history();
           continue;
        }
         if (input == "!!") {
            if (history_count > 0) {
                execute_from_history(history_count);
            } else {
                cout << "No commands in history." << endl;
            }
            continue;
        }

        if (input.size() > 1 && input[0] == '!') {
            string num_str = input.substr(1);
            int index = stoi(num_str);
            execute_from_history(index);
            continue;
        }

        add_to_history(input);

        char array[200] = {};
        tokenization(input.c_str(), array);

        bool background_flag = false;
        if (array[strlen(array) - 1] == '&') {
            background_flag = true;
            array[strlen(array) - 1] = '\0';
        }

        char* command = strtok(array, "|");
        char* next_command;
        int prev_pipe_read = STDIN_FILENO;

        while (command != nullptr) {
            string trimmed_command = command;
            trimmed_command.erase(0, trimmed_command.find_first_not_of(" \t"));
            trimmed_command.erase(trimmed_command.find_last_not_of(" \t") + 1);

            bool has_redirection = false;
            char* redirection = nullptr;
            if (trimmed_command.find('<') != string::npos || 
                trimmed_command.find('>') != string::npos) {
                has_redirection = true;
                redirection = const_cast<char*>(trimmed_command.c_str());
            }

            bool has_fifo = false;
            char* fifo = nullptr;
            if (trimmed_command.find('|') != string::npos) {
                has_fifo = true;
                fifo = const_cast<char*>(trimmed_command.c_str());
            }

            if (has_redirection || has_fifo) {
                int pipe_fd[2];
                if (pipe(pipe_fd) == -1) {
                    perror("Pipe creation error");
                    break;
                }

                pid_t pid = fork();
                if (pid == -1) {
                    perror("Forking Error ");
                } else if (pid == 0) {
                    if (has_redirection) {
                        // Handle input/output redirection
                        char* in_file = nullptr;
                        char* out_file = nullptr;
                        bool append_mode = false;

                        char* token = strtok(redirection, "<>");
                        while (token != nullptr) {
                            string arg = token;
                            size_t start = arg.find_first_not_of(" \t");
                            size_t end = arg.find_last_not_of(" \t");
                            if (start != string::npos && end != string::npos) {
                                arg = arg.substr(start, end - start + 1);
                            }

                            if (arg[0] == '<') {
                                in_file = const_cast<char*>(arg.substr(1).c_str());
                            } else if (arg[0] == '>') {
                                if (arg.size() > 1 && arg[1] == '>') {
                                    append_mode = true;
                                    out_file = const_cast<char*>(arg.substr(2).c_str());
                                } else {
                                    out_file = const_cast<char*>(arg.substr(1).c_str());
                                }
                            }

                            token = strtok(nullptr, "<>");
                        }

                        if (in_file != nullptr) {
                            int in_fd = open(in_file, O_RDONLY);
                            if (in_fd == -1) {
                                perror("Error opening input file");
                                exit(1);
                            }
                            dup2(in_fd, STDIN_FILENO);
                            close(in_fd);
                        }

                        if (out_file != nullptr) {
                            int flags = O_WRONLY | O_CREAT;
                            if (append_mode) {
                                flags |= O_APPEND;
                            } else {
                                flags |= O_TRUNC;
                            }
                            int out_fd = open(out_file, flags, 0666);
                            if (out_fd == -1) {
                                perror("Error opening output file");
                                exit(1);
                            }
                            dup2(out_fd, STDOUT_FILENO);
                            close(out_fd);
                        }
                    }

                    if (has_fifo) {
                        char* fifo_name = strtok(fifo, "|");
                        int fifo_fd = open(fifo_name, O_WRONLY);
                        if (fifo_fd == -1) {
                            perror("Error opening FIFO");
                            exit(1);
                        }
                        dup2(fifo_fd, STDOUT_FILENO);
                        close(fifo_fd);
                    }

                    char* args[20];
                    char* arg = strtok(trimmed_command.data(), " ");
                    int arg_count = 0;
                    while (arg != nullptr) {
                        args[arg_count++] = arg;
                        arg = strtok(nullptr, " ");
                    }
                    args[arg_count] = nullptr;

                    execute_command(args);
                } else {
                    int status;
                    waitpid(pid, &status, 0);
                }
            } else {
                // Execute command directly
                char* args[20];
                char* arg = strtok(trimmed_command.data(), " ");
                int arg_count = 0;
                while (arg != nullptr) {
                    args[arg_count++] = arg;
                    arg = strtok(nullptr, " ");
                }
                args[arg_count] = nullptr;

                pid_t pid = fork();
                if (pid == -1) {
                    perror("Forking Error ");
                } else if (pid == 0) {
                    execute_command(args);
                } else {
                    if (!background_flag) {
                        int status;
                        waitpid(pid, &status, 0);
                    }
                }
            }

            command = strtok(nullptr, "|");
        }

    } while (true);

    cout << "Program Ended!!" << endl;
    return 0;
}

