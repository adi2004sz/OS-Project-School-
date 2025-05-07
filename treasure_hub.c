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

#define CMD_FILE  "/tmp/treasure_hub_cmd"
#define ARG_FILE  "/tmp/treasure_hub_arg"
#define MAX_CMD   256

pid_t monitor_pid = -1;
int monitor_running = 0;
int waiting_for_termination = 0;


void start_monitor();
void handle_user_command(char *cmd);
void handle_child_signal(int sig);
void handle_monitor_signal(int sig);
void list_hunts();
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, const char *treasure_id);
void stop_monitor();



void start_monitor() {
    if (monitor_running) {
        printf("Monitor already running\n");
        return;
    }
    open(CMD_FILE, O_CREAT | O_WRONLY, 0644) && close(open(CMD_FILE, O_CREAT, 0644));
    open(ARG_FILE, O_CREAT | O_WRONLY, 0644) && close(open(ARG_FILE, O_CREAT, 0644));
    pid_t pid = fork();
    if (pid == 0) {
        printf("[Monitor] started (PID %d)\n", getpid());
        while (1) pause();
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        monitor_pid = pid;
        monitor_running = 1;
        printf("Monitor process started with PID: %d\n", monitor_pid);
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}


void handle_user_command(char *cmd) {
    char *argv[3] = {0};
    int argc = 0;
    char *tok = strtok(cmd, " ");
    while (tok && argc < 3) argv[argc++] = tok, tok = strtok(NULL, " ");

    if (argc == 0) return;
    if (strcmp(argv[0], "start_monitor") == 0) {
        start_monitor();
    }
    else if (strcmp(argv[0], "list_hunts") == 0 && monitor_running) {
        int fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        write(fd, "list_hunts", strlen("list_hunts")); close(fd);
        kill(monitor_pid, SIGUSR1);
    }
    else if (strcmp(argv[0], "list_treasures") == 0 && argc == 2 && monitor_running) {
        int fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        write(fd, "list_treasures", strlen("list_treasures")); close(fd);
        fd = open(ARG_FILE, O_WRONLY | O_TRUNC);
        write(fd, argv[1], strlen(argv[1])); close(fd);
        kill(monitor_pid, SIGUSR1);
    }
    else if (strcmp(argv[0], "view_treasure") == 0 && argc == 3 && monitor_running) {
        int fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        write(fd, "view_treasure", strlen("view_treasure")); close(fd);
        fd = open(ARG_FILE, O_WRONLY | O_TRUNC);
        char buf[MAX_CMD]; snprintf(buf, sizeof(buf), "%s %s", argv[1], argv[2]);
        write(fd, buf, strlen(buf)); close(fd);
        kill(monitor_pid, SIGUSR1);
    }
    else if (strcmp(argv[0], "stop_monitor") == 0 && monitor_running) {
        waiting_for_termination = 1;
        int fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        write(fd, "stop_monitor", strlen("stop_monitor")); close(fd);
        kill(monitor_pid, SIGUSR1);
    }
    else if (strcmp(argv[0], "exit") == 0) {
        if (monitor_running) printf("Stop the monitor first.\n");
        else exit(EXIT_SUCCESS);
    }
    else {
        printf("Unknown or invalid command\n");
    }
}

void handle_child_signal(int sig) {
    int status;
    pid_t pid;
    (void)sig;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == monitor_pid) {
            monitor_running = 0;
            if (waiting_for_termination) {
                printf("Monitor has terminated.\n");
                waiting_for_termination = 0;
            }
        }
    }
}

void handle_monitor_signal(int sig) {
    (void)sig;
    int fd = open(CMD_FILE, O_RDONLY);
    if (fd < 0) return;
    char cmd[MAX_CMD]; ssize_t n = read(fd, cmd, sizeof(cmd)-1);
    close(fd);
    if (n <= 0) return;
    cmd[n] = '\0';

    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    }
    else if (strcmp(cmd, "list_treasures") == 0) {
        char hunt_id[MAX_CMD];
        fd = open(ARG_FILE, O_RDONLY);
        read(fd, hunt_id, sizeof(hunt_id)-1);
        close(fd);
        list_treasures(hunt_id);
    }
    else if (strcmp(cmd, "view_treasure") == 0) {
        char args[MAX_CMD]; char *h, *t;
        fd = open(ARG_FILE, O_RDONLY);
        read(fd, args, sizeof(args)-1);
        close(fd);
        args[sizeof(args)-1] = '\0';
        h = strtok(args, " "); t = strtok(NULL, " ");
        view_treasure(h, t);
    }
    else if (strcmp(cmd, "stop_monitor") == 0) {
        exit(0);
    }
}

void list_hunts() {
    DIR *d = opendir("."); if (!d) return;
    struct dirent *e; int count = 0;
    printf("\n[Monitor] Available hunts:\n");
    while ((e = readdir(d))) {
        if (e->d_type==DT_DIR && strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
            printf("- %s\n", e->d_name);
            count++;
        }
    }
    closedir(d);
    if (!count) printf("No hunts found.\n");
}

void list_treasures(const char *hunt_id) {
    printf("[Monitor] Listing treasures for '%s'...\n", hunt_id);
}

void view_treasure(const char *hunt_id, const char *treasure_id) {
    printf("[Monitor] Viewing treasure %s in hunt %s...\n", treasure_id, hunt_id);
}

void stop_monitor() {
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_child_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = handle_monitor_signal;
    sigaction(SIGUSR1, &sa, NULL);

    char input[MAX_CMD];
    while (1) {
        printf("Enter command: ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        handle_user_command(input);
    }
    return 0;
}
