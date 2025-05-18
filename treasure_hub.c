#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#define MAX_CMD_LENGTH 256
#define MAX_ARGS 10
#define CMD_FILE "/tmp/treasure_hub_cmd"
#define ARG_FILE "/tmp/treasure_hub_arg"
#define PIPE_BUF_SIZE 4096

pid_t monitor_pid = -1;
int monitor_running = 0;
int waiting_for_termination = 0;
int pipe_fd[2]; //monitor <-> main process

void start_monitor();
void handle_user_command(char *cmd);
void setup_signal_handlers();
void handle_child_signal(int sig);
void handle_monitor_signal(int sig);
void calculate_score();

void handle_monitor_signal(int sig) {
    (void)sig; 
    
    int cmd_fd = open(CMD_FILE, O_RDONLY);
    if (cmd_fd == -1) {
        perror("Error opening command file");
        return;
    }
    
    char cmd[MAX_CMD_LENGTH];
    ssize_t bytes_read = read(cmd_fd, cmd, MAX_CMD_LENGTH - 1);
    close(cmd_fd);
    
    if (bytes_read <= 0) {
        return;
    }
    
    cmd[bytes_read] = '\0';
    
    char buffer[PIPE_BUF_SIZE] = {0};
    int buffer_pos = 0;
    
    if (strcmp(cmd, "list_hunts") == 0) {
        DIR *dir = opendir(".");
        if (!dir) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos, 
                                  "Error opening current directory: %s\n", strerror(errno));
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        struct dirent *entry;
        struct stat st;
        int hunt_count = 0;
        
        buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                              "\nAvailable hunts:\n");
        buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                              "--------------------------------------------------\n");
        
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && 
                strcmp(entry->d_name, ".") != 0 && 
                strcmp(entry->d_name, "..") != 0) {
                
                char treasure_path[512]; 
                int ret = snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.dat", entry->d_name);
                if (ret < 0 || ret >= (int)sizeof(treasure_path)) {
                    buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                          "Path too long for %s/treasures.dat\n", entry->d_name);
                    continue;
                }
                
                if (stat(treasure_path, &st) == 0) {
                    int treasure_count = st.st_size / sizeof(struct {
                        int id;
                        char username[32];
                        double latitude;
                        double longitude;
                        char clue[128];
                        int value;
                    });
                    
                    buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                          "Hunt: %s  -  Treasures: %d\n", entry->d_name, treasure_count);
                    hunt_count++;
                }
            }
        }
        
        if (hunt_count == 0) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "No hunts found.\n");
        }
        
        buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                              "--------------------------------------------------\n");
        closedir(dir);
        
        write(pipe_fd[1], buffer, buffer_pos);
        
    } else if (strcmp(cmd, "list_treasures") == 0) {
        int arg_fd = open(ARG_FILE, O_RDONLY);
        if (arg_fd == -1) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "Error opening argument file: %s\n", strerror(errno));
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        char hunt_id[MAX_CMD_LENGTH];
        ssize_t bytes_read = read(arg_fd, hunt_id, MAX_CMD_LENGTH - 1);
        close(arg_fd);
        
        if (bytes_read <= 0) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "No hunt ID provided.\n");
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        hunt_id[bytes_read] = '\0';
        
        char hunt_dir[512]; 
        int ret = snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);
        if (ret < 0 || ret >= (int)sizeof(hunt_dir)) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "Hunt ID path too long\n");
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        int tm_pipe[2];
        if (pipe(tm_pipe) == -1) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "Error creating pipe: %s\n", strerror(errno));
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        pid_t list_pid = fork();
        if (list_pid == 0) {
            close(tm_pipe[0]); 
            
            dup2(tm_pipe[1], STDOUT_FILENO);
            close(tm_pipe[1]);
            
            execl("./treasure_manager", "treasure_manager", "list", hunt_id, NULL);
            perror("Error executing treasure_manager");
            exit(EXIT_FAILURE);
        }
        
        close(tm_pipe[1]);
        
        char tm_buffer[PIPE_BUF_SIZE];
        ssize_t tm_bytes;
        
        while ((tm_bytes = read(tm_pipe[0], tm_buffer, PIPE_BUF_SIZE - 1)) > 0) {
            tm_buffer[tm_bytes] = '\0';
            if (buffer_pos + tm_bytes < PIPE_BUF_SIZE) {
                memcpy(buffer + buffer_pos, tm_buffer, tm_bytes);
                buffer_pos += tm_bytes;
            } else {
                write(pipe_fd[1], buffer, buffer_pos);
                memcpy(buffer, tm_buffer, tm_bytes);
                buffer_pos = tm_bytes;
            }
        }
        
        close(tm_pipe[0]);
        waitpid(list_pid, NULL, 0);
        
        write(pipe_fd[1], buffer, buffer_pos);
        
    } else if (strcmp(cmd, "view_treasure") == 0) {
        int arg_fd = open(ARG_FILE, O_RDONLY);
        if (arg_fd == -1) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "Error opening argument file: %s\n", strerror(errno));
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        char args[MAX_CMD_LENGTH];
        ssize_t bytes_read = read(arg_fd, args, MAX_CMD_LENGTH - 1);
        close(arg_fd);
        
        if (bytes_read <= 0) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "No arguments provided.\n");
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        args[bytes_read] = '\0';
        
        char hunt_id[MAX_CMD_LENGTH/2];
        char treasure_id[MAX_CMD_LENGTH/2];
        
        if (sscanf(args, "%63s %63s", hunt_id, treasure_id) != 2) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "Invalid arguments. Format: hunt_id treasure_id\n");
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        int tm_pipe[2];
        if (pipe(tm_pipe) == -1) {
            buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                                  "Error creating pipe: %s\n", strerror(errno));
            write(pipe_fd[1], buffer, buffer_pos);
            return;
        }
        
        pid_t view_pid = fork();
        if (view_pid == 0) {
            close(tm_pipe[0]);
            
            dup2(tm_pipe[1], STDOUT_FILENO);
            close(tm_pipe[1]);
            
            execl("./treasure_manager", "treasure_manager", "view", hunt_id, treasure_id, NULL);
            perror("Error executing treasure_manager");
            exit(EXIT_FAILURE);
        }
        
        close(tm_pipe[1]);
        
        char tm_buffer[PIPE_BUF_SIZE];
        ssize_t tm_bytes;
        
        while ((tm_bytes = read(tm_pipe[0], tm_buffer, PIPE_BUF_SIZE - 1)) > 0) {
            tm_buffer[tm_bytes] = '\0';
            if (buffer_pos + tm_bytes < PIPE_BUF_SIZE) {
                memcpy(buffer + buffer_pos, tm_buffer, tm_bytes);
                buffer_pos += tm_bytes;
            } else {
                write(pipe_fd[1], buffer, buffer_pos);
                memcpy(buffer, tm_buffer, tm_bytes);
                buffer_pos = tm_bytes;
            }
        }
        
        close(tm_pipe[0]);
        waitpid(view_pid, NULL, 0);
        
        write(pipe_fd[1], buffer, buffer_pos);
        
    } else if (strcmp(cmd, "stop_monitor") == 0) {
        buffer_pos += snprintf(buffer + buffer_pos, PIPE_BUF_SIZE - buffer_pos,
                              "Monitor is shutting down...\n");
        write(pipe_fd[1], buffer, buffer_pos);
        
        usleep(2000000);  
        close(pipe_fd[1]);
        exit(EXIT_SUCCESS);
    }
}

void setup_signal_handlers() {
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_child_signal;
    sigaction(SIGCHLD, &sa, NULL);
}

void handle_child_signal(int sig) {
    int status;
    pid_t pid;
    
    (void)sig; 
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == monitor_pid) {
            monitor_pid = -1;
            monitor_running = 0;
            
            if (waiting_for_termination) {
                printf("Monitor process has terminated with status: ");
                if (WIFEXITED(status)) {
                    printf("exit status %d\n", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("killed by signal %d\n", WTERMSIG(status));
                }
                waiting_for_termination = 0;
            }
        }
    }
}

void start_monitor() {
    if (monitor_running) {
        printf("Monitor is already running!\n");
        return;
    }
    
    if (pipe(pipe_fd) == -1) {
        perror("Error creating pipe");
        return;
    }
    
    int cmd_fd = open(CMD_FILE, O_WRONLY | O_CREAT, 0644);
    if (cmd_fd != -1) {
        close(cmd_fd);
    }
    
    int arg_fd = open(ARG_FILE, O_WRONLY | O_CREAT, 0644);
    if (arg_fd != -1) {
        close(arg_fd);
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    } else if (pid == 0) {
        close(pipe_fd[0]);
        
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = handle_monitor_signal;
        
        sigaction(SIGUSR1, &sa, NULL);
        
        printf("Monitor started (PID: %d)\n", getpid());
        
        while (1) {
            sleep(1);
        }
    } else {
        
        close(pipe_fd[1]);
        
        monitor_pid = pid;
        monitor_running = 1;
        printf("Monitor process started with PID: %d\n", monitor_pid);
    }
}

void calculate_score() {
    if (!monitor_running) {
        printf("Error: Monitor is not running. Use 'start_monitor' first.\n");
        return;
    }
    
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Error opening current directory");
        return;
    }
    
    struct dirent *entry;
    struct stat st;
    int hunt_count = 0;
    
    printf("\nCalculating scores for all hunts:\n");
    printf("--------------------------------------------------\n");
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
            
            char treasure_path[512]; 
            int ret = snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.dat", entry->d_name);
            if (ret < 0 || ret >= (int)sizeof(treasure_path)) {
                fprintf(stderr, "Path too long for %s/treasures.dat\n", entry->d_name);
                continue;
            }
            
            if (stat(treasure_path, &st) == 0) {
                int score_pipe[2];
                if (pipe(score_pipe) == -1) {
                    perror("Error creating pipe for score calculation");
                    continue;
                }
                
                pid_t score_pid = fork();
                if (score_pid == 0) {
                    close(score_pipe[0]);
                    
                    dup2(score_pipe[1], STDOUT_FILENO);
                    close(score_pipe[1]);
                    
                    execl("./score_calculator", "score_calculator", entry->d_name, NULL);
                    perror("Error executing score_calculator");
                    exit(EXIT_FAILURE);
                }
                
                close(score_pipe[1]); 
                
                char score_buffer[PIPE_BUF_SIZE];
                ssize_t score_bytes;
                
                while ((score_bytes = read(score_pipe[0], score_buffer, PIPE_BUF_SIZE - 1)) > 0) {
                    score_buffer[score_bytes] = '\0';
                    printf("%s", score_buffer);
                }
                
                close(score_pipe[0]);
                waitpid(score_pid, NULL, 0);
                hunt_count++;
            }
        }
    }
    
    if (hunt_count == 0) {
        printf("No hunts found.\n");
    }
    
    closedir(dir);
}

void handle_user_command(char *cmd) {
    char *args[MAX_ARGS];
    char *token;
    int arg_count = 0;
    
    token = strtok(cmd, " ");
    while (token != NULL && arg_count < MAX_ARGS) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    
    if (arg_count == 0) {
        return;
    }
    
    if (strcmp(args[0], "start_monitor") == 0) {
        start_monitor();
    } else if (strcmp(args[0], "list_hunts") == 0) {
        if (!monitor_running) {
            printf("Error: Monitor is not running. Use 'start_monitor' first.\n");
            return;
        }
        
        int cmd_fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        if (cmd_fd == -1) {
            perror("Error opening command file");
            return;
        }
        
        write(cmd_fd, "list_hunts", strlen("list_hunts"));
        close(cmd_fd);
        
        kill(monitor_pid, SIGUSR1);
        
        char buffer[PIPE_BUF_SIZE];
        ssize_t bytes_read = read(pipe_fd[0], buffer, PIPE_BUF_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        
    } else if (strcmp(args[0], "list_treasures") == 0) {
        if (!monitor_running) {
            printf("Error: Monitor is not running. Use 'start_monitor' first.\n");
            return;
        }
        
        if (arg_count < 2) {
            printf("Usage: list_treasures <hunt_id>\n");
            return;
        }
        
        int cmd_fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        if (cmd_fd == -1) {
            perror("Error opening command file");
            return;
        }
        
        write(cmd_fd, "list_treasures", strlen("list_treasures"));
        close(cmd_fd);
        
        int arg_fd = open(ARG_FILE, O_WRONLY | O_TRUNC);
        if (arg_fd == -1) {
            perror("Error opening argument file");
            return;
        }
        
        write(arg_fd, args[1], strlen(args[1]));
        close(arg_fd);
        
        kill(monitor_pid, SIGUSR1);
        
        char buffer[PIPE_BUF_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_fd[0], buffer, PIPE_BUF_SIZE - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        
    } else if (strcmp(args[0], "view_treasure") == 0) {
        if (!monitor_running) {
            printf("Error: Monitor is not running. Use 'start_monitor' first.\n");
            return;
        }
        
        if (arg_count < 3) {
            printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
            return;
        }
        
        int cmd_fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        if (cmd_fd == -1) {
            perror("Error opening command file");
            return;
        }
        
        write(cmd_fd, "view_treasure", strlen("view_treasure"));
        close(cmd_fd);
        
        int arg_fd = open(ARG_FILE, O_WRONLY | O_TRUNC);
        if (arg_fd == -1) {
            perror("Error opening argument file");
            return;
        }
        
        char args_str[MAX_CMD_LENGTH];
        int ret = snprintf(args_str, sizeof(args_str), "%s %s", args[1], args[2]);
        if (ret < 0 || ret >= (int)sizeof(args_str)) {
            fprintf(stderr, "Arguments too long for buffer\n");
            return;
        }
        write(arg_fd, args_str, strlen(args_str));
        close(arg_fd);
        
        kill(monitor_pid, SIGUSR1);
        
        char buffer[PIPE_BUF_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_fd[0], buffer, PIPE_BUF_SIZE - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        
    } else if (strcmp(args[0], "calculate_score") == 0) {
        calculate_score();
        
    } else if (strcmp(args[0], "stop_monitor") == 0) {
        if (!monitor_running) {
            printf("Error: Monitor is not running.\n");
            return;
        }
        
        int cmd_fd = open(CMD_FILE, O_WRONLY | O_TRUNC);
        if (cmd_fd == -1) {
            perror("Error opening command file");
            return;
        }
        
        write(cmd_fd, "stop_monitor", strlen("stop_monitor"));
        close(cmd_fd);
        
        kill(monitor_pid, SIGUSR1);
        
        char buffer[PIPE_BUF_SIZE];
        ssize_t bytes_read = read(pipe_fd[0], buffer, PIPE_BUF_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        
        close(pipe_fd[0]);
        waiting_for_termination = 1;
        
    } else if (strcmp(args[0], "exit") == 0) {
        if (monitor_running) {
            printf("Error: Monitor is still running. Use 'stop_monitor' first.\n");
            return;
        }
        
        printf("Exiting Treasure Hub. Goodbye!\n");
        exit(EXIT_SUCCESS);
        
    } else {
        printf("Unknown command: %s\n", args[0]);
        printf("Available commands: start_monitor, list_hunts, list_treasures, view_treasure, calculate_score, stop_monitor, exit\n");
    }
}

int main() {
    char cmd[MAX_CMD_LENGTH];
    
    setup_signal_handlers();
    
    printf("Welcome to Treasure Hub!\n");
    printf("Available commands: start_monitor, list_hunts, list_treasures, view_treasure, calculate_score, stop_monitor, exit\n");
    
    while (1) {
        printf("treasure_hub> ");
        if (fgets(cmd, MAX_CMD_LENGTH, stdin) == NULL) {
            break;
        }
        
        cmd[strcspn(cmd, "\n")] = 0;
        
        handle_user_command(cmd);
    }
    
    return 0;
}