#include <iostream>
#include <unistd.h>      // fork, getpid, getppid
#include <sys/types.h>   // pid_t
#include <sys/wait.h>    // waitpid

int main()
{
    std::cout << "before fork: pid = " << getpid() << '\n';

    pid_t pid = fork();

    if (pid < 0)
    {
        std::cerr << "fork failed\n";
        return 1;
    }
    else if (pid == 0)
    {
        std::cout << "child:  my pid = " << getpid()
                  << ", my parent = " << getppid() << '\n';
    }
    else
    {
        waitpid(pid, nullptr, 0);    // wait until the child finishes
        std::cout << "parent: my pid = " << getpid()
                  << ", my child = " << pid << "has finished\n";
    }

    std::cout << "after if: pid = " << getpid() << '\n';
    return 0;
}