#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_USERNAME_LEN 31
#define MAX_CLUE_LEN 127
#define TREASURE_FILE "treasures.dat"
#define MAX_USERS 100

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN + 1];
    double latitude;
    double longitude;
    char clue[MAX_CLUE_LEN + 1];
    int value;
} Treasure;

typedef struct {
    char username[MAX_USERNAME_LEN + 1];
    int total_value;
    int treasure_count;
} UserScore;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hunt_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *hunt_dir = argv[1];
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error: Could not open treasure file for hunt '%s'\n", hunt_dir);
        return EXIT_FAILURE;
    }


    UserScore users[MAX_USERS];
    int user_count = 0;


    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, treasure.username) == 0) {
                users[i].total_value += treasure.value;
                users[i].treasure_count++;
                found = 1;
                break;
            }
        }

        if (!found && user_count < MAX_USERS) {
            strncpy(users[user_count].username, treasure.username, MAX_USERNAME_LEN);
            users[user_count].total_value = treasure.value;
            users[user_count].treasure_count = 1;
            user_count++;
        }
    }
    close(fd);

    printf("Hunt: %s\n", hunt_dir);
    printf("-------------------------------------------------\n");
    printf("%-20s | %-12s | %-8s\n", "Username", "Total Value", "Treasures");
    printf("-------------------------------------------------\n");
    
    for (int i = 0; i < user_count; i++) {
        printf("%-20s | %-12d | %-8d\n", users[i].username, users[i].total_value, users[i].treasure_count);
    }
    
    if (user_count == 0) {
        printf("No users found in this hunt.\n");
    }
    printf("--------------------------------------------------\n");

    return EXIT_SUCCESS;
}