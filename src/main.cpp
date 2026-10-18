#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <fcntl.h>
#include <cstdio>

std::vector<std::string> split(const std::string& line)
{
    std::vector<std::string> words;
    std::istringstream iss(line);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

struct Command {
    std::vector<std::string> args;
    std::string in_file;
    std::string out_file;
};

bool parse_command(const std::vector<std::string>& words, Command& cmd)
{
    for (std::size_t i = 0; i < words.size(); i++) {
        if (words[i] == ">") {
            if (i + 1 >= words.size()) {
                std::cerr << "syntax error: missing file after >\n";
                return false;
            }
            cmd.out_file = words[i + 1];
            i++; // Skip the filename token
        } else if (words[i] == "<") {
            if (i + 1 >= words.size()) {
                std::cerr << "syntax error: missing file after <\n";
                return false;
            }
            cmd.in_file = words[i + 1];
            i++; // Skip the filename token
        } else {
            cmd.args.push_back(words[i]);
        }
    }
    return true;
}

std::string pretty_path (const std::string& full_path)
{
    std::string home_path = getenv("HOME");
    if (full_path.size() < home_path.size()) {
        return full_path;
    }
    for (size_t i = 0; i < home_path.size(); ++i)
    {
        if (full_path[i]!=home_path[i])
        {
            return full_path;
        }
    }
    if (full_path.size() == home_path.size() || full_path[home_path.size()] == '/')
    {
        return ('~' + full_path.substr(home_path.size()));
    }else
    {
        return full_path;
    }
    
}

int run_command(const Command& cmd)
{
    if (cmd.args.empty()) {
        return 0;
    }

    std::vector<char*> argv;
    for (const std::string& w : cmd.args) {
        argv.push_back(const_cast<char*>(w.data()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork fail");
        return 1;
    } else if (pid == 0) {
        // Handle input redirection (<)
        if (!cmd.in_file.empty()) {
            int fd_in = open(cmd.in_file.c_str(), O_RDONLY);
            if (fd_in < 0) {
                perror("open input file");
                _exit(1);
            }
            if (dup2(fd_in, STDIN_FILENO) < 0) {
                perror("dup2 input");
                _exit(1);
            }
            close(fd_in);
        }

        // Handle output redirection (>)
        if (!cmd.out_file.empty()) {
            int fd_out = open(cmd.out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("open output file");
                _exit(1);
            }
            if (dup2(fd_out, STDOUT_FILENO) < 0) {
                perror("dup2 output");
                _exit(1);
            }
            close(fd_out);
        }

        execvp(argv[0], argv.data());
        std::cerr << argv[0] << ": command not found\n";
        _exit(127);
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 128 + WTERMSIG(status);
    }
}

std::string current_dir()
{
    char buf[4096];
    if (getcwd(buf, sizeof(buf)) == nullptr) {
        perror("getcwd");
        return "?";        
    }
    return buf;
}

int run_cd (const std::vector<std::string> &words){
    std::string target_path;
    if (words.size() == 1){
        target_path = getenv("HOME");
    }else{
        if (words.size()>2){
            std::cerr << "cd: too many arguments\n";
            return 1;
        }else{
            target_path = words[1];
            if (!target_path.empty() && target_path[0] == '~') {
                target_path = std::string(getenv("HOME")) + target_path.substr(1);
            }
        }
    }
    if (chdir(target_path.c_str()) == -1){//when chdir fail return -1,success return 0
        perror("cd");
        return 1;
    }else {
        return 0;
    }
}

int main()
{
    std::string line;
    int last_status = 0;
    while (true)
    {
        if (last_status != 0) {
            std::cout << "[" << last_status << "] ";
        }
        std::cout << pretty_path(current_dir()) << "> ";
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;                       // Ctrl+D
        }

        std::vector<std::string> words = split(line);
        if (words.empty()) {
            continue;                    // empty line
        }

        if (words[0] == "exit") {
            break;
        }
        if (words[0] == "cd") {
            last_status = run_cd(words);
            continue;
        }

        Command cmd;
        if (!parse_command(words, cmd)) {
            last_status = 1;
            continue;
        }

        if (cmd.args.empty()) {
            continue;
        }

        last_status = run_command(cmd);
        
    }
    return 0;
}