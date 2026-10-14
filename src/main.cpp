#include <iostream>
#include <unistd.h>      // fork, execvp
#include <sys/wait.h>    // waitpid, WEXITSTATUS

int main()
{
    std::cout << "parent: about to run ls -l\n";

    pid_t pid = fork();

    if (pid < 0)
    {
        std::cerr << "fork failed\n";
        return 1;
    }

    if (pid == 0)
    {
        // child: become "ls -l"
        char cmd[] = "ls";
        char arg1[] = "nofile";
        char* args[] = {cmd, arg1, nullptr};

        execvp(args[0], args);

        // this line only runs if exec failed
        std::cerr << "exec failed\n";
        return 1;
    }

    // parent: wait for the child
    int status = 0;
    waitpid(pid, &status, 0);
    std::cout << "parent: child finished, exit code = " << WEXITSTATUS(status) << '\n';

    return 0;
}