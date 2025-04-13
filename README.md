# OS-Project-School-
Task : Build a C program in the UNIX environment simulating a treasure hunt game system that allows users to create, manage, and participate in digital treasure hunts.
A symbolic link is created in the root directory for each hunt


---

## Commands and Functionality

### 1. `add_treasure <hunt_id>`

**Functionality**:
- Creates the hunt directory if it doesn’t exist.
- Appends a treasure to `treasures.dat` in binary format.
- Treasure structure contains:
  - `int id`
  - `char username[32]`
  - `double latitude, longitude`
  - `char clue[128]`
  - `int value`
- Logs the operation in `logged_hunt`.
- Creates symbolic link `logged_hunt-<hunt_id>` if missing.

### 2. `list_treasures <hunt_id>`

**Functionality**:
- Reads `treasures.dat`.
- Prints:
  - Hunt directory name
  - File size in bytes
  - Last modified time of `treasures.dat`
- Then, for each treasure:
  - Prints ID, username, GPS coordinates, clue, and value.

### 3. `view_treasure <hunt_id> <id>`

**Functionality**:
- Reads `treasures.dat` sequentially.
- Finds treasure with matching ID.
- Prints its full details.
- If not found, displays a message.

### 4. `remove_treasure <hunt_id> <id>`

**Functionality**:
- Opens `treasures.dat` for reading.
- Creates a temporary file.
- Copies all records *except* the one with the specified ID.
- Replaces the original file with the temp file.
- Logs the removal if successful.
- If not found, shows a message and aborts the replacement.

### 5. `remove_hunt <hunt_id>`

**Functionality**:
- Deletes all files inside the hunt directory.
- Removes the hunt directory itself.
- If the directory does not exist, prints an error.

---

###  Log File: `logged_hunt`

Each operation is logged in the hunt’s `logged_hunt` file, with a timestamp

### Symbolic Links

For each `logged_hunt`, a symbolic link is created in the root directory
If the symlink already exists, it is ignored. If it fails to create, an error is shown.

##  Build & Run
Compile with ggc : `gcc -o treasure_manager treasure_manager.c`
And run the command you want : `./treasure_manager <command> ...`

