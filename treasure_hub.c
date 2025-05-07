#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/dirent.h>

#define CMD_FILE "/tmp/treasure_hub_cmd"

pid_t monitor_pid = -1;
int monitor_running = 0;
int waiting_for_termination = 0;


void start_monitor();
void handle_user_command(char *cmd);
void handle_child_signal(int sig);
void list_hunts();

void handle_child_signal(int sig) {
    wait(NULL);
    if (waiting_for_termination) {
        printf("Monitor has terminated.\n");
        waiting_for_termination = 0;
    }
    monitor_running = 0;
    printf("Monitor process ended\n");
}

void start_monitor() {
    monitor_pid = fork();
    if (monitor_pid == 0) {
        while (1) {
            sleep(1);
        }
        exit(0);
    } else if (monitor_pid > 0) {
        monitor_running = 1;
        printf("Monitor started with PID %d\n", monitor_pid);
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}


void handle_user_command(char *cmd) {
    if (strcmp(cmd, "start_monitor") == 0) {
        if (!monitor_running) {
            start_monitor();
        } else {
            printf("Monitor already running\n");
        }
    } else if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else if (strcmp(cmd, "list_treasures") == 0) {
        list_treasures();
    } else if (strcmp(cmd, "view_treasure") == 0) {
        view_treasure();
    } else if (strcmp(cmd, "stop_monitor") == 0) {
        stop_monitor();
    } else if (strcmp(cmd, "exit") == 0) {
        if (monitor_running) {
            printf("Stop the monitor first.\n");
        } else {
            printf("Exiting.\n");
            exit(0);
        }
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}

void list_treasures() {
    printf("Listing treasures for current hunt...\n");
}

void view_treasure() {
    printf("Viewing selected treasure...\n");
}

void stop_monitor() {
    if (monitor_running) {
        waiting_for_termination = 1;
        kill(monitor_pid, SIGUSR1);
    } else {
        printf("Monitor not running.\n");
    }
}

void list_hunts() {
    DIR *dir = opendir(".");
    if (!dir) return;

    struct dirent *entry;
    struct stat st;
    int hunt_count = 0;

    printf("Available hunts:\n");
    printf("----------------------------\n");

    srand(time(NULL));

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            int progress = rand() % 101;
            printf("Hunt: %s — Progress: %d%%\n", entry->d_name, progress);
            hunt_count++;
        }
    }

    closedir(dir);
    if (hunt_count == 0) {
        printf("No hunts found.\n");
    }
}

int main() {
    signal(SIGCHLD, handle_child_signal);

    char input[128];
    while (1) {
        printf("Enter command: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        handle_user_command(input);
    }
    return 0;
}
