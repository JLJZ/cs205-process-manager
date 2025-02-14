#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "procman.h"
#include "argparse.h"

#define error(msg) do { perror("[procman] " msg); } while (0);

#define BUFFER_SIZE 64

typedef enum pstatus {
    RUNNING,
    READY,
    STOPPED,
    TERMINATED,
} pstatus;

typedef struct process process;
typedef struct process {
    pid_t pid;
    pstatus status;
    process *previous;
    process *next;
} process;

struct procman {
    int pipe[2];
    pid_t server_pid;
    process *processes;

    process **processes_running;
    size_t processes_running_max;
    size_t processes_running_count;
};

static process *pm_find_process(procman *pm, pid_t pid) {
    for (process *p = pm->processes; p != NULL; p = p->next) {
        /* Assuming there are never duplicated pid stored */
        if (p->pid == pid) {
            return p;
        }
    }

    return NULL;
}

static void terminate_process(process *p) {
    assert(p);

    if (p->status == TERMINATED) {
        error("process already terminated");

    } else {
        int status;
        
        kill(p->pid, SIGTERM);

        if (waitpid(p->pid, &status, 0) < 0) {
            error("failed to terminate process");
        }

        p->status = TERMINATED;
    }

}

static void pm_server_spawn_process(procman *pm, char *const argv[]) {

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        return;

    case 0: {

        /* Send SIGTERM to child when parent dies  */
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) < 0) {
            error("Failed to link parent death signal");
            exit(EXIT_FAILURE);
        }
        
        if (getppid() != pm->server_pid) {
            /* Parent died before prctl() */
            exit(EXIT_FAILURE);
        }

        puts("Child process spawned");

        if (raise(SIGSTOP) == 0) {
            puts("Child process started");

            if (execvp(argv[0], argv) < 0) {
                error("exec() failed");
            }
        }

        exit(EXIT_FAILURE);
    }

    default: {

         /* Sleep for 1 second to allow child to spawn and suspend.*/
         /* Yes, it may suffer race conditions but highly unlikely */
         /* for my assignment. Simple is best right.               */
        sleep(1);

        /* Enqueue process */
        process *p = malloc(sizeof(process));
        p->pid = pid;
        p->status = READY;
        p->previous = NULL;
        p->next = NULL;
        if (pm->processes != NULL) {
            pm->processes->previous = p;
            p->next = pm->processes;
        }
        pm->processes = p;
        break;
    }
    }
}

static size_t read_pipe(int fd, char **out) {
    assert(*out == NULL);
    char *str = NULL;
    ssize_t size = 0;

    char buffer[BUFFER_SIZE];
    ssize_t read_size = read(fd, buffer, BUFFER_SIZE);

    while (read_size > 0) {
        str = realloc(str, (size_t)(size + read_size));
        memcpy(str + size, buffer, (size_t)read_size);
        size += read_size;

        /* End of buffer reached */
        if (read_size < BUFFER_SIZE) {
            break;
        }

        read_size = read(fd, buffer, BUFFER_SIZE);
    }
    
    if (read_size == -1) {
        if (errno != EAGAIN) { /* Ignore empty pipe */
            error("read() pipe failed on server");
            if (str != NULL) {
                free(str);
            }
            str = NULL;
            size = 0;
        }
    }
    
    *out = str;
    return (size_t)size;
}

static void pm_list_processes(procman *pm) {
    for (process *p = pm->processes; p != NULL; p = p->next) {
        printf("%d,%d\n", p->pid, p->status);
    }
}

static pid_t parse_pid(const char *pid) {
    int num = atoi(pid);
    /* atoi() returns 0 on invalid input but 0 is also an invalid pid for us */
    if (num == 0) {
        error("Invalid pid");
    }
    return num;
}

static bool ensure_args_length(args *a, size_t n, const char *message) {
    if (a->token_count < n) {
        fwrite(message, sizeof(char), strlen(message) + 1, stderr);
        return false;
    }
    
    return true;
}

static void dispatch(procman *pm, args *a) {
    /* No-op when empty command received */
    if (a->token_count == 0) {
        return;
    }

    const char *command = a->argv[0];

    if (!strcmp(command, "run")) {
        if (ensure_args_length(a, 2, "USAGE: run [program] [arguments]\n")) {
            pm_server_spawn_process(pm, a->argv + 1);
        }

    } else if (!strcmp(command, "kill")) {
        if (ensure_args_length(a, 2, "USAGE: kill [PID]\n")) {
            pid_t pid = parse_pid(a->argv[1]);
            process *p = pm_find_process(pm, pid);
            if (p) {
                terminate_process(p);
            }
        }

    } else if (!strcmp(command, "list")) {
        pm_list_processes(pm);

    } else {
        fprintf(stderr, "Unrecognised command\n");
    }
}

static void pm_server_reap_terminated_process(procman *pm) {
    int status = 0;

    pid_t pid = waitpid(-1, &status, WNOHANG | WCONTINUED | WSTOPPED);
    
    switch (pid) {
        case -1:
            if (ECHILD != errno) {  /* Ignore if there's no children */
                perror("wait() failed");
            }
        case 0:
            return;

        default: {
            process *p = pm_find_process(pm, pid);
            
            if (WIFSTOPPED(status)) {
                puts("STOPPED");

            } else if (WIFCONTINUED(status)) {
                puts("CONTINUED");

            } else if (WIFEXITED(status)) {
                p->status = TERMINATED;
                puts("EXITED");

            } else if (WIFSIGNALED(status)) {
                psignal(WTERMSIG(status), "Exit signal");

            } else {
                perror("Unknown process status after wait()");
            }
        }
    }
}

static void pm_server_reschedule_processes(procman *pm) {
    if (pm->processes_running_count < pm->processes_running_max) {

        /* Find earliest ready process */
        process *p = pm->processes;
        while (p != NULL) {
            if (p->status == READY) {
                break;
            }
            p = p->next;
        }
        
        /* No ready processes */
        if (p == NULL) {
            return;
        }

        printf("Ready found (%d)\n", p->pid);

        /* Run the process */
        for (size_t i = 0; i < pm->processes_running_max; ++i) {
            if (pm->processes_running[i] == NULL) {
                pm->processes_running[i] = p;
                if (kill(p->pid, SIGCONT) < 0) {
                    error("failed to continue process");
                    return;
                }
                p->status = RUNNING;
                pm->processes_running_count += 1;
                puts("REGISTER");
                break;
            }
        }
    }
}

static void pm_server_init(procman *pm) {
    if (close(pm->pipe[1]) < 0) {
        error("close() pipe failed on server init");
        return;
    }
    
    while (true) {
        char *cmd_str = NULL;
        size_t size = read_pipe(pm->pipe[0], &cmd_str);

        pm_server_reap_terminated_process(pm);
        pm_server_reschedule_processes(pm);

        /* Ignore further processing on errors */
        if (size < 1) {
            continue;
        }
        
        args a;
        args_cast(&a, cmd_str, size);

        dispatch(pm, &a);
        
        args_free(&a);
    }
    
    if (close(pm->pipe[0]) < 0) {
        error("close() pipe failed on server init");
        return;
    }

    exit(EXIT_SUCCESS);
}

static void pm_client_init(procman *pm) {
    if (close(pm->pipe[0]) < 0) {
        error("close() pipe failed on client init");
        return;
    }
}

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
        pm_client_init(pm);
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
