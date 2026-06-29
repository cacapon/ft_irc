*This project has been created as part of the 42 curriculum by ttsubo hminemur*

## Description
- `ft_irc` is an IRC server implementation written in C++98.
- A key technical feature is that it handles multiple clients simultaneously using a single-threaded event loop based on `poll()`.
  - To comply with the project requirements, it does not use `fork` or `thread` at all.
- It is compatible with standard IRC clients. Please try it with `nc` or `irssi`.

## Instructions
### Makefile commands
- `make`: Builds only the files that have been updated and creates .o object files. It then links the .o files to generate the executable.
- `make all`: Same as `make`.
- `make re`: Builds all files, regardless of whether they have been updated.
- `make clean`: Deletes the .o files and the unit_tests binary.
- `make fclean`: Deletes the executable, .o files, and unit_tests binaries.
- `make test`: Runs run_tests.sh to execute integration tests and unit tests.
- `make unit_test`: Compiles and runs the unit tests.
- `make setup-hooks`: Sets up Git hooks for the project.

### Run
- After running the `make` command, run `./ircserv <port> <password>`.
- You will then be able to access the server using a standard client.

### Verifying Operation
#### When using nc
- If you are using nc, run the following command:
  - Mac: `nc -c 127.0.0.1 <port>`
  - Ubuntu: `nc -C 127.0.0.1 <port>`
- After that, execute the commands in the order PASS, NICK, and USER.
Example
```sh
PASS pass
NICK client1
USER client1 0 * :client1
```

To disconnect from the server, press `Ctrl+C`.

#### When using irssi
- If you are using irssi, please run the following command:
  - `irssi -c localhost -p <port> -n <username> -w <password>`
- To disconnect, use `/quit`.

### Tests
- This project uses two types of tests: integration tests and unit tests.
  - Unit tests verify parsing and reply generation.
  - Integration tests perform actual socket communication to verify that the correct messages are sent.
- Additionally, `tests/run_tests.sh` is automatically executed via a Git hook upon commit.
- Individual tests can be run using `make test` and `make unit_test`.
- To run an integration test on its own, execute `bash tests/Integration/test_main.sh`.

### Coding style
- After cloning, run `make setup-hooks` once to enable a pre-commit hook.
- On every commit, the hook auto-formats staged `.cpp`/`.hpp`/`.h` files with `clang-format` and runs the test suite; the commit is blocked if tests fail.

## Resources
- RFC 2812 (IRC Client Protocol) — Reference for the protocol specification
- `man poll` — Reference for I/O multiplexing

### AI Activity
This project utilized AI (Claude) in various stages of development.
- Coding assistance for command implementation and parsing
- Creation of unit and integration tests
- Research on IRC protocol specifications and consultation on class design
- Repository management, such as maintaining `.gitignore`
