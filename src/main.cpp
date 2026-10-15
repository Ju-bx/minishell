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

void run_command(std::vector<std::string>& words)
{
    std::vector<char*> argv;
    for (std::string& w : words) {
        argv.push_back(w.data());
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0){
        std::cerr << "fork fail\n" ;
    }else if (pid == 0){
        execvp(argv[0], argv.data());
        std::cerr << argv[0] << ": command not found\n";
        _exit(127);
    } else {
        int status = 0;
        waitpid(pid, &status, 0); 
    }
}

int main()
{
    std::string line;
    while (true)
    {
        std::cout << "minishell> ";
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;                       // Ctrl+D
        }

        std::vector<std::string> words = split(line);
        if (words.empty()) {
            continue;                    // empty line
        }

        run_command(words);
    }
    return 0;
}