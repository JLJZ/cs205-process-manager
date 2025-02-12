#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "procman.h"
#include "argparse.h"

#define error(msg) do { perror("[procman] " msg); } while (0);

#define BUFFER_SIZE 64

typedef struct process process;
typedef struct process {
    pid_t pid;
    process *previous;
    process *next;
} process;

struct procman {
    int pipe[2];
    pid_t server_pid;
    process *processes;
};

static void handle_sigchld(int signum, siginfo_t *siginfo, void *ucontext) {
    assert(signum == SIGCHLD);

    (void) ucontext;

    switch (siginfo->si_code)
    {
    case CLD_KILLED:
    case CLD_DUMPED:
    case CLD_EXITED:
        printf("Terminated PID (%d)\n", siginfo->si_pid);
        break;

    case CLD_STOPPED:
        printf("Stopped PID (%d)\n", siginfo->si_pid);
        break;

    case CLD_CONTINUED:
    case CLD_TRAPPED:
        break;

    default: /* Unknown code */
        assert(false);
        break;
    }
}

static void pm_server_spawn_process(procman *pm, char *const argv[]) {
    
    /* Block child continuation signal */
    sigset_t setmask;
    sigemptyset(&setmask);
    sigaddset(&setmask, SIGCONT);

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        return;

    case 0: {
        puts("Child process spawned");
        sigprocmask(SIG_BLOCK, &setmask, NULL);
        puts("Child process started");
        if (execvp(argv[0], argv) < 0) {
            error("exec() failed");
        }
        exit(EXIT_FAILURE);
    }

    default: {
        struct sigaction action;
        sigemptyset(&action.sa_mask);
        action.sa_sigaction = handle_sigchld;
        /* Restart reads that are interrupted and fill siginfo for handler */
        action.sa_flags = SA_RESTART | SA_SIGINFO;  
        sigaction(SIGCHLD, &action, NULL);

        /* Enqueue process */
        process *p = calloc(1, sizeof(p));
        p->pid = pid;
        p->previous = NULL;
        p->next = NULL;
        if (pm->processes != NULL) {
            pm->processes->previous = p;
            p->next = pm->processes;
        }
        pm->processes = p;

        /* Continue paused child */
        kill(pid, SIGCONT);
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
        error("read() pipe failed on server");
        if (str != NULL) {
            free(str);
        }
        str = NULL;
        size = 0;
    }
    
    *out = str;
    return (size_t)size;
}

static void pm_list_processes(procman *pm) {
    process *p = pm->processes;
    while (p != NULL) {
        printf("%d\n", p->pid);
        p = p->next;
    }
}

static void dispatch(procman *pm, args *a) {
    const char *command = a->argv[0];
    if (!strcmp(command, "run")) {
        pm_server_spawn_process(pm, a->argv + 1);
    } else if (!strcmp(command, "list")) {
        pm_list_processes(pm);
    } else {
        fprintf(stderr, "Unrecognised command\n");
    }
}

static void pm_server_init(procman *sp) {
    if (close(sp->pipe[1]) < 0) {
        error("close() pipe failed on server init");
        return;
    }
    
    while (true) {
        char *cmd_str = NULL;
        size_t size = read_pipe(sp->pipe[0], &cmd_str);

        /* Ignore further processing on errors */
        if (size < 1) {
            continue;
        }
        
        args a;
        args_cast(&a, cmd_str, size);

        dispatch(sp, &a);
        
        args_free(&a);
    }
    
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on server init");
        return;
    }

    exit(EXIT_SUCCESS);
}

static void pm_client_init(procman *sp) {
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on client init");
        return;
    }
}

procman *pm_init(void) {
    procman *sp = calloc(1, sizeof(procman));
    sp->processes = NULL;
    
    if (pipe(sp->pipe) < 0) {
        error("pipe() init failed");
        goto err;
    }

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        goto err;

    case 0:
        sp->server_pid = getpid();
        pm_server_init(sp);
        break;

    default:
        sp->server_pid = pid;
        pm_client_init(sp);
        break;
    }
    
    return sp;
err:
    free(sp);
    return NULL;
}

void pm_execute(procman *sp, const char *cmd_string) {
    args cmd;
    args_parse(&cmd, cmd_string);
    
    if (write(sp->pipe[1], cmd.bytes, cmd.length) < 0) {
        error("write() pipe failed on client");
    }
    
    args_free(&cmd);
}

void pm_shutdown(procman *sp, unsigned int timeout) {
    int status;
    pid_t pid = 0;
    pid = waitpid(sp->server_pid, &status, WNOHANG);
    
    do {
        if (kill(sp->server_pid, SIGTERM) < 0) {
            if (kill(sp->server_pid, SIGKILL) < 0) {
                error("error sending kill signals to server");
            }
        }

        pid = waitpid(sp->server_pid, &status, WNOHANG);
        sleep(1);

    } while (pid != sp->server_pid && timeout-- > 0);
    
    if (pid == -1) {
        error("failed to properly shutdown server")
    }

    free(sp);
}
