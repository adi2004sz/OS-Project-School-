#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

pid_t monitor_pid = -1;
int monitor_running = 0;

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

void handle_child_signal(int sig) {
    wait(NULL);
    monitor_running = 0;
    printf("Monitor process ended\n");
}

int main() {
    signal(SIGCHLD, handle_child_signal);
    start_monitor();
    printf("Treasure Hub running...\n");
    pause();
    return 0;
}