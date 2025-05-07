#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>


#define MAX_USERNAME_LEN 31
#define MAX_CLUE_LEN 127
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"


typedef struct {
    int id;
    char username[MAX_USERNAME_LEN + 1];
    double latitude;
    double longitude;
    char clue[MAX_CLUE_LEN + 1];
    int value;
} Treasure;



void log_operation(const char *hunt_dir, const char *operation) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_dir, LOG_FILE);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening log file");
        return;
    }

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", timestamp, operation);
    write(fd, log_entry, strlen(log_entry));
    close(fd);
}


void create_symlink(const char *hunt_id, const char *hunt_dir) {
    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);

    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_dir, LOG_FILE);

    if (symlink(log_path, symlink_name) == -1 && errno != EEXIST) {
        perror("Error creating symbolic link");
    }
}


int add_treasure(const char *hunt_dir, const Treasure *treasure) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, TREASURE_FILE);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening treasure file");
        return -1;
    }

    if (write(fd, treasure, sizeof(Treasure)) == -1) {
        perror("Error writing treasure data");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


void list_treasures(const char *hunt_dir) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, TREASURE_FILE);

    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        perror("Error retrieving file stats");
        return;
    }

    printf("Hunt Directory: %s\n", hunt_dir);
    printf("File Size: %ld bytes\n", file_stat.st_size);
    printf("Last Modified: %s", ctime(&file_stat.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }

    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) > 0) {
        printf("ID: %d, Username: %s, Latitude: %.6f, Longitude: %.6f, Clue: %s, Value: %d\n",
               treasure.id, treasure.username, treasure.latitude, treasure.longitude,
               treasure.clue, treasure.value);
    }

    close(fd);
}


void view_treasure(const char *hunt_dir, int treasure_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }

    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) > 0) {
        if (treasure.id == treasure_id) {
            printf("ID: %d, Username: %s, Latitude: %.6f, Longitude: %.6f, Clue: %s, Value: %d\n",
                   treasure.id, treasure.username, treasure.latitude, treasure.longitude,
                   treasure.clue, treasure.value);
            close(fd);
            return;
        }
    }

    printf("Treasure with ID %d not found.\n", treasure_id);
    close(fd);
}

void remove_hunt(const char *hunt_id) {
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);

    DIR *dir = opendir(hunt_dir);
    if (!dir) {
        perror("Error opening hunt directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        
        char file_path[512];
        int result = snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, entry->d_name);

        
        if (result < 0 || (size_t)result >= sizeof(file_path)) {
            fprintf(stderr, "Error: Path too long for file '%s/%s'\n", hunt_dir, entry->d_name);
            continue;
        }

        
        if (unlink(file_path) == -1) {
            perror("Error removing file");
        }
    }
    closedir(dir);

    
    if (rmdir(hunt_dir) == -1) {
        perror("Error removing hunt directory");
    } else {
        printf("Hunt '%s' removed successfully.\n", hunt_id);
    }

    // remove  symlink
    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);
}


void remove_treasure(const char *hunt_dir, int treasure_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file for reading");
        return;
    }

    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s/temp_%s", hunt_dir, TREASURE_FILE);
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        perror("Error creating temp file");
        close(fd);
        return;
    }

    int found = 0;
    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (treasure.id == treasure_id) {
            found = 1;
            continue;
        }
        if (write(temp_fd, &treasure, sizeof(Treasure)) == -1) {
            perror("Error writing to temp file");
            close(fd);
            close(temp_fd);
            return;
        }
    }

    close(fd);
    close(temp_fd);

    if (!found) {
        printf("Treasure with ID %d not found.\n", treasure_id);
        unlink(temp_path);
        return;
    }

   
    if (rename(temp_path, file_path) == -1) {
        perror("Error replacing original file");
        return;
    }

    log_operation(hunt_dir, "Removed treasure");
    printf("Treasure with ID %d removed successfully.\n", treasure_id);
}


// added for Phase 2 to support monitor process
int count_treasures(const char *hunt_dir) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, TREASURE_FILE);

    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        return 0;
    }

    return file_stat.st_size / sizeof(Treasure);
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <command> <hunt_id> [options]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *command = argv[1];
    const char *hunt_id = argv[2];
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);

    if (strcmp(command, "add") == 0) {
        if (argc < 9) {
            fprintf(stderr, "Usage: %s add <hunt_id> <id> <username> <latitude> <longitude> <clue> <value>\n", argv[0]);
            return EXIT_FAILURE;
        }

        Treasure treasure;
        treasure.id = atoi(argv[3]);
        strncpy(treasure.username, argv[4], MAX_USERNAME_LEN);
        treasure.latitude = atof(argv[5]);
        treasure.longitude = atof(argv[6]);
        strncpy(treasure.clue, argv[7], MAX_CLUE_LEN);
        treasure.value = atoi(argv[8]);

        if (mkdir(hunt_dir, 0755) == -1 && errno != EEXIST) {
            perror("Error creating hunt directory");
            return EXIT_FAILURE;
        }

        if (add_treasure(hunt_dir, &treasure) == 0) {
            log_operation(hunt_dir, "Added treasure");
            create_symlink(hunt_id, hunt_dir);
        }

    } else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_dir);

    } else if (strcmp(command, "view") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s view <hunt_id> <id>\n", argv[0]);
            return EXIT_FAILURE;
        }
        int treasure_id = atoi(argv[3]);
        view_treasure(hunt_dir, treasure_id);

    } else if (strcmp(command, "remove_treasure") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s remove_treasure <hunt_id> <id>\n", argv[0]);
            return EXIT_FAILURE;
        }
        int treasure_id = atoi(argv[3]);
        remove_treasure(hunt_dir, treasure_id);

    } else if (strcmp(command, "remove_hunt") == 0) {
        remove_hunt(hunt_id);
        
    } else if (strcmp(command, "count") == 0) {
        int count = count_treasures(hunt_dir);
        printf("%d\n", count);
        
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}