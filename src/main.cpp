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

bool is_builtin(const std::vector<std::string>& words)
{
    return words[0] == "cd" || words[0] == "exit";
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

        if (words[0] == "exit") {
            break;
        }

        if (words[0] == "cd") {
            // TODO: 決定要去哪裡
            //   - 有給資料夾（words 有兩個以上的字）→ 用 words[1]
            //   - 沒給 → 用 getenv("HOME")
            // TODO: 呼叫 chdir，失敗（回傳 -1）就 perror("cd")
            std::string target_path;
            if (words.size() == 1){
                target_path = getenv("HOME");
            }else{
                if (words.size()>2){
                    std::cerr << "cd: too many arguments\n";
                    continue;
                }else{
                    target_path = words[1];
                }
            }
            if (chdir(target_path.c_str()) == -1){//when chdir fail return -1,success return 0
                perror("cd");
            }
            continue;
        }

        run_command(words);
    }
    return 0;
}