# OS-Project-School-
Task : Build a C program in the UNIX environment simulating a treasure hunt game system that allows users to create, manage, and participate in digital treasure hunts, manages game state through processes and signals, and facilitates interaction between different components through pipes and redirects.
A symbolic link is created in the root directory for each hunt.



## Features

- Create and manage multiple treasure hunts
- Add treasures with coordinates, clues, and value information
- List all available hunts and their treasure counts
- View detailed information about specific treasures
- Monitor system for real-time command processing
- Activity logging for hunt operations
- inter-process communication using pipes integrates an external score calculator.

## Components

The system consists of 3 main components:

1. **treasure_hub** (`treasure_hub.c`): The main interface that handles user commands and manages the monitor process.
2. **treasure_managerr** (`treasure_manager.c`): Handles the actual treasure hunt data operations.

3. **score_calculator** (`score_calculator.c`) : Calculates user scores for each hunt.


## Treasure Data Fields

The `treasure_manager` program stores treasure information with the following fields:

* Treasure ID (int)
* User name (unique, as text) (max length: 31 characters)
* GPS coordinates (latitude and longitude as floating-point numbers)
* Clue text (string) (max length: 127 characters)
* Value (integer)


## Treasure Manager Operations

The `treasure_manager` program supports the following operations, invoked via command-line arguments:

* `add <hunt_id>`: Adds a new treasure to the specified hunt (game session). Each hunt is stored in a separate directory.

* `list <hunt_id>`: Lists all treasures in the specified hunt. First, print the hunt name, the (total) file size and last modification time of its treasure file(s), then list the treasures. 

* `view <hunt_id> <id>`: Views details of a specific treasure.

* `remove_treasure <hunt_id> <id>`: Removes a treasure. Note that this operation may require reorganizing the entire file (e.g., when a user removes a record in the middle of the file).

* `remove_hunt <hunt_id>`: Removes an entire hunt.

All operations are logged in a text file named `logged_hunt` in the hunt's directory. A symbolic link `logged_hunt-<hunt_id>` is created for easier access to the log file.

The `treasure_manager` program uses the following system calls for file operations: `open()`, `close()`, `read()`, `write()`, `lseek()`, `stat()` or `lstat()` to retrieve file information, `mkdir()`. 

The `treasure_manager` also includes a function `sanitize_hunt_id()` to prevent directory traversal attacks by validating the `hunt_id`.


## Score Calculator

The `score_calculator` program calculates the scores of users in a specified hunt.  The score is the sum of the values of all treasures owned by a user.

The `score_calculator` program uses `open()`, `read()`, and `close()` for file operations.


## Treasure Hub

The `treasure_hub` program acts as the main interface for the treasure hunt game. It manages game flow, interacts with the `treasure_manager`, and presents information to the user. 

The `treasure_hub` uses temporary files `/tmp/treasure_hub_cmd` and `/tmp/treasure_hub_arg` for communication and a pipe for data transfer between processes.

Available commands: `start_monitor`, `list_hunts`, `list_treasures`, `view_treasure`, `calculate_score`, `stop_monitor`, `exit`.

The `treasure_hub`program utilizes `fork()`, `exec()`, `pipe()`, `dup2()`, `waitpid()`, `signal()`, `sigaction()`, `open()`, `read()`, `write()`, `close()`, `opendir()`, `readdir()`, `stat()`, `snprintf()`, `strcmp()`, and `atoi()` system calls and functions.


## Building the Project

Use the `Makefile` to build the project:
* `make all`: Builds all executables (`treasure_manager`, `treasure_hub`, `score_calculator`).
* `make treasure_manager`: Builds the `treasure_manager` executable.
* `make treasure_hub`: Builds the `treasure_hub` executable.
* `make score_calculator`: Builds the `score_calculator` executable.
* `make debug`: Builds executables with debugging symbols. This adds the `DEBUG_FLAGS` to `CFLAGS`. 

## Testing

The `Makefile` provides testing options:

* `make test`: Runs basic functionality tests.  This includes testing `treasure_manager` and `score_calculator`. For `treasure_hub`, manual testing instructions are provided.
* `make test_advanced`: Creates multiple hunt scenarios for more thorough testing. This includes testing score calculation for multiple hunts. 
## Memory Leak Detection

* `make memcheck`: Tests for memory leaks using `valgrind` (if available). Requires `valgrind` to be installed.

## Usage

### Starting the Application

```bash
./treasure_hub
```

### Available Commands

- `start_monitor`: Start the monitoring process
- `list_hunts`: Display all available treasure hunts
- `list_treasures <hunt_id>`: List all treasures in a specific hunt
- `view_treasure <hunt_id> <treasure_id>`: View details of a specific treasure
- `stop_monitor`: Stop the monitoring process
- `exit`: Exit the application (monitor must be stopped first)

## Examples

### Create a new hunt with a treasure

```bash
./treasure_manager add coastal_adventure 1 explorer42 34.052235 -118.243683 "Look under the large palm tree near the rocky outcrop" 50
```

### List all treasures in a hunt

```bash
./treasure_hub
treasure_hub> start_monitor
treasure_hub> list_treasures coastal_adventure
```

### View a specific treasure

```bash
./treasure_hub
treasure_hub> start_monitor
treasure_hub> view_treasure coastal_adventure 1
```

## Notes

- The monitor process must be running before using most commands
- Always stop the monitor before exiting the application
- Hunt directories are created automatically when adding treasures

## Troubleshooting

- If commands fail, ensure the monitor is running (`start_monitor`)
- Check file permissions if you encounter access errors
- Make sure both executables (`treasure_hub` and `treasure_manager`) are in the same directory