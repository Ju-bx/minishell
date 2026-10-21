# minishell

A small Unix shell written in C++17 for Linux, built to learn how a shell runs programs using system calls such as `fork`, `execvp`, `waitpid`, `chdir`, `pipe`, and `dup2`.

## Features

- Runs any program on the system (`ls`, `echo`, `git`, ...) with arguments
- Built-in commands: `cd` and `exit`
- `cd` with no argument goes to the home directory; `~` is expanded in `cd` paths
- Output redirection with `>` and input redirection with `<`
- Pipes between two commands: `ls | grep txt`
- Pipes and redirection can be combined: `cat < in.txt | sort > out.txt`
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
~/Desktop/minishell> ls | wc -l
6
~/Desktop/minishell> ls > files.txt
~/Desktop/minishell> sort < files.txt | grep src
src
~/Desktop/minishell> ls | grep nothing
[1] ~/Desktop/minishell> nosuchcmd
nosuchcmd: command not found
[127] ~/Desktop/minishell> ls |
syntax error near |
[2] ~/Desktop/minishell> cd /
/> cd
~> exit
```

## How it works

Each line goes through the same steps:

```
read a line → split into words → built-in? → run it in the shell
                                → contains | ? → split into two commands → run a pipeline
                                → otherwise    → run one command
```

Each command is parsed into a `Command`: its arguments, plus an optional input file (`<`) and output file (`>`).

### External commands

The shell calls `fork()` to create a child process. The child calls `execvp()`, which replaces it with the requested program. The parent calls `waitpid()` to wait for the child and read its exit code.

```
minishell (parent)
   ├── fork() ──→ child ──→ execvp("ls") ──→ ls runs and exits
   └── waitpid() ←─────────────────────────── exit code
```

### Redirection

Every process has file descriptors: `0` for input, `1` for output, `2` for errors. A program like `ls` simply writes to descriptor `1`; it does not know where that goes.

For `ls > files.txt`, the child opens the file and uses `dup2()` to make descriptor `1` point to it, then calls `execvp()`. The descriptor table survives `exec`, so `ls` writes into the file without any change to `ls` itself.

```
child:  open("files.txt")   →  fd 3 → files.txt
        dup2(3, 1)          →  fd 1 → files.txt
        close(3)
        execvp("ls")        →  ls writes to fd 1 → files.txt
```

This happens in the child, not in the shell. Otherwise the shell's own output, including the prompt, would also go into the file.

### Pipes

For `ls | wc -l`, the shell creates a pipe with `pipe()`, which returns a read end and a write end. It then forks two children:

```
              pipe
child A ──→ [write end ══════ read end] ──→ child B
  ls                                         wc -l
fd 1 → write end                         fd 0 → read end
```

- Child A connects its output (`fd 1`) to the write end, then runs the left command
- Child B connects its input (`fd 0`) to the read end, then runs the right command
- Both children and the parent close the pipe ends they do not use

Closing unused ends matters: the reader only sees end-of-input when every write end is closed. If the parent kept the write end open, `wc` would wait forever.

The exit code of a pipeline is the exit code of the right command, as in bash.

### Built-ins

**Why `cd` must be a built-in.** Every process has its own current directory. If a child process changed directory, only the child would move, and the change would disappear when it exits. So `cd` calls `chdir()` in the shell process itself. Child processes started afterwards inherit the new directory.

**Why `exit` must be a built-in.** Running `exit` in a child would only end the child. The shell itself has to stop its loop.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | `cd` failed, a redirection file could not be opened, or `fork` failed |
| `2` | Syntax error, such as `ls >` or `ls \|` |
| `127` | Command not found |
| `128 + N` | Program was killed by signal N |
| other | Exit code returned by the program itself |

## Known limitations

- Only one `|` per line: `ls | grep a | wc -l` is not supported
- No append redirection (`>>`) and no redirection of errors (`2>`)
- No quote handling: `echo "a b"` passes `"a` and `b"` as two arguments
- `~` is only expanded for `cd`, not for other commands
- No environment variable expansion (`$HOME`) or wildcards (`*.cpp`)
- No background jobs (`&`) and no signal handling: Ctrl+C also stops the shell

## Development

Memory errors were checked with AddressSanitizer:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
cmake --build build-asan
./build-asan/minishell
```