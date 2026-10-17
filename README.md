# minishell

A small Unix shell written in C++17 for Linux, built to learn how a shell runs programs using system calls such as `fork`, `execvp`, `waitpid`, and `chdir`.

## Features

- Runs any program on the system (`ls`, `echo`, `git`, ...) with arguments
- Built-in commands: `cd` and `exit`
- `cd` with no argument goes to the home directory; `~` is expanded in `cd` paths
- Prompt shows the current directory, with the home directory shortened to `~`
- Prompt shows the exit code of the last command when it failed
- `command not found` handling with exit code 127, matching bash
- Exit with `exit` or Ctrl+D

## Build

Requirements: g++ with C++17 support, CMake 3.16 or later, Linux.

```bash
git clone git@github.com:Ju-bx/minishell.git
cd minishell
cmake -S . -B build
cmake --build build
./build/minishell
```

## Example

```
~/Desktop/minishell> ls
build  CMakeLists.txt  README.md  src
~/Desktop/minishell> nosuchcmd
nosuchcmd: command not found
[127] ~/Desktop/minishell> ls nofile
ls: cannot access 'nofile': No such file or directory
[2] ~/Desktop/minishell> cd /
/> cd
~> exit
```

## How it works

Each line goes through the same loop:

```
read a line → split into words → built-in? → run it in the shell
                                → otherwise → fork + execvp + waitpid
```

**External commands.** The shell calls `fork()` to create a child process. The child calls `execvp()`, which replaces it with the requested program. The parent calls `waitpid()` to wait for the child and read its exit code.

```
minishell (parent)
   ├── fork() ──→ child ──→ execvp("ls") ──→ ls runs and exits
   └── waitpid() ←─────────────────────────── exit code
```

**Why `cd` must be a built-in.** Every process has its own current directory. If a child process changed directory, only the child would move, and the change would disappear when it exits. So `cd` calls `chdir()` in the shell process itself. Child processes started afterwards inherit the new directory.

**Why `exit` must be a built-in.** Running `exit` in a child would only end the child. The shell itself has to stop its loop.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | `cd` failed, or `fork` failed |
| `127` | Command not found |
| `128 + N` | Program was killed by signal N |
| other | Exit code returned by the program itself |

## Known limitations

- No quote handling: `echo "a b"` passes `"a` and `b"` as two arguments
- `~` is only expanded for `cd`, not for other commands
- No environment variable expansion (`$HOME`) or wildcards (`*.cpp`)
- No pipes, redirection, or background jobs yet (planned for v2.0)

## Roadmap

v2.0 will add:

- Redirection: `>` and `<`
- Pipes: `ls | grep cpp`
- Background jobs: `sleep 10 &`
- Signal handling: Ctrl+C stops the running command, not the shell

## Development

Memory errors were checked with AddressSanitizer:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
cmake --build build-asan
./build-asan/minishell
```