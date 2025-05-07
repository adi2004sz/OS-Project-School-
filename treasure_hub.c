#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMD_FILE "/tmp/treasure_hub_cmd"

pid_t monitor_pid = -1;
int monitor_running = 0;

void start_monitor();
void handle_user_command(char *cmd);
void handle_child_signal(int sig);

void handle_child_signal(int sig) {
    wait(NULL);
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
    } else {
        printf("Unknown command: %s\n", cmd);
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