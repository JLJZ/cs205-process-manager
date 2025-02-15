#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "procman.h"
#include "argparse.h"

#define error(msg) do { perror("[procman] " msg); } while (0);

#define BUFFER_SIZE 64

/* Linked to server source code */
extern void pm_server_init(procman *pm);

procman *pm_init(size_t max_running_processes) {
    procman *pm = calloc(1, sizeof(procman));
    pm->processes = NULL;
    pm->processes_running_count = 0;
    pm->processes_running_max = max_running_processes;
    pm->processes_running = malloc(max_running_processes * sizeof(process *));

    for (size_t i = 0; i < max_running_processes; i++) {
        pm->processes_running[i] = NULL;
    }
    
    
    if (pipe(pm->pipe) < 0) {
        error("pipe() init failed");
        goto err;
    }

    if (fcntl(pm->pipe[0], F_SETFL, O_NONBLOCK) < 0) {
        error("pipe() init failed");
        goto err;
    }

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        goto err;

    case 0:
        pm->server_pid = getpid();
        pm_server_init(pm);
        break;

    default:
        pm->server_pid = pid;
        if (close(pm->pipe[0]) < 0) {
            error("close() pipe failed on client init");
        }
        break;
    }
    
    return pm;
err:
    free(pm);
    return NULL;
}

void pm_execute(procman *pm, const char *cmd_string) {
    args cmd;
    args_parse(&cmd, cmd_string);
    
    if (write(pm->pipe[1], cmd.bytes, cmd.bytes_size) < 0) {
        error("write() pipe failed on client");
    }
    
    args_free(&cmd);
}

void pm_shutdown(procman *pm, unsigned int timeout) {
    int status;
    pid_t pid = 0;
    pid = waitpid(pm->server_pid, &status, WNOHANG);
    
    do {
        if (kill(pm->server_pid, SIGTERM) < 0) {
            if (kill(pm->server_pid, SIGKILL) < 0) {
                error("error sending kill signals to server");
            }
        }

        pid = waitpid(pm->server_pid, &status, WNOHANG);
        sleep(1);

    } while (pid != pm->server_pid && timeout-- > 0);
    
    if (pid == -1) {
        error("failed to properly shutdown server")
    }

    free(pm);
}
