#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

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

bool is_builtin(const std::vector<std::string>& words)
{
    return words[0] == "cd" || words[0] == "exit";
}

int run_command(std::vector<std::string>& words)
{
    std::vector<char*> argv;
    for (std::string& w : words) {
        argv.push_back(w.data());
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0){
        perror("fork fail") ;
        return 1;
    }else if (pid == 0){
        execvp(argv[0], argv.data());
        std::cerr << argv[0] << ": command not found\n";
        _exit(127);
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);        // 正常結束：回傳它的 exit code
        }
        return 128 + WTERMSIG(status);         // 被 signal 殺掉（例如 Ctrl+C）
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
        //std::cout << "minishell> ";
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
            std::string target_path;
            if (words.size() == 1){
                target_path = getenv("HOME");
            }else{
                if (words.size()>2){
                    std::cerr << "cd: too many arguments\n";
                    continue;
                }else{
                    target_path = words[1];
                    if (!target_path.empty() && target_path[0] == '~') {
                        target_path = std::string(getenv("HOME")) + target_path.substr(1);
                    }
                    
                }
            }
            if (chdir(target_path.c_str()) == -1){//when chdir fail return -1,success return 0
                perror("cd");
                last_status = 1;
            }else {
                last_status = 0;
            }
            continue;
        }

        last_status = run_command(words);
        
    }
    return 0;
}