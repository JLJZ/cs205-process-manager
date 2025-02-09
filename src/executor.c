#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "executor.h"

struct executor {
    int pipe[2];
    pid_t processes[3];
    bool is_child_process;
};

static void process_child_termination(executor *e) {
    const size_t n = sizeof(e->processes) / sizeof(e->processes[0]);

    for (size_t i = 0; i < n; ++i) {
        pid_t pid = e->processes[i];
        pid_t child_pid = waitpid(pid, NULL, WNOHANG);
        if (child_pid == -1 || child_pid == pid) {
            e->processes[i] = 0;
        }
    }
}

static bool max_processes_are_running(executor *e) {
    for (size_t i = 0; i < sizeof(e->processes); ++i) {
        if (e->processes[i] == 0) {
            return false;
        }
    }
    return true;
}

static char *poll_command(executor *e) {
    char *str = calloc(1, sizeof(char));

    if (!e->is_child_process) {
        return str;
    }
    
    const size_t BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];

    ssize_t str_len = 0;
    ssize_t read_size = read(e->pipe[0], buffer, BUFFER_SIZE);
    while (read_size > 0) {
        memcpy(str + str_len, buffer, sizeof(char) * (size_t)read_size);
        str_len += read_size;
        read_size = read(e->pipe[0], buffer, BUFFER_SIZE);
    }

    *(str + str_len) = '\0';
    
    return str;
}

static void executor_run(executor *e) {
    while (true) {
        process_child_termination(e);
        char *cmd = poll_command(e);

        if (!strcmp(cmd, "exit")) {
            free(cmd);
            break;
        }

        free(cmd);
    }
}

executor *executor_init(void) {
    executor *e = malloc(sizeof(executor));

    if (-1 == pipe(e->pipe)) {
        fprintf(stderr, "Error creating pipe\n");
        goto error;
    }

    /* Set flags on pipe for non-blocking mode */
    if (-1 == fcntl(e->pipe[0], F_SETFL, O_NONBLOCK)) {
        fprintf(stderr, "Error creating pipe\n");
        goto error;
    }

    pid_t pid = fork();
    
    switch (pid) {
        case -1:
            fprintf(stderr, "Error forking process\n");
            goto error;
        case 0:
            close(e->pipe[1]);
            e->is_child_process = true;
            executor_run(e);
            close(e->pipe[0]);
            exit(EXIT_SUCCESS);
        default:
            close(e->pipe[0]);
            e->is_child_process = false;
            break;
    }
    
    return e;

error:
    free(e);
    return NULL;
}

void executor_send_command(executor *e, const char *cmd) {
    if (e->is_child_process) {
        return;
    }
    
    if (-1 == write(e->pipe[1], cmd, strlen(cmd) + 1)) {
        fprintf(stderr, "Error writing to pipe\n");
    }
}

void executor_enqueue_task(executor *e, const char *prog, char *const *args) {
    if (max_processes_are_running(e)) {
        fprintf(stderr, "Too many processes\n");
        return;
    }

    pid_t pid = fork();
    
    switch (pid) {
        case -1:
            fprintf(stderr, "Error forking process\n");
            break;

        case 0:
            fprintf(stderr, "Error %d\n", execvp(prog, args));
            exit(EXIT_FAILURE);
            break;

        default: ;
            size_t i = sizeof(e->processes);
            while (i --> 0) {
                if (e->processes[i] == 0) {
                    e->processes[i] = pid;
                    return;
                }
            }
            break;
    }
}
