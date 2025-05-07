# OS-Project-School-
Task : Build a C program in the UNIX environment simulating a treasure hunt game system that allows users to create, manage, and participate in digital treasure hunts.
A symbolic link is created in the root directory for each hunt



## Features

- Create and manage multiple treasure hunts
- Add treasures with coordinates, clues, and value information
- List all available hunts and their treasure counts
- View detailed information about specific treasures
- Monitor system for real-time command processing
- Activity logging for hunt operations

## Components

The system consists of two main components:

1. **Treasure Hub** (`treasure_hub.c`): The main interface that handles user commands and manages the monitor process.
2. **Treasure Manager** (`treasure_manager.c`): Handles the actual treasure hunt data operations.

## Building the Application

To compile the application:

```bash
# Compile the Treasure Hub
gcc -o treasure_hub treasure_hub.c -Wall

# Compile the Treasure Manager
gcc -o treasure_manager treasure_manager.c -Wall
```

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

### Using Treasure Manager Directly

The treasure manager can also be used directly for some operations:

```bash
# Add a treasure
./treasure_manager add <hunt_id> <id> <username> <latitude> <longitude> <clue> <value>

# Remove a treasure
./treasure_manager remove_treasure <hunt_id> <treasure_id>

# Remove an entire hunt
./treasure_manager remove_hunt <hunt_id>

# Count treasures in a hunt
./treasure_manager count <hunt_id>
```

## Data Storage

- Each hunt is stored as a directory with the hunt's name
- Treasure data is stored in binary format in a `treasures.dat` file within each hunt directory
- Operations are logged in a `logged_hunt` file within each hunt directory
- Symbolic links are created for easy access to hunt logs

## Treasure Data Structure

Each treasure contains:
- ID number
- Username of the creator
- GPS coordinates (latitude and longitude)
- Clue text
- Value (points or difficulty rating)

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